/*++

Copyright (c) 2017 Trent Nelson <trent@trent.me>

Module Name:

    TraceStoreSqlite3ExtInit.c

Abstract:

    This module implements the sqlite3 extension initialization function
    TraceStoreSqlite3ExtInit, which is called by TraceStoreSqlite3Ext module
    when sqlite3 has loaded it.

--*/

#include "stdafx.h"

typedef
VOID
(CALLBACK TRACE_STORE_SQLITE3_INIT_CU)(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context
    );
typedef TRACE_STORE_SQLITE3_INIT_CU *PTRACE_STORE_SQLITE3_INIT_CU;

typedef
BOOL
(LOAD_FILE)(
    _In_ PRTL Rtl,
    _In_ PCUNICODE_STRING Path,
    _Out_ PHANDLE FileHandlePointer,
    _Out_ PHANDLE MappingHandlePointer,
    _Out_ PPVOID BaseAddressPointer
    );
typedef LOAD_FILE *PLOAD_FILE;

LOAD_FILE LoadFile;

_Use_decl_annotations_
BOOL
LoadFile(
    PRTL Rtl,
    PCUNICODE_STRING Path,
    PHANDLE FileHandlePointer,
    PHANDLE MappingHandlePointer,
    PPVOID BaseAddressPointer
    )
{
    BOOL Success;
    ULONG LastError;
    HANDLE FileHandle = NULL;
    PVOID BaseAddress = NULL;
    HANDLE MappingHandle = NULL;

    //
    // Clear the caller's pointers up-front.
    //

    *FileHandlePointer = NULL;
    *BaseAddressPointer = NULL;
    *MappingHandlePointer = NULL;

    FileHandle = CreateFileW(Path->Buffer,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED  |
                             FILE_ATTRIBUTE_NORMAL |
                             FILE_FLAG_SEQUENTIAL_SCAN,
                             NULL);

    if (!FileHandle || FileHandle == INVALID_HANDLE_VALUE) {
        FileHandle = NULL;
        LastError = GetLastError();
        __debugbreak();
        goto Error;
    }

    MappingHandle = CreateFileMappingNuma(FileHandle,
                                          NULL,
                                          PAGE_READONLY,
                                          0,
                                          0,
                                          NULL,
                                          NUMA_NO_PREFERRED_NODE);

    if (!MappingHandle || MappingHandle == INVALID_HANDLE_VALUE) {
        MappingHandle = NULL;
        LastError = GetLastError();
        __debugbreak();
        goto Error;
    }

    BaseAddress = Rtl->MapViewOfFileExNuma(MappingHandle,
                                           FILE_MAP_READ,
                                           0,
                                           0,
                                           0,
                                           BaseAddress,
                                           NUMA_NO_PREFERRED_NODE);

    if (!BaseAddress) {
        LastError = GetLastError();
        __debugbreak();
        goto Error;
    }

    //
    // We've successfully opened, created a section for, and then subsequently
    // mapped, the requested PTX file.  Update the caller's pointers and return
    // success.
    //

    *FileHandlePointer = FileHandle;
    *BaseAddressPointer = BaseAddress;
    *MappingHandlePointer = MappingHandle;

    Success = TRUE;
    goto End;

Error:

    if (MappingHandle) {
        CloseHandle(MappingHandle);
        MappingHandle = NULL;
    }

    if (FileHandle) {
        CloseHandle(FileHandle);
        FileHandle = NULL;
    }

    Success = FALSE;

    //
    // Intentional follow-on to End.
    //

End:

    return Success;
}



TRACE_STORE_SQLITE3_INIT_CU TraceStoreSqlite3InitCu;

#define NUM_PTX_FILES 1

