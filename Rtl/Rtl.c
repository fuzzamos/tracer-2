/*++

Copyright (c) 2016 Trent Nelson <trent@trent.me>

Module Name:

    Rtl.c

Abstract:

    This module provides implementations for most Rtl (Run-time Library)
    routines.

--*/

#include "stdafx.h"

//
// Keep Cu.h out of the pre-compiled header for now whilst it's in constant
// fluctuation.
//

#include "Cu.h"

//
// Temp hack: need to include RtlConstants.c directly.
//

#include "RtlConstants.c"

static PRTL_COMPARE_STRING _RtlCompareString = NULL;

INIT_ONCE InitOnceSystemTimerFunction = INIT_ONCE_STATIC_INIT;

PVECTORED_EXCEPTION_HANDLER VectoredExceptionHandler = NULL;

INIT_ONCE InitOnceCSpecificHandler = INIT_ONCE_STATIC_INIT;

CONST static UNICODE_STRING ExtendedLengthVolumePrefixW = \
    RTL_CONSTANT_STRING(L"\\\\?\\");

CONST static STRING ExtendedLengthVolumePrefixA = \
    RTL_CONSTANT_STRING("\\\\?\\");

//
// As we don't link to the CRT, we don't get a __C_specific_handler entry,
// which the linker will complain about as soon as we use __try/__except.
// What we do is define a __C_specific_handler_impl pointer to the original
// function (that lives in ntdll), then implement our own function by the
// same name that calls the underlying impl pointer.  In order to do this
// we have to disable some compiler/linker warnings regarding mismatched
// stuff.
//

static P__C_SPECIFIC_HANDLER __C_specific_handler_impl = NULL;

#pragma warning(push)
#pragma warning(disable: 4028 4273 28251)

EXCEPTION_DISPOSITION
__cdecl
__C_specific_handler(
    PEXCEPTION_RECORD ExceptionRecord,
    ULONG_PTR Frame,
    PCONTEXT Context,
    struct _DISPATCHER_CONTEXT *Dispatch
)
{
    return __C_specific_handler_impl(ExceptionRecord,
                                     Frame,
                                     Context,
                                     Dispatch);
}

#pragma warning(pop)

_Use_decl_annotations_
PVOID
CopyToMemoryMappedMemory(
    PRTL Rtl,
    PVOID Destination,
    LPCVOID Source,
    SIZE_T Size
    )
{

    //
    // Writing to memory mapped memory could raise a STATUS_IN_PAGE_ERROR
    // if there has been an issue with the backing store (such as memory
    // mapping a file on a network drive, then having the network fail,
    // or running out of disk space on the volume).  Catch such exceptions
    // and return NULL.
    //

    __try {

        return Rtl->RtlCopyMemory(Destination, Source, Size);

    } __except (GetExceptionCode() == STATUS_IN_PAGE_ERROR ?
                EXCEPTION_EXECUTE_HANDLER :
                EXCEPTION_CONTINUE_SEARCH)
    {
        return NULL;
    }
}

BOOL
TestExceptionHandler(VOID)
{
    //
    // Try assigning '1' to the memory address 0x10.
    //

    __try {

        (*(volatile *)(PCHAR)10) = '1';

    }
    __except (GetExceptionCode() == STATUS_ACCESS_VIOLATION ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH)
    {
        return TRUE;
    }

    //
    // This should be unreachable.
    //

    return FALSE;
}

_Use_decl_annotations_
BOOL
PrefaultPages(
    PVOID Address,
    ULONG NumberOfPages
    )
{
    ULONG Index;
    PCHAR Pointer = Address;

    TRY_MAPPED_MEMORY_OP {

        for (Index = 0; Index < NumberOfPages; Index++) {
            PrefaultPage(Pointer);
            Pointer += PAGE_SIZE;
        }

    } CATCH_STATUS_IN_PAGE_ERROR {
        return FALSE;
    }

    return TRUE;
}

BOOL
LoadShlwapiFunctions(
    _In_    HMODULE             ShlwapiModule,
    _In_    PSHLWAPI_FUNCTIONS  ShlwapiFunctions
    )
{
    if (!ARGUMENT_PRESENT(ShlwapiModule)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(ShlwapiFunctions)) {
        return FALSE;
    }

#define TRY_RESOLVE_SHLWAPI_FUNCTION(Type, Name) ( \
    ShlwapiFunctions->##Name = (Type)(             \
        GetProcAddress(                            \
            ShlwapiModule,                         \
            #Name                                  \
        )                                          \
    )                                              \
)

#define RESOLVE_SHLWAPI_FUNCTION(Type, Name)                         \
    if (!TRY_RESOLVE_SHLWAPI_FUNCTION(Type, Name)) {                 \
        OutputDebugStringA("Failed to resolve Shlwapi!" #Name "\n"); \
        return FALSE;                                                \
    }


    RESOLVE_SHLWAPI_FUNCTION(PPATH_CANONICALIZEA, PathCanonicalizeA);

    return TRUE;

}

RTL_API
BOOL
LoadShlwapi(PRTL Rtl)
{
    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (Rtl->ShlwapiModule) {
        return TRUE;
    }

    if (!(Rtl->ShlwapiModule = LoadLibraryA("shlwapi"))) {
        return FALSE;
    }

    return LoadShlwapiFunctions(Rtl->ShlwapiModule, &Rtl->ShlwapiFunctions);
}

RTL_API
BOOL
LoadDbgHelp(PRTL Rtl)
{
    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (Rtl->DbgHelpModule) {
        return TRUE;
    }

    if (!(Rtl->DbgHelpModule = LoadLibraryA("dbghelp"))) {
        OutputDebugStringA("Rtl: Failed to load dbghelp.");
        return FALSE;
    }

    return ResolveDbgHelpFunctions(Rtl, Rtl->DbgHelpModule, &Rtl->Dbg);
}

RTL_API
BOOL
InitializeCom(
    _In_ PRTL Rtl
    )
{

    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (Rtl->Flags.ComInitialized) {
        return TRUE;
    }

    if (!(Rtl->Ole32Module = LoadLibraryA("ole32"))) {
        OutputDebugStringA("Rtl: Failed to load ole32.");
        return FALSE;
    }

    if (!(Rtl->CoInitializeEx = (PCO_INITIALIZE_EX)
        GetProcAddress(Rtl->Ole32Module, "CoInitializeEx"))) {
        OutputDebugStringA("Failed to resolve CoInitializeEx.\n");
        return FALSE;
    }

    Rtl->Flags.ComInitialized = TRUE;
    return TRUE;
}

RTL_API
BOOL
LoadDbgEng(
    _In_ PRTL Rtl
    )
{

    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (Rtl->DbgEngModule) {
        return TRUE;
    }

    if (!Rtl->InitializeCom(Rtl)) {
        return FALSE;
    }

    if (!(Rtl->DbgEngModule = LoadLibraryA("dbgeng"))) {
        OutputDebugStringA("Rtl: Failed to load dbgeng.");
        return FALSE;
    }

    if (!(Rtl->DebugCreate = (PDEBUG_CREATE)
        GetProcAddress(Rtl->DbgEngModule, "DebugCreate"))) {

        OutputDebugStringA("DbgEng: failed to resolve 'DebugCreate'");
        return FALSE;
    }

    return TRUE;
}

RTL_API
BOOL
InitializeInjection(PRTL Rtl)
{
    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (Rtl->InjectionThunkRoutine) {
        return TRUE;
    }

    if (!IsValidMinimumDirectoryUnicodeString(&Rtl->InjectionThunkDllPath)) {

        //
        // SetInjectionThunkDllPath() needs to be called.
        //

        __debugbreak();
        return FALSE;
    }

    Rtl->InjectionThunkModule = LoadLibraryW(Rtl->InjectionThunkDllPath.Buffer);
    if (!Rtl->InjectionThunkModule) {
        __debugbreak();
        return FALSE;
    }

    Rtl->InjectionThunkRoutine = (
        GetProcAddress(
            Rtl->InjectionThunkModule,
            "InjectionThunk"
        )
    );

    if (!Rtl->InjectionThunkRoutine) {
        __debugbreak();
        return FALSE;
    }

    return TRUE;
}

_Use_decl_annotations_
BOOL
ResolveNvcudaFunctions(
    PRTL Rtl,
    HMODULE NvcudaModule,
    PCU_FUNCTIONS CuFunctions
    )
{
    BOOL Success;
    ULONG NumberOfResolvedSymbols;
    ULONG ExpectedNumberOfResolvedSymbols;
    PULONG_PTR Functions = (PULONG_PTR)CuFunctions;

#ifdef Names
#undef Names
#endif
#define Names CuFunctionNames

    ULONG BitmapBuffer[(ALIGN_UP(ARRAYSIZE(Names), sizeof(ULONG) << 3) >> 5)+1];
    RTL_BITMAP FailedBitmap = { ARRAYSIZE(Names)+1, (PULONG)&BitmapBuffer };

    ExpectedNumberOfResolvedSymbols = ARRAYSIZE(Names);

    Success = LoadSymbols(
        Names,
        ARRAYSIZE(Names),
        Functions,
        sizeof(*CuFunctions) / sizeof(ULONG_PTR),
        NvcudaModule,
        &FailedBitmap,
        &NumberOfResolvedSymbols
    );

    if (!Success) {
        __debugbreak();
    }

    if (ExpectedNumberOfResolvedSymbols != NumberOfResolvedSymbols) {
        PCSTR FirstFailedSymbolName;
        ULONG FirstFailedSymbol;
        ULONG NumberOfFailedSymbols;

        NumberOfFailedSymbols = Rtl->RtlNumberOfSetBits(&FailedBitmap);
        FirstFailedSymbol = Rtl->RtlFindSetBits(&FailedBitmap, 1, 0);
        FirstFailedSymbolName = Names[FirstFailedSymbol-1];
        __debugbreak();
    }

#undef Names

    return TRUE;
}


RTL_API
BOOL
GetCu(
    PRTL Rtl,
    PCU *CuPointer
    )
{
    BOOL Success;
    PCU Cu;
    CU_RESULT Result;

    if (!Rtl->Flags.NvcudaInitialized) {
        Rtl->NvcudaModule = LoadLibraryA("nvcuda");
        Cu = Rtl->Cu = (PCU)HeapAlloc(Rtl->HeapHandle, 0, sizeof(*Rtl->Cu));
        if (!Cu) {
            return FALSE;
        }

        ZeroStructPointer(Cu);
        Cu->SizeOfStruct = sizeof(*Cu);
        Cu->NumberOfFunctions = sizeof(CU_FUNCTIONS) / sizeof(ULONG_PTR);
        Success = ResolveNvcudaFunctions(Rtl,
                                         Rtl->NvcudaModule,
                                         &Cu->Functions);
        if (!Success) {
            goto Error;
        }

        Result = Cu->Init(0);
        if (CU_FAILED(Result)) {
            goto Error;
        }

        Rtl->Flags.NvcudaInitialized = TRUE;
    }

    *CuPointer = Rtl->Cu;
    return TRUE;

Error:

    HeapFree(Rtl->HeapHandle, 0, Cu);
    Rtl->Cu = NULL;
    return FALSE;
}

