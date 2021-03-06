#include "stdafx.h"

_Use_decl_annotations_
BOOL
TlsTracerHeapThreadAttach(
    HMODULE Module,
    ULONG   Reason,
    LPVOID  Reserved
    )
/*++

Routine Description:

    Creates a new thread-local ALLOCATOR and associates it with the TlsIndex
    slot obtained when the DLL was loaded.

Arguments:

    Module - Unused.

    Reason - Unused.

    Reserved - Unused.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    BOOL Success;
    PALLOCATOR GlobalAllocator;
    PALLOCATOR TlsAllocator;

    UNREFERENCED_PARAMETER(Module);
    UNREFERENCED_PARAMETER(Reason);
    UNREFERENCED_PARAMETER(Reserved);

    //
    // Ensure the global TracerConfig and TlsIndex variables are valid.
    //

    if (!TracerConfig) {
        return FALSE;
    }

    if (TlsIndex == TLS_OUT_OF_INDEXES) {
        return FALSE;
    }

    //
    // Create a new ALLOCATOR struct using the original allocator's Calloc
    // routine routine (which we saved earlier).
    //
    // (We can't use GlobalAllocator->Calloc as this may have already been
    // replaced by the TLS-aware version.)
    //

    GlobalAllocator = TracerConfig->Allocator;

    TlsAllocator = (PALLOCATOR)(
        OriginalCalloc(
            GlobalAllocator->Context,
            1,
            sizeof(*TlsAllocator)
        )
    );

    if (!TlsAllocator) {
        return FALSE;
    }

    //
    // Initialize the new TLS allocator.
    //

    Success = TlsHeapInitializeAllocator(TlsAllocator);

    if (!Success) {
        goto Error;
    }

    if (InterlockedIncrement(&GlobalAllocator->NumberOfThreads) == 1) {

        //
        // This is the first thread attached to the allocator, so toggle
        // the TLS-aware allocation routines.
        //

        GlobalAllocator->Malloc = TlsAwareMalloc;
        GlobalAllocator->Calloc = TlsAwareCalloc;
        GlobalAllocator->Realloc = TlsAwareRealloc;
        GlobalAllocator->Free = TlsAwareFree;
        GlobalAllocator->FreePointer = TlsAwareFreePointer;

        GlobalAllocator->Flags.IsTlsAware = TRUE;
        GlobalAllocator->Flags.IsTlsRedirectionEnabled = TRUE;

    }

    //
    // We're done.
    //

    return TRUE;

Error:


    if (TlsAllocator) {

        //
        // Free the allocated struct.
        //

        OriginalFreePointer(GlobalAllocator->Context, &TlsAllocator);
    }

    return FALSE;
}


// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
