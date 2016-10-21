/*++

Copyright (c) 2016 Trent Nelson <trent@trent.me>

Module Name:

    TraceStorePrivate.h

Abstract:

    This is the private header file for the TraceStore component.  It defines
    function typedefs and function declarations for all major (i.e. not local
    to the module) functions available for use by individual modules within
    this component.

--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////
// Function typedefs and inline functions for internal modules.
////////////////////////////////////////////////////////////////////////////////

//
// TraceStorePath-related functions.
//

typedef
_Success_(return != 0)
BOOL
(FIND_LONGEST_TRACE_STORE_FILENAME)(
    _Out_ PUSHORT LengthPointer
    );
typedef FIND_LONGEST_TRACE_STORE_FILENAME *PFIND_LONGEST_TRACE_STORE_FILENAME;
FIND_LONGEST_TRACE_STORE_FILENAME FindLongestTraceStoreFileName;

typedef
_Success_(return != 0)
ULONG
(GET_LONGEST_TRACE_STORE_FILENAME)(VOID);
typedef GET_LONGEST_TRACE_STORE_FILENAME *PGET_LONGEST_TRACE_STORE_FILENAME;
GET_LONGEST_TRACE_STORE_FILENAME GetLongestTraceStoreFileName;

typedef
_Success_(return != 0)
BOOL
(INITIALIZE_TRACE_STORE_PATH)(
    _In_    PCWSTR          Path,
    _In_    PTRACE_STORE    TraceStore
    );
typedef INITIALIZE_TRACE_STORE_PATH *PINITIALIZE_TRACE_STORE_PATH;
INITIALIZE_TRACE_STORE_PATH InitializeTraceStorePath;

//
// TraceStoresRundown-related data structures and functions.
//

typedef struct _TRACE_STORES_RUNDOWN_FLAGS {
    ULONG IsActive:1;
} TRACE_STORES_RUNDOWN_FLAGS, *PTRACE_STORES_RUNDOWN_FLAGS;

typedef _Struct_size_bytes_(SizeOfStruct) struct _TRACE_STORES_RUNDOWN {

    //
    // Size of the structure, in bytes.
    //

    _Field_range_(==, sizeof(struct _TRACE_STORES_RUNDOWN)) USHORT SizeOfStruct;

    //
    // Pad out to 4-bytes.
    //

    USHORT Padding1;

    //
    // Flags.
    //

    TRACE_STORES_RUNDOWN_FLAGS Flags;

    //
    // Critical section protecting the rundown list head.
    //

    CRITICAL_SECTION CriticalSection;

    //
    // Rundown list head.
    //

    _Guarded_by_(CriticalSection)
    LIST_ENTRY ListHead;

} TRACE_STORES_RUNDOWN, *PTRACE_STORES_RUNDOWN;

typedef
_Requires_lock_not_held_(Rundown->CriticalSection)
VOID
(RUNDOWN_TRACE_STORES)(
    _In_ PTRACE_STORES_RUNDOWN Rundown
    );
typedef RUNDOWN_TRACE_STORES *PRUNDOWN_TRACE_STORES;
RUNDOWN_TRACE_STORES RundownTraceStores;

typedef
BOOL
(IS_TRACE_STORES_RUNDOWN_ACTIVE)(
    _In_ PTRACE_STORES_RUNDOWN Rundown
    );
typedef IS_TRACE_STORES_RUNDOWN_ACTIVE *PIS_TRACE_STORES_RUNDOWN_ACTIVE;
IS_TRACE_STORES_RUNDOWN_ACTIVE IsTraceStoresRundownActive;

typedef
_Success_(return != 0)
_Check_return_
BOOL
(INITIALIZE_TRACE_STORES_RUNDOWN)(
    _In_ PTRACE_STORES_RUNDOWN Rundown
    );
typedef INITIALIZE_TRACE_STORES_RUNDOWN \
      *PINITIALIZE_TRACE_STORES_RUNDOWN;
INITIALIZE_TRACE_STORES_RUNDOWN InitializeTraceStoresRundown;

typedef
VOID
(DESTROY_TRACE_STORES_RUNDOWN)(
    _In_ PTRACE_STORES_RUNDOWN Rundown
    );
typedef DESTROY_TRACE_STORES_RUNDOWN \
      *PDESTROY_TRACE_STORES_RUNDOWN;
DESTROY_TRACE_STORES_RUNDOWN DestroyTraceStoresRundown;

typedef
_Requires_lock_held_(Rundown->CriticalSection)
VOID
(ADD_TRACE_STORES_TO_RUNDOWN)(
    _In_ PTRACE_STORES_RUNDOWN Rundown,
    _In_ PTRACE_STORES TraceStores
    );
typedef ADD_TRACE_STORES_TO_RUNDOWN *PADD_TRACE_STORES_TO_RUNDOWN;
ADD_TRACE_STORES_TO_RUNDOWN AddTraceStoresToRundown;

typedef
_Requires_lock_held_(TraceStores->Rundown->CriticalSection)
VOID
(REMOVE_TRACE_STORES_FROM_RUNDOWN)(
    _In_ PTRACE_STORES TraceStores
    );
typedef REMOVE_TRACE_STORES_FROM_RUNDOWN *PREMOVE_TRACE_STORES_FROM_RUNDOWN;
REMOVE_TRACE_STORES_FROM_RUNDOWN RemoveTraceStoresFromRundown;

typedef
_Success_(return != 0)
_Check_return_
BOOL
(REGISTER_TRACE_STORES)(
    _In_ PTRACE_STORES_RUNDOWN Rundown,
    _In_ PTRACE_STORES TraceStores
    );
typedef REGISTER_TRACE_STORES *PREGISTER_TRACE_STORES;
REGISTER_TRACE_STORES RegisterTraceStores;

typedef
_Success_(return != 0)
_Check_return_
BOOL
(UNREGISTER_TRACE_STORES)(
    _In_ PTRACE_STORES TraceStores
    );
typedef UNREGISTER_TRACE_STORES *PUNREGISTER_TRACE_STORES;
UNREGISTER_TRACE_STORES UnregisterTraceStores;

//
// TraceStoreGlobalRundown-related functions.
//

typedef
PTRACE_STORES_RUNDOWN
(GET_GLOBAL_TRACE_STORES_RUNDOWN)(
    VOID
    );
typedef GET_GLOBAL_TRACE_STORES_RUNDOWN *PGET_GLOBAL_TRACE_STORES_RUNDOWN;
GET_GLOBAL_TRACE_STORES_RUNDOWN GetGlobalTraceStoresRundown;

typedef
_Success_(return != 0)
_Check_return_
BOOL
(REGISTER_GLOBAL_TRACE_STORES)(
    _In_ PTRACE_STORES TraceStores
    );
typedef REGISTER_GLOBAL_TRACE_STORES *PREGISTER_GLOBAL_TRACE_STORES;
REGISTER_GLOBAL_TRACE_STORES RegisterGlobalTraceStores;

typedef
_Success_(return != 0)
_Check_return_
BOOL
(UNREGISTER_GLOBAL_TRACE_STORES)(
    _In_ PTRACE_STORES TraceStores
    );
typedef UNREGISTER_GLOBAL_TRACE_STORES *PUNREGISTER_GLOBAL_TRACE_STORES;
UNREGISTER_GLOBAL_TRACE_STORES UnregisterGlobalTraceStores;

//
// TraceStoreMetadata-related functions.
//

typedef
_Success_(return != 0)
BOOL
(INITIALIZE_TRACE_STORE_METADATA)(
    _In_ PTRACE_STORE MetadataStore
    );
typedef INITIALIZE_TRACE_STORE_METADATA *PINITIALIZE_TRACE_STORE_METADATA;

INITIALIZE_TRACE_STORE_METADATA InitializeMetadataInfoMetadata;
INITIALIZE_TRACE_STORE_METADATA InitializeAllocationMetadata;
INITIALIZE_TRACE_STORE_METADATA InitializeRelocationMetadata;
INITIALIZE_TRACE_STORE_METADATA InitializeAddressMetadata;
INITIALIZE_TRACE_STORE_METADATA InitializeBitmapMetadata;
INITIALIZE_TRACE_STORE_METADATA InitializeInfoMetadata;
INITIALIZE_TRACE_STORE_METADATA InitializeMetadataFromRecordSize;
INITIALIZE_TRACE_STORE_METADATA ZeroInitializeMetadata;

typedef
PTRACE_STORE_TRAITS
(TRACE_STORE_METADATA_ID_TO_TRAITS)(
    _In_ TRACE_STORE_METADATA_ID TraceStoreMetadataId
    );
typedef TRACE_STORE_METADATA_ID_TO_TRAITS \
      *PTRACE_STORE_METADATA_ID_TO_TRAITS;
TRACE_STORE_METADATA_ID_TO_TRAITS TraceStoreMetadataIdToTraits;


typedef
PINITIALIZE_TRACE_STORE_METADATA
(TRACE_STORE_METADATA_ID_TO_INITIALIZER)(
    _In_ TRACE_STORE_METADATA_ID TraceStoreMetadataId
    );
typedef TRACE_STORE_METADATA_ID_TO_INITIALIZER \
      *PTRACE_STORE_METADATA_ID_TO_INITIALIZER;
TRACE_STORE_METADATA_ID_TO_INITIALIZER TraceStoreMetadataIdToInitializer;

typedef
ULONG
(TRACE_STORE_METADATA_ID_TO_RECORD_SIZE)(
    _In_ TRACE_STORE_METADATA_ID TraceStoreMetadataId
    );
typedef TRACE_STORE_METADATA_ID_TO_RECORD_SIZE \
      *PTRACE_STORE_METADATA_ID_TO_RECORD_SIZE;
TRACE_STORE_METADATA_ID_TO_RECORD_SIZE TraceStoreMetadataIdToRecordSize;

typedef
PTRACE_STORE_INFO
(TRACE_STORE_METADATA_ID_TO_INFO)(
    _In_ PTRACE_STORE TraceStore,
    _In_ TRACE_STORE_METADATA_ID TraceStoreMetadataId
    );
typedef TRACE_STORE_METADATA_ID_TO_INFO \
      *PTRACE_STORE_METADATA_ID_TO_INFO;
TRACE_STORE_METADATA_ID_TO_INFO TraceStoreMetadataIdToInfo;

//
// TraceStoreContext-related macros and functions.
//

#define BIND_STORE(Name)                                              \
    if (!BindTraceStoreToTraceContext(##Name##Store, TraceContext)) { \
        return FALSE;                                                 \
    }

//
// N.B. BIND_STORE(Trace) must always go last in the macro below as it requires
//      the metadata stores to be bound and mapped before it can finalize its
//      own binding.
//

#define BIND_STORES()         \
    BIND_STORE(MetadataInfo); \
    BIND_STORE(Allocation);   \
    BIND_STORE(Relocation);   \
    BIND_STORE(Address);      \
    BIND_STORE(Bitmap);       \
    BIND_STORE(Info);         \
    BIND_STORE(Trace)

typedef
_Check_return_
_Success_(return != 0)
BOOL
(BIND_TRACE_STORE_TO_TRACE_CONTEXT)(
    _In_ PTRACE_STORE TraceStore,
    _In_ PTRACE_CONTEXT TraceContext
    );
typedef BIND_TRACE_STORE_TO_TRACE_CONTEXT *PBIND_TRACE_STORE_TO_TRACE_CONTEXT;
BIND_TRACE_STORE_TO_TRACE_CONTEXT BindTraceStoreToTraceContext;

typedef
_Check_return_
_Success_(return != 0)
BOOL
(FINALIZE_FIRST_TRACE_STORE_MEMORY_MAP)(
    _In_ PTRACE_STORE TraceStore
    );
typedef FINALIZE_FIRST_TRACE_STORE_MEMORY_MAP \
      *PFINALIZE_FIRST_TRACE_STORE_MEMORY_MAP;
FINALIZE_FIRST_TRACE_STORE_MEMORY_MAP FinalizeFirstTraceStoreMemoryMap;

//
// TraceStoreReadonlyContext-related functions and macros.
//

#define BIND_READONLY_STORE(Name)                                      \
    if (!BindTraceStoreToReadonlyTraceContext(##Name##Store,           \
                                              ReadonlyTraceContext)) { \
        return FALSE;                                                  \
    }

#define BIND_READONLY_STORES()         \
    BIND_READONLY_STORE(MetadataInfo); \
    BIND_READONLY_STORE(Allocation);   \
    BIND_READONLY_STORE(Relocation);   \
    BIND_READONLY_STORE(Address);      \
    BIND_READONLY_STORE(Bitmap);       \
    BIND_READONLY_STORE(Info);         \
    BIND_READONLY_STORE(Trace)

typedef
_Check_return_
_Success_(return != 0)
BOOL
(BIND_TRACE_STORE_TO_READONLY_TRACE_CONTEXT)(
    _In_ PTRACE_STORE TraceStore,
    _In_ PREADONLY_TRACE_CONTEXT ReadonlyTraceContext
    );
typedef BIND_TRACE_STORE_TO_READONLY_TRACE_CONTEXT \
      *PBIND_TRACE_STORE_TO_READONLY_TRACE_CONTEXT;
BIND_TRACE_STORE_TO_READONLY_TRACE_CONTEXT BindTraceStoreToReadonlyTraceContext;

FINALIZE_FIRST_TRACE_STORE_MEMORY_MAP \
    FinalizeFirstReadonlyTraceStoreMemoryMap;

//
// TraceStoreTime-related functions.
//

typedef
_Check_return_
_Success_(return != 0)
BOOL
(INITIALIZE_TRACE_STORE_TIME)(
    _In_    PRTL                Rtl,
    _In_    PTRACE_STORE_TIME   Time
    );
typedef INITIALIZE_TRACE_STORE_TIME *PINITIALIZE_TRACE_STORE_TIME;
INITIALIZE_TRACE_STORE_TIME InitializeTraceStoreTime;

//
// TraceStoreMemoryMap-related functions.
//

typedef
_Success_(return != 0)
_Check_return_
BOOL
(CREATE_MEMORY_MAPS_FOR_TRACE_STORE)(
    _In_ PTRACE_STORE TraceStore
    );
typedef CREATE_MEMORY_MAPS_FOR_TRACE_STORE *PCREATE_MEMORY_MAPS_FOR_TRACE_STORE;
CREATE_MEMORY_MAPS_FOR_TRACE_STORE CreateMemoryMapsForTraceStore;

typedef
_Success_(return != 0)
BOOL
(PREPARE_NEXT_TRACE_STORE_MEMORY_MAP)(
    _In_ PTRACE_STORE TraceStore
    );
typedef  PREPARE_NEXT_TRACE_STORE_MEMORY_MAP \
       *PPREPARE_NEXT_TRACE_STORE_MEMORY_MAP;
PREPARE_NEXT_TRACE_STORE_MEMORY_MAP \
    PrepareNextTraceStoreMemoryMap;


typedef
_Success_(return != 0)
BOOL
(RELEASE_PREV_TRACE_STORE_MEMORY_MAP)(
    _In_ PTRACE_STORE TraceStore
    );
typedef  RELEASE_PREV_TRACE_STORE_MEMORY_MAP \
       *PRELEASE_PREV_TRACE_STORE_MEMORY_MAP;
RELEASE_PREV_TRACE_STORE_MEMORY_MAP ReleasePrevTraceStoreMemoryMap;

typedef
_Check_return_
_Success_(return != 0)
BOOL
(FLUSH_TRACE_STORE_MEMORY_MAP)(
    _In_ PTRACE_STORE_MEMORY_MAP MemoryMap
    );
typedef FLUSH_TRACE_STORE_MEMORY_MAP *PFLUSH_TRACE_STORE_MEMORY_MAP;
FLUSH_TRACE_STORE_MEMORY_MAP FlushTraceStoreMemoryMap;

typedef
_Success_(return != 0)
BOOL
(UNMAP_TRACE_STORE_MEMORY_MAP)(
    _In_ PTRACE_STORE_MEMORY_MAP MemoryMap
    );
typedef UNMAP_TRACE_STORE_MEMORY_MAP *PUNMAP_TRACE_STORE_MEMORY_MAP;
UNMAP_TRACE_STORE_MEMORY_MAP UnmapTraceStoreMemoryMap;

typedef
VOID
(RUNDOWN_TRACE_STORE_MEMORY_MAP)(
    _In_opt_ PTRACE_STORE_MEMORY_MAP MemoryMap
    );
typedef RUNDOWN_TRACE_STORE_MEMORY_MAP *PRUNDOWN_TRACE_STORE_MEMORY_MAP;
RUNDOWN_TRACE_STORE_MEMORY_MAP RundownTraceStoreMemoryMap;

typedef
_Success_(return != 0)
BOOL
(CONSUME_NEXT_TRACE_STORE_MEMORY_MAP)(
    _In_ PTRACE_STORE TraceStore
    );
typedef  CONSUME_NEXT_TRACE_STORE_MEMORY_MAP \
       *PCONSUME_NEXT_TRACE_STORE_MEMORY_MAP;
CONSUME_NEXT_TRACE_STORE_MEMORY_MAP ConsumeNextTraceStoreMemoryMap;

typedef
VOID
(SUBMIT_CLOSE_MEMORY_MAP_THREADPOOL_WORK)(
    _In_ PTRACE_STORE TraceStore,
    _Inout_ PPTRACE_STORE_MEMORY_MAP MemoryMap
    );
typedef  SUBMIT_CLOSE_MEMORY_MAP_THREADPOOL_WORK \
       *PSUBMIT_CLOSE_MEMORY_MAP_THREADPOOL_WORK;
SUBMIT_CLOSE_MEMORY_MAP_THREADPOOL_WORK \
    SubmitCloseMemoryMapThreadpoolWork;

//
// TraceStoreCallback-related functions.
//

typedef
VOID
(CALLBACK FINALIZE_FIRST_TRACE_STORE_MEMORY_MAP_CALLBACK)(
    _In_ PTP_CALLBACK_INSTANCE Instance,
    _In_ PVOID Context,
    _In_ PTP_WORK Work
    );
typedef  FINALIZE_FIRST_TRACE_STORE_MEMORY_MAP_CALLBACK \
       *PFINALIZE_FIRST_TRACE_STORE_MEMORY_MAP_CALLBACK;
FINALIZE_FIRST_TRACE_STORE_MEMORY_MAP_CALLBACK \
    FinalizeFirstTraceStoreMemoryMapCallback;

FINALIZE_FIRST_TRACE_STORE_MEMORY_MAP_CALLBACK \
    FinalizeFirstReadonlyTraceStoreMemoryMapCallback;

typedef
VOID
(CALLBACK PREFAULT_FUTURE_TRACE_STORE_PAGE_CALLBACK)(
    _In_ PTP_CALLBACK_INSTANCE   Instance,
    _In_ PVOID                   Context,
    _In_ PTP_WORK                Work
    );
typedef  PREFAULT_FUTURE_TRACE_STORE_PAGE_CALLBACK \
       *PPREFAULT_FUTURE_TRACE_STORE_PAGE_CALLBACK;
PREFAULT_FUTURE_TRACE_STORE_PAGE_CALLBACK \
    PrefaultFutureTraceStorePageCallback;

typedef
VOID
(CALLBACK PREPARE_NEXT_TRACE_STORE_MEMORY_MAP_CALLBACK)(
    _In_ PTP_CALLBACK_INSTANCE Instance,
    _In_ PVOID Context,
    _In_ PTP_WORK Work
    );
typedef  PREPARE_NEXT_TRACE_STORE_MEMORY_MAP_CALLBACK \
       *PPREPARE_NEXT_TRACE_STORE_MEMORY_MAP_CALLBACK;
PREPARE_NEXT_TRACE_STORE_MEMORY_MAP_CALLBACK \
    PrepareNextTraceStoreMemoryMapCallback;

typedef
VOID
(CALLBACK RELEASE_PREV_TRACE_STORE_MEMORY_MAP_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE   Instance,
    _Inout_opt_ PVOID                   Context,
    _Inout_     PTP_WORK                Work
    );
typedef  RELEASE_PREV_TRACE_STORE_MEMORY_MAP_CALLBACK \
       *PRELEASE_PREV_TRACE_STORE_MEMORY_MAP_CALLBACK;
RELEASE_PREV_TRACE_STORE_MEMORY_MAP_CALLBACK \
    ReleasePrevTraceStoreMemoryMapCallback;

typedef
VOID
(CALLBACK BIND_TRACE_STORE_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE   Instance,
    _Inout_opt_ PVOID                   Context,
    _Inout_     PTP_WORK                Work
    );
typedef BIND_TRACE_STORE_CALLBACK *PBIND_TRACE_STORE_CALLBACK;
BIND_TRACE_STORE_CALLBACK BindTraceStoreCallback;

typedef
VOID
(CALLBACK LOAD_METADATA_INFO_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE   Instance,
    _Inout_opt_ PVOID                   Context,
    _Inout_     PTP_WORK                Work
    );
typedef LOAD_METADATA_INFO_CALLBACK *PLOAD_METADATA_INFO_CALLBACK;
LOAD_METADATA_INFO_CALLBACK LoadMetadataInfoCallback;

typedef
VOID
(CALLBACK LOAD_REMAINING_METADATA_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE   Instance,
    _Inout_opt_ PVOID                   Context,
    _Inout_     PTP_WORK                Work
    );
typedef LOAD_REMAINING_METADATA_CALLBACK *PLOAD_REMAINING_METADATA_CALLBACK;
LOAD_REMAINING_METADATA_CALLBACK LoadRemainingMetadataCallback;

typedef
VOID
(CALLBACK LOAD_TRACE_STORE_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE   Instance,
    _Inout_opt_ PVOID                   Context,
    _Inout_     PTP_WORK                Work
    );
typedef LOAD_TRACE_STORE_CALLBACK *PLOAD_TRACE_STORE_CALLBACK;
LOAD_TRACE_STORE_CALLBACK LoadTraceStoreCallback;

//
// TraceStoreMemory-map related inline functions.
//

FORCEINLINE
VOID
ReturnFreeTraceStoreMemoryMap(
    _In_ PTRACE_STORE TraceStore,
    _In_ PTRACE_STORE_MEMORY_MAP MemoryMap
    )
{
    SecureZeroMemory(MemoryMap, sizeof(*MemoryMap));

    InterlockedPushEntrySList(
        &TraceStore->FreeMemoryMaps,
        &MemoryMap->ListEntry
    );

    if (!InterlockedDecrement(&TraceStore->NumberOfActiveMemoryMaps)) {
        SetEvent(TraceStore->AllMemoryMapsAreFreeEvent);
    }
}

FORCEINLINE
_Check_return_
_Success_(return != 0)
BOOL
PopFreeTraceStoreMemoryMap(
    _In_  PTRACE_STORE TraceStore,
    _Out_ PPTRACE_STORE_MEMORY_MAP MemoryMap
    )
{
    PSLIST_HEADER ListHead;
    PSLIST_ENTRY ListEntry;

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(TraceStore)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(MemoryMap)) {
        return FALSE;
    }

    ListHead = &TraceStore->FreeMemoryMaps;

    ListEntry = InterlockedPopEntrySList(ListHead);
    if (!ListEntry) {
        return FALSE;
    }

    *MemoryMap = CONTAINING_RECORD(ListEntry,
                                   TRACE_STORE_MEMORY_MAP,
                                   ListEntry);

    InterlockedIncrement(&TraceStore->NumberOfActiveMemoryMaps);

    return TRUE;
}

FORCEINLINE
_Check_return_
_Success_(return != 0)
BOOL
PopTraceStoreMemoryMap(
    _In_  PSLIST_HEADER ListHead,
    _Out_ PPTRACE_STORE_MEMORY_MAP MemoryMap
    )
{
    PSLIST_ENTRY ListEntry;

    ListEntry = InterlockedPopEntrySList(ListHead);
    if (!ListEntry) {
        return FALSE;
    }

    *MemoryMap = CONTAINING_RECORD(ListEntry,
                                   TRACE_STORE_MEMORY_MAP,
                                   ListEntry);

    return TRUE;
}

FORCEINLINE
VOID
PushTraceStoreMemoryMap(
    _In_ PSLIST_HEADER ListHead,
    _In_ PTRACE_STORE_MEMORY_MAP MemoryMap
    )
{
    InterlockedPushEntrySList(ListHead, &MemoryMap->ListEntry);
}

FORCEINLINE
BOOL
GetTraceStoreMemoryMapFileInfo(
    _In_  PTRACE_STORE_MEMORY_MAP MemoryMap,
    _Out_ PFILE_STANDARD_INFO FileInfo
    )
{
    return GetFileInformationByHandleEx(
        MemoryMap->FileHandle,
        (FILE_INFO_BY_HANDLE_CLASS)FileStandardInfo,
        FileInfo,
        sizeof(*FileInfo)
    );
}

//
// TraceStoreAddress-related functions.
//

typedef
_Check_return_
_Success_(return != 0)
BOOL
(LOAD_NEXT_TRACE_STORE_ADDRESS)(
    _In_  PTRACE_STORE TraceStore,
    _Out_ PPTRACE_STORE_ADDRESS AddressPointer
    );
typedef LOAD_NEXT_TRACE_STORE_ADDRESS *PLOAD_NEXT_TRACE_STORE_ADDRESS;
LOAD_NEXT_TRACE_STORE_ADDRESS LoadNextTraceStoreAddress;

//
// TraceStoreAllocation-related functions.
//

ALLOCATE_RECORDS TraceStoreAllocateRecords;

typedef
_Success_(return != 0)
BOOL
(RECORD_TRACE_STORE_ALLOCATION)(
    _In_ PTRACE_STORE     TraceStore,
    _In_ PULARGE_INTEGER  RecordSize,
    _In_ PULARGE_INTEGER  NumberOfRecords
    );
typedef RECORD_TRACE_STORE_ALLOCATION *PRECORD_TRACE_STORE_ALLOCATION;
RECORD_TRACE_STORE_ALLOCATION RecordTraceStoreAllocation;

//
// TraceStoreRelocation-related functions.
//

typedef
_Success_(return != 0)
BOOL
(SAVE_TRACE_STORE_RELOCATION_INFO)(
    _In_ PTRACE_STORE TraceStore
    );
typedef SAVE_TRACE_STORE_RELOCATION_INFO \
      *PSAVE_TRACE_STORE_RELOCATION_INFO;
SAVE_TRACE_STORE_RELOCATION_INFO SaveTraceStoreRelocationInfo;

typedef
_Success_(return != 0)
BOOL
(LOAD_TRACE_STORE_RELOCATION_INFO)(
    _In_ PTRACE_STORE TraceStore
    );
typedef LOAD_TRACE_STORE_RELOCATION_INFO \
      *PLOAD_TRACE_STORE_RELOCATION_INFO;
LOAD_TRACE_STORE_RELOCATION_INFO LoadTraceStoreRelocationInfo;

//
// TraceStoreTraits-related functions.
//

typedef
_Success_(return != 0)
BOOL
(SAVE_TRACE_STORE_TRAITS)(
    _In_ PTRACE_STORE TraceStore
    );
typedef SAVE_TRACE_STORE_TRAITS \
      *PSAVE_TRACE_STORE_TRAITS;
SAVE_TRACE_STORE_TRAITS SaveTraceStoreTraits;

typedef
_Success_(return != 0)
BOOL
(LOAD_TRACE_STORE_TRAITS)(
    _In_ PTRACE_STORE TraceStore
    );
typedef LOAD_TRACE_STORE_TRAITS \
      *PLOAD_TRACE_STORE_TRAITS;
LOAD_TRACE_STORE_TRAITS LoadTraceStoreTraits;

typedef
_Success_(return != 0)
BOOL
(INITIALIZE_TRACE_STORE_TRAITS)(
    _In_ PTRACE_STORE TraceStore
    );
typedef INITIALIZE_TRACE_STORE_TRAITS \
      *PINITIALIZE_TRACE_STORE_TRAITS;
INITIALIZE_TRACE_STORE_TRAITS InitializeTraceStoreTraits;

//
// TraceStoreSession-related functions.
//

INITIALIZE_TRACE_SESSION InitializeTraceSession;

//
// TraceStore-related functions.
//

typedef
_Check_return_
_Success_(return != 0)
BOOL
(INITIALIZE_STORE)(
    _In_     PCWSTR Path,
    _In_     PTRACE_STORE TraceStore,
    _In_opt_ ULONG InitialSize,
    _In_opt_ ULONG MappingSize
    );
typedef INITIALIZE_STORE *PINITIALIZE_STORE;
INITIALIZE_STORE InitializeStore;

typedef
_Check_return_
_Success_(return != 0)
BOOL
(INITIALIZE_TRACE_STORE)(
    _In_ PRTL Rtl,
    _In_ PCWSTR Path,
    _In_ PTRACE_STORE TraceStore,
    _In_ PTRACE_STORE MetadataInfoStore,
    _In_ PTRACE_STORE AllocationStore,
    _In_ PTRACE_STORE RelocationStore,
    _In_ PTRACE_STORE AddressStore,
    _In_ PTRACE_STORE BitmapStore,
    _In_ PTRACE_STORE InfoStore,
    _In_ ULONG InitialSize,
    _In_ ULONG MappingSize,
    _In_ PTRACE_FLAGS TraceFlags,
    _In_ PTRACE_STORE_RELOC Reloc
    );
typedef INITIALIZE_TRACE_STORE *PINITIALIZE_TRACE_STORE;
INITIALIZE_TRACE_STORE InitializeTraceStore;

typedef
VOID
(INITIALIZE_TRACE_STORE_SLIST_HEADERS)(
    _In_ PTRACE_STORE TraceStore
    );
typedef INITIALIZE_TRACE_STORE_SLIST_HEADERS \
      *PINITIALIZE_TRACE_STORE_SLIST_HEADERS;
INITIALIZE_TRACE_STORE_SLIST_HEADERS InitializeTraceStoreSListHeaders;

typedef
_Check_return_
_Success_(return != 0)
BOOL
(CREATE_TRACE_STORE_EVENTS)(
    _In_ PTRACE_STORE TraceStore
    );
typedef CREATE_TRACE_STORE_EVENTS *PCREATE_TRACE_STORE_EVENTS;
CREATE_TRACE_STORE_EVENTS CreateTraceStoreEvents;

typedef
_Check_return_
_Success_(return != 0)
BOOL
(CREATE_TRACE_STORE_THREADPOOL_WORK_ITEMS)(
    _In_ PTRACE_STORE TraceStore,
    _In_ PTP_CALLBACK_ENVIRON ThreadpoolCallbackEnvironment,
    _In_ PFINALIZE_FIRST_TRACE_STORE_MEMORY_MAP_CALLBACK
        FinalizeFirstMemoryMapCallback
    );
typedef CREATE_TRACE_STORE_THREADPOOL_WORK_ITEMS \
      *PCREATE_TRACE_STORE_THREADPOOL_WORK_ITEMS;
CREATE_TRACE_STORE_THREADPOOL_WORK_ITEMS CreateTraceStoreThreadpoolWorkItems;

typedef
_Success_(return != 0)
BOOL
(TRUNCATE_STORE)(
    _In_ PTRACE_STORE TraceStore
    );
typedef TRUNCATE_STORE *PTRUNCATE_STORE;
TRUNCATE_STORE TruncateStore;

typedef
VOID
(CLOSE_STORE)(
    _In_ PTRACE_STORE TraceStore
    );
typedef CLOSE_STORE *PCLOSE_STORE;
CLOSE_STORE CloseStore;

typedef
VOID
(CLOSE_TRACE_STORE)(
    _In_ PTRACE_STORE TraceStore
    );
typedef CLOSE_TRACE_STORE *PCLOSE_TRACE_STORE;
CLOSE_TRACE_STORE CloseTraceStore;

FORCEINLINE
VOID
CloseTraceStoresInline(
    _In_ PTRACE_STORES TraceStores
    )
{
    USHORT Index;
    USHORT StoreIndex;

    FOR_EACH_TRACE_STORE(TraceStores, Index, StoreIndex) {
        CloseTraceStore(&TraceStores->Stores[StoreIndex]);
    }
}

typedef
VOID
(RUNDOWN_STORE)(
    _In_ PTRACE_STORE TraceStore
    );
typedef RUNDOWN_STORE *PRUNDOWN_STORE;
RUNDOWN_STORE RundownStore;

typedef
VOID
(RUNDOWN_TRACE_STORE)(
    _In_ PTRACE_STORE TraceStore
    );
typedef RUNDOWN_TRACE_STORE *PRUNDOWN_TRACE_STORE;
RUNDOWN_TRACE_STORE RundownTraceStore;

FORCEINLINE
VOID
RundownTraceStoresInline(
    _In_ PTRACE_STORES TraceStores
    )
{
    USHORT Index;
    USHORT StoreIndex;

    FOR_EACH_TRACE_STORE(TraceStores, Index, StoreIndex) {
        RundownTraceStore(&TraceStores->Stores[StoreIndex]);
    }
}

#define CLOSE_METADATA_STORE(Name)             \
    if (TraceStore->##Name##Store) {           \
        CloseStore(TraceStore->##Name##Store); \
        TraceStore->##Name##Store = NULL;      \
    }

#define CLOSE_METADATA_STORES()         \
    CLOSE_METADATA_STORE(Allocation);   \
    CLOSE_METADATA_STORE(Relocation);   \
    CLOSE_METADATA_STORE(Address);      \
    CLOSE_METADATA_STORE(Bitmap);       \
    CLOSE_METADATA_STORE(Info);         \
    CLOSE_METADATA_STORE(MetadataInfo);

#define RUNDOWN_METADATA_STORE(Name)             \
    if (TraceStore->##Name##Store) {             \
        RundownStore(TraceStore->##Name##Store); \
        TraceStore->##Name##Store = NULL;        \
    }

#define RUNDOWN_METADATA_STORES()         \
    RUNDOWN_METADATA_STORE(Allocation);   \
    RUNDOWN_METADATA_STORE(Relocation);   \
    RUNDOWN_METADATA_STORE(Address);      \
    RUNDOWN_METADATA_STORE(Bitmap);       \
    RUNDOWN_METADATA_STORE(Info);         \
    RUNDOWN_METADATA_STORE(MetadataInfo);

//
// TraceStore-related inline functions.
//

FORCEINLINE
_Check_return_
_Success_(return != 0)
BOOL
PopTraceStore(
    _In_  PSLIST_HEADER ListHead,
    _In_  SHORT FieldOffset,
    _Out_ PPTRACE_STORE TraceStore
    )
{
    PSLIST_ENTRY ListEntry;

    ListEntry = InterlockedPopEntrySList(ListHead);
    if (!ListEntry) {
        return FALSE;
    }

    *TraceStore = (PTRACE_STORE)RtlOffsetToPointer(ListEntry, -FieldOffset);

    return TRUE;
}

FORCEINLINE
VOID
PushTraceStore(
    _In_ PSLIST_HEADER ListHead,
    _In_ USHORT FieldOffset,
    _In_ PTRACE_STORE TraceStore
    )
{
    PSLIST_ENTRY ListEntry;

    ListEntry = (PSLIST_ENTRY)RtlOffsetToPointer(TraceStore, FieldOffset);
    InterlockedPushEntrySList(ListHead, ListEntry);
}

#define PopBindTraceStore(TraceContext, TraceStorePointer) \
    PopTraceStore(                                         \
        &TraceContext->BindTraceStoreWork->ListHead,       \
        TraceStorePointer,                                 \
        FIELD_OFFSET(TRACE_STORE, BindTraceStoreListEntry) \
    )

#define PushBindTraceStore(TraceContext, TraceStorePointer) \
    PushTraceStore(                                         \
        &TraceContext->BindTraceStoreWork->ListHead,        \
        TraceStorePointer,                                  \
        FIELD_OFFSET(TRACE_STORE, BindTraceStoreListEntry)  \
    )


//
// TraceStoreSystemTimer-related functions.
//

typedef
_Success_(return != 0)
PTIMER_FUNCTION
(TRACE_STORE_GET_TIMER_FUNCTION)(VOID);
typedef TRACE_STORE_GET_TIMER_FUNCTION *PTRACE_STORE_GET_TIMER_FUNCTION;
TRACE_STORE_GET_TIMER_FUNCTION TraceStoreGetTimerFunction;

typedef
_Success_(return != 0)
BOOL
(TRACE_STORE_CALL_TIMER)(
    _Out_       PFILETIME   SystemTime,
    _Inout_opt_ PPTIMER_FUNCTION ppTimerFunction
    );
typedef TRACE_STORE_CALL_TIMER *PTRACE_STORE_CALL_TIMER;
TRACE_STORE_CALL_TIMER TraceStoreCallTimer;

//
// TraceStorePrefault-related inline functions.
//

FORCEINLINE
VOID
PrefaultFutureTraceStorePage(
    _In_ PTRACE_STORE TraceStore
    )
/*++

Routine Description:

    This routine pops a memory map off the TraceStore's PrefaultMemoryMaps
    list and reads a single byte from the memory map's NextAddress address.
    This has the effect of bringing the page into the process's memory if it
    isn't already in it -- either via a hard fault, soft fault, or simply
    priming the TLB with the mapping if the underlying page is already valid
    within our memory space (which is often the case because the cache manager
    aggressively reads ahead when we start mapping views of the file).

    Because a hard or soft fault can only by satisfied by blocking the thread
    (because the relevant page may need to be read off the backing disk/store),
    pre-faults are performed ahead of time by threadpool threads off the tracing
    hot-path.

Arguments:

    TraceStore - Supplies a pointer to a TRACE_STORE structure that has a
        TraceStoreMemoryMap pushed to the PrefaultMemoryMaps SLIST_HEAD.

Return Value:

    None.

--*/
{
    BOOL Success;
    PTRACE_STORE_MEMORY_MAP PrefaultMemoryMap;
    PSLIST_HEADER PrefaultList;

    //
    // Validate arguments.
    //

    if (!TraceStore) {
        return;
    }

    //
    // Pop an entry off the TraceStore's PrefaultMemoryMap list.  (There should
    // be a 1:1 correspondence between SubmitThreadpoolWork() calls and push
    // calls to the prefault list, so we should always be able to pop something
    // off.)
    //

    PrefaultList = &TraceStore->PrefaultMemoryMaps;
    Success = PopTraceStoreMemoryMap(PrefaultList, &PrefaultMemoryMap);

    if (!Success) {
        return;
    }

    //
    // Prefault the page.
    //

    TraceStore->Rtl->PrefaultPages(PrefaultMemoryMap->NextAddress, 1);

    //
    // Return the memory map to the free list.
    //

    ReturnFreeTraceStoreMemoryMap(TraceStore, PrefaultMemoryMap);
}


#ifdef __cplusplus
}; // extern "C"
#endif

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