BOOL
CALLBACK
SetCSpecificHandlerCallback(
    PINIT_ONCE InitOnce,
    PVOID Parameter,
    PVOID *lpContext
    )
{
    PROC Handler;
    HMODULE Module;
    BOOL Success = FALSE;

    Module = (HMODULE)Parameter;

    if (Handler = GetProcAddress(Module, "__C_specific_handler")) {
        __C_specific_handler_impl = (P__C_SPECIFIC_HANDLER)Handler;
        Success = TRUE;
    }

    return Success;
}

BOOL
SetCSpecificHandler(
    _In_ HMODULE Module
    )
{
    BOOL Status;

    Status = InitOnceExecuteOnce(
        &InitOnceCSpecificHandler,
        SetCSpecificHandlerCallback,
        Module,
        NULL
    );

    return Status;
}

_Success_(return != 0)
BOOL
CALLBACK
GetSystemTimerFunctionCallback(
    _Inout_     PINIT_ONCE  InitOnce,
    _Inout_     PVOID       Parameter,
    _Inout_opt_ PVOID       *lpContext
    )
{
    HMODULE Module;
    FARPROC Proc;
    static SYSTEM_TIMER_FUNCTION SystemTimerFunction = { 0 };

    if (!lpContext) {
        return FALSE;
    }

    Module = GetModuleHandle(TEXT("kernel32"));
    if (!Module || Module == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    Proc = GetProcAddress(Module, "GetSystemTimePreciseAsFileTime");
    if (Proc) {
        SystemTimerFunction.GetSystemTimePreciseAsFileTime = (
            (PGETSYSTEMTIMEPRECISEASFILETIME)Proc
        );
    } else {
        Module = LoadLibrary(TEXT("ntdll"));
        if (!Module || Module == INVALID_HANDLE_VALUE) {
            return FALSE;
        }
        Proc = GetProcAddress(Module, "NtQuerySystemTime");
        if (!Proc) {
            return FALSE;
        }
        SystemTimerFunction.NtQuerySystemTime = (PNTQUERYSYSTEMTIME)Proc;
    }

    *((PPSYSTEM_TIMER_FUNCTION)lpContext) = &SystemTimerFunction;
    return TRUE;
}

PSYSTEM_TIMER_FUNCTION
GetSystemTimerFunction(
    VOID
    )
{
    BOOL Status;
    PSYSTEM_TIMER_FUNCTION SystemTimerFunction;

    Status = InitOnceExecuteOnce(
        &InitOnceSystemTimerFunction,
        GetSystemTimerFunctionCallback,
        NULL,
        (LPVOID *)&SystemTimerFunction
    );

    if (!Status) {
        return NULL;
    } else {
        return SystemTimerFunction;
    }
}

_Check_return_
BOOL
CallSystemTimer(
    _Out_       PFILETIME   SystemTime,
    _Inout_opt_ PPSYSTEM_TIMER_FUNCTION ppSystemTimerFunction
    )
{
    PSYSTEM_TIMER_FUNCTION SystemTimerFunction = NULL;

    if (ppSystemTimerFunction) {
        if (*ppSystemTimerFunction) {
            SystemTimerFunction = *ppSystemTimerFunction;
        } else {
            SystemTimerFunction = GetSystemTimerFunction();
            *ppSystemTimerFunction = SystemTimerFunction;
        }
    } else {
        SystemTimerFunction = GetSystemTimerFunction();
    }

    if (!SystemTimerFunction) {
        return FALSE;
    }

    if (SystemTimerFunction->GetSystemTimePreciseAsFileTime) {
        SystemTimerFunction->GetSystemTimePreciseAsFileTime(SystemTime);
    } else if (SystemTimerFunction->NtQuerySystemTime) {
        BOOL Success = SystemTimerFunction->NtQuerySystemTime(
            (PLARGE_INTEGER)SystemTime
        );
        if (!Success) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    return TRUE;
}


BOOL
FindCharsInUnicodeString(
    _In_     PRTL                Rtl,
    _In_     PCUNICODE_STRING    String,
    _In_     WCHAR               CharToFind,
    _Inout_  PRTL_BITMAP         Bitmap,
    _In_     BOOL                Reverse
    )
{
    USHORT Index;
    USHORT NumberOfCharacters = String->Length >> 1;
    ULONG  Bit;
    WCHAR  Char;

    //
    // We use two loop implementations in order to avoid an additional
    // branch during the loop (after we find a character match).
    //

    if (Reverse) {
        for (Index = 0; Index < NumberOfCharacters; Index++) {
            Char = String->Buffer[Index];
            if (Char == CharToFind) {
                Bit = NumberOfCharacters - Index;
                FastSetBit(Bitmap, Bit);
            }
        }
    }
    else {
        for (Index = 0; Index < NumberOfCharacters; Index++) {
            Char = String->Buffer[Index];
            if (Char == CharToFind) {
                FastSetBit(Bitmap, Index);
            }
        }
    }

    return TRUE;
}

BOOL
FindCharsInString(
    _In_     PRTL         Rtl,
    _In_     PCSTRING     String,
    _In_     CHAR         CharToFind,
    _Inout_  PRTL_BITMAP  Bitmap,
    _In_     BOOL         Reverse
    )
{
    USHORT Index;
    USHORT NumberOfCharacters = String->Length;
    ULONG  Bit;
    CHAR Char;
    PRTL_SET_BIT RtlSetBit = Rtl->RtlSetBit;

    //
    // We use two loop implementations in order to avoid an additional
    // branch during the loop (after we find a character match).
    //

    if (Reverse) {
        for (Index = 0; Index < NumberOfCharacters; Index++) {
            Char = String->Buffer[Index];
            if (Char == CharToFind) {
                Bit = NumberOfCharacters - Index;
                FastSetBit(Bitmap, Bit);
            }
        }
    }
    else {
        for (Index = 0; Index < NumberOfCharacters; Index++) {
            Char = String->Buffer[Index];
            if (Char == CharToFind) {
                FastSetBit(Bitmap, Index);
            }
        }
    }

    return TRUE;
}


_Check_return_
BOOL
CreateBitmapIndexForUnicodeString(
    _In_     PRTL                Rtl,
    _In_     PCUNICODE_STRING    String,
    _In_     WCHAR               Char,
    _Inout_  PHANDLE             HeapHandlePointer,
    _Inout_  PPRTL_BITMAP        BitmapPointer,
    _In_     BOOL                Reverse,
    _In_opt_ PFIND_CHARS_IN_UNICODE_STRING FindCharsFunction
    )

/*++

Routine Description:

    This is a helper function that simplifies creating bitmap indexes for
    UNICODE_STRING structures.  The routine will use the user-supplied bitmap
    if it is big enough (governed by the SizeOfBitMap field).  If it isn't,
    a new buffer will be allocated.  If no bitmap is provided at all, the
    entire structure plus the bitmap buffer space will be allocated from the
    heap.

    Typically, callers would provide their own pointer to a stack-allocated
    RTL_BITMAP struct if they only need the bitmap for the scope of their
    function call.  For longer-lived bitmaps, a pointer to a NULL pointer
    would be provided, indicating that the entire structure should be heap
    allocated.

    Caller is responsible for freeing either the entire RTL_BITMAP or the
    underlying Bitmap->Buffer if a heap allocation took place.

Arguments:

    Rtl - Supplies the pointer to the RTL structure (mandatory).

    String - Supplies the UNICODE_STRING structure to create the bitmap
        index for (mandatory).

    Char - Supplies the character to create the bitmap index for.  This is
        passed directly to FindCharsInUnicodeString().

    HeapHandlePointer - Supplies a pointer to the underlying heap handle
        to use for allocation.  If a heap allocation is required and this
        pointer points to a NULL value, the default process heap handle
        will be used (obtained via GetProcessHeap()), and the pointed-to
        location will be updated with the handle value.  (The caller will
        need this in order to perform the subsequent HeapFree() of the
        relevant structure.)

    BitmapPointer - Supplies a pointer to a PRTL_BITMAP structure.  If the
        pointed-to location is NULL, additional space for the RTL_BITMAP
        structure will be allocated on top of the bitmap buffer space, and
        the pointed-to location will be updated with the resulting address.
        If the pointed-to location is non-NULL and the SizeOfBitMap field
        is greater than or equal to the required bitmap size, the bitmap
        will be used directly and no heap allocations will take place.
        The SizeOfBitMap field in this example will be altered to match the
        required size.  If a heap allocation takes place, user is responsible
        for cleaning it up (i.e. either freeing the entire PRTL_BITMAP struct
        returned, or just the Bitmap->Buffer, depending on usage).  The macro
        MAYBE_FREE_BITMAP_BUFFER() should be used for this.  (See Examples.)

    Reverse - Supplies a boolean flag indicating the bitmap index should be
        created in reverse.  This is passed to FindCharsInUnicodeString().

    FindCharsInUnicodeString - Supplies an optional pointer to a function that
        conforms to the PFIND_CHARS_IN_UNICODE_STRING signature.

Return Value:

    TRUE on success, FALSE on error.

Examples:

    A stack-allocated bitmap structure and 256-byte buffer:

        CHAR StackBitmapBuffer[256];
        RTL_BITMAP Bitmap = { 32 << 3, (PULONG)&StackBitmapBuffer };
        PRTL_BITMAP BitmapPointer = &Bitmap;
        HANDLE HeapHandle;

        BOOL Success = CreateBitmapIndexForUnicodeString(Rtl,
                                                         String,
                                                         L'\\',
                                                         &HeapHandle,
                                                         &BitmapPointer,
                                                         FALSE);

        ...

        MAYBE_FREE_BITMAP_BUFFER(BitmapPointer, StackBitmapBuffer);

        return;

--*/

{
    USHORT NumberOfCharacters;
    USHORT AlignedNumberOfCharacters;
    SIZE_T BitmapBufferSizeInBytes;
    BOOL UpdateBitmapPointer;
    BOOL UpdateHeapHandlePointer;
    BOOL Success;
    HANDLE HeapHandle = NULL;
    PRTL_BITMAP Bitmap = NULL;
    PFIND_CHARS_IN_UNICODE_STRING FindChars;

    //
    // Verify arguments.
    //

    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(String)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(HeapHandlePointer)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(BitmapPointer)) {
        return FALSE;
    }

    //
    // Resolve the number of characters, then make sure it's aligned to the
    // platform's pointer size.
    //

    NumberOfCharacters = String->Length >> 1;
    AlignedNumberOfCharacters = (
        ALIGN_UP_USHORT_TO_POINTER_SIZE(
            NumberOfCharacters
        )
    );

    BitmapBufferSizeInBytes = AlignedNumberOfCharacters >> 3;


    //
    // If *BitmapPointer is non-NULL, see if it's big enough to hold the bitmap.
    //

    if (*BitmapPointer) {

        if ((*BitmapPointer)->SizeOfBitMap >= AlignedNumberOfCharacters) {

            //
            // The user-provided bitmap is big enough.  Jump straight to the
            // starting point.
            //

            Bitmap = *BitmapPointer;
            UpdateHeapHandlePointer = FALSE;
            UpdateBitmapPointer = FALSE;

            goto Start;
        }
    }

    if (!*HeapHandlePointer) {

        //
        // If the pointer to the heap handle to use is NULL, default to using
        // the default process heap via GetProcessHeap().  Note that we also
        // assign back to the user's pointer, such that they get a copy of the
        // heap handle that was used for allocation.
        //

        HeapHandle = GetProcessHeap();

        if (!HeapHandle) {
            return FALSE;
        }

        UpdateHeapHandlePointer = TRUE;
    }
    else {

        //
        // Use the handle the user provided.
        //

        HeapHandle = *HeapHandlePointer;
        UpdateHeapHandlePointer = FALSE;
    }

    if (!*BitmapPointer) {

        //
        // If the pointer to the PRTL_BITMAP structure is NULL, the caller
        // wants us to allocate the space for the RTL_BITMAP structure as
        // well.
        //

        SIZE_T AllocationSize = BitmapBufferSizeInBytes + sizeof(RTL_BITMAP);

        Bitmap = (PRTL_BITMAP)HeapAlloc(HeapHandle, 0, AllocationSize);

        if (!Bitmap) {
            return FALSE;
        }

        //
        // Point the bitmap buffer to the end of the RTL_BITMAP struct.
        //

        Bitmap->Buffer = (PULONG)(
            RtlOffsetToPointer(
                Bitmap,
                sizeof(RTL_BITMAP)
            )
        );

        //
        // Make a note that we need to update the user's bitmap pointer.
        //

        UpdateBitmapPointer = TRUE;

    }
    else {

        //
        // The user has provided an existing PRTL_BITMAP structure, so we
        // only need to allocate memory for the actual underlying bitmap
        // buffer.
        //

        Bitmap = *BitmapPointer;

        Bitmap->Buffer = (PULONG)(
            HeapAlloc(
                HeapHandle,
                0,
                BitmapBufferSizeInBytes
            )
        );

        if (!Bitmap->Buffer) {
            return FALSE;
        }

        //
        // Make a note that we do *not* need to update the user's bitmap
        // pointer.
        //

        UpdateBitmapPointer = FALSE;

    }

Start:

    //
    // There will be one bit per character.
    //

    Bitmap->SizeOfBitMap = AlignedNumberOfCharacters;

    if (!Bitmap->Buffer) {
        __debugbreak();
    }

    //
    // Clear all bits in the bitmap.
    //

    Rtl->RtlClearAllBits(Bitmap);


    //
    // Fill in the bitmap index.
    //

    FindChars = FindCharsFunction;
    if (!FindChars) {
        FindChars = FindCharsInUnicodeString;
    }

    Success = FindChars(Rtl, String, Char, Bitmap, Reverse);

    if (!Success && HeapHandle) {

        //
        // HeapHandle will only be set if we had to do heap allocations.
        //

        if (UpdateBitmapPointer) {

            //
            // Free the entire structure.
            //

            HeapFree(HeapHandle, 0, Bitmap);

        } else {

            //
            // Free just the buffer.
            //

            HeapFree(HeapHandle, 0, Bitmap->Buffer);

        }

        return FALSE;
    }

    //
    // Update caller's pointers if applicable, then return successfully.
    //

    if (UpdateBitmapPointer) {
        *BitmapPointer = Bitmap;
    }

    if (UpdateHeapHandlePointer) {
        *HeapHandlePointer = HeapHandle;
    }

    return TRUE;
}

_Check_return_
BOOL
CreateBitmapIndexForString(
    _In_     PRTL           Rtl,
    _In_     PCSTRING       String,
    _In_     CHAR           Char,
    _Inout_  PHANDLE        HeapHandlePointer,
    _Inout_  PPRTL_BITMAP   BitmapPointer,
    _In_     BOOL           Reverse,
    _In_opt_ PFIND_CHARS_IN_STRING FindCharsFunction
    )

/*++

Routine Description:

    This is a helper function that simplifies creating bitmap indexes for
    STRING structures.  The routine will use the user-supplied bitmap
    if it is big enough (governed by the SizeOfBitMap field).  If it isn't,
    a new buffer will be allocated.  If no bitmap is provided at all, the
    entire structure plus the bitmap buffer space will be allocated from the
    heap.

    Typically, callers would provide their own pointer to a stack-allocated
    RTL_BITMAP struct if they only need the bitmap for the scope of their
    function call.  For longer-lived bitmaps, a pointer to a NULL pointer
    would be provided, indicating that the entire structure should be heap
    allocated.

    Caller is responsible for freeing either the entire RTL_BITMAP or the
    underlying Bitmap->Buffer if a heap allocation took place.

Arguments:

    Rtl - Supplies the pointer to the RTL structure (mandatory).

    String - Supplies the STRING structure to create the bitmap index for.

    Char - Supplies the character to create the bitmap index for.  This is
        passed directly to FindCharsInString().

    HeapHandlePointer - Supplies a pointer to the underlying heap handle
        to use for allocation.  If a heap allocation is required and this
        pointer points to a NULL value, the default process heap handle
        will be used (obtained via GetProcessHeap()), and the pointed-to
        location will be updated with the handle value.  (The caller will
        need this in order to perform the subsequent HeapFree() of the
        relevant structure.)

    BitmapPointer - Supplies a pointer to a PRTL_BITMAP structure.  If the
        pointed-to location is NULL, additional space for the RTL_BITMAP
        structure will be allocated on top of the bitmap buffer space, and
        the pointed-to location will be updated with the resulting address.
        If the pointed-to location is non-NULL and the SizeOfBitMap field
        is greater than or equal to the required bitmap size, the bitmap
        will be used directly and no heap allocations will take place.
        The SizeOfBitMap field in this example will be altered to match the
        required size.  If a heap allocation takes place, user is responsible
        for cleaning it up (i.e. either freeing the entire PRTL_BITMAP struct
        returned, or just the Bitmap->Buffer, depending on usage).  The macro
        MAYBE_FREE_BITMAP_BUFFER() should be used for this.  (See Examples.)

    Reverse - Supplies a boolean flag indicating the bitmap index should be
        created in reverse.  This is passed to FindCharsInString().

    FindCharsInString - Supplies an optional pointer to a function that conforms
        to the PFIND_CHARS_IN_STRING signature.

Return Value:

    TRUE on success, FALSE on error.

Examples:

    A stack-allocated bitmap structure and buffer:

        CHAR StackBitmapBuffer[256];
        RTL_BITMAP Bitmap = { 32 << 3, (PULONG)&StackBitmapBuffer };
        PRTL_BITMAP BitmapPointer = &Bitmap;
        HANDLE HeapHandle;

        BOOL Success = CreateBitmapIndexForString(Rtl,
                                                  String,
                                                  '\\',
                                                  &HeapHandle,
                                                  &BitmapPointer,
                                                  FALSE);

        ...

        MAYBE_FREE_BITMAP_BUFFER(BitmapPointer, StackBitmapBuffer);

        return;

--*/

{
    USHORT NumberOfCharacters;
    USHORT AlignedNumberOfCharacters;
    SIZE_T BitmapBufferSizeInBytes;
    BOOL UpdateBitmapPointer;
    BOOL UpdateHeapHandlePointer;
    BOOL Success;
    HANDLE HeapHandle = NULL;
    PRTL_BITMAP Bitmap = NULL;
    PFIND_CHARS_IN_STRING FindChars;

    //
    // Verify arguments.
    //

    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(String)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(HeapHandlePointer)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(BitmapPointer)) {
        return FALSE;
    }

    //
    // Resolve the number of characters, then make sure it's aligned to the
    // platform's pointer size.
    //

    NumberOfCharacters = String->Length;
    AlignedNumberOfCharacters = (
        ALIGN_UP_USHORT_TO_POINTER_SIZE(
            NumberOfCharacters
        )
    );

    BitmapBufferSizeInBytes = AlignedNumberOfCharacters >> 3;

    //
    // If *BitmapPointer is non-NULL, see if it's big enough to hold the bitmap.
    //

    if (*BitmapPointer) {

        if ((*BitmapPointer)->SizeOfBitMap >= AlignedNumberOfCharacters) {

            //
            // The user-provided bitmap is big enough.  Jump straight to the
            // starting point.
            //

            Bitmap = *BitmapPointer;
            UpdateHeapHandlePointer = FALSE;
            UpdateBitmapPointer = FALSE;

            goto Start;
        }
    }

    if (!*HeapHandlePointer) {

        //
        // If the pointer to the heap handle to use is NULL, default to using
        // the default process heap via GetProcessHeap().  Note that we also
        // assign back to the user's pointer, such that they get a copy of the
        // heap handle that was used for allocation.
        //

        HeapHandle = GetProcessHeap();

        if (!HeapHandle) {
            return FALSE;
        }

        UpdateHeapHandlePointer = TRUE;
    }
    else {

        //
        // Use the handle the user provided.
        //

        HeapHandle = *HeapHandlePointer;
        UpdateHeapHandlePointer = FALSE;
    }

    if (!*BitmapPointer) {

        //
        // If the pointer to the PRTL_BITMAP structure is NULL, the caller
        // wants us to allocate the space for the RTL_BITMAP structure as
        // well.
        //

        SIZE_T AllocationSize = BitmapBufferSizeInBytes + sizeof(RTL_BITMAP);

        Bitmap = (PRTL_BITMAP)HeapAlloc(HeapHandle, 0, AllocationSize);

        if (!Bitmap) {
            return FALSE;
        }

        //
        // Point the bitmap buffer to the end of the RTL_BITMAP struct.
        //

        Bitmap->Buffer = (PULONG)(
            RtlOffsetToPointer(
                Bitmap,
                sizeof(RTL_BITMAP)
            )
        );

        //
        // Make a note that we need to update the user's bitmap pointer.
        //

        UpdateBitmapPointer = TRUE;

    }
    else {

        //
        // The user has provided an existing PRTL_BITMAP structure, so we
        // only need to allocate memory for the actual underlying bitmap
        // buffer.
        //

        Bitmap = *BitmapPointer;

        Bitmap->Buffer = (PULONG)(
            HeapAlloc(
                HeapHandle,
                HEAP_ZERO_MEMORY,
                BitmapBufferSizeInBytes
            )
        );

        if (!Bitmap->Buffer) {
            return FALSE;
        }

        //
        // Make a note that we do *not* need to update the user's bitmap
        // pointer.
        //

        UpdateBitmapPointer = FALSE;

    }

Start:

    if (!Bitmap->Buffer) {
        __debugbreak();
    }

    //
    // Clear the bitmap.  We use SecureZeroMemory() instead of RtlClearAllBits()
    // as the latter is dependent upon the SizeOfBitMap field, which a) isn't
    // set here, and b) will be set to NumberOfCharacters when it is set, which
    // may be less than AlignedNumberOfCharacters, which means some trailing
    // bits could be non-zero if we are re-using the caller's stack-allocated
    // bitmap buffer.
    //

    SecureZeroMemory(Bitmap->Buffer, BitmapBufferSizeInBytes);

    //
    // There will be one bit per character.
    //
    //

    Bitmap->SizeOfBitMap = NumberOfCharacters;

    //
    // Fill in the bitmap index.
    //

    FindChars = FindCharsFunction;
    if (!FindChars) {
        FindChars = FindCharsInString;
    }

    Success = FindChars(Rtl, String, Char, Bitmap, Reverse);

    if (!Success && HeapHandle) {

        //
        // HeapHandle will only be set if we had to do heap allocations.
        //

        if (UpdateBitmapPointer) {

            //
            // Free the entire structure.
            //

            HeapFree(HeapHandle, 0, Bitmap);

        }
        else {

            //
            // Free just the buffer.
            //

            HeapFree(HeapHandle, 0, Bitmap->Buffer);

        }

        return FALSE;
    }

    //
    // Update caller's pointers if applicable, then return successfully.
    //

    if (UpdateBitmapPointer) {
        *BitmapPointer = Bitmap;
    }

    if (UpdateHeapHandlePointer) {
        *HeapHandlePointer = HeapHandle;
    }

    return TRUE;
}