_Use_decl_annotations_
VOID
TraceStoreSqlite3InitCu(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID TpContext
    )
{
    BOOL Success;
    PCU Cu;
    PRTL Rtl;
    USHORT Index;
    USHORT FuncIndex;
    ULONG Ordinal;
    CU_DEVICE Device;
    PCU_MODULE Module;
    PCU_CONTEXT Context;
    PCU_FUNCTION Function;
    PCSZ FunctionName;
    PTRACE_STORE_SQLITE3_DB Db;
    TRACER_PTX_PATH_ID PtxPathIds[NUM_PTX_FILES] = {
        TraceStoreKernelsPtxPathId,
    };

    PCSZ TraceStoreKernelFunctionNames[] = {
        (PCSZ)"SinglePrecisionAlphaXPlusY",
        (PCSZ)"DeltaTimestamp",
    };

    PCSZ *PtxFunctionNames[] = {

        //
        // TraceStoreKernels
        //

        TraceStoreKernelFunctionNames,
    };

    PTRACER_CONFIG TracerConfig;
    PTRACER_PATHS Paths;
    TRACER_PTX_PATH_ID PathId;
    HANDLE FileHandle;
    PVOID BaseAddress;
    HANDLE MappingHandle;
    PCUNICODE_STRING Path;
    PCSZ PtxContents;
    CHAR JitLogBuffer[1024];
    CU_RESULT Result;
    CU_JIT_OPTION JitOptions[] = {
        CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES,
        CU_JIT_INFO_LOG_BUFFER,
        CU_JIT_MAX_REGISTERS,
    };
    PVOID JitOptionValues[] = {
        (PVOID)sizeof(JitLogBuffer),
        (PVOID)JitLogBuffer,
        (PVOID)32LL,
    };
    USHORT NumberOfJitOptions = ARRAYSIZE(JitOptions);
    USHORT NumberOfFunctions = ARRAYSIZE(TraceStoreKernelFunctionNames);

    //
    // Resolve local aliases.
    //

    Db = (PTRACE_STORE_SQLITE3_DB)TpContext;
    Rtl = Db->Rtl;
    TracerConfig = Db->TracerConfig;
    Paths = &TracerConfig->Paths;

    //
    // Register a SetEvent() against our loading complete event for when this
    // callback returns.  The TraceStoreSqlite3ExtInit() routine will perform
    // a WaitForSingleObject(INFINITE) against this event prior to returning
    // to the caller.
    //

    SetEventWhenCallbackReturns(Instance, Db->CuLoadingCompleteEvent);

    //
    // Load Cu.
    //

    Success = Rtl->GetCu(Rtl, &Db->Cu);
    if (!Success) {

        //
        // If we couldn't load nvcuda, there's nothing more to do.
        //

        return;
    }

    //
    // The initial CU interface was created successfully.  Attempt to get a
    // handle to the device indicated by TracerConfig, then create a context.
    //

    Cu = Db->Cu;
    Ordinal = TracerConfig->CuDeviceOrdinal;
    Result = Cu->GetDevice(&Device, Ordinal);
    if (CU_FAILED(Result)) {
        goto Error;
    }

    Result = Cu->CreateContext(&Context, 0, Device);
    if (CU_FAILED(Result)) {
        goto Error;
    }

    //
    // Now enumerate through each PTX path and create a corresponding module,
    // then load the module's functions.
    //

    for (Index = 0; Index < NUM_PTX_FILES; Index++) {

        PathId = PtxPathIds[Index];

        //
        // Resolve the path of the given PTX file.
        //

        Path = TracerPtxPathIdToUnicodeString(Paths, PathId);

        Success = LoadFile(Rtl,
                           Path,
                           &FileHandle,
                           &MappingHandle,
                           &BaseAddress);

        if (!Success) {
            __debugbreak();
            goto Error;
        }

        PtxContents = (PCSZ)BaseAddress;

        Result = Cu->LoadModuleDataEx(&Module,
                                      BaseAddress,
                                      NumberOfJitOptions,
                                      JitOptions,
                                      JitOptionValues);

        if (CU_FAILED(Result)) {
            __debugbreak();
            goto Error;
        }

        for (FuncIndex = 0; FuncIndex < NumberOfFunctions; FuncIndex++) {
            FunctionName = PtxFunctionNames[Index][FuncIndex];
            Result = Cu->GetModuleFunction(&Function, Module, FunctionName);
            if (CU_FAILED(Result)) {
                goto Error;
            }
            Db->CuFunctions[FuncIndex] = Function;
        }

        UnmapViewOfFile(BaseAddress);
        CloseHandle(MappingHandle);
        CloseHandle(FileHandle);

        FileHandle = NULL;
        BaseAddress = NULL;
        MappingHandle = NULL;
    }

    Success = TRUE;
    goto End;

Error:

    Success = FALSE;

End:

    return;
}



