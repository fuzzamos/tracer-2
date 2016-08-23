#pragma once

#include "Memory.h"

MALLOC DefaultHeapMalloc;
CALLOC DefaultHeapCalloc;
REALLOC DefaultHeapRealloc;
FREE DefaultHeapFree;
INITIALIZE_ALLOCATOR DefaultHeapInitialize;
DESTROY_ALLOCATOR DefaultHeapDestroy;

// vim:set ts=8 sw=4 sts=4 tw=80 expandtab                                     :