_Check_return_
BOOL
FilesExistW(
    _In_      PRTL             Rtl,
    _In_      PUNICODE_STRING  Directory,
    _In_      USHORT           NumberOfFilenames,
    _In_      PPUNICODE_STRING Filenames,
    _Out_     PBOOL            Exists,
    _Out_opt_ PUSHORT          WhichIndex,
    _Out_opt_ PPUNICODE_STRING WhichFilename
    )
{
    USHORT Index;
    PWCHAR HeapBuffer;
    ULONG CombinedSizeInBytes;
    USHORT DirectoryLength;
    USHORT MaxFilenameLength = 0;
    UNICODE_STRING Path;
    PUNICODE_STRING Filename;
    DWORD Attributes;
    BOOL Success = FALSE;
    HANDLE HeapHandle = NULL;
    WCHAR StackBuffer[_MAX_PATH];

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Directory)) {
        return FALSE;
    }

    if (NumberOfFilenames == 0) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Filenames) || !ARGUMENT_PRESENT(Filenames[0])) {
        return FALSE;
    }

    for (Index = 0; Index < NumberOfFilenames; Index++) {
        BOOL SanityCheck;

        Filename = Filenames[Index];

        //
        // Quick sanity check that the Filename pointer in the array
        // entry is non-NULL, the Length member is greater than 0,
        // and the buffer has a non-NULL value.
        //

        SanityCheck = (
            Filename &&
            Filename->Length > 0 &&
            Filename->Buffer != NULL
        );

        if (!SanityCheck) {
            __debugbreak();
        }

        //
        // Update our local maximum filename length variable if applicable.
        //

        if (Filename->Length > MaxFilenameLength) {
            MaxFilenameLength = Filename->Length;
        }
    }


    //
    // See if the combined size of the extended volume prefix ("\\?\"),
    // directory, joining backslash, maximum filename length and terminating
    // NUL is less than or equal to _MAX_PATH.  If it is, we can use the
    // stack-allocated Path buffer above; if not, allocate a new buffer from
    // the default heap.
    //

    CombinedSizeInBytes = (
        ExtendedLengthVolumePrefixW.Length  +
        Directory->Length                   +
        sizeof(WCHAR)                       + // joining backslash
        MaxFilenameLength                   +
        sizeof(WCHAR)                         // terminating NUL
    );

    //
    // Point Path->Buffer at the stack or heap buffer depending on the
    // combined size.
    //

    if (CombinedSizeInBytes <= _MAX_PATH) {

        //
        // We can use our stack buffer.
        //

        Path.Buffer = &StackBuffer[0];

    } else if (CombinedSizeInBytes > MAX_USTRING) {

        goto Error;

    } else {

        //
        // The combined size exceeds _MAX_PATH so allocate the required memory
        // from the heap.
        //

        HeapHandle = GetProcessHeap();
        if (!HeapHandle) {
            return FALSE;
        }

        HeapBuffer = (PWCHAR)HeapAlloc(HeapHandle, 0, CombinedSizeInBytes);

        if (!HeapBuffer) {
            return FALSE;
        }

        Path.Buffer = HeapBuffer;

    }

    Path.Length = 0;
    Path.MaximumLength = (USHORT)CombinedSizeInBytes;

    //
    // Copy the volume prefix, then append the directory and joining backslash.
    //

    Rtl->RtlCopyUnicodeString(&Path, &ExtendedLengthVolumePrefixW);

    if (FAILED(Rtl->RtlAppendUnicodeStringToString(&Path, Directory)) ||
        !AppendUnicodeCharToUnicodeString(&Path, L'\\')) {

        goto Error;
    }

    //
    // Make a note of the length at this point as we'll need to revert to it
    // after each unsuccessful file test.
    //

    DirectoryLength = Path.Length;

    //
    // Enumerate over the array of filenames and look for the first one that
    // exists.
    //

    for (Index = 0; Index < NumberOfFilenames; Index++) {
        Filename = Filenames[Index];

        //
        // We've already validated our lengths, so these should never fail.
        //

        if (FAILED(Rtl->RtlAppendUnicodeStringToString(&Path, Filename)) ||
            !AppendUnicodeCharToUnicodeString(&Path, L'\0')) {

            goto Error;
        }

        //
        // We successfully constructed the path, so we can now look up the file
        // attributes.
        //

        Attributes = GetFileAttributesW(Path.Buffer);

        if (Attributes == INVALID_FILE_ATTRIBUTES ||
            (Attributes & FILE_ATTRIBUTE_DIRECTORY)) {

            //
            // File doesn't exist or is a directory.  Reset the path length
            // and continue.
            //

            Path.Length = DirectoryLength;

            continue;
        }

        //
        // Success!  File exists and *isn't* a directory.  We're done.
        //

        Success = TRUE;
        break;
    }

    if (!Success) {

        *Exists = FALSE;

        //
        // The files didn't exist, but no error occurred, so we return success.
        //

        Success = TRUE;

    } else {

        *Exists = TRUE;

        //
        // Update the user's pointers if applicable.
        //

        if (ARGUMENT_PRESENT(WhichIndex)) {
            *WhichIndex = Index;
        }

        if (ARGUMENT_PRESENT(WhichFilename)) {
            *WhichFilename = Filename;
        }

    }

    //
    // Intentional follow-on to "Error"; Success code will be set
    // appropriately by this stage.
    //

