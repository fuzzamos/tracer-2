/*++

Copyright (c) 2017 Trent Nelson <trent@trent.me>

Module Name:

    DebugEngineConstants.c

Abstract:

    This module defines constants used by the DebugEngine component.

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdafx.h"

#pragma component(browser, off)
DEBUG_ENGINE_DATA CGUID IID_IUNKNOWN;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_ADVANCED;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_SYMBOLS;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_SYMBOLGROUP;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_DATASPACES;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_CLIENT;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_CONTROL;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_REGISTERS;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_EVENT_CALLBACKS;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_INPUT_CALLBACKS;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_OUTPUT_CALLBACKS;
DEBUG_ENGINE_DATA CGUID IID_IDEBUG_OUTPUT_CALLBACKS2;

extern CONST DEBUGEVENTCALLBACKS DebugEventCallbacks;
extern CONST DEBUGINPUTCALLBACKS DebugInputCallbacks;
extern CONST DEBUGOUTPUTCALLBACKS DebugOutputCallbacks;
extern CONST DEBUGOUTPUTCALLBACKS2 DebugOutputCallbacks2;

DEBUG_ENGINE_DATA CONST CHAR StringTableDelimiter;
DEBUG_ENGINE_DATA CONST STRING ExamineSymbolsPrefixes;
DEBUG_ENGINE_DATA CONST STRING ExamineSymbolsBasicTypes1;
DEBUG_ENGINE_DATA CONST STRING ExamineSymbolsBasicTypes2;
DEBUG_ENGINE_DATA CONST STRING FunctionArgumentTypes1;
DEBUG_ENGINE_DATA CONST STRING FunctionArgumentTypes2;
DEBUG_ENGINE_DATA CONST STRING FunctionArgumentVectorTypes1;

DEBUG_ENGINE_DATA CONST STRING CommandLineOptions;
DEBUG_ENGINE_DATA CONST USHORT NumberOfCommandLineMatchIndexOptions;
DEBUG_ENGINE_DATA CONST SHORT CommandLineMatchIndexToOption[];

DEBUG_ENGINE_DATA CONST STRING DisplayTypeCustomStructureName;
DEBUG_ENGINE_DATA CONST STRING ExamineSymbolsCustomStructureName;
DEBUG_ENGINE_DATA CONST STRING UnassembleFunctionCustomStructureName;

#pragma component(browser, on)

#ifdef __cplusplus
}; // extern "C"
#endif

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