DECLSPEC_DLLEXPORT TRACE_STORE_SQLITE3_EXT_INIT TraceStoreSqlite3ExtInit;

_Use_decl_annotations_
LONG
TraceStoreSqlite3ExtInit(
    PRTL Rtl,
    PALLOCATOR Sqlite3Allocator,
    PTRACER_CONFIG TracerConfig,
    PSQLITE3_DB Sqlite3Db,
    PCSZ *ErrorMessagePointer,
    PCSQLITE3 Sqlite3
    )
/*++

Routine Description:

    This is the main entry point for the TraceStore sqlite3 extension module.
    It is called by our extension loader thunk (TraceStoreSqlite3ExtLoader).

    It is responsible for registering as a module to sqlite3.

Arguments:

    Rtl - Supplies a pointer to an initialized RTL structure.

    Sqlite3Allocator - Supplies a pointer to an ALLOCATOR structure that has
        been initialized from the sqlite3 API (i.e. it will dispatch malloc etc
        calls to Sqlite3->Malloc64).

    TracerConfig - Supplies a pointer to an initialized TRACER_CONFIG structure.

    Sqlite3Db - Supplies a pointer to the active sqlite3 database.

    ErrorMessagePointer - Supplies a variable that optionally receives the
        address of an error message if initialization fails.

    Sqlite3 - Supplies a pointer to the sqlite3 API routines.

Return Value:

    SQLITE_OK on success, either SQLITE_NOMEM or SQLITE_ERROR on error.

--*/
{
    BOOL Success;
    BOOL DispatchedInitCu = TRUE;
    PCHAR Char;
    PVOID Buffer;
    USHORT Count;
    USHORT Length;
    USHORT Index;
    ULONG Result;
    ULONG RequiredSize;
    USHORT TotalNumberOfTraceStores;
    PCSZ DatabaseFilename;
    STRING Filename;
    PALLOCATOR Allocator;
    PUNICODE_STRING Path;
    PTRACER_PATHS Paths;
    PTRACE_STORES TraceStores;
    PTRACE_CONTEXT TraceContext;
    PTRACE_SESSION TraceSession;
    PTRACE_STORE_SQLITE3_DB Db;
    HANDLE LoadingCompleteEvent = NULL;
    TRACE_FLAGS TraceFlags;
    TRACE_CONTEXT_FLAGS TraceContextFlags;
    PSET_ATEXITEX SetAtExitEx;
    PSET_C_SPECIFIC_HANDLER SetCSpecificHandler;
    PCTRACE_STORE_SQLITE3_FUNCTION Function;

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(Rtl)) {
        return SQLITE_ERROR;
    }

    if (!ARGUMENT_PRESENT(Sqlite3Allocator)) {
        return SQLITE_ERROR;
    }

    if (!ARGUMENT_PRESENT(TracerConfig)) {
        return SQLITE_ERROR;
    }

    if (!ARGUMENT_PRESENT(Sqlite3Db)) {
        return SQLITE_ERROR;
    }

    if (!ARGUMENT_PRESENT(ErrorMessagePointer)) {
        return SQLITE_ERROR;
    }

    if (!ARGUMENT_PRESENT(Sqlite3)) {
        return SQLITE_ERROR;
    }

    //
    // Allocate space for the TRACE_STORE_SQLITE3_DB structure.
    //

    Db = (PTRACE_STORE_SQLITE3_DB)Sqlite3->Malloc(sizeof(*Db));
    if (!Db) {
        return SQLITE_NOMEM;
    }

    SecureZeroMemory(Db, sizeof(*Db));

    //
    // Initialize fields.
    //

    Db->Rtl = Rtl;
    Db->Sqlite3 = Sqlite3;
    Db->Sqlite3Db = Sqlite3Db;
    Db->TracerConfig = TracerConfig;
    Db->Sqlite3Allocator = Sqlite3Allocator;

    Db->SizeOfStruct = sizeof(*Db);

    //
    // Create an event for the CUDA handles.
    //

    Db->CuLoadingCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!Db->CuLoadingCompleteEvent) {
        *ErrorMessagePointer = "CreateEvent(CuLoadingCompleteEvent) failed.\n";
        goto Error;
    }

    //
    // Dispatch the InitCu() work via the default threadpool.
    //

    if (!TrySubmitThreadpoolCallback(TraceStoreSqlite3InitCu, Db, NULL)) {
        __debugbreak();
        *ErrorMessagePointer = "TrySubmitThreadpoolCallback(InitCu) failed.\n";
        DispatchedInitCu = FALSE;
        goto Error;
    }