Error:
    if (HeapHandle) {
        HeapFree(HeapHandle, 0, Path.Buffer);
    }

    return Success;
}

_Check_return_
BOOL
FilesExistExW(
    _In_      PRTL             Rtl,
    _In_      PUNICODE_STRING  Directory,
    _In_      USHORT           NumberOfFilenames,
    _In_      PPUNICODE_STRING Filenames,
    _Out_     PBOOL            Exists,
    _Out_opt_ PUSHORT          WhichIndex,
    _Out_opt_ PPUNICODE_STRING WhichFilename,
    _In_      PALLOCATOR       Allocator
    )
{
    USHORT Index;
    PWCHAR HeapBuffer;
    ULONG CombinedSizeInBytes;
    USHORT DirectoryLength;
    USHORT MaxFilenameLength = 0;
    UNICODE_STRING Path;
    PUNICODE_STRING Filename;
    DWORD Attributes;
    BOOL Success = FALSE;
    HANDLE HeapHandle = NULL;
    WCHAR StackBuffer[_MAX_PATH];

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Directory)) {
        return FALSE;
    }

    if (NumberOfFilenames == 0) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Filenames) || !ARGUMENT_PRESENT(Filenames[0])) {
        return FALSE;
    }

    for (Index = 0; Index < NumberOfFilenames; Index++) {
        BOOL SanityCheck;

        Filename = Filenames[Index];

        //
        // Quick sanity check that the Filename pointer in the array
        // entry is non-NULL, the Length member is greater than 0,
        // and the buffer has a non-NULL value.
        //

        SanityCheck = (
            Filename &&
            Filename->Length > 0 &&
            Filename->Buffer != NULL
        );

        if (!SanityCheck) {
            __debugbreak();
        }

        //
        // Update our local maximum filename length variable if applicable.
        //

        if (Filename->Length > MaxFilenameLength) {
            MaxFilenameLength = Filename->Length;
        }
    }


    //
    // See if the combined size of the extended volume prefix ("\\?\"),
    // directory, joining backslash, maximum filename length and terminating
    // NUL is less than or equal to _MAX_PATH.  If it is, we can use the
    // stack-allocated Path buffer above; if not, allocate a new buffer from
    // the default heap.
    //

    CombinedSizeInBytes = (
        ExtendedLengthVolumePrefixW.Length  +
        Directory->Length                   +
        sizeof(WCHAR)                       + // joining backslash
        MaxFilenameLength                   +
        sizeof(WCHAR)                         // terminating NUL
    );

    //
    // Point Path->Buffer at the stack or heap buffer depending on the
    // combined size.
    //

    if (CombinedSizeInBytes <= _MAX_PATH) {

        //
        // We can use our stack buffer.
        //

        Path.Buffer = &StackBuffer[0];

    } else if (CombinedSizeInBytes > MAX_USTRING) {

        goto Error;

    } else {

        //
        // The combined size exceeds _MAX_PATH so allocate the required memory.
        //

        HeapBuffer = (PWCHAR)(
            Allocator->Calloc(
                Allocator->Context,
                1,
                CombinedSizeInBytes
            )
        );

        if (!HeapBuffer) {
            return FALSE;
        }

        Path.Buffer = HeapBuffer;

    }

    Path.Length = 0;
    Path.MaximumLength = (USHORT)CombinedSizeInBytes;

    //
    // Copy the volume prefix, then append the directory and joining backslash.
    //

    Rtl->RtlCopyUnicodeString(&Path, &ExtendedLengthVolumePrefixW);

    if (FAILED(Rtl->RtlAppendUnicodeStringToString(&Path, Directory)) ||
        !AppendUnicodeCharToUnicodeString(&Path, L'\\')) {

        goto Error;
    }

    //
    // Make a note of the length at this point as we'll need to revert to it
    // after each unsuccessful file test.
    //

    DirectoryLength = Path.Length;

    //
    // Enumerate over the array of filenames and look for the first one that
    // exists.
    //

    for (Index = 0; Index < NumberOfFilenames; Index++) {
        Filename = Filenames[Index];

        //
        // We've already validated our lengths, so these should never fail.
        //

        if (FAILED(Rtl->RtlAppendUnicodeStringToString(&Path, Filename)) ||
            !AppendUnicodeCharToUnicodeString(&Path, L'\0')) {

            goto Error;
        }

        //
        // We successfully constructed the path, so we can now look up the file
        // attributes.
        //

        Attributes = GetFileAttributesW(Path.Buffer);

        if (Attributes == INVALID_FILE_ATTRIBUTES ||
            (Attributes & FILE_ATTRIBUTE_DIRECTORY)) {

            //
            // File doesn't exist or is a directory.  Reset the path length
            // and continue.
            //

            Path.Length = DirectoryLength;

            continue;
        }

        //
        // Success!  File exists and *isn't* a directory.  We're done.
        //

        Success = TRUE;
        break;
    }

    if (!Success) {

        *Exists = FALSE;

        //
        // The files didn't exist, but no error occurred, so we return success.
        //

        Success = TRUE;

    } else {

        *Exists = TRUE;

        //
        // Update the user's pointers if applicable.
        //

        if (ARGUMENT_PRESENT(WhichIndex)) {
            *WhichIndex = Index;
        }

        if (ARGUMENT_PRESENT(WhichFilename)) {
            *WhichFilename = Filename;
        }

    }

    //
    // Intentional follow-on to "Error"; Success code will be set
    // appropriately by this stage.
    //

