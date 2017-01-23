/*++

Copyright (c) 2017 Trent Nelson <trent@trent.me>

Module Name:

    DebugEngine.c

Abstract:

    This module implements functionality related to the Windows Debugger
    Engine COM facilities exposed by the DbgEng.dll library.

--*/

#include "stdafx.h"

//
// Locally define the IIDs of the interfaces we want in order to avoid the
// EXTERN_C linkage we'll pick up if we use the values directly from DbgEng.h.
//

#define DEFINE_GUID_EX(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const GUID DECLSPEC_SELECTANY name                                  \
        = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

//
// IID_IUnknown: 00000000-0000-0000-C000-000000000046
//

DEFINE_GUID_EX(IID_IUNKNOWN, 0x00000000, 0x0000, 0x0000,
               0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

//
// IID_IDebugAdvanced4: d1069067-2a65-4bf0-ae97-76184b67856b
//

DEFINE_GUID_EX(IID_IDEBUG_ADVANCED, 0xd1069067, 0x2a65, 0x4bf0,
               0xae, 0x97, 0x76, 0x18, 0x4b, 0x67, 0x85, 0x6b);

//
// IDebugSymbols5: c65fa83e-1e69-475e-8e0e-b5d79e9cc17e
//

DEFINE_GUID_EX(IID_IDEBUG_SYMBOLS, 0xc65fa83e, 0x1e69, 0x475e,
               0x8e, 0x0e, 0xb5, 0xd7, 0x9e, 0x9c, 0xc1, 0x7e);

//
// IDebugSymbolGroup:  f2528316-0f1a-4431-aeed-11d096e1e2ab
// IDebugSymbolGroup2: 6a7ccc5f-fb5e-4dcc-b41c-6c20307bccc7
//

DEFINE_GUID_EX(IID_IDEBUG_SYMBOLGROUP, 0xf2528316, 0x0f1a, 0x4431,
               0xae, 0xed, 0x11, 0xd0, 0x96, 0xe1, 0xe2, 0xab);

//DEFINE_GUID_EX(IID_IDEBUG_SYMBOLGROUP, 0x6a7ccc5f, 0xfb5e, 0x4dcc,
//               0xb4, 0x1c, 0x6c, 0x20, 0x30, 0x7b, 0xcc, 0xc7);

//
// IDebugDataSpaces4: d98ada1f-29e9-4ef5-a6c0-e53349883212
//

DEFINE_GUID_EX(IID_IDEBUG_DATASPACES, 0xd98ada1f, 0x29e9, 0x4ef5,
               0xa6, 0xc0, 0xe5, 0x33, 0x49, 0x88, 0x32, 0x12);

//
// IID_IDebugClient7: 13586be3-542e-481e-b1f2-8497ba74f9a9
//

DEFINE_GUID_EX(IID_IDEBUG_CLIENT, 0x13586be3, 0x542e, 0x481e,
               0xb1, 0xf2, 0x84, 0x97, 0xba, 0x74, 0xf9, 0xa9);

//
// IID_IDebugControl7: b86fb3b1-80d4-475b-aea3-cf06539cf63a
//

DEFINE_GUID_EX(IID_IDEBUG_CONTROL, 0xb86fb3b1, 0x80d4, 0x475b,
               0xae, 0xa3, 0xcf, 0x06, 0x53, 0x9c, 0xf6, 0x3a);

//
// IID_IDebugEventCallbacksWide: 0690e046-9c23-45ac-a04f-987ac29ad0d3
//

DEFINE_GUID_EX(IID_IDEBUG_EVENT_CALLBACKS, 0x0690e046, 0x9c23, 0x45ac,
               0xa0, 0x4f, 0x98, 0x7a, 0xc2, 0x9a, 0xd0, 0xd3);

//
// IID_IDebugOutputCallbacks2: 67721fe9-56d2-4a44-a325-2b65513ce6eb
//

DEFINE_GUID_EX(IID_IDEBUG_OUTPUT_CALLBACKS, 0x67721fe9, 0x56d2, 0x4a44,
               0xa3, 0x25, 0x2b, 0x65, 0x51, 0x3c, 0xe6, 0xeb);

//
// IID_IDebugInputCallbacks: 9f50e42c-f136-499e-9a97-73036c94ed2d
//

DEFINE_GUID_EX(IID_IDEBUG_INPUT_CALLBACKS, 0x9f50e42c, 0xf136, 0x499e,
               0x9a, 0x97, 0x73, 0x03, 0x6c, 0x94, 0xed, 0x2d);

//
// Instances of callback structures primed with relevant function pointers.
//

CONST DEBUGEVENTCALLBACKS DebugEventCallbacks = {
    DebugEventQueryInterface,
    DebugEventAddRef,
    DebugEventRelease,
    DebugEventGetInterestMaskCallback,
    DebugEventBreakpointCallback,
    DebugEventExceptionCallback,
    DebugEventCreateThreadCallback,
    DebugEventExitThreadCallback,
    DebugEventCreateProcessCallback,
    DebugEventExitProcessCallback,
    DebugEventLoadModuleCallback,
    DebugEventUnloadModuleCallback,
    DebugEventSystemErrorCallback,
    DebugEventSessionStatusCallback,
    DebugEventChangeDebuggeeStateCallback,
    DebugEventChangeEngineStateCallback,
    DebugEventChangeSymbolStateCallback,
};

CONST DEBUGOUTPUTCALLBACKS DebugOutputCallbacks = {
    DebugOutputQueryInterface,
    DebugOutputAddRef,
    DebugOutputRelease,
    DebugOutputOutputCallback,
    DebugOutputGetInterestMaskCallback,
    DebugOutputOutput2Callback,
};

CONST DEBUGINPUTCALLBACKS DebugInputCallbacks = {
    DebugInputQueryInterface,
    DebugInputAddRef,
    DebugInputRelease,
    DebugInputStartInputCallback,
    DebugInputEndInputCallback,
};

//
// Functions.
//

_Use_decl_annotations_
BOOL
InitializeDebugEngineSession(
    PRTL Rtl,
    PALLOCATOR Allocator,
    DEBUG_ENGINE_SESSION_INIT_FLAGS InitFlags,
    PPDEBUG_ENGINE_SESSION SessionPointer
    )
/*++

Routine Description:

    This routine creates a debug client object.

Arguments:

    Rtl - Supplies a pointer to an RTL structure.

    Allocator - Supplies a pointer to an ALLOCATOR structure.

    EnginePointer - Supplies an address to a variable that will receive the
        address of the newly created debug client.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    BOOL Success;
    NTSTATUS Status;
    HRESULT Result;
    HMODULE Shell32Module = NULL;
    PDEBUGCLIENT Client;
    PIDEBUGCLIENT IClient;
    PDEBUGCONTROL Control;
    PIDEBUGCONTROL IControl;
    PDEBUGSYMBOLS Symbols;
    PIDEBUGSYMBOLS ISymbols;
    PDEBUG_ENGINE Engine;
    PDEBUG_ENGINE_SESSION Session = NULL;
    PCOMMAND_LINE_TO_ARGVW CommandLineToArgvW;
    ULONG64 Base;
    ULONG Index;
    //ULONG TypeId;
    ULONG64 Offset;
    ULONG64 Offset2;
    ULONG Flags = 0;
    ULONG64 Address = (ULONG64)0x000000001e0c5450;
    CHAR Buffer[256];
    PSTR Dest;
    ULONG BufferSize = sizeof(Buffer);
    ULONG DisassemblySize;
    ULONG OffsetLine;
    ULONG64 StartOffset;
    ULONG64 EndOffset;
    ULONG64 LineOffsets;
    ULONG64 EffectiveOffset;
    ULONG DisassemblyFlags = (
        DEBUG_DISASM_MATCHING_SYMBOLS  |
        DEBUG_DISASM_EFFECTIVE_ADDRESS |
        DEBUG_DISASM_SOURCE_FILE_NAME  |
        DEBUG_DISASM_SOURCE_LINE_NUMBER
    );

    PWSTR UnassemblePy_IncRef = L"uf python27!Py_IncRef";
    PWSTR ListPy27Functions = L"x /D /f python!Py*";

    *SessionPointer = NULL;

    CHECKED_MSG(InitFlags.InitializeFromCommandLine,
                "CreateAndInitializeDebugEngineSession(): InitFlags "
                "missing InitializeFromCommandLine bit set");

    CHECKED_MSG(Rtl->LoadDbgEng(Rtl), "Rtl!LoadDbgEng()");
    ALLOCATE_TYPE(Session, DEBUG_ENGINE_SESSION, Allocator);

    *SessionPointer = Session;

    CHECKED_MSG(InitializeDebugEngine(Rtl, &Session->Engine),
                "InitializeDebugEngine()");

    LOAD_LIBRARY_A(Shell32Module, Shell32);

    RESOLVE_FUNCTION(CommandLineToArgvW,
                     Shell32Module,
                     PCOMMAND_LINE_TO_ARGVW,
                     CommandLineToArgvW);

    CHECKED_MSG(Session->CommandLineA = GetCommandLineA(), "GetCommandLineA()");
    CHECKED_MSG(Session->CommandLineW = GetCommandLineW(), "GetCommandLineW()");

    Session->ArgvW = CommandLineToArgvW(Session->CommandLineW,
                                        &Session->NumberOfArguments);

    CHECKED_MSG(Session->ArgvW, "Shell32!CommandLineToArgvW()");

    CHECKED_MSG(
        Rtl->ArgvWToArgvA(
            Session->ArgvW,
            Session->NumberOfArguments,
            &Session->ArgvA,
            NULL,
            Allocator
        ),
        "Rtl!ArgvWToArgA"
    );

    CHECKED_MSG(Session->NumberOfArguments == 2,
                "Invalid usage.  Usage: TracerDebugEngine <pid>");

    //
    // Extract the process ID to attach to.
    //

    CHECKED_NTSTATUS_MSG(
        Rtl->RtlCharToInteger(
            Session->ArgvA[1],
            10,
            &Session->TargetProcessId
        ),
        "Rtl->RtlCharToInteger(ArgvA[1])"
    );

    //
    // Try open a handle to the process with all access.
    //

    Session->TargetProcessHandle = OpenProcess(PROCESS_ALL_ACCESS,
                                               TRUE,
                                               Session->TargetProcessId);
    if (!Session->TargetProcessHandle) {
        OutputDebugStringA("DbgEng:OpenProcess(PROCESS_ALL_ACCESS) failed.");
    }

    //
    // Set the start engine method.
    //

    Session->Start = StartDebugEngineSession;

    //
    // Attach to the process with the debug client.
    //

    Engine = &Session->Engine;
    Engine->SizeOfStruct = sizeof(*Engine);
    Client = Engine->Client;
    IClient = Engine->IClient;

    CHECKED_MSG(InitializeCallbacks(Session), "InitializeCallbacks()");

    CHECKED_HRESULT_MSG(
        Client->AttachProcess(
            IClient,
            0,
            Session->TargetProcessId,
            //DEBUG_ATTACH_INVASIVE_RESUME_PROCESS
            //DEBUG_ATTACH_DEFAULT
            DEBUG_ATTACH_NONINVASIVE |
            DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND
        ),
        "IDebugClient->AttachProcess()"
    );

    Control = Engine->Control;
    IControl = Engine->IControl;

    Symbols = Engine->Symbols;
    ISymbols = Engine->ISymbols;

    Result = Control->WaitForEvent(IControl, 0, 0);

    CHECKED_HRESULT_MSG(
        Control->GetExecutionStatus(IControl, &Engine->ExecutionStatus.LowPart),
        "Control->GetExecutionStatus()"
    );

    Result = Symbols->GetModuleByModuleName(
        ISymbols,
        "python27",
        0,
        &Index,
        &Base
    );

    Result = Symbols->GetOffsetByName(
        ISymbols,
        "python27!Py_IncRef",
        &Offset
    );

    Result = Symbols->GetOffsetByName(
        ISymbols,
        "python27!PyGC_Collect",
        &Offset2
    );


    Result = Control->ExecuteWide(
        IControl,
        DEBUG_OUTCTL_ALL_CLIENTS,
        UnassemblePy_IncRef,
        DEBUG_EXECUTE_ECHO
    );

    OutputDebugStringA("XXX");

    CHECKED_HRESULT_MSG(
        Control->SetAssemblyOptions(
            IControl,
            DEBUG_ASMOPT_IGNORE_OUTPUT_WIDTH |
            DEBUG_ASMOPT_SOURCE_LINE_NUMBER
        ),
        "Control->SetAssemblyOptions()"
    );

    /*
    Result = Control->ExecuteWide(
        IControl,
        DEBUG_OUTCTL_ALL_CLIENTS,
        ListPy27Functions,
        DEBUG_EXECUTE_ECHO
    );
    */

    SecureZeroMemory(&Buffer, sizeof(Buffer));

    Dest = Buffer;

    Result = Control->Disassemble(IControl,
                                  Offset,
                                  Flags,
                                  Dest,
                                  BufferSize,
                                  &DisassemblySize,
                                  &EndOffset);
    if (FAILED(Result)) {
        OutputDebugStringA("Failed: Control->Disassemble()\n");
    }

    Result = Control->GetDisassembleEffectiveOffset(
        IControl,
        &EffectiveOffset
    );

    Dest += DisassemblySize;

    Result = Control->Disassemble(IControl,
                                  EndOffset+1,
                                  Flags,
                                  Dest,
                                  BufferSize - DisassemblySize,
                                  &DisassemblySize,
                                  &EndOffset);
    if (FAILED(Result)) {
        OutputDebugStringA("Failed: Control->Disassemble()\n");
    }

    Result = Control->OutputDisassemblyLines(IControl,
                                             DEBUG_OUTCTL_ALL_CLIENTS,
                                             2,
                                             5,
                                             Offset2,
                                             DisassemblyFlags,
                                             &OffsetLine,
                                             &StartOffset,
                                             &EndOffset,
                                             &LineOffsets);

    if (FAILED(Result)) {
        OutputDebugStringA("Failed: Control->OutputDisassemblyLines()\n");
    }

    return TRUE;

    //
    // AttachProcess work, create a debug control object.
    //

    //CHECKED_MSG(CreateDebugInterfaces(Session),
    //            "CreateDebugInterfaces(Session)");

    CHECKED_MSG(InitializeCallbacks(Session), "InitializeCallbacks()");

    Result = Control->WaitForEvent(IControl, 0, 0);

    CHECKED_HRESULT_MSG(
        Control->GetExecutionStatus(IControl, &Engine->ExecutionStatus.LowPart),
        "Control->GetExecutionStatus()"
    );

    Result = Control->GetActualProcessorType(
        IControl,
        &Engine->ActualProcessorType
    );
    if (FAILED(Result)) {
        OutputDebugStringA("Failed: Control->GetActualProcessorType().\n");
    }

    CHECKED_HRESULT_MSG(
        Control->SetInterrupt(IControl, DEBUG_INTERRUPT_ACTIVE),
        "Control->SetInterrupt(DEBUG_INTERRUPT_ACTIVE)"
    );

    CHECKED_HRESULT_MSG(
        Control->GetExecutionStatus(IControl, &Engine->ExecutionStatus.LowPart),
        "Control->GetExecutionStatus()"
    );

    while (Engine->ExecutionStatus.LowPart != DEBUG_STATUS_BREAK) {
        CHECKED_HRESULT_MSG(
            Client->DispatchCallbacks(IClient, INFINITE),
            "Client->DispatchCallbacks(INFINITE)"
        );
    }

    //
    // We've got a debug break.
    //

    //
    // Set the start pointer.
    //

    Session->Start = StartDebugEngineSession;

    //
    // Try disassemble a known address.
    //

    if (1) {
        //ULONG Flags = DEBUG_DISASM_EFFECTIVE_ADDRESS;
        ULONG Flags = 0;
        ULONG64 Address = (ULONG64)0x000000001e0c5450;
        CHAR Buffer[256];
        ULONG BufferSize = sizeof(Buffer);
        ULONG DisassemblySize;
        ULONG OffsetLine;
        ULONG64 StartOffset;
        ULONG64 EndOffset;
        ULONG64 LineOffsets;

        SecureZeroMemory(&Buffer, sizeof(Buffer));

        Result = Control->Disassemble(IControl,
                                      Address,
                                      Flags,
                                      (PSTR)&Buffer,
                                      BufferSize,
                                      &DisassemblySize,
                                      &EndOffset);
        if (FAILED(Result)) {
            OutputDebugStringA("Failed: Control->Disassemble()\n");
        }

        Result = Control->OutputDisassemblyLines(IControl,
                                                 DEBUG_OUTCTL_THIS_CLIENT,
                                                 2,
                                                 5,
                                                 Address,
                                                 DEBUG_DISASM_MATCHING_SYMBOLS,
                                                 &OffsetLine,
                                                 &StartOffset,
                                                 &EndOffset,
                                                 &LineOffsets);

        if (FAILED(Result)) {
            OutputDebugStringA("Failed: Control->OutputDisassemblyLines()\n");
        }

    }

    {
        PWSTR Command = L"uf python27!PyGC_Collect";
        Result = Control->ExecuteWide(IControl,
                                      DEBUG_OUTCTL_ALL_CLIENTS,
                                      Command,
                                      DEBUG_EXECUTE_DEFAULT);

        if (Result != S_OK) {
            OutputDebugStringA("ExecuteWide() failed.");
        }
    }

    //
    // Update the caller's pointer.
    //

    *SessionPointer = Session;

    Success = TRUE;
    goto End;

Error:
    Success = FALSE;

    if (Session) {
        MAYBE_CLOSE_HANDLE(Session->TargetProcessHandle);
    }

    //MAYBE_FREE_POINTER(Session, Allocator);

End:
    MAYBE_FREE_LIBRARY(Shell32Module);

    return Success;
}

