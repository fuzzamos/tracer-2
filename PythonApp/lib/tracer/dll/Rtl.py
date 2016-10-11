#===============================================================================
# Imports
#===============================================================================

from ctypes import (
    Union,
    Structure,

    POINTER,
    CFUNCTYPE,
)

from ..wintypes import (
    BOOL,
    PULONG
)

#===============================================================================
# Structures
#===============================================================================

class RTL(Structure):
    pass
PRTL = POINTER(RTL)

#===============================================================================
# Functions
#===============================================================================

INITIALIZE_RTL = CFUNCTYPE(BOOL, PRTL, PULONG)

# vim:set ts=8 sw=4 sts=4 tw=80 ai et                                          :