Error:
    if (HeapBuffer) {
        Allocator->Free(Allocator->Context, HeapBuffer);
    }

    return Success;
}

_Success_(return != 0)
_Check_return_
BOOL
FilesExistA(
    _In_      PRTL      Rtl,
    _In_      PSTRING   Directory,
    _In_      USHORT    NumberOfFilenames,
    _In_      PPSTRING  Filenames,
    _Out_     PBOOL     Exists,
    _Out_opt_ PUSHORT   WhichIndex,
    _Out_opt_ PPSTRING  WhichFilename
    )
{
    USHORT Index;
    PCHAR HeapBuffer;
    ULONG CombinedSizeInBytes;
    USHORT DirectoryLength;
    USHORT MaxFilenameLength = 0;
    STRING Path;
    PSTRING Filename;
    DWORD Attributes;
    BOOL Success = FALSE;
    HANDLE HeapHandle = NULL;
    CHAR StackBuffer[_MAX_PATH];

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Directory)) {
        return FALSE;
    }

    if (NumberOfFilenames == 0) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Filenames) || !ARGUMENT_PRESENT(Filenames[0])) {
        return FALSE;
    }

    for (Index = 0; Index < NumberOfFilenames; Index++) {
        BOOL SanityCheck;

        Filename = Filenames[Index];

        //
        // Quick sanity check that the Filename pointer in the array
        // entry is non-NULL, the Length member is greater than 0,
        // and the buffer has a non-NULL value.
        //

        SanityCheck = (
            Filename &&
            Filename->Length > 0 &&
            Filename->Buffer != NULL
        );

        if (!SanityCheck) {
            __debugbreak();
        }

        if (Filename->Length > MaxFilenameLength) {
            MaxFilenameLength = Filename->Length;
        }
    }


    //
    // See if the combined size of the extended volume prefix ("\\?\"),
    // directory, joining backslash, maximum filename length and terminating
    // NUL is less than or equal to _MAX_PATH.  If it is, we can use the
    // stack-allocated Path buffer above; if not, allocate a new buffer from
    // the default heap.
    //

    CombinedSizeInBytes = (
        ExtendedLengthVolumePrefixA.Length +
        Directory->Length                  +
        sizeof(CHAR)                       + // joining backslash
        MaxFilenameLength                  +
        sizeof(CHAR)                         // terminating NUL
    );

    //
    // Point Path->Buffer at the stack or heap buffer depending on the
    // combined size.
    //

    if (CombinedSizeInBytes <= _MAX_PATH) {

        //
        // We can use our stack buffer.
        //

        Path.Buffer = &StackBuffer[0];

    } else if (CombinedSizeInBytes > MAX_STRING) {

        goto Error;

    } else {

        //
        // The combined size exceeds _MAX_PATH so allocate the required memory
        // from the heap.
        //

        HeapHandle = GetProcessHeap();
        if (!HeapHandle) {
            return FALSE;
        }

        HeapBuffer = (PCHAR)HeapAlloc(HeapHandle, 0, CombinedSizeInBytes);

        if (!HeapBuffer) {
            return FALSE;
        }

        Path.Buffer = HeapBuffer;

    }

    Path.Length = 0;
    Path.MaximumLength = (USHORT)CombinedSizeInBytes;

    //
    // Copy the volume prefix, then append the directory and joining backslash.
    //

    if (!CopyString(&Path, &ExtendedLengthVolumePrefixA)) {
        goto Error;
    }

    if (!AppendStringAndCharToString(&Path, Directory, '\\')) {
        goto Error;
    }

    //
    // Make a note of the length at this point as we'll need to revert to it
    // after each unsuccessful file test.
    //

    DirectoryLength = Path.Length;

    //
    // Enumerate over the array of filenames and look for the first one that
    // exists.
    //

    for (Index = 0; Index < NumberOfFilenames; Index++) {
        Filename = Filenames[Index];

        //
        // We've already validated our lengths, so these should never fail.
        //

        if (!AppendStringAndCharToString(&Path, Filename, '\0')) {
            goto Error;
        }

        //
        // We successfully constructed the path, so we can now look up the file
        // attributes.
        //

        Attributes = GetFileAttributesA(Path.Buffer);

        if (Attributes == INVALID_FILE_ATTRIBUTES ||
            (Attributes & FILE_ATTRIBUTE_DIRECTORY)) {

            //
            // File doesn't exist or is a directory.  Reset the path length
            // and continue.
            //

            Path.Length = DirectoryLength;

            continue;
        }

        //
        // Success!  File exists and *isn't* a directory.  We're done.
        //

        Success = TRUE;
        break;
    }

    if (!Success) {

        *Exists = FALSE;

        //
        // The files didn't exist, but no error occurred, so we return success.
        //

        Success = TRUE;

    } else {

        *Exists = TRUE;

        //
        // Update the user's pointers if applicable.
        //

        if (ARGUMENT_PRESENT(WhichIndex)) {
            *WhichIndex = Index;
        }

        if (ARGUMENT_PRESENT(WhichFilename)) {
            *WhichFilename = Filename;
        }

    }

    //
    // Intentional follow-on to "Error"; Success code will be set
    // appropriately by this stage.
    //

Error:
    if (HeapHandle) {
        HeapFree(HeapHandle, 0, Path.Buffer);
    }

    return Success;
}

_Success_(return != 0)
BOOL
CreateUnicodeString(
    _In_  PRTL                  Rtl,
    _In_  PCUNICODE_STRING      Source,
    _Out_ PPUNICODE_STRING      Destination,
    _In_  PALLOCATION_ROUTINE   AllocationRoutine,
    _In_  PVOID                 AllocationContext
    )
{
    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Source)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Destination)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(AllocationRoutine)) {
        return FALSE;
    }

    return CreateUnicodeStringInline(Rtl,
                                     Source,
                                     Destination,
                                     AllocationRoutine,
                                     AllocationContext);

}

_Check_return_
BOOL
LoadRtlSymbols(_Inout_ PRTL Rtl)
{

    if (!(Rtl->Kernel32Module = LoadLibraryA("kernel32"))) {
        return FALSE;
    }

    if (!(Rtl->KernelBaseModule = LoadLibraryA("kernelbase"))) {
        return FALSE;
    }

    if (!(Rtl->NtdllModule = LoadLibraryA("ntdll"))) {
        return FALSE;
    }

    if (!(Rtl->NtosKrnlModule = LoadLibraryA("ntoskrnl.exe"))) {
        return FALSE;
    }

    Rtl->GetSystemTimePreciseAsFileTime = (PGETSYSTEMTIMEPRECISEASFILETIME)
        GetProcAddress(Rtl->Kernel32Module, "GetSystemTimePreciseAsFileTime");

    Rtl->NtQuerySystemTime = (PNTQUERYSYSTEMTIME)
        GetProcAddress(Rtl->NtdllModule, "NtQuerySystemTime");

    if (Rtl->GetSystemTimePreciseAsFileTime) {
        Rtl->SystemTimerFunction.GetSystemTimePreciseAsFileTime =
            Rtl->GetSystemTimePreciseAsFileTime;
    } else if (Rtl->NtQuerySystemTime) {
        Rtl->SystemTimerFunction.NtQuerySystemTime =
            Rtl->NtQuerySystemTime;
    } else {
        return FALSE;
    }

    if (!ResolveRtlFunctions(Rtl)) {
        return FALSE;
    }

    //
    // This is a hack; we need RtlCompareString() from within PCRTCOMPARE-type
    // functions passed to bsearch/qsort.
    //

    _RtlCompareString = Rtl->RtlCompareString;

    return TRUE;
}

_Use_decl_annotations_
BOOL
ResolveRtlFunctions(
    PRTL Rtl
    )
{
    BOOL Success;
    ULONG NumberOfResolvedSymbols;
    ULONG ExpectedNumberOfResolvedSymbols;
    PULONG_PTR Functions = (PULONG_PTR)&Rtl->RtlFunctions;

    HMODULE Modules[] = {
        Rtl->KernelBaseModule,
        Rtl->Kernel32Module,
        Rtl->NtdllModule,
        Rtl->NtosKrnlModule,
    };

    //
    // Temp hack in lieu of proper refactoring.
    //

#ifdef Names
#undef Names
#endif
#define Names RtlFunctionNames

    ULONG BitmapBuffer[(ALIGN_UP(ARRAYSIZE(Names), sizeof(ULONG) << 3) >> 5)+1];
    RTL_BITMAP FailedBitmap = { ARRAYSIZE(Names)+1, (PULONG)&BitmapBuffer };

    ExpectedNumberOfResolvedSymbols = ARRAYSIZE(Names);

    Success = LoadSymbolsFromMultipleModules(
        Names,
        ARRAYSIZE(Names),
        Functions,
        sizeof(Rtl->RtlFunctions) / sizeof(ULONG_PTR),
        Modules,
        ARRAYSIZE(Modules),
        &FailedBitmap,
        &NumberOfResolvedSymbols
    );

    if (!Success) {
        __debugbreak();
    }

    if (ExpectedNumberOfResolvedSymbols != NumberOfResolvedSymbols) {
        PCSTR FirstFailedSymbolName;
        ULONG FirstFailedSymbol;
        ULONG NumberOfFailedSymbols;

        NumberOfFailedSymbols = Rtl->RtlNumberOfSetBits(&FailedBitmap);
        FirstFailedSymbol = Rtl->RtlFindSetBits(&FailedBitmap, 1, 0);
        FirstFailedSymbolName = Names[FirstFailedSymbol-1];
        __debugbreak();
    }

#undef Names

    return TRUE;
}