_Use_decl_annotations_
VOID
DestroyDebugEngineSession(
    PPDEBUG_ENGINE_SESSION SessionPointer
    )
/*++

Routine Description:

    This routine destroys a previously created debug engine session.

Arguments:

    SessionPointer - Supplies the address of a variable that contains the
        address of the DEBUG_ENGINE_SESSION structure to destroy.  This pointer
        will be cleared by this routine.

Return Value:

    None.

--*/
{
    return;
}


BOOL
InitializeDebugEngine(
    PRTL Rtl,
    PDEBUG_ENGINE Engine
    )
/*++

Routine Description:

    This routine initializes a DEBUG_ENGINE structure.

Arguments:

    Rtl - Supplies a pointer to an RTL structure.

    Engine - Supplies a pointer to a DEBUG_ENGINE structure to initialize.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    BOOL Success;
    HRESULT Result;

    //
    // Initialize COM.
    //

    CHECKED_HRESULT_MSG(
        Rtl->CoInitializeEx(
            NULL,
            COINIT_APARTMENTTHREADED
        ),
        "Rtl->CoInitializeEx()"
    );

#define CREATE_INTERFACE(Name, Upper)                        \
    CHECKED_HRESULT_MSG(                                     \
        Rtl->DebugCreate(                                    \
            &IID_IDEBUG_##Upper,                             \
            &Engine->I##Name                                 \
        ),                                                   \
        "Rtl->DebugCreate(IID_IDEBUG_" #Upper ")"            \
    );                                                       \
    Engine->##Name = (PDEBUG##Upper)Engine->I##Name->lpVtbl; \
    Engine->IID_##Name = &IID_IDEBUG_##Upper

    CREATE_INTERFACE(Client, CLIENT);
    CREATE_INTERFACE(Control, CONTROL);
    CREATE_INTERFACE(Symbols, SYMBOLS);
    CREATE_INTERFACE(Advanced, ADVANCED);
    CREATE_INTERFACE(DataSpaces, DATASPACES);

    Success = TRUE;
    goto End;

Error:
    Success = FALSE;


End:
    return Success;
}

_Use_decl_annotations_
BOOL
CreateDebugInterfaces(
    PDEBUG_ENGINE_SESSION Session
    )
/*++

Routine Description:

    Creates an IDebugControl object.

Arguments:

    Session - Supplies a pointer to a DEBUG_ENGINE_SESSION structure.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    BOOL Success;
    HRESULT Result;
    PDEBUG_ENGINE Engine;
    PDEBUGCLIENT Client;
    PIDEBUGCLIENT IClient;
    PDEBUGCONTROL Control;
    PIDEBUGCONTROL IControl;

    //
    // Create the IDebugControl interface.
    //

    Engine = &Session->Engine;
    Client = Engine->Client;
    IClient = Engine->IClient;

    CHECKED_HRESULT_MSG(
        Client->QueryInterface(
            IClient,
            (REFIID)&IID_IDEBUG_CONTROL,
            &IControl
        ),
        "Client->QueryInterface(&IID_IDEBUG_CONTROL)"
    );

    Control = (PDEBUGCONTROL)IControl->lpVtbl;

    Engine->IID_Control = &IID_IDEBUG_CONTROL;
    Engine->IControl = IControl;
    Engine->Control = Control;

    Success = TRUE;
    goto End;

Error:
    Success = FALSE;

End:
    return Success;
}

_Use_decl_annotations_
BOOL
InitializeCallbacks(
    PDEBUG_ENGINE_SESSION Session
    )
/*++

Routine Description:

    This routine initializes callbacks.

Arguments:

    Session - Supplies a pointer to a DEBUG_ENGINE_SESSION.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    HRESULT Result;
    PDEBUG_ENGINE Engine;
    PDEBUGEVENTCALLBACKS EventCallbacks;
    PIDEBUGEVENTCALLBACKS IEventCallbacks;
    PDEBUGOUTPUTCALLBACKS OutputCallbacks;
    PIDEBUGOUTPUTCALLBACKS IOutputCallbacks;
    PDEBUGINPUTCALLBACKS InputCallbacks;
    PIDEBUGINPUTCALLBACKS IInputCallbacks;

    Engine = &Session->Engine;

    //
    // Register for all event callbacks.
    //

    Engine->EventCallbacksInterestMask = (
        DEBUG_EVENT_BREAKPOINT              |
        DEBUG_EVENT_EXCEPTION               |
        DEBUG_EVENT_CREATE_THREAD           |
        DEBUG_EVENT_EXIT_THREAD             |
        DEBUG_EVENT_CREATE_PROCESS          |
        DEBUG_EVENT_EXIT_PROCESS            |
        DEBUG_EVENT_LOAD_MODULE             |
        DEBUG_EVENT_UNLOAD_MODULE           |
        DEBUG_EVENT_SYSTEM_ERROR            |
        DEBUG_EVENT_SESSION_STATUS          |
        DEBUG_EVENT_CHANGE_DEBUGGEE_STATE   |
        DEBUG_EVENT_CHANGE_ENGINE_STATE     |
        DEBUG_EVENT_CHANGE_SYMBOL_STATE
    );

    EventCallbacks = &Engine->EventCallbacks;
    CopyIDebugEventCallbacks(EventCallbacks);

    IEventCallbacks = &Engine->IEventCallbacks;
    IEventCallbacks->lpVtbl = EventCallbacks;

    CHECKED_HRESULT_MSG(
        Engine->Client->SetEventCallbacksWide(
            Engine->IClient,
            IEventCallbacks
        ),
        "Client->SetEventCallbacks()"
    );

    //
    // Initialize output callbacks.
    //

    Engine->OutputCallbacksInterestMask = DEBUG_OUTCBI_TEXT;
    OutputCallbacks = &Engine->OutputCallbacks;
    CopyIDebugOutputCallbacks(OutputCallbacks);

    IOutputCallbacks = &Engine->IOutputCallbacks;
    IOutputCallbacks->lpVtbl = OutputCallbacks;

    CHECKED_HRESULT_MSG(
        Engine->Client->SetOutputCallbacks(
            Engine->IClient,
            (PDEBUG_OUTPUT_CALLBACKS)IOutputCallbacks
        ),
        "Client->SetOutputCallbacks()"
    );

    return TRUE;

    //
    // Initialize input callbacks.
    //

    InputCallbacks = &Engine->InputCallbacks;
    CopyIDebugInputCallbacks(InputCallbacks);

    IInputCallbacks = &Engine->IInputCallbacks;
    IInputCallbacks->lpVtbl = InputCallbacks;

    CHECKED_HRESULT_MSG(
        Engine->Client->SetInputCallbacks(
            Engine->IClient,
            IInputCallbacks
        ),
        "Client->SetInputCallbacks()"
    );

    //
    // We're done, return success.
    //

    return TRUE;

Error:
    return FALSE;
}

_Use_decl_annotations_
BOOL
StartDebugEngineSession(
    PDEBUG_ENGINE_SESSION Session
    )
/*++

Routine Description:

    This routine starts a debug engine session.

Arguments:

    Session - Supplies a pointer to a DEBUG_ENGINE_SESSION.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Debug Callbacks
////////////////////////////////////////////////////////////////////////////////

//
// Helper macros.
//

#define DEBUG_CALLBACK_PROLOGUE(Name)                                        \
    PDEBUG_ENGINE Engine;                                                    \
    Engine = CONTAINING_RECORD(This->lpVtbl, DEBUG_ENGINE, Name##Callbacks);

#define DEBUG_ADD_REF(Name) \
    InterlockedIncrement(&Engine->##Name##CallbackRefCount)

#define DEBUG_RELEASE(Name)                                 \
    InterlockedIncrement(&Engine->##Name##CallbackRefCount)

#define DEBUG_QUERY_INTERFACE(Name, Upper)                                 \
    if (InlineIsEqualGUID(InterfaceId, &IID_IUNKNOWN) ||                   \
        InlineIsEqualGUID(InterfaceId, &IID_IDEBUG_##Upper##_CALLBACKS)) { \
        InterlockedIncrement(&Engine->##Name##CallbackRefCount);           \
        *Interface = &Engine->I##Name##Callbacks;                          \
        return S_OK;                                                       \
    }                                                                      \
    *Interface = NULL

////////////////////////////////////////////////////////////////////////////////
// IDebugEventCallbacks
////////////////////////////////////////////////////////////////////////////////

#define DEBUG_EVENT_CALLBACK_PROLOGUE() DEBUG_CALLBACK_PROLOGUE(Event)
#define DEBUG_EVENT_QUERY_INTERFACE() DEBUG_QUERY_INTERFACE(Event, EVENT)
#define DEBUG_EVENT_ADD_REF() DEBUG_ADD_REF(Event)
#define DEBUG_EVENT_RELEASE() DEBUG_RELEASE(Event)

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventQueryInterface(
    PIDEBUGEVENTCALLBACKS This,
    REFIID InterfaceId,
    PPVOID Interface
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();
    DEBUG_EVENT_QUERY_INTERFACE();
    return E_NOINTERFACE;
}

_Use_decl_annotations_
ULONG
STDAPICALLTYPE
DebugEventAddRef(
    PIDEBUGEVENTCALLBACKS This
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();
    return DEBUG_EVENT_ADD_REF();
}

_Use_decl_annotations_
ULONG
STDAPICALLTYPE
DebugEventRelease(
    PIDEBUGEVENTCALLBACKS This
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();
    return DEBUG_EVENT_RELEASE();
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->GetInterestMask()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventGetInterestMaskCallback(
    PIDEBUGEVENTCALLBACKS This,
    PULONG Mask
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    *Mask = Engine->EventCallbacksInterestMask;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->Breakpoint()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventBreakpointCallback(
    PIDEBUGEVENTCALLBACKS This,
    PDEBUG_BREAKPOINT2 Breakpoint
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    return DEBUG_STATUS_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->Exception()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventExceptionCallback(
    PIDEBUGEVENTCALLBACKS This,
    PEXCEPTION_RECORD64 Exception,
    ULONG FirstChance
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    return DEBUG_STATUS_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->CreateThread()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventCreateThreadCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG64 Handle,
    ULONG64 DataOffset,
    ULONG64 StartOffset
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    return DEBUG_STATUS_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->ExitThread()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventExitThreadCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG ExitCode
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    return DEBUG_STATUS_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->CreateProcess()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
DebugEventCreateProcessCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG64 ImageFileHandle,
    ULONG64 Handle,
    ULONG64 BaseOffset,
    ULONG ModuleSize,
    PCWSTR ModuleName,
    PCWSTR ImageName,
    ULONG CheckSum,
    ULONG TimeDateStamp,
    ULONG64 InitialThreadHandle,
    ULONG64 ThreadDataOffset,
    ULONG64 StartOffset
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    return DEBUG_STATUS_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->ExitProcess()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventExitProcessCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG ExitCode
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    return DEBUG_STATUS_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->LoadModule()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventLoadModuleCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG64 ImageFileHandle,
    ULONG64 BaseOffset,
    ULONG ModuleSize,
    PCWSTR ModuleName,
    PCWSTR ImageName,
    ULONG CheckSum,
    ULONG TimeDateStamp
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    return DEBUG_STATUS_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->UnloadModule()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
DebugEventUnloadModuleCallback(
    PIDEBUGEVENTCALLBACKS This,
    PCWSTR ImageBaseName,
    ULONG64 BaseOffset
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    return DEBUG_STATUS_NO_CHANGE;
}


////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->SystemError()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventSystemErrorCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG Error,
    ULONG Level
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    return DEBUG_STATUS_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->SessionStatus()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventSessionStatusCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG Status
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    Engine->SessionStatus = Status;

    return DEBUG_STATUS_NO_CHANGE;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->ChangeDebuggeeState()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventChangeDebuggeeStateCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG Flags,
    ULONG64 Argument
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    Engine->DebuggeeState = Flags;
    Engine->DebuggeeStateArg = Argument;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->ChangeEngineState()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventChangeEngineStateCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG Flags,
    ULONG64 Argument
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    Engine->ChangeEngineState.AsULong = Flags;

    if (Engine->ChangeEngineState.ExecutionStatus) {
        Engine->ExecutionStatus.AsULongLong = Argument;
        if (Engine->ExecutionStatus.InsideWait) {
            //OutputDebugStringA("ChangeEngineState()->ExecutionStatus->InWait");
        }
    }

    Engine->EngineState = Flags;
    Engine->EngineStateArg = Argument;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//      IDebugEventCallbacks->ChangeSymbolState()
////////////////////////////////////////////////////////////////////////////////

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugEventChangeSymbolStateCallback(
    PIDEBUGEVENTCALLBACKS This,
    ULONG Flags,
    ULONG64 Argument
    )
{
    DEBUG_EVENT_CALLBACK_PROLOGUE();

    Engine->SymbolState = Flags;
    Engine->SymbolStateArg = Argument;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// IDebugOutputCallbacks
////////////////////////////////////////////////////////////////////////////////

#define DEBUG_OUTPUT_CALLBACK_PROLOGUE() DEBUG_CALLBACK_PROLOGUE(Output)
#define DEBUG_OUTPUT_QUERY_INTERFACE() DEBUG_QUERY_INTERFACE(Output, OUTPUT)
#define DEBUG_OUTPUT_ADD_REF() DEBUG_ADD_REF(Output)
#define DEBUG_OUTPUT_RELEASE() DEBUG_RELEASE(Output)

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugOutputQueryInterface(
    PIDEBUGOUTPUTCALLBACKS This,
    REFIID InterfaceId,
    PPVOID Interface
    )
{
    DEBUG_OUTPUT_CALLBACK_PROLOGUE();
    DEBUG_OUTPUT_QUERY_INTERFACE();
    return E_NOINTERFACE;
}

_Use_decl_annotations_
ULONG
STDAPICALLTYPE
DebugOutputAddRef(
    PIDEBUGOUTPUTCALLBACKS This
    )
{
    DEBUG_OUTPUT_CALLBACK_PROLOGUE();
    return DEBUG_OUTPUT_ADD_REF();
}

_Use_decl_annotations_
ULONG
STDAPICALLTYPE
DebugOutputRelease(
    PIDEBUGOUTPUTCALLBACKS This
    )
{
    DEBUG_OUTPUT_CALLBACK_PROLOGUE();
    return DEBUG_OUTPUT_RELEASE();
}

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugOutputOutputCallback(
    PIDEBUGOUTPUTCALLBACKS This,
    ULONG Mask,
    PCSTR String
    )
{
    OutputDebugStringA(String);
    return S_OK;
}

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugOutputGetInterestMaskCallback(
    PIDEBUGOUTPUTCALLBACKS This,
    PULONG Mask
    )
{
    DEBUG_OUTPUT_CALLBACK_PROLOGUE();

    *Mask = Engine->OutputCallbacksInterestMask;
    return S_OK;
}

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugOutputOutput2Callback(
    PIDEBUGOUTPUTCALLBACKS This,
    ULONG Which,
    ULONG Flags,
    ULONG64 Args,
    PCWSTR String
    )
{
    DEBUG_OUTPUT_CALLBACK_PROLOGUE();

    if (Which == DEBUG_OUTCB_TEXT && String) {
        OutputDebugStringW(String);
    }

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// IDebugInputCallbacks
////////////////////////////////////////////////////////////////////////////////

#define DEBUG_INPUT_CALLBACK_PROLOGUE() DEBUG_CALLBACK_PROLOGUE(Input)
#define DEBUG_INPUT_QUERY_INTERFACE() DEBUG_QUERY_INTERFACE(Input, INPUT)
#define DEBUG_INPUT_ADD_REF() DEBUG_ADD_REF(Input)
#define DEBUG_INPUT_RELEASE() DEBUG_RELEASE(Input)

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugInputQueryInterface(
    PIDEBUGINPUTCALLBACKS This,
    REFIID InterfaceId,
    PPVOID Interface
    )
{
    DEBUG_INPUT_CALLBACK_PROLOGUE();
    DEBUG_INPUT_QUERY_INTERFACE();
    return E_NOINTERFACE;
}

_Use_decl_annotations_
ULONG
STDAPICALLTYPE
DebugInputAddRef(
    PIDEBUGINPUTCALLBACKS This
    )
{
    DEBUG_INPUT_CALLBACK_PROLOGUE();
    return DEBUG_INPUT_ADD_REF();
}

_Use_decl_annotations_
ULONG
STDAPICALLTYPE
DebugInputRelease(
    PIDEBUGINPUTCALLBACKS This
    )
{
    DEBUG_INPUT_CALLBACK_PROLOGUE();
    return DEBUG_INPUT_RELEASE();
}

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugInputStartInputCallback(
    PIDEBUGINPUTCALLBACKS This,
    ULONG BufferSize
    )
{
    OutputDebugStringA("Entered DebugStartInputCallback().\n");
    return S_OK;
}

_Use_decl_annotations_
HRESULT
STDAPICALLTYPE
DebugInputEndInputCallback(
    PIDEBUGINPUTCALLBACKS This
    )
{
    OutputDebugStringA("Entered DebugEndInputCallback().\n");
    return S_OK;
}

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :