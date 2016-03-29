// Copyright(c) Trent Nelson <trent@trent.me>
// All rights reserved.

#pragma once

#ifdef __cpplus
extern "C" {
#endif

#include <Windows.h>
#include "../Rtl/Rtl.h"
#include "../Tracer/Tracer.h"
#include "../Tracer/Tracing.h"
#include "../Python/Python.h"

typedef enum _TraceEventType {
    // PyTrace_* constants.
    TraceEventType_PyTrace_CALL = 0,
    TraceEventType_PyTrace_EXCEPTION = 1,
    TraceEventType_PyTrace_LINE = 2,
    TraceEventType_PyTrace_RETURN = 3,
    TraceEventType_PyTrace_C_CALL = 4,
    TraceEventType_PyTrace_C_EXCEPTION = 5,
    TraceEventType_PyTrace_C_RETURN = 6,
} TraceEventType;

typedef struct _EVENT_TYPE {
    TraceEventType  Id;
    PCWSTR          Name;
    PCSTR           NameA;
} EVENT_TYPE, *PEVENT_TYPE;

static const EVENT_TYPE EventTypes[] = {
    { TraceEventType_PyTrace_CALL,          L"PyTrace_CALL",        "PyTrace_CALL" },
    { TraceEventType_PyTrace_EXCEPTION,     L"PyTrace_EXCEPTION",   "PyTrace_EXCEPTION" },
    { TraceEventType_PyTrace_LINE,          L"PyTrace_LINE",        "PyTrace_LINE" },
    { TraceEventType_PyTrace_RETURN,        L"PyTrace_RETURN",      "PyTrace_RETURN" },
    { TraceEventType_PyTrace_C_CALL,        L"PyTrace_C_CALL",      "PyTrace_C_CALL" },
    { TraceEventType_PyTrace_C_EXCEPTION,   L"PyTrace_C_EXCEPTION", "PyTrace_C_EXCEPTION" },
    { TraceEventType_PyTrace_C_RETURN,      L"PyTrace_C_RETURN",    "PyTrace_C_RETURN" },
};

static const DWORD NumberOfTraceEventTypes = (
    sizeof(EventTypes) /
    sizeof(EVENT_TYPE)
);

typedef struct _PYTHON_MODULE {
    PPYOBJECT ModuleFilenameObject;
    PUNICODE_STRING Name;
    PUNICODE_STRING Path;
} PYTHON_MODULE, *PPYTHON_MODULE;

typedef struct _PYTHON_FUNCTION {
    PPYOBJECT CodeObject;
    PUNICODE_STRING ClassName;
    PUNICODE_STRING FunctionName;
    PUNICODE_STRING ModuleName;
    PUNICODE_STRING FileName;
} PYTHON_FUNCTION, *PPYTHON_FUNCTION;

typedef struct _PYTHON_TRACE_CONTEXT {
    ULONG             Size;
    PRTL              Rtl;
    PPYTHON           Python;
    PTRACE_CONTEXT    TraceContext;
    PPYTRACEFUNC      PythonTraceFunction;
    PVOID             UserData;
    PPYFUNCTIONOBJECT FunctionObject;
    PRTL_GENERIC_TABLE FunctionsTable;
    PRTL_GENERIC_TABLE ModulesTable;

} PYTHON_TRACE_CONTEXT, *PPYTHON_TRACE_CONTEXT;

TRACER_API
LONG
PyTraceCallbackDummy(
    _In_        PPYTHON_TRACE_CONTEXT   PythonTraceContext,
    _In_        PPYFRAMEOBJECT          FrameObject,
    _In_opt_    LONG                    EventType,
    _In_opt_    PPYOBJECT               ArgObject
);

TRACER_API
LONG
PyTraceCallbackBasic(
    _In_        PPYTHON_TRACE_CONTEXT   PythonTraceContext,
    _In_        PPYFRAMEOBJECT          FrameObject,
    _In_opt_    LONG                    EventType,
    _In_opt_    PPYOBJECT               ArgObject
);

TRACER_API
LONG
PyTraceCallbackFast(
    _In_        PPYTHON_TRACE_CONTEXT   PythonTraceContext,
    _In_        PPYFRAMEOBJECT          FrameObject,
    _In_opt_    LONG                    EventType,
    _In_opt_    PPYOBJECT               ArgObject
);


TRACER_API
BOOL
InitializePythonTraceContext(
    _In_                                        PRTL                    Rtl,
    _Out_bytecap_(*SizeOfPythonTraceContext)    PPYTHON_TRACE_CONTEXT   PythonTraceContext,
    _Inout_                                     PULONG                  SizeOfPythonTraceContext,
    _In_                                        PPYTHON                 Python,
    _In_                                        PTRACE_CONTEXT          TraceContext,
    _In_opt_                                    PPYTRACEFUNC            PythonTraceFunction,
    _In_opt_                                    PVOID                   UserData
);

TRACER_API
BOOL
StartTracing(
    _In_    PPYTHON_TRACE_CONTEXT   PythonTraceContext
);

TRACER_API
BOOL
StopTracing(
    _In_    PPYTHON_TRACE_CONTEXT   PythonTraceContext
);

TRACER_API
BOOL
StartProfiling(
    _In_    PPYTHON_TRACE_CONTEXT   PythonTraceContext
);

TRACER_API
BOOL
StopProfiling(
    _In_    PPYTHON_TRACE_CONTEXT   PythonTraceContext
);

TRACER_API
BOOL
AddFunction(
    _In_    PPYTHON_TRACE_CONTEXT   PythonTraceContext,
    _In_    PVOID                   FunctionObject
);