_Use_decl_annotations_
BOOL
ResolveRtlExFunctions(
    PRTL Rtl,
    HMODULE RtlExModule,
    PRTLEXFUNCTIONS RtlExFunctions
    )
{
    BOOL Success;
    ULONG NumberOfResolvedSymbols;
    ULONG ExpectedNumberOfResolvedSymbols;
    PULONG_PTR Functions = (PULONG_PTR)RtlExFunctions;

#ifdef Names
#undef Names
#endif
#define Names RtlExFunctionNames

    ULONG BitmapBuffer[(ALIGN_UP(ARRAYSIZE(Names), sizeof(ULONG) << 3) >> 5)+1];
    RTL_BITMAP FailedBitmap = { ARRAYSIZE(Names)+1, (PULONG)&BitmapBuffer };

    ExpectedNumberOfResolvedSymbols = ARRAYSIZE(Names);

    Success = LoadSymbols(
        Names,
        ARRAYSIZE(Names),
        Functions,
        sizeof(*RtlExFunctions) / sizeof(ULONG_PTR),
        RtlExModule,
        &FailedBitmap,
        &NumberOfResolvedSymbols
    );

    if (!Success) {
        __debugbreak();
    }

    if (ExpectedNumberOfResolvedSymbols != NumberOfResolvedSymbols) {
        PCSTR FirstFailedSymbolName;
        ULONG FirstFailedSymbol;
        ULONG NumberOfFailedSymbols;

        NumberOfFailedSymbols = Rtl->RtlNumberOfSetBits(&FailedBitmap);
        FirstFailedSymbol = Rtl->RtlFindSetBits(&FailedBitmap, 1, 0);
        FirstFailedSymbolName = Names[FirstFailedSymbol-1];
        __debugbreak();
    }

#undef Names

    return TRUE;
}

_Check_return_
BOOL
ResolveDbgHelpFunctions(
    _In_ PRTL Rtl,
    _In_ HMODULE DbgHelpModule,
    _In_ PDBG Dbg
    )
{
    BOOL Success;
    ULONG NumberOfResolvedSymbols;
    ULONG ExpectedNumberOfResolvedSymbols;
    PULONG_PTR Functions = (PULONG_PTR)Dbg;

#ifdef Names
#undef Names
#endif
#define Names DbgHelpFunctionNames

    //
    // End of auto-generated section.
    //

    ULONG BitmapBuffer[(ALIGN_UP(ARRAYSIZE(Names), sizeof(ULONG) << 3) >> 5)+1];
    RTL_BITMAP FailedBitmap = { ARRAYSIZE(Names)+1, (PULONG)&BitmapBuffer };

    ExpectedNumberOfResolvedSymbols = ARRAYSIZE(Names);

    Success = LoadSymbols(
        Names,
        ARRAYSIZE(Names),
        Functions,
        sizeof(*Dbg) / sizeof(ULONG_PTR),
        DbgHelpModule,
        &FailedBitmap,
        &NumberOfResolvedSymbols
    );

    if (!Success) {
        __debugbreak();
    }

    if (ExpectedNumberOfResolvedSymbols != NumberOfResolvedSymbols) {
        PCSTR FirstFailedSymbolName;
        ULONG FirstFailedSymbol;
        ULONG NumberOfFailedSymbols;

        NumberOfFailedSymbols = Rtl->RtlNumberOfSetBits(&FailedBitmap);
        FirstFailedSymbol = Rtl->RtlFindSetBits(&FailedBitmap, 1, 0);
        FirstFailedSymbolName = Names[FirstFailedSymbol-1];
        __debugbreak();
    }

#undef Names

    return TRUE;
}


RTL_API
BOOLEAN
RtlCheckBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG BitPosition
    )
{
#ifdef _M_AMD64
    return BitTest64((LONG64 const *)BitMapHeader->Buffer, (LONG64)BitPosition);
#else
    return BitTest((LONG const *)BitMapHeader->Buffer, (LONG)BitPosition);
#endif
}

//
// Functions for Splay Macros
//

RTL_API
VOID
RtlInitializeSplayLinks(
    _Out_ PRTL_SPLAY_LINKS Links
    )
{
    Links->Parent = Links;
    Links->LeftChild = NULL;
    Links->RightChild = NULL;
}

RTL_API
PRTL_SPLAY_LINKS
RtlParent(_In_ PRTL_SPLAY_LINKS Links)
{
    return Links->Parent;
}

RTL_API
PRTL_SPLAY_LINKS
RtlLeftChild(_In_ PRTL_SPLAY_LINKS Links)
{
    return Links->LeftChild;
}

RTL_API
PRTL_SPLAY_LINKS
RtlRightChild(_In_ PRTL_SPLAY_LINKS Links)
{
    return Links->RightChild;
}

RTL_API
BOOLEAN
RtlIsRoot(_In_ PRTL_SPLAY_LINKS Links)
{
    return (RtlParent(Links) == Links);
}

RTL_API
BOOLEAN
RtlIsLeftChild(_In_ PRTL_SPLAY_LINKS Links)
{
    return (RtlLeftChild(RtlParent(Links)) == Links);
}

RTL_API
BOOLEAN
RtlIsRightChild(_In_ PRTL_SPLAY_LINKS Links)
{
    return (RtlRightChild(RtlParent(Links)) == Links);
}

RTL_API
VOID
RtlInsertAsLeftChild (
    _Inout_ PRTL_SPLAY_LINKS ParentLinks,
    _Inout_ PRTL_SPLAY_LINKS ChildLinks
    )
{
    ParentLinks->LeftChild = ChildLinks;
    ChildLinks->Parent = ParentLinks;
}

RTL_API
VOID
RtlInsertAsRightChild (
    _Inout_ PRTL_SPLAY_LINKS ParentLinks,
    _Inout_ PRTL_SPLAY_LINKS ChildLinks
    )
{
    ParentLinks->RightChild = ChildLinks;
    ChildLinks->Parent = ParentLinks;
}

RTL_API
LONG
CompareStringCaseInsensitive(
    _In_ PCSTRING String1,
    _In_ PCSTRING String2
    )
{
    return _RtlCompareString(String1, String2, FALSE);
}

_Check_return_
BOOL
LoadRtlExSymbols(
    _In_opt_ HMODULE RtlExModule,
    _Inout_  PRTL    Rtl
    )
{
    HMODULE Module;

    if (!Rtl) {
        return FALSE;
    }

    if (RtlExModule) {
        Module = RtlExModule;

    } else {
        LPCTSTR Target = (LPCTSTR)ResolveRtlExFunctions;

        DWORD Flags = (
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS          |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
        );

        if (!GetModuleHandleEx(Flags, Target, &Module)) {
            return FALSE;
        }

        if (!Module) {
            return FALSE;
        }
    }

    if (!ResolveRtlExFunctions(Rtl, Module, &Rtl->RtlExFunctions)) {
        return FALSE;
    }

    return TRUE;

}

_Check_return_
_Success_(return != 0)
BOOL
InitializeWindowsDirectories(
    _In_ PRTL Rtl
    )
{
    PWSTR Dest;
    ULONG_INTEGER SizeInBytesExcludingNull;
    ULONG_INTEGER SizeInBytesIncludingNull;
    ULONG_INTEGER LengthInCharsExcludingNull;
    ULONG_INTEGER LengthInCharsIncludingNull;
    PUNICODE_STRING WindowsDirectory;
    PUNICODE_STRING WindowsSxSDirectory;
    PUNICODE_STRING WindowsSystemDirectory;
    const UNICODE_STRING WinSxS = RTL_CONSTANT_STRING(L"\\WinSxS");

    //
    // Initialize aliases.
    //

    WindowsDirectory = &Rtl->WindowsDirectory;

    LengthInCharsIncludingNull.LongPart = GetWindowsDirectory(NULL, 0);
    if (!LengthInCharsIncludingNull.LongPart) {
        Rtl->LastError = GetLastError();
        return FALSE;
    }

    //
    // Sanity check the size isn't above MAX_USHORT.
    //

    SizeInBytesIncludingNull.LongPart = (
        LengthInCharsIncludingNull.LongPart << 1
    );

    if (SizeInBytesIncludingNull.HighPart) {
        __debugbreak();
        return FALSE;
    }

    //
    // Allocate space for the buffer.
    //

    WindowsDirectory->Buffer = (PWSTR)(
        HeapAlloc(
            Rtl->HeapHandle,
            0,
            SizeInBytesIncludingNull.LongPart
        )
    );

    if (!WindowsDirectory->Buffer) {
        Rtl->LastError = GetLastError();
        return FALSE;
    }

    //
    // Initialize lengths.
    //

    SizeInBytesExcludingNull.LongPart = (
        SizeInBytesIncludingNull.LongPart - sizeof(WCHAR)
    );
    WindowsDirectory->Length = SizeInBytesExcludingNull.LowPart;
    WindowsDirectory->MaximumLength = SizeInBytesIncludingNull.LowPart;

    //
    // Call GetWindowsDirectory() again with the newly allocated buffer.
    //

    LengthInCharsExcludingNull.LongPart = (
        GetWindowsDirectory(
            WindowsDirectory->Buffer,
            LengthInCharsIncludingNull.LongPart
        )
    );

    if (LengthInCharsExcludingNull.LongPart + 1 !=
        LengthInCharsIncludingNull.LongPart) {
        Rtl->LastError = GetLastError();
        return FALSE;
    }

    //
    // Now process WinSxS directory.
    //

    WindowsSxSDirectory = &Rtl->WindowsSxSDirectory;

    //
    // Add the length of the "\\WinSxS" suffix.  Use WinSxS.Length as we've
    // already accounted for the trailing NULL.
    //

    LengthInCharsIncludingNull.LongPart += WinSxS.Length >> 1;

    //
    // Convert into size in bytes.
    //

    SizeInBytesIncludingNull.LongPart = (
        LengthInCharsIncludingNull.LongPart << 1
    );

    //
    // Sanity check the size isn't above MAX_USHORT.
    //

    if (SizeInBytesIncludingNull.HighPart) {
        __debugbreak();
        return FALSE;
    }

    //
    // Allocate space for the buffer.
    //

    WindowsSxSDirectory->Buffer = (PWSTR)(
        HeapAlloc(
            Rtl->HeapHandle,
            0,
            SizeInBytesIncludingNull.LongPart
        )
    );

    if (!WindowsSxSDirectory->Buffer) {
        Rtl->LastError = GetLastError();
        return FALSE;
    }

    //
    // Initialize lengths.
    //

    SizeInBytesExcludingNull.LongPart = (
        SizeInBytesIncludingNull.LongPart - sizeof(WCHAR)
    );
    WindowsSxSDirectory->Length = SizeInBytesExcludingNull.LowPart;
    WindowsSxSDirectory->MaximumLength = SizeInBytesIncludingNull.LowPart;

    //
    // Copy the Windows directory prefix over, excluding the terminating NULL.
    //

    Dest = WindowsSxSDirectory->Buffer;
    __movsw((PWORD)Dest,
            (PWORD)WindowsDirectory->Buffer,
            WindowsDirectory->Length >> 1);

    //
    // Copy the "\\WinSxS" suffix.
    //

    Dest += (WindowsDirectory->Length >> 1);
    __movsw((PWORD)Dest,
            (PWORD)WinSxS.Buffer,
            WinSxS.Length >> 1);

    //
    // Add terminating NULL.
    //

    Dest += (WinSxS.Length >> 1);
    *Dest = L'\0';

    //
    // Sanity check things are where they should be.
    //

    if (WindowsSxSDirectory->Buffer[WindowsDirectory->Length >> 1] != L'\\') {
        __debugbreak();
    }

    if (WindowsSxSDirectory->Buffer[WindowsSxSDirectory->Length>>1] != L'\0') {
        __debugbreak();
    }

    //
    // Now do the Windows system directory.
    //

    WindowsSystemDirectory = &Rtl->WindowsSystemDirectory;

    LengthInCharsIncludingNull.LongPart = GetSystemDirectory(NULL, 0);
    if (!LengthInCharsIncludingNull.LongPart) {
        Rtl->LastError = GetLastError();
        return FALSE;
    }

    //
    // Sanity check the size isn't above MAX_USHORT.
    //

    SizeInBytesIncludingNull.LongPart = (
        LengthInCharsIncludingNull.LongPart << 1
    );

    if (SizeInBytesIncludingNull.HighPart) {
        __debugbreak();
        return FALSE;
    }

    //
    // Allocate space for the buffer.
    //

    WindowsSystemDirectory->Buffer = (PWSTR)(
        HeapAlloc(
            Rtl->HeapHandle,
            0,
            SizeInBytesIncludingNull.LongPart
        )
    );

    if (!WindowsSystemDirectory->Buffer) {
        Rtl->LastError = GetLastError();
        return FALSE;
    }

    //
    // Initialize lengths.
    //

    SizeInBytesExcludingNull.LongPart = (
        SizeInBytesIncludingNull.LongPart - sizeof(WCHAR)
    );
    WindowsSystemDirectory->Length = SizeInBytesExcludingNull.LowPart;
    WindowsSystemDirectory->MaximumLength = SizeInBytesIncludingNull.LowPart;

    //
    // Call GetSystemDirectory() again with the newly allocated buffer.
    //

    LengthInCharsExcludingNull.LongPart = (
        GetSystemDirectory(
            WindowsSystemDirectory->Buffer,
            LengthInCharsIncludingNull.LongPart
        )
    );

    if (LengthInCharsExcludingNull.LongPart + 1 !=
        LengthInCharsIncludingNull.LongPart) {
        Rtl->LastError = GetLastError();
        return FALSE;
    }

    return TRUE;
}

