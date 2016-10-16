/*++

Copyright (c) 2016 Trent Nelson <trent@trent.me>

Module Name:

    TraceStoreTraits.c

Abstract:

    This module implements functionality related to trace store traits.

--*/

#include "stdafx.h"

_Use_decl_annotations_
BOOL
InitializeTraceStoreTraits(
    PTRACE_STORE TraceStore
    )
/*++

Routine Description:

    This routine initializes trace store traits for a trace store.  If the
    store is readonly, the traits will be read from the TRACE_STORE_TRAITS
    structure stored in the info metadata store via the LoadTraceStoreTraits()
    routine.  If the trace store is not readonly, the traits will be obtained
    from the TraceStoreTraits[] array.

    N.B.: The description above is how the routine *should* work.  How it
          currently works is less sophisticated: it always obtains the traits
          from the TraceStoreTraits[] array and updates TraceStore->pTraits
          accordingly.

          The split between initialization and binding to a context needs to
          be refactored before the logic can above can be implemented cleanly.

Arguments:

    TraceStore - Supplies a pointer to a TRACE_STORE structure.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    USHORT Index;

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(TraceStore)) {
        return FALSE;
    }

    //
    // Make sure we're not a metadata store; they load traits via INIT_METADATA
    // macro.
    //

    if (TraceStore->IsMetadata) {
        return FALSE;
    }

    //
    // Get the array index for this trace store, then load the traits from
    // the static array.
    //

    Index = TraceStoreIdToArrayIndex(TraceStore->TraceStoreId);
    TraceStore->pTraits = (PTRACE_STORE_TRAITS)&TraceStoreTraits[Index];

    return TRUE;
}

_Use_decl_annotations_
BOOL
SaveTraceStoreTraits(
    PTRACE_STORE TraceStore
    )
/*++

Routine Description:

    This routine saves the traits of a trace store into the TRACE_STORE_TRAITS
    structure of the metadata Info store.  The trace store must not be set to
    readonly.

Arguments:

    TraceStore - Supplies a pointer to a TRACE_STORE structure that is not
        readonly.

Return Value:

    TRUE on success, FALSE on failure.  Failure will be because the trace
    store is marked readonly or the TraceStore was invalid.

--*/
{
    USHORT Index;

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(TraceStore)) {
        return FALSE;
    }

    if (TraceStore->IsReadonly) {
        return FALSE;
    }

    //
    // Get the array index for this trace store, then load the traits from
    // the static array.
    //

    Index = TraceStoreIdToArrayIndex(TraceStore->TraceStoreId);
    TraceStore->pTraits = (PTRACE_STORE_TRAITS)&TraceStoreTraits[Index];

    return TRUE;
}

_Use_decl_annotations_
BOOL
LoadTraceStoreTraits(
    PTRACE_STORE TraceStore
    )
/*++

Routine Description:

    This routine loads trace store traits from a trace store's :info metadata.

Arguments:

    TraceStore - Supplies a pointer to a TRACE_STORE structure that is marked
        as readonly.

Return Value:

    TRUE on success, FALSE on failure.  Failure will be because the trace
    store is not marked as readonly or the TraceStore was invalid.

--*/
{
    USHORT Index;

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(TraceStore)) {
        return FALSE;
    }

    if (!TraceStore->IsReadonly) {
        return FALSE;
    }

    //
    // Compatibility shim: if the size of the TRACE_STORE_INFO structure is
    // 128, it will not contain the TRACE_STORE_TRAITS field.  In this case,
    // just load the traits from the TraceStoreTraits array.
    //

    if (sizeof(TRACE_STORE_INFO) == 128) {
        Index = TraceStoreIdToArrayIndex(TraceStore->TraceStoreId);
        TraceStore->pTraits = (PTRACE_STORE_TRAITS)&TraceStoreTraits[Index];
        *TraceStore->Traits = *TraceStore->pTraits;
    }

    //
    // Get the array index for this trace store, then load the traits from
    // the static array.
    //

    return TRUE;
}


// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