#define LOAD_DLL(Name) do {                                       \
    Db->Name##Module = LoadLibraryW(Paths->Name##DllPath.Buffer); \
    if (!Db->Name##Module) {                                      \
        OutputDebugStringA("Failed to load " #Name);              \
        goto Error;                                               \
    }                                                             \
} while (0)

    Paths = &TracerConfig->Paths;

    LOAD_DLL(TracerHeap);
    LOAD_DLL(StringTable);

    //
    // Resolve the functions.
    //

#define RESOLVE(Module, Type, Name) do {                                     \
    Db->Name = (Type)GetProcAddress(Db->Module, #Name);                      \
    if (!Db->Name) {                                                         \
        *ErrorMessagePointer = "Failed to resolve " #Module " !" #Name "\n"; \
        goto Error;                                                          \
    }                                                                        \
} while (0)

    RESOLVE(TracerHeapModule,
            PINITIALIZE_HEAP_ALLOCATOR_EX,
            InitializeHeapAllocatorEx);

    //
    // Initialize our Db-specific Allocator.
    //

    Allocator = &Db->Allocator;
    Success = Db->InitializeHeapAllocatorEx(&Db->Allocator, 0, 0, 0);
    if (!Success) {
        __debugbreak();
        goto Error;
    }

    //
    // Load our owning module name.
    //

    Db->OwningModule = GetModuleHandle(NULL);

    Success = Rtl->GetModuleRtlPath(Rtl,
                                    Db->OwningModule,
                                    Allocator,
                                    &Db->OwningModulePath);

    if (!Success) {
        *ErrorMessagePointer = "Rtl->GetModuleRtlPath() failed.\n";
        goto Error;
    }

    //
    // Path initialization.
    //

    DatabaseFilename = Sqlite3->DbFilename(Sqlite3Db, "main");
    Length = (USHORT)strlen(DatabaseFilename);

    //
    // Point our Char pointer at the end of the string.
    //

    Count = 0;
    Char = (PSZ)(DatabaseFilename + Length);
    while (*Char != '\\') {
        Char--;
        Count++;
    }

    Filename.Buffer = (PSZ)DatabaseFilename;
    Filename.Length = Length - Count;
    Filename.MaximumLength = Filename.Length + 1;

    Success = ConvertUtf8StringToUtf16StringSlow(&Filename,
                                                 &Db->TraceSessionDirectory,
                                                 Allocator,
                                                 NULL);

    if (!Success) {
        __debugbreak();
        goto Error;
    }

    Path = Db->TraceSessionDirectory;
    if (Path->Buffer[Path->Length >> 1] != L'\0') {
        __debugbreak();
        goto Error;
    }

    //
    // All of our modules modules use the same pattern for initialization
    // whereby the required structure size can be obtained by passing a NULL
    // for the destination pointer argument and PULONG as the size argument.
    // We leverage that here, using the local &RequiredSize variable and some
    // macro glue.
    //