RTL_API CRYPT_GEN_RANDOM RtlCryptGenRandom;

_Use_decl_annotations_
BOOL
RtlCryptGenRandom(
    PRTL Rtl,
    ULONG SizeOfBufferInBytes,
    PBYTE Buffer
    )
{
    if (!ARGUMENT_PRESENT(Rtl)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Rtl->CryptProv)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Buffer)) {
        return FALSE;
    }

    if (!CryptGenRandom(Rtl->CryptProv, SizeOfBufferInBytes, Buffer)) {
        Rtl->LastError = GetLastError();
        return FALSE;
    }

    return TRUE;
}

BOOL
RtlInitializeInjection(
    VOID
    )
{
    return TRUE;
}

RTL_API RTL_SET_DLL_PATH RtlpSetDllPath;

_Use_decl_annotations_
BOOL
RtlpSetDllPath(
    PRTL Rtl,
    PALLOCATOR Allocator,
    PCUNICODE_STRING Path
    )
{
    return AllocateAndCopyUnicodeString(Allocator, Path, &Rtl->RtlDllPath);
}

RTL_API RTL_SET_INJECTION_THUNK_DLL_PATH RtlpSetInjectionThunkDllPath;

_Use_decl_annotations_
BOOL
RtlpSetInjectionThunkDllPath(
    PRTL Rtl,
    PALLOCATOR Allocator,
    PCUNICODE_STRING Path
    )
{
    return AllocateAndCopyUnicodeString(Allocator,
                                        Path,
                                        &Rtl->InjectionThunkDllPath);
}

RTL_API RTL_CREATE_NAMED_EVENT RtlpCreateNamedEvent;

#if 0
_Use_decl_annotations_
BOOL
RtlpCreateNamedEvent(
    PRTL Rtl,
    PHANDLE HandlePointer,
    LPSECURITY_ATTRIBUTES EventAttributes,
    BOOL ManualReset,
    BOOL InitialState,
    PCUNICODE_STRING Prefix,
    PCUNICODE_STRING Suffix,
    PUNICODE_STRING EventName
    )
{
    BOOL Success;
    HRESULT Result;
    USHORT BytesRemaining;
    HANDLE Handle;
    const UNICODE_STRING Local = RTL_CONSTANT_STRING(L"Local\\");
    BYTE LocalBuffer[64];
    WCHAR WideBase64Buffer[86];
    ULONG WideBase64BufferLengthInChars = ARRAYSIZE(WideBase64Buffer);
    ULONG WideBase64BufferSizeInBytes = sizeof(WideBase64Buffer);

    //
    // Clear the caller's pointer up-front.
    //

    *HandlePointer = NULL;

    if (EventName->Length != 0) {
        __debugbreak();
        return FALSE;
    }

    Result = Rtl->RtlAppendUnicodeStringToString(EventName, &Local);
    if (FAILED(Result)) {
        __debugbreak();
        return FALSE;
    }

    if (ARGUMENT_PRESENT(Prefix)) {

        Result = Rtl->RtlAppendUnicodeStringToString(EventName, Prefix);
        if (FAILED(Result)) {
            __debugbreak();
            return FALSE;
        }

    }

    BytesRemaining = (
        (EventName->MaximumLength - sizeof(WCHAR)) -
        EventName->Length
    );

    if (ARGUMENT_PRESENT(Suffix)) {
        BytesRemaining -= Suffix->Length;
    }

    if (BytesRemaining <= 7) {
        __debugbreak();
        return FALSE;
    }

    //
    // Cap the size to the number of bytes remaining.
    //

    if (BytesRemaining >= WideBase64BufferSizeInBytes) {
        BytesRemaining = (USHORT)WideBase64BufferSizeInBytes;
    }

    //
    // (I'm being lazy; just generate 64 bytes of random data into the local
    //  buffer instead of fiddling with exact lengths and whatnot.)
    //

    Success = Rtl->CryptGenRandom(Rtl,
                                  sizeof(LocalBuffer)-1,
                                  (PBYTE)&LocalBuffer);
    if (!Success) {
        __debugbreak();
        return FALSE;
    }

    Success = CryptBinaryToStringW((const PBYTE)&LocalBuffer,
                                   sizeof(LocalBuffer)-1,
                                   CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                                   (LPWSTR)&WideBase64Buffer,
                                   &WideBase64BufferLengthInChars);

    if (!Success) {
        Rtl->LastError = GetLastError();
        __debugbreak();
        return FALSE;
    }

    //
    // Forcibly NULL-terminate the wide character buffer based on our number of
    // bytes remaining.
    //

    WideBase64Buffer[(BytesRemaining >> 1)] = L'\0';

    //
    // Copy the random data over.
    //

    Result = Rtl->RtlAppendUnicodeToString(EventName, WideBase64Buffer);
    if (FAILED(Result)) {
        __debugbreak();
        return FALSE;
    }

    //
    // If there was a suffix, copy that over.
    //

    if (ARGUMENT_PRESENT(Suffix)) {

        Result = Rtl->RtlAppendUnicodeStringToString(EventName, Suffix);
        if (FAILED(Result)) {
            __debugbreak();
            return FALSE;
        }

    }

    //
    // Invariant checks.
    //

    if (EventName->Length >= EventName->MaximumLength) {
        __debugbreak();
        return FALSE;
    }

    //
    // NULL-terminate the Unicode string.
    //

    EventName->Buffer[(EventName->Length >> 1)] = L'\0';

    //
    // Now create the event.
    //

    Handle = CreateEventW(EventAttributes,
                          ManualReset,
                          InitialState,
                          EventName->Buffer);

    if (!Handle || Handle == INVALID_HANDLE_VALUE) {
        __debugbreak();
        return FALSE;
    }

    //
    // Update the caller's pointer and return TRUE.
    //

    *HandlePointer = Handle;

    return TRUE;
}
#endif

BOOL
InitializeTsx(PRTL Rtl)
{
    GUARDED_LIST List;
    LIST_ENTRY Entry;

    InitializeGuardedListHead(&List);
    InitializeListHead(&Entry);

    Rtl->Flags.TsxAvailable = TRUE;

    TRY_AVX {
        InsertTailGuardedListTsxInline(&List, &Entry);
    } CATCH_EXCEPTION_ILLEGAL_INSTRUCTION {
        Rtl->Flags.TsxAvailable = FALSE;
    }

    return TRUE;
}

VIRTUAL_ALLOC RtlpTryLargePageVirtualAlloc;

_Use_decl_annotations_
LPVOID
RtlpLargePageVirtualAlloc(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  flAllocationType,
    DWORD  flProtect
    )
{
    return VirtualAlloc(lpAddress,
                        max(dwSize, GetLargePageMinimum()),
                        flAllocationType | MEM_LARGE_PAGES,
                        flProtect);
}

VIRTUAL_ALLOC_EX RtlpTryLargePageVirtualAllocEx;

_Use_decl_annotations_
LPVOID
RtlpLargePageVirtualAllocEx(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  flAllocationType,
    DWORD  flProtect
    )
{
    return VirtualAllocEx(hProcess,
                          lpAddress,
                          max(dwSize, GetLargePageMinimum()),
                          flAllocationType | MEM_LARGE_PAGES,
                          flProtect);
}

BOOL
InitializeLargePages(PRTL Rtl)
{
    Rtl->Flags.IsLargePageEnabled = Rtl->EnableLockMemoryPrivilege();
    Rtl->LargePageMinimum = GetLargePageMinimum();

    if (Rtl->Flags.IsLargePageEnabled) {
        Rtl->TryLargePageVirtualAlloc = RtlpLargePageVirtualAlloc;
        Rtl->TryLargePageVirtualAllocEx = RtlpLargePageVirtualAllocEx;
    } else {
        Rtl->TryLargePageVirtualAlloc = VirtualAlloc;
        Rtl->TryLargePageVirtualAllocEx = VirtualAllocEx;
    }

    return TRUE;
}

