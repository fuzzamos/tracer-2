/*++

Copyright (c) 2017 Trent Nelson <trent@trent.me>

Module Name:

    DebugEngineOutput.c

Abstract:

    This module implements functionality related to the DEBUG_ENGINE_OUTPUT
    structure.

--*/

#include "stdafx.h"

_Use_decl_annotations_
BOOL
InitializeDebugEngineOutput(
    PDEBUG_ENGINE_OUTPUT Output,
    PDEBUG_ENGINE_SESSION DebugEngineSession,
    PALLOCATOR Allocator,
    PDEBUG_ENGINE_PARTIAL_OUTPUT_CALLBACK PartialOutputCallback,
    PDEBUG_ENGINE_OUTPUT_COMPLETE_CALLBACK OutputCompleteCallback,
    PVOID Context,
    PRTL_PATH ModulePath
    )
/*++

Routine Description:

    This routine initializes a DEBUG_ENGINE_OUTPUT structure.  It is called
    prior to preparing and executing a command.  It serves as the mechanism
    to persist and encapsulate state when processing multiple output callbacks
    from the debug engine for a given command.

Arguments:

    Output - Supplies a pointer to the DEBUG_ENGINE_OUTPUT structure to
        initialize.  The SizeOfStruct field must be set to the correct size
        (i.e. equal to sizeof(DEBUG_ENGINE_OUTPUT)) otherwise an error will
        be returned.

    DebugEngineSession - Supplies a pointer to the debug engine session to
        associate with this output.

    Allocator - Supplies a pointer to an ALLOCATOR structure that the debug
        engine can use for temporary allocations (such as string buffer
        allocation when constructing dynamic commands).

    PartialOutputCallback - Supplies a pointer to a callback that will be
        invoked one or more times as the debug engine generates output.

    OutputCompleteCallback - Supplies a pointer to a callback that will be
        invoked once after a command has finished executing.

    Context - Optionally supplies an opaque context pointer that will be
        stored in the DEBUG_ENGINE_OUTPUT structure.

    ModulePath - Optionally supplies a pointer to an RTL_PATH structure that
        represents the module associated with a given command, if applicable.

Return Value:

    TRUE on Success, FALSE if an error occurred.

--*/
{
    //
    // Validate size.
    //

    if (Output->SizeOfStruct != sizeof(*Output)) {
        return FALSE;
    }

    //
    // Clear the entire structure.
    //

    SecureZeroMemory(Output, sizeof(*Output));

    //
    // Initialize fields.
    //

    Output->SizeOfStruct = sizeof(*Output);
    Output->Allocator = Allocator;
    Output->Session = DebugEngineSession;
    Output->PartialOutputCallback = PartialOutputCallback;
    Output->OutputCompleteCallback = OutputCompleteCallback;

    //
    // Set the optional fields if applicable.
    //

    if (ARGUMENT_PRESENT(Context)) {
        Output->Context = Context;
    }

    if (ARGUMENT_PRESENT(ModulePath)) {
        Output->ModulePath = ModulePath;
    }

    Output->State.Initialized = TRUE;

    //
    // Return success.
    //

    return TRUE;
}

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