#undef ALLOCATE
#define ALLOCATE(Target, Type)                                                \
    Db->Target = (Type)(                                                      \
        Allocator->Calloc(                                                    \
            Allocator->Context,                                               \
            1,                                                                \
            RequiredSize                                                      \
        )                                                                     \
    );                                                                        \
    if (!Db->Target) {                                                        \
        *ErrorMessagePointer = "Allocation failed for " #Target " struct.\n"; \
        goto Error;                                                           \
    }

    //
    // StringTable
    //

    RESOLVE(StringTableModule,
            PINITIALIZE_STRING_TABLE_ALLOCATOR,
            InitializeStringTableAllocator);

    RESOLVE(StringTableModule,
            PCREATE_STRING_TABLE,
            CreateStringTable);

    RESOLVE(StringTableModule,
            PCREATE_STRING_TABLE_FROM_DELIMITED_STRING,
            CreateStringTableFromDelimitedString);

    RESOLVE(StringTableModule,
            PCREATE_STRING_TABLE_FROM_DELIMITED_ENVIRONMENT_VARIABLE,
            CreateStringTableFromDelimitedEnvironmentVariable);

    //
    // If any of our DLLs use structured exception handling (i.e. contain
    // code that uses __try/__except), they'll also export SetCSpecificHandler
    // which needs to be called now with the __C_specific_handler that Rtl will
    // have initialized.
    //

#define INIT_C_SPECIFIC_HANDLER(Name) do {                      \
    SetCSpecificHandler = (PSET_C_SPECIFIC_HANDLER)(            \
        GetProcAddress(Db->Name##Module, "SetCSpecificHandler") \
    );                                                          \
    if (SetCSpecificHandler) {                                  \
        SetCSpecificHandler(Rtl->__C_specific_handler);         \
    }                                                           \
} while (0)

    INIT_C_SPECIFIC_HANDLER(TracerHeap);
    INIT_C_SPECIFIC_HANDLER(StringTable);

    //
    // Use the same approach for any DLLs that require extended atexit support.
    //