PVOID
TryMapViewOfFileNuma2(
    PRTL Rtl,
    HANDLE FileMappingHandle,
    HANDLE ProcessHandle,
    ULONG64 Offset,
    PVOID BaseAddress,
    SIZE_T ViewSize,
    ULONG AllocationType,
    ULONG PageProtection,
    ULONG PreferredNode
    )
{
    LARGE_INTEGER FileOffset;

    if (!Rtl->MapViewOfFileNuma2) {
        goto Fallback;
    }

    AllocationType = FilterLargePageFlags(Rtl, AllocationType);

    return Rtl->MapViewOfFileNuma2(FileMappingHandle,
                                   ProcessHandle,
                                   Offset,
                                   BaseAddress,
                                   ViewSize,
                                   AllocationType,
                                   PageProtection,
                                   PreferredNode);

Fallback:

    FileOffset.QuadPart = Offset;
    return Rtl->MapViewOfFileExNuma(FileMappingHandle,
                                    PageProtection,
                                    FileOffset.HighPart,
                                    FileOffset.LowPart,
                                    ViewSize,
                                    BaseAddress,
                                    PreferredNode);
}

RTL_API PROBE_FOR_READ ProbeForRead;

_Use_decl_annotations_
BOOL
ProbeForRead(
    PRTL Rtl,
    PVOID Address,
    SIZE_T NumberOfBytes,
    PULONG NumberOfValidPages
    )
{
    BOOL Success;
    ULONG Index;
    ULONG ValidPages;
    ULONG NumberOfPages;
    PBYTE Byte;
    PBYTE Buffer;
    SIZE_T PageAlignedSize;

    ValidPages = 0;
    PageAlignedSize = ROUND_TO_PAGES(NumberOfBytes);
    NumberOfPages = (ULONG)(PageAlignedSize >> PAGE_SHIFT);
    Buffer = (PBYTE)Address;
    Success = TRUE;

    TRY_PROBE_MEMORY {

        for (Index = 0; Index < NumberOfPages; Index++) {
            Byte = Buffer + (Index * (1 << PAGE_SHIFT));
            PrefaultPage(Byte);
            ValidPages++;
        }

    } CATCH_STATUS_IN_PAGE_ERROR_OR_ACCESS_VIOLATION {

        Success = FALSE;
    }

    if (ARGUMENT_PRESENT(NumberOfValidPages)) {
        *NumberOfValidPages = ValidPages;
    }

    return Success;
}

_Use_decl_annotations_
BOOL
InitializeRtl(
    PRTL   Rtl,
    PULONG SizeOfRtl
    )
{
    BOOL Success;
    HANDLE HeapHandle;
    PRTL_LDR_NOTIFICATION_TABLE Table;

    if (!Rtl) {
        if (SizeOfRtl) {
            *SizeOfRtl = sizeof(*Rtl);
        }
        return FALSE;
    }

    if (!SizeOfRtl) {
        return FALSE;
    }

    if (*SizeOfRtl < sizeof(*Rtl)) {
        *SizeOfRtl = sizeof(*Rtl);
        return FALSE;
    } else {
        *SizeOfRtl = sizeof(*Rtl);
    }

    HeapHandle = GetProcessHeap();
    if (!HeapHandle) {
        return FALSE;
    }

    SecureZeroMemory(Rtl, sizeof(*Rtl));

    if (!LoadRtlSymbols(Rtl)) {
        return FALSE;
    }

    Rtl->SizeOfStruct = sizeof(*Rtl);

    SetCSpecificHandler(Rtl->NtdllModule);
    Rtl->__C_specific_handler = __C_specific_handler_impl;
    if (!Rtl->__C_specific_handler) {
        return FALSE;
    }

    Rtl->HeapHandle = HeapHandle;

    if (!LoadRtlExSymbols(NULL, Rtl)) {
        return FALSE;
    }

    if (!InitializeWindowsDirectories(Rtl)) {
        return FALSE;
    }

    if (!InitializeTsx(Rtl)) {
        return FALSE;
    }

    if (!InitializeLargePages(Rtl)) {
        return FALSE;
    }

    Rtl->atexit = atexit_impl;
    Rtl->AtExitEx = AtExitExImpl;
    Rtl->RundownGlobalAtExitFunctions = RundownGlobalAtExitFunctions;

    Rtl->InitializeInjection = InitializeInjection;
    Rtl->InjectThunk = InjectThunk;

    Rtl->GetCu = GetCu;

    //
    // Windows 8 onward.
    //

    Rtl->MapViewOfFileExNuma = (PMAP_VIEW_OF_FILE_EX_NUMA)(
        GetProcAddress(
            Rtl->Kernel32Module,
            "MapViewOfFileExNuma"
        )
    );

    //
    // Windows 10 1703 onward.
    //

    Rtl->MapViewOfFileNuma2 = (PMAP_VIEW_OF_FILE_NUMA2)(
        GetProcAddress(
            Rtl->KernelBaseModule,
            "MapViewOfFileNuma2"
        )
    );

    Rtl->TryMapViewOfFileNuma2 = TryMapViewOfFileNuma2;

    Rtl->OutputDebugStringA = OutputDebugStringA;
    Rtl->OutputDebugStringW = OutputDebugStringW;

    Rtl->MaximumFileSectionSize = Rtl->MmGetMaximumFileSectionSize();

    Table = Rtl->LoaderNotificationTable = (PRTL_LDR_NOTIFICATION_TABLE)(
        HeapAlloc(
            HeapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(*Rtl->LoaderNotificationTable)
        )
    );

    if (!Table) {
        return FALSE;
    }

    Success = InitializeRtlLdrNotificationTable(Rtl, Table);
    if (!Success) {
        HeapFree(HeapHandle, 0, Table);
        Rtl->LoaderNotificationTable = NULL;
    }

    Rtl->Multiplicand.QuadPart = TIMESTAMP_TO_SECONDS;
    QueryPerformanceFrequency(&Rtl->Frequency);

    Success = CryptAcquireContextW(&Rtl->CryptProv,
                                   NULL,
                                   NULL,
                                   PROV_RSA_FULL,
                                   CRYPT_VERIFYCONTEXT);

    if (!Success) {
        Rtl->LastError = GetLastError();
        return FALSE;
    }

    Rtl->CryptGenRandom = RtlCryptGenRandom;
    //Rtl->CryptBinaryToStringA = CryptBinaryToStringA;
    //Rtl->CryptBinaryToStringW = CryptBinaryToStringW;
    Rtl->CreateEventA = CreateEventA;
    Rtl->CreateEventW = CreateEventW;

    Rtl->InitializeCom = InitializeCom;
    Rtl->LoadDbgEng = LoadDbgEng;
    Rtl->CopyPages = CopyPagesNonTemporalAvx2_v4;
    Rtl->FillPages = FillPagesNonTemporalAvx2_v1;
    Rtl->ProbeForRead = ProbeForRead;

    Rtl->SetDllPath = RtlpSetDllPath;
    Rtl->SetInjectionThunkDllPath = RtlpSetInjectionThunkDllPath;
    Rtl->CopyFunction = CopyFunction;

    //Rtl->CreateNamedEvent = RtlpCreateNamedEvent;

#ifdef _RTL_TEST
    Rtl->TestLoadSymbols = TestLoadSymbols;
    Rtl->TestLoadSymbolsFromMultipleModules = (
        TestLoadSymbolsFromMultipleModules
    );
#endif

    return Success;
}

RTL_API
BOOL
InitializeRtlManually(PRTL Rtl, PULONG SizeOfRtl)
{
    return InitializeRtlManuallyInline(Rtl, SizeOfRtl);
}

_Use_decl_annotations_
VOID
DestroyRtl(
    PPRTL RtlPointer
    )
{
    PRTL Rtl;

    if (!ARGUMENT_PRESENT(RtlPointer)) {
        return;
    }

    Rtl = *RtlPointer;

    if (!ARGUMENT_PRESENT(Rtl)) {
        return;
    }

    //
    // Clear the caller's pointer straight away.
    //

    *RtlPointer = NULL;

    if (Rtl->NtdllModule) {
        FreeLibrary(Rtl->NtdllModule);
        Rtl->NtdllModule = NULL;
    }

    if (Rtl->Kernel32Module) {
        FreeLibrary(Rtl->Kernel32Module);
        Rtl->Kernel32Module = NULL;
    }

    if (Rtl->KernelBaseModule) {
        FreeLibrary(Rtl->KernelBaseModule);
        Rtl->KernelBaseModule = NULL;
    }

    if (Rtl->NtosKrnlModule) {
        FreeLibrary(Rtl->NtosKrnlModule);
        Rtl->NtosKrnlModule = NULL;
    }

    return;
}

VOID
Debugbreak()
{
    __debugbreak();
}

_Use_decl_annotations_
PLIST_ENTRY
RemoveHeadGuardedListTsx(
    PGUARDED_LIST GuardedList
    )
{
    return RemoveHeadGuardedListTsxInline(GuardedList);
}

_Use_decl_annotations_
PLIST_ENTRY
RemoveTailGuardedListTsx(
    PGUARDED_LIST GuardedList
    )
{
    return RemoveTailGuardedListTsxInline(GuardedList);
}

_Use_decl_annotations_
VOID
InsertTailGuardedListTsx(
    PGUARDED_LIST GuardedList,
    PLIST_ENTRY Entry
    )
{
    InsertTailGuardedListTsxInline(GuardedList, Entry);
}

_Use_decl_annotations_
VOID
AppendTailGuardedListTsx(
    PGUARDED_LIST GuardedList,
    PLIST_ENTRY Entry
    )
{
    AppendTailGuardedListTsxInline(GuardedList, Entry);
}

#ifndef VECTORCALL
#define VECTORCALL __vectorcall
#endif

RTL_API
XMMWORD
VECTORCALL
DummyVectorCall1(
    _In_ XMMWORD Xmm0,
    _In_ XMMWORD Xmm1,
    _In_ XMMWORD Xmm2,
    _In_ XMMWORD Xmm3
    )
{
    XMMWORD Temp1;
    XMMWORD Temp2;
    Temp1 = _mm_xor_si128(Xmm0, Xmm1);
    Temp2 = _mm_xor_si128(Xmm2, Xmm3);
    return _mm_xor_si128(Temp1, Temp2);
}

typedef struct _TEST_HVA3 {
    XMMWORD X;
    XMMWORD Y;
    XMMWORD Z;
} TEST_HVA3;

RTL_API
TEST_HVA3
VECTORCALL
DummyHvaCall1(
    _In_ TEST_HVA3 Hva3
    )
{
    Hva3.X = _mm_xor_si128(Hva3.Y, Hva3.Z);
    return Hva3;
}

#if 0
typedef struct _TEST_HFA3 {
    DOUBLE X;
    DOUBLE Y;
    DOUBLE Z;
} TEST_HFA3;

RTL_API
TEST_HFA3
VECTORCALL
DummyHfaCall1(
    _In_ TEST_HFA3 Hfa3
)
{
    __m128d Double;
    Double = _mm_setr_pd(Hfa3.Y, Hfa3.Z);
    Hfa3.X = Double.m128d_f64[0];
    return Hfa3;
}
#endif


// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