#define INIT_ATEXITEX(Name) do {                        \
    SetAtExitEx = (PSET_ATEXITEX)(                      \
        GetProcAddress(Db->Name##Module, "SetAtExitEx") \
    );                                                  \
    if (SetAtExitEx) {                                  \
        SetAtExitEx(Rtl->AtExitEx);                     \
    }                                                   \
} while (0)

    INIT_ATEXITEX(TracerHeap);
    INIT_ATEXITEX(StringTable);

    //
    // Initialize string table allocators then create string tables.
    //

    Success = Db->InitializeStringTableAllocator(&Db->StringTableAllocator);
    if (!Success) {
        *ErrorMessagePointer = (
            "InitializeStringTableAllocator(StringTable) failed.\n"
        );
        goto Error;
    }

    Success = Db->InitializeStringTableAllocator(&Db->StringArrayAllocator);
    if (!Success) {
        *ErrorMessagePointer = (
            "InitializeStringTableAllocator(StringArray) failed.\n"
        );
        goto Error;
    }

    Db->FunctionStringTable1 = (
        Db->CreateStringTableFromDelimitedString(
            Rtl,
            &Db->StringTableAllocator,
            &Db->StringArrayAllocator,
            &TraceStoreSqlite3FunctionsString,
            TraceStoreSqlite3StringTableDelimiter
        )
    );

    if (!Db->FunctionStringTable1) {
        goto Error;
    }

    Db->NumberOfFunctionStringTables = 1;

    //
    // Allocate and initialize TraceSession.
    //

    RequiredSize = 0;
    InitializeTraceSession(Rtl, NULL, &RequiredSize);
    ALLOCATE(TraceSession, PTRACE_SESSION);
    Success = InitializeTraceSession(Rtl,
                                     Db->TraceSession,
                                     &RequiredSize);

    if (!Success) {
        *ErrorMessagePointer = "Db->InitializeTraceSession() failed.\n";
        goto Error;
    }

    //
    // Initialize flags.
    //

    TraceFlags.AsULong = 0;
    TraceFlags.Readonly = TRUE;

    //
    // Get the required size of the TRACE_STORES structure.
    //

    RequiredSize = 0;
    InitializeTraceStores(
        NULL,           // Rtl
        NULL,           // Allocator
        NULL,           // TracerConfig
        NULL,           // BaseDirectory
        NULL,           // TraceStores
        &RequiredSize,  // SizeOfTraceStores
        NULL,           // InitialFileSizes
        NULL,           // MappingSizes
        &TraceFlags,    // TraceFlags
        NULL            // FieldRelocations
    );

    //
    // Use a large page allocation for trace stores.
    //

    RequiredSize = ALIGN_UP(RequiredSize, (ULONG)Rtl->LargePageMinimum);
    Buffer = Rtl->TryLargePageVirtualAlloc(NULL,
                                           RequiredSize,
                                           MEM_COMMIT,
                                           PAGE_READWRITE);
    if (!Buffer) {
        __debugbreak();
        goto Error;
    }
    Db->TraceStores = (PTRACE_STORES)Buffer;

    Success = InitializeTraceStores(
        Rtl,
        Allocator,
        Db->TracerConfig,
        Db->TraceSessionDirectory->Buffer,
        Db->TraceStores,
        &RequiredSize,
        NULL,
        NULL,
        &TraceFlags,
        NULL //Db->PythonTracerTraceStoreRelocations
    );

    if (!Success) {
        *ErrorMessagePointer = "InitializeTraceStores() failed.\n";
        goto Error;
    }

    //
    // Get the maximum number of processors and create a threadpool
    // and environment.
    //

    Db->MaximumProcessorCount = GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);

    Db->Threadpool = CreateThreadpool(NULL);
    if (!Db->Threadpool) {
        *ErrorMessagePointer = "CreateThreadpool() failed.\n";
        goto Error;
    }

    //
    // Limit the threadpool to 2 times the number of processors in the system.
    //

    SetThreadpoolThreadMinimum(Db->Threadpool, Db->MaximumProcessorCount);
    SetThreadpoolThreadMaximum(Db->Threadpool, Db->MaximumProcessorCount * 2);

    //
    // Initialize the threadpool environment and bind it to the threadpool.
    //

    InitializeThreadpoolEnvironment(&Db->ThreadpoolCallbackEnviron);
    SetThreadpoolCallbackPool(
        &Db->ThreadpoolCallbackEnviron,
        Db->Threadpool
    );

    //
    // Create a cancellation threadpool with one thread.
    //

    Db->CancellationThreadpool = CreateThreadpool(NULL);
    if (!Db->CancellationThreadpool) {
        OutputDebugStringA("CreateThreadpool() failed for cancellation.\n");
        goto Error;
    }

    SetThreadpoolThreadMinimum(Db->CancellationThreadpool, 1);
    SetThreadpoolThreadMaximum(Db->CancellationThreadpool, 1);
    InitializeThreadpoolEnvironment(
        &Db->CancellationThreadpoolCallbackEnviron
    );

    //
    // Clear the trace context flags and set readonly.
    //

    TraceContextFlags.AsULong = 0;
    TraceContextFlags.Readonly = TRUE;

    //
    // Get the required size of a TRACE_CONTEXT structure.
    //

    RequiredSize = 0;
    InitializeTraceContext(
        NULL,           // Rtl
        NULL,           // Allocator
        NULL,           // TracerConfig
        NULL,           // TraceContext
        &RequiredSize,  // SizeOfTraceContext
        NULL,           // TraceSession
        NULL,           // TraceStores
        NULL,           // ThreadpoolCallbackEnviron
        NULL,           // CancellationThreadpoolCallbackEnviron
        NULL,           // TraceContextFlags
        NULL            // UserData
    );

    //
    // Allocate sufficient space then initialize the trace context.
    //

    ALLOCATE(TraceContext, PTRACE_CONTEXT);
    Success = InitializeTraceContext(
        Rtl,
        Allocator,
        Db->TracerConfig,
        Db->TraceContext,
        &RequiredSize,
        Db->TraceSession,
        Db->TraceStores,
        &Db->ThreadpoolCallbackEnviron,
        &Db->CancellationThreadpoolCallbackEnviron,
        &TraceContextFlags,
        (PVOID)Db
    );

    if (!Success) {
        *ErrorMessagePointer = "InitializeTraceContext() failed.\n";
        goto Error;
    }

    LoadingCompleteEvent = Db->TraceContext->LoadingCompleteEvent;

    //
    // Initialize aliases.
    //

    TraceStores = Db->TraceStores;
    TraceContext = Db->TraceContext;
    TraceSession = Db->TraceSession;

    //
    // We've successfully loaded the trace stores in this directory.  Proceed
    // with creation of the sqlite3 modules for each trace store.
    //

    TotalNumberOfTraceStores = (
        TraceStores->NumberOfTraceStores *
        TraceStores->ElementsPerTraceStore
    );

    for (Index = 0; Index < TotalNumberOfTraceStores; Index++) {
        PTRACE_STORE TraceStore;
        PSQLITE3_MODULE Module;
        PCSZ *Schema;
        PCSZ *VirtualTableName;

        TraceStore = &TraceStores->Stores[Index];

        TraceStore->Db = Db;
        TraceStore->Sqlite3Column = TraceStoreSqlite3Columns[Index];
        Module = &TraceStore->Sqlite3Module;
        Schema = &TraceStore->Sqlite3Schema;
        VirtualTableName = &TraceStore->Sqlite3VirtualTableName;

        CopySqlite3ModuleInline(Module, &TraceStoreSqlite3Module);

        *Schema = TraceStoreSchemas[Index];
        *VirtualTableName = TraceStoreSqlite3VirtualTableNames[Index];

        if (TraceStore->TraceStoreMetadataId == TraceStoreMetadataInfoId) {
            if (*Schema != TraceStoreInfoSchema) {
                __debugbreak();
            }
        }

        Result = Sqlite3->CreateModule(Sqlite3Db,
                                       *VirtualTableName,
                                       Module,
                                       TraceStore);

        if (Result != SQLITE_OK) {
            __debugbreak();
            goto Error;
        }
    }

    //
    // Register all of our functions.
    //

    FOR_EACH_TRACE_STORE_SQLITE3_FUNCTION(Function) {

        Result = Sqlite3->OverloadFunction(Sqlite3Db,
                                           Function->Name,
                                           Function->NumberOfArguments);

        if (Result != SQLITE_OK) {
            __debugbreak();
            goto Error;
        }

        Result = Sqlite3->CreateFunctionV2(Sqlite3Db,
                                           Function->Name,
                                           Function->NumberOfArguments,
                                           Function->TextEncoding,
                                           Db,
                                           Function->ScalarFunction,
                                           Function->AggregateStepFunction,
                                           Function->AggregateFinalFunction,
                                           Function->DestroyFunction);

        if (Result != SQLITE_OK) {
            __debugbreak();
            goto Error;
        }
    }

    if (DispatchedInitCu) {
        LONG WaitResult;

        WaitResult = WaitForSingleObject(Db->CuLoadingCompleteEvent, INFINITE);
        if (WaitResult != WAIT_OBJECT_0) {
            __debugbreak();
            Db->Cu = NULL;
            goto Error;
        }
    }

    Result = SQLITE_OK_LOAD_PERMANENTLY;

    goto End;

Error:

    Success = FALSE;

    //
    // If the loading complete event handle is non-NULL, we need to wait on
    // this first before we can destroy the traced python session.
    //

    if (LoadingCompleteEvent) {
        HRESULT WaitResult;

        OutputDebugStringA("Destroy: waiting for LoadingCompleteEvent.\n");
        WaitResult = WaitForSingleObject(LoadingCompleteEvent, INFINITE);
        if (WaitResult != WAIT_OBJECT_0) {

            //
            // The wait was abandoned or we encountered some other error.  Go
            // straight to the end and skip the destroy step.
            //

            OutputDebugStringA("Wait failed, skipping destroy.\n");
            goto End;
        }
    }

    //DestroyTracedPythonSession(&Session);

End:

    return Result;
}

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
