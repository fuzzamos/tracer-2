// Python Tools for Visual Studio
// Copyright(c) Microsoft Corporation
// All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the License); you may not use
// this file except in compliance with the License. You may obtain a copy of the
// License at http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS
// OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY
// IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
//
// See the Apache Version 2.0 License for specific language governing
// permissions and limitations under the License.

#include "stdafx.h"
#include <Windows.h>
#include "Python.h"
#include "../Tracer/Tracing.h"

static const PYCODEOBJECTOFFSETS PyCodeObjectOffsets25_27 = {
    FIELD_OFFSET(PYCODEOBJECT25_27, ArgumentCount),
    0, // KeywordOnlyArgCount
    FIELD_OFFSET(PYCODEOBJECT25_27, NumberOfLocals),
    FIELD_OFFSET(PYCODEOBJECT25_27, StackSize),
    FIELD_OFFSET(PYCODEOBJECT25_27, Flags),
    FIELD_OFFSET(PYCODEOBJECT25_27, Code),
    FIELD_OFFSET(PYCODEOBJECT25_27, Constants),
    FIELD_OFFSET(PYCODEOBJECT25_27, Names),
    FIELD_OFFSET(PYCODEOBJECT25_27, LocalVariableNames),
    FIELD_OFFSET(PYCODEOBJECT25_27, FreeVariableNames),
    FIELD_OFFSET(PYCODEOBJECT25_27, CellVariableNames),
    FIELD_OFFSET(PYCODEOBJECT25_27, Filename),
    FIELD_OFFSET(PYCODEOBJECT25_27, Name),
    FIELD_OFFSET(PYCODEOBJECT25_27, FirstLineNumber),
    FIELD_OFFSET(PYCODEOBJECT25_27, LineNumberTable),
    0 // ZombieFrame
};

static const PYCODEOBJECTOFFSETS PyCodeObjectOffsets30_32 = {
    FIELD_OFFSET(PYCODEOBJECT30_32, ArgumentCount),
    FIELD_OFFSET(PYCODEOBJECT30_32, KeywordOnlyArgumentCount),
    FIELD_OFFSET(PYCODEOBJECT30_32, NumberOfLocals),
    FIELD_OFFSET(PYCODEOBJECT30_32, StackSize),
    FIELD_OFFSET(PYCODEOBJECT30_32, Flags),
    FIELD_OFFSET(PYCODEOBJECT30_32, Code),
    FIELD_OFFSET(PYCODEOBJECT30_32, Constants),
    FIELD_OFFSET(PYCODEOBJECT30_32, Names),
    FIELD_OFFSET(PYCODEOBJECT30_32, LocalVariableNames),
    FIELD_OFFSET(PYCODEOBJECT30_32, FreeVariableNames),
    FIELD_OFFSET(PYCODEOBJECT30_32, CellVariableNames),
    FIELD_OFFSET(PYCODEOBJECT30_32, Filename),
    FIELD_OFFSET(PYCODEOBJECT30_32, Name),
    FIELD_OFFSET(PYCODEOBJECT30_32, FirstLineNumber),
    FIELD_OFFSET(PYCODEOBJECT30_32, LineNumberTable),
    0 // ZombieFrame
};

static const PYCODEOBJECTOFFSETS PyCodeObjectOffsets33_35 = {
    FIELD_OFFSET(PYCODEOBJECT33_35, ArgumentCount),
    FIELD_OFFSET(PYCODEOBJECT33_35, KeywordOnlyArgumentCount),
    FIELD_OFFSET(PYCODEOBJECT33_35, NumberOfLocals),
    FIELD_OFFSET(PYCODEOBJECT33_35, StackSize),
    FIELD_OFFSET(PYCODEOBJECT33_35, Flags),
    FIELD_OFFSET(PYCODEOBJECT33_35, Code),
    FIELD_OFFSET(PYCODEOBJECT33_35, Constants),
    FIELD_OFFSET(PYCODEOBJECT33_35, Names),
    FIELD_OFFSET(PYCODEOBJECT33_35, LocalVariableNames),
    FIELD_OFFSET(PYCODEOBJECT33_35, FreeVariableNames),
    FIELD_OFFSET(PYCODEOBJECT33_35, CellVariableNames),
    FIELD_OFFSET(PYCODEOBJECT33_35, Filename),
    FIELD_OFFSET(PYCODEOBJECT33_35, Name),
    FIELD_OFFSET(PYCODEOBJECT33_35, FirstLineNumber),
    FIELD_OFFSET(PYCODEOBJECT33_35, LineNumberTable),
    FIELD_OFFSET(PYCODEOBJECT33_35, ZombieFrame),
};

_Check_return_
BOOL
LoadPythonData(
    _In_    HMODULE     PythonModule,
    _Out_   PPYTHONDATA PythonData
)
{
    if (!PythonModule) {
        return FALSE;
    }

    if (!PythonData) {
        return FALSE;
    }

    PythonData->PyCode_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyCode_Type");
    if (!PythonData->PyCode_Type) {
        goto error;
    }

    PythonData->PyDict_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyDict_Type");
    if (!PythonData->PyDict_Type) {
        goto error;
    }

    PythonData->PyTuple_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyTuple_Type");
    if (!PythonData->PyTuple_Type) {
        goto error;
    }

    PythonData->PyType_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyType_Type");
    if (!PythonData->PyType_Type) {
        goto error;
    }

    PythonData->PyFunction_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyFunction_Type");
    if (!PythonData->PyFunction_Type) {
        goto error;
    }

    PythonData->PyString_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyString_Type");
    if (!PythonData->PyString_Type) {
        PythonData->PyBytes_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyBytes_Type");
        if (!PythonData->PyBytes_Type) {
            goto error;
        }
    }

    PythonData->PyUnicode_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyUnicode_Type");
    if (!PythonData->PyUnicode_Type) {
        goto error;
    }

    PythonData->PyCFunction_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyCFunction_Type");
    if (!PythonData->PyCFunction_Type) {
        goto error;
    }

    PythonData->PyInstance_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyInstance_Type");
    if (!PythonData->PyInstance_Type) {
        goto error;
    }

    PythonData->PyModule_Type = (PPYOBJECT)GetProcAddress(PythonModule, "PyModule_Type");
    if (!PythonData->PyModule_Type) {
        goto error;
    }

    return TRUE;

error:
    return FALSE;
}

_Check_return_
BOOL
LoadPythonFunctions(
    _In_    HMODULE             PythonModule,
    _Inout_ PPYTHONFUNCTIONS    PythonFunctions
)
{
    if (!PythonModule) {
        return FALSE;
    }

    if (!PythonFunctions) {
        return FALSE;
    }

    PythonFunctions->PyFrame_GetLineNumber = (PPYFRAME_GETLINENUMBER)GetProcAddress(PythonModule, "PyFrame_GetLineNumber");
    if (!PythonFunctions->PyFrame_GetLineNumber) {
        goto error;
    }

    PythonFunctions->PyEval_SetProfile = (PPYEVAL_SETPROFILE)GetProcAddress(PythonModule, "PyEval_SetProfile");
    if (!PythonFunctions->PyEval_SetProfile) {
        goto error;
    }

    PythonFunctions->PyEval_SetTrace = (PPYEVAL_SETTRACE)GetProcAddress(PythonModule, "PyEval_SetTrace");
    if (!PythonFunctions->PyEval_SetTrace) {
        goto error;
    }

    PythonFunctions->PyDict_GetItemString = (PPYDICT_GETITEMSTRING)GetProcAddress(PythonModule, "PyDict_GetItemString");
    if (!PythonFunctions->PyDict_GetItemString) {
        goto error;
    }

    PythonFunctions->Py_IncRef = (PPY_INCREF)GetProcAddress(PythonModule, "Py_IncRef");
    if (!PythonFunctions->Py_IncRef) {
        goto error;
    }

    PythonFunctions->Py_DecRef = (PPY_DECREF)GetProcAddress(PythonModule, "Py_DecRef");
    if (!PythonFunctions->Py_DecRef) {
        goto error;
    }

    PythonFunctions->PyGILState_Ensure = (PPYGILSTATE_ENSURE)GetProcAddress(PythonModule, "PyGILState_Ensure");
    if (!PythonFunctions->PyGILState_Ensure) {
        goto error;
    }

    PythonFunctions->PyGILState_Release = (PPYGILSTATE_RELEASE)GetProcAddress(PythonModule, "PyGILState_Release");
    if (!PythonFunctions->PyGILState_Release) {
        goto error;
    }

    PythonFunctions->PyUnicode_AsUnicode = (PPYUNICODE_ASUNICODE)GetProcAddress(PythonModule, "PyUnicode_AsUnicode");
    PythonFunctions->PyUnicode_GetLength = (PPYUNICODE_GETLENGTH)GetProcAddress(PythonModule, "PyUnicode_GetLength");

    return TRUE;

error:
    return FALSE;
}

_Check_return_
BOOL
LoadPythonSymbols(
    _In_    HMODULE     PythonModule,
    _Out_   PPYTHON     Python
)
{
    if (!PythonModule) {
        return FALSE;
    }

    if (!Python) {
        return FALSE;
    }

    if (!LoadPythonData(PythonModule, &Python->PythonData)) {
        return FALSE;
    }

    if (!LoadPythonFunctions(PythonModule, &Python->PythonFunctions)) {
        return FALSE;
    }

    Python->PythonModule = PythonModule;

    return TRUE;
}

_Check_return_
BOOL
LoadPythonExData(
    _In_    HMODULE         PythonModule,
    _Out_   PPYTHONEXDATA   PythonExData
)
{
    if (!PythonModule) {
        return FALSE;
    }

    if (!PythonExData) {
        return FALSE;
    }

    return TRUE;
}

_Check_return_
BOOL
LoadPythonExFunctions(
    _In_opt_    HMODULE             PythonExModule,
    _Inout_     PPYTHONEXFUNCTIONS  PythonExFunctions
)
{

    if (!PythonExModule) {
        return FALSE;
    }

    if (!PythonExFunctions) {
        return FALSE;
    }

    PythonExFunctions->GetUnicodeLengthForPythonString = (PGETUNICODELENGTHFORPYTHONSTRING)GetProcAddress(PythonExModule, "GetUnicodeLengthForPythonString");

    PythonExFunctions->ConvertPythonStringToUnicodeString = (PCONVERTPYSTRINGTOUNICODESTRING)GetProcAddress(PythonExModule, "ConvertPythonStringToUnicodeString");

    PythonExFunctions->CopyPythonStringToUnicodeString = (PCOPY_PYTHON_STRING_TO_UNICODE_STRING)GetProcAddress(PythonExModule, "CopyPythonStringToUnicodeString");

    PythonExFunctions->ResolveFrameObjectDetails = (PRESOLVEFRAMEOBJECTDETAILS)GetProcAddress(PythonExModule, "ResolveFrameObjectDetails");

    PythonExFunctions->ResolveFrameObjectDetailsFast = (PRESOLVEFRAMEOBJECTDETAILS)GetProcAddress(PythonExModule, "ResolveFrameObjectDetailsFast");

    PythonExFunctions->GetModuleNameAndQualifiedPathFromModuleFilename = (PGET_MODULE_NAME_AND_QUALIFIED_PATH_FROM_MODULE_FILENAME)GetProcAddress(PythonExModule, "GetModuleNameAndQualifiedPathFromModuleFilename");

    return TRUE;
}

_Check_return_
BOOL
LoadPythonExRuntime(
    _In_opt_    HMODULE PythonExModule,
    _Inout_     PPYTHONEXRUNTIME PythonExRuntime
)
{
    if (!PythonExModule) {
        return FALSE;
    }

    if (!PythonExRuntime) {
        return FALSE;
    }

    PythonExRuntime->StringsHeap = HeapCreate(HEAP_NO_SERIALIZE | HEAP_GENERATE_EXCEPTIONS, 0, 0);

    if (!PythonExRuntime->StringsHeap) {
        goto error;
    }

    return TRUE;
error:
    return FALSE;
}

_Check_return_
BOOL
LoadPythonExSymbols(
    _In_opt_    HMODULE PythonExModule,
    _Inout_     PPYTHON Python
)
{
    HMODULE Module;

    if (!Python) {
        return FALSE;
    }

    if (PythonExModule) {
        Module = PythonExModule;

    } else {
        DWORD Flags = (
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS          |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
        );

        if (!GetModuleHandleEx(Flags, (LPCTSTR)&LoadPythonExFunctions, &Module)) {
            return FALSE;
        }

        if (!Module) {
            return FALSE;
        }
    }

    if (!LoadPythonExData(Module, &Python->PythonExData)) {
        return FALSE;
    }

    if (!LoadPythonExFunctions(Module, &Python->PythonExFunctions)) {
        return FALSE;
    }

    if (!LoadPythonExRuntime(Module, &Python->PythonExRuntime)) {
        return FALSE;
    }

    Python->PythonExModule = Module;

    return TRUE;
}


_Check_return_
BOOL
IsSupportedPythonVersion(_In_ PPYTHON Python)
{
    return (
        (Python->MajorVersion == 2 && (Python->MinorVersion >= 4 && Python->MinorVersion <= 7)) ||
        (Python->MajorVersion == 3 && (Python->MinorVersion >= 0 && Python->MinorVersion <= 5))
    );
}

_Check_return_
BOOL
ResolvePythonOffsets(_In_ PPYTHON Python)
{
    if (!Python) {
        return FALSE;
    }

    if (!IsSupportedPythonVersion(Python)) {
        return FALSE;
    }

    switch (Python->MajorVersion) {
        case 2:
            Python->PyCodeObjectOffsets = &PyCodeObjectOffsets25_27;
            break;
        case 3:
            switch (Python->MinorVersion) {
                case 0:
                case 1:
                case 2:
                    Python->PyCodeObjectOffsets = &PyCodeObjectOffsets30_32;
                    break;
                case 3:
                case 4:
                case 5:
                    Python->PyCodeObjectOffsets = &PyCodeObjectOffsets33_35;
                    break;
                default:
                    return FALSE;
            };
        default:
            return FALSE;
    };

    return TRUE;
}

_Check_return_
BOOL
ResolveAndVerifyPythonVersion(
    _In_    HMODULE     PythonModule,
    _Inout_ PPYTHON     Python
)
{
    PCCH Version;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG PatchLevel = 0;
    CHAR VersionString[2] = { 0 };
    PRTLCHARTOINTEGER RtlCharToInteger;

    RtlCharToInteger = Python->Rtl->RtlCharToInteger;

    Python->Py_GetVersion = (PPY_GETVERSION)GetProcAddress(PythonModule, "Py_GetVersion");
    if (!Python->Py_GetVersion) {
        goto error;
    }
    Python->VersionString = Python->Py_GetVersion();
    if (!Python->VersionString) {
        goto error;
    }

    VersionString[0] = Python->VersionString[0];
    if (FAILED(RtlCharToInteger(VersionString, 0, &MajorVersion))) {
        goto error;
    }
    Python->MajorVersion = (USHORT)MajorVersion;

    Python->MinorVersion = 0;
    Python->PatchLevel = 0;
    Version = &Python->VersionString[1];
    if (*Version == '.') {
        Version++;
        VersionString[0] = *Version;
        if (FAILED(RtlCharToInteger(VersionString, 0, &MinorVersion))) {
            goto error;
        }
        Python->MinorVersion = (USHORT)MinorVersion;

        Version++;
        if (*Version == '.') {
            Version++;
            VersionString[0] = *Version;
            if (FAILED(RtlCharToInteger(VersionString, 0, &PatchLevel))) {
                goto error;
            }
            Python->PatchLevel = (USHORT)PatchLevel;
            Version++;
            if (*Version && *Version >= '0' || *Version <= '9') {
                PatchLevel = 0;
                VersionString[0] = *Version;
                if (FAILED(RtlCharToInteger(VersionString, 0, &PatchLevel))) {
                    goto error;
                }
                Python->PatchLevel = (USHORT)((Python->PatchLevel * 10) + PatchLevel);
            }
        }
    }

    return ResolvePythonOffsets(Python);

error:
    return FALSE;
}

_Check_return_
BOOL
InitializePython(
    _In_        PRTL        Rtl,
    _In_        HMODULE     PythonModule,
    _Out_       PPYTHON     Python,
    _Inout_     PULONG      SizeOfPython
)
{

    if (!Python) {
        if (SizeOfPython) {
            *SizeOfPython = sizeof(*Python);
        }
        return FALSE;
    }

    if (!SizeOfPython) {
        return FALSE;
    }

    if (*SizeOfPython < sizeof(*Python)) {
        return FALSE;
    } else if (*SizeOfPython == 0) {
        *SizeOfPython = sizeof(*Python);
    }

    if (!PythonModule) {
        return FALSE;
    }

    if (!Rtl) {
        return FALSE;
    }

    SecureZeroMemory(Python, sizeof(*Python));

    Python->Rtl = Rtl;

    if (!ResolveAndVerifyPythonVersion(PythonModule, Python)) {
        goto error;
    }

    if (!LoadPythonSymbols(PythonModule, Python)) {
        goto error;
    }

    if (!LoadPythonExSymbols(NULL, Python)) {
        goto error;
    }

    Python->Size = *SizeOfPython;
    return TRUE;

error:
    // Clear any partial state.
    SecureZeroMemory(Python, sizeof(*Python));
    return FALSE;
}

BOOL
GetUnicodeLengthForPythonString(
    _In_    PPYTHON         Python,
    _In_    PPYOBJECT       StringOrUnicodeObject,
    _Out_   PULONG          UnicodeLength
)
{
    if (!Python) {
        return FALSE;
    }

    if (!StringOrUnicodeObject) {
        return FALSE;
    }

    if (!UnicodeLength) {
        return FALSE;
    }

    if (StringOrUnicodeObject->TypeObject == Python->PyString_Type) {


    } else if (StringOrUnicodeObject->TypeObject == Python->PyUnicode_Type) {

    } else {
        return FALSE;
    }

    return FALSE;
}

_Check_return_
BOOL
ConvertPythonStringToUnicodeString(
    _In_    PPYTHON             Python,
    _In_    PPYOBJECT           StringOrUnicodeObject,
    _Out_   PPUNICODE_STRING    UnicodeString,
    _In_    BOOL                AllocateMaximumSize
)
{
    PUNICODE_STRING String;
    LARGE_INTEGER RequiredSizeInBytes;
    LARGE_INTEGER AllocationSizeInBytes;

    if (!Python) {
        return FALSE;
    }

    if (!StringOrUnicodeObject) {
        return FALSE;
    }

    if (!UnicodeString) {
        return FALSE;
    }

    if (StringOrUnicodeObject->TypeObject == Python->PyString_Type) {
        ULONG Index;
        PPYSTRINGOBJECT StringObject = (PPYSTRINGOBJECT)StringOrUnicodeObject;
        RequiredSizeInBytes.QuadPart = StringObject->ObjectSize * sizeof(WCHAR);
        if (RequiredSizeInBytes.QuadPart > (MAX_USTRING-2-sizeof(UNICODE_STRING))) {
            return FALSE;
        }

        if (RequiredSizeInBytes.HighPart != 0) {
            return FALSE;
        }

        if (AllocateMaximumSize) {
            AllocationSizeInBytes.QuadPart = MAX_USTRING - 2;
        } else {
            AllocationSizeInBytes.QuadPart = RequiredSizeInBytes.LowPart + sizeof(UNICODE_STRING);
        }

        if (AllocationSizeInBytes.HighPart != 0) {
            return FALSE;
        }


        String = (PUNICODE_STRING)HeapAlloc(Python->StringsHeap,
                                            HEAP_ZERO_MEMORY,
                                            AllocationSizeInBytes.LowPart);
        if (!String) {
            return FALSE;
        }

        String->Length = (USHORT)RequiredSizeInBytes.LowPart;
        String->MaximumLength = (USHORT)AllocationSizeInBytes.LowPart-sizeof(UNICODE_STRING);
        String->Buffer = (PWSTR)RtlOffsetToPointer(String, sizeof(UNICODE_STRING));

        for (Index = 0; Index < StringObject->ObjectSize; Index++) {
            String->Buffer[Index] = (WCHAR)StringObject->Value[Index];
        }

        *UnicodeString = String;
        return TRUE;

    } else if (StringOrUnicodeObject->TypeObject == Python->PyUnicode_Type) {

        if (Python->PyUnicode_AsUnicode && Python->PyUnicode_GetLength) {

        }

        return FALSE;

    } else {
        return FALSE;
    }

    return FALSE;

}

BOOL
GetPythonStringInformation(
    _In_     PPYTHON             Python,
    _In_     PPYOBJECT           StringOrUnicodeObject,
    _Out_    PSIZE_T             Length,
    _Out_    PUSHORT             Width,
    _Out_    PPVOID              Buffer
)
{
    if (StringOrUnicodeObject->TypeObject == Python->PyString_Type) {

        PPYSTRINGOBJECT StringObject = (PPYSTRINGOBJECT)StringOrUnicodeObject;

        *Length = StringObject->ObjectSize;
        *Buffer = StringObject->Value;

        *Width = sizeof(CHAR);

    } else if (StringOrUnicodeObject->TypeObject == Python->PyUnicode_Type) {

        PPYUNICODEOBJECT UnicodeObject = (PPYUNICODEOBJECT)StringOrUnicodeObject;

        if (Python->PyUnicode_AsUnicode && Python->PyUnicode_GetLength) {

            *Length = Python->PyUnicode_GetLength(StringOrUnicodeObject);
            *Buffer = Python->PyUnicode_AsUnicode(StringOrUnicodeObject);

        } else {

            *Length = UnicodeObject->Length;
            *Buffer = UnicodeObject->String;

        }

        *Width = sizeof(WCHAR);

    } else {

        return FALSE;

    }

    return TRUE;
}

BOOL
CopyPythonStringToUnicodeString(
    _In_     PPYTHON             Python,
    _In_     PPYOBJECT           StringOrUnicodeObject,
    _Inout_  PPUNICODE_STRING    UnicodeString,
    _In_opt_ USHORT              AllocationSize,
    _In_     PALLOCATION_ROUTINE AllocationRoutine,
    _In_opt_ PVOID               AllocationContext
)
{
    PRTL Rtl;
    PUNICODE_STRING String;
    LARGE_INTEGER RequiredSizeInBytes;
    LARGE_INTEGER AllocationSizeInBytes;

    if (!Python) {
        return FALSE;
    }

    if (!StringOrUnicodeObject) {
        return FALSE;
    }

    if (!UnicodeString) {
        return FALSE;
    }

    Rtl = Python->Rtl;

    if (!Rtl) {
        return FALSE;
    }

    if (StringOrUnicodeObject->TypeObject == Python->PyString_Type) {
        ULONG Index;
        PPYSTRINGOBJECT StringObject = (PPYSTRINGOBJECT)StringOrUnicodeObject;
        RequiredSizeInBytes.QuadPart = StringObject->ObjectSize * sizeof(WCHAR);

        if (RequiredSizeInBytes.QuadPart > (MAX_USTRING-2-sizeof(UNICODE_STRING))) {
            return FALSE;
        }

        if (RequiredSizeInBytes.HighPart != 0) {
            return FALSE;
        }

        if (AllocationSize && (AllocationSize > (RequiredSizeInBytes.LowPart + sizeof(UNICODE_STRING)))) {

            AllocationSizeInBytes.QuadPart = AllocationSize + sizeof(UNICODE_STRING);

        } else {

            AllocationSizeInBytes.QuadPart = RequiredSizeInBytes.LowPart + sizeof(UNICODE_STRING);
        }

        if (AllocationSizeInBytes.QuadPart > (MAX_USTRING-2-sizeof(UNICODE_STRING))) {
            return FALSE;
        }

        if (AllocationSizeInBytes.HighPart != 0) {
            return FALSE;
        }

        String = (PUNICODE_STRING)AllocationRoutine(
            AllocationContext,
            AllocationSizeInBytes.LowPart
        );

        if (!String) {
            return FALSE;
        }

        String->Length = (USHORT)RequiredSizeInBytes.LowPart;
        String->MaximumLength = (USHORT)AllocationSizeInBytes.LowPart-sizeof(UNICODE_STRING);
        String->Buffer = (PWSTR)RtlOffsetToPointer(String, sizeof(UNICODE_STRING));

        for (Index = 0; Index < StringObject->ObjectSize; Index++) {
            String->Buffer[Index] = (WCHAR)StringObject->Value[Index];
        }

        *UnicodeString = String;
        return TRUE;

    } else if (StringOrUnicodeObject->TypeObject == Python->PyUnicode_Type) {

        PPYUNICODEOBJECT UnicodeObject = (PPYUNICODEOBJECT)StringOrUnicodeObject;
        UNICODE_STRING Source;

        if (Python->PyUnicode_AsUnicode && Python->PyUnicode_GetLength) {

            RequiredSizeInBytes.QuadPart = Python->PyUnicode_GetLength(StringOrUnicodeObject) * sizeof(WCHAR);
            Source.Buffer = Python->PyUnicode_AsUnicode(StringOrUnicodeObject);

        } else {

            RequiredSizeInBytes.QuadPart = UnicodeObject->Length * sizeof(WCHAR);
            Source.Buffer = UnicodeObject->String;
        }

        if (RequiredSizeInBytes.QuadPart > (MAX_USTRING-2-sizeof(UNICODE_STRING))) {
            return FALSE;
        }

        if (RequiredSizeInBytes.HighPart != 0) {
            return FALSE;
        }

        Source.Length = (USHORT)RequiredSizeInBytes.LowPart;
        Source.MaximumLength = Source.Length;

        if (AllocationSize && (AllocationSize > (RequiredSizeInBytes.LowPart + sizeof(UNICODE_STRING)))) {

            AllocationSizeInBytes.QuadPart = AllocationSize + sizeof(UNICODE_STRING);

        } else {

            AllocationSizeInBytes.QuadPart = RequiredSizeInBytes.LowPart + sizeof(UNICODE_STRING);
        }


        if (AllocationSizeInBytes.QuadPart > (MAX_USTRING-2-sizeof(UNICODE_STRING))) {
            return FALSE;
        }

        if (AllocationSizeInBytes.HighPart != 0) {
            return FALSE;
        }

        String = (PUNICODE_STRING)AllocationRoutine(
            AllocationContext,
            AllocationSizeInBytes.LowPart
        );

        if (!String) {
            return FALSE;
        }

        String->Length = Source.Length;
        String->MaximumLength = (USHORT)AllocationSizeInBytes.LowPart - sizeof(UNICODE_STRING);
        String->Buffer = (PWSTR)RtlOffsetToPointer(String, sizeof(UNICODE_STRING));
        Rtl->RtlCopyUnicodeString(String, &Source);

        return TRUE;

    } else {
        return FALSE;
    }

    return FALSE;
}

BOOL
GetModuleNameFromQualifiedPath(
    _In_     PPYTHON             Python,
    _In_     PUNICODE_STRING     Path,
    _Out_    PPSTRING            ModuleName,
    _In_     PALLOCATION_ROUTINE AllocationRoutine,
    _In_opt_ PVOID               AllocationContext
    )
{
    BOOL Success = FALSE;
    USHORT Offset = 0;
    USHORT Limit = 0;
    USHORT Index;
    USHORT ReversedIndex;
    PRTL Rtl;
    HANDLE HeapHandle;
    RTL_BITMAP Bitmap;
    SIZE_T BitmapBufferSizeInBytes;
    PRTL_FIND_SET_BITS FindSetBits;
    UNICODE_STRING Package;
    CONST static UNICODE_STRING __init__py = RTL_CONSTANT_STRING(L"\\__init__.py");

    if (!ARGUMENT_PRESENT(Python)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Path)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(ModuleName)) {
        return FALSE;
    }

    Rtl = Python->Rtl;
    FindSetBits = Rtl->RtlFindSetBits;

    //
    // Determine if the path ends with __init__.py.
    //

    if (Path->Length >= __init__py.Length) {
        SIZE_T Size, Matched;
        PWCH Filename;

        //
        // Get the offset into the path where "\\__init__.py" would be found,
        // if present.
        //

        Offset = Path->Length - __init__py.Length;
        Filename = (PWCH)Path->Buffer[Offset];

        //
        // Shift length left to account for 2-byte WCHAR strings.
        //

        Size = __init__py.Length << 1;

        //
        // Compare the filename with our "\\__init__.py" string.
        //

        Matched = Rtl->RtlCompareMemory(Filename, __init__py.Buffer, Size);

        if (Matched == Size) {

            //
            // It's a match; the fully qualified path is a filename ending in
            // "__init__.py", so we use the parent directory name for the module
            // name.
            //

            Limit = Offset - 1;

        }
        else {

            //
            // We're not dealing with an __init__.py file, so the current filename
            // will be used in the module name.  If the filename has an extension,
            // we remove it (by adjusting Limit to point to the character before
            // the period).
            //

            WCHAR Char;
            BOOL Found = FALSE;

            Index = Path->Length;

            do {
                //
                // Reverse through the path looking for either a
                // period or backslash.
                //

                Char = Path->Buffer[Index];

                if (Char == L'.') {
                    Found = TRUE;
                    break;
                }
                else if (Char == L'\\') {
                    break;
                }

            } while (Index--);

            if (Found) {
                Limit = Index - 1;
            }

        }
    }

    //
    // Limit marks the end of the path we're using for the module name.
    // If it hasn't been set at this point, default to using the length
    // of the entire path.
    //

    if (!Limit) {
        Limit = Path->Length;
    }

    //
    // Identify the starting offset into the path that we're going to use
    // as the starting point for the module name.  Reversing through the
    // path string, this will be the last directory we find with a file
    // named "__init__.py" (indicating that the directory is a module).
    //

    //
    // Create a reversed bitmap for the backslashes in the path.
    //
    HeapHandle = GetProcessHeap();
    if (!HeapHandle) {
        return FALSE;
    }

    Bitmap.SizeOfBitMap = Path->Length;

    BitmapBufferSizeInBytes = Bitmap.SizeOfBitMap >> 3;

    Bitmap.Buffer = (PULONG)HeapAlloc(HeapHandle,
                                      HEAP_ZERO_MEMORY,
                                      BitmapBufferSizeInBytes);

    if (!Bitmap.Buffer) {
        return FALSE;
    }

    Success = Rtl->FindCharsInUnicodeString(Rtl, Path, L'\\', &Bitmap, TRUE);

    if (!Success) {
        HeapFree(HeapHandle, 0, Bitmap.Buffer);
        return FALSE;
    }

    Package.Buffer = (PWCH)HeapAlloc(HeapHandle,
                                     HEAP_ZERO_MEMORY,
                                     Path->Length);

    if (!Package.Buffer) {
        HeapFree(HeapHandle, 0, Bitmap.Buffer);
        return FALSE;
    }

    Package.Length = Path->Length;
    Package.MaximumLength = Path->MaximumLength;

    RtlCopyMemory(Package.Buffer, Path->Buffer, Path->Length);

    ReversedIndex = 0;

    do {
        ULONG NextBackslashReversed;
        USHORT NextBackslash;

        //
        // Find the next backslash.
        //
        NextBackslashReversed = FindSetBits(&Bitmap, 1, ReversedIndex);

        if (NextBackslashReversed == BITS_NOT_FOUND) {
            //
            // We shouldn't ever get here.
            //
            __debugbreak();
        }

        //
        // Convert the reversed bitmap position into the offset into the
        // buffer.
        //
        NextBackslash = Path->Length - (USHORT)NextBackslashReversed;

    } while (Index--);

    return FALSE;

}

BOOL
GetModuleNameAndQualifiedPathFromModuleFilename(
    _In_     PPYTHON             Python,
    _In_     PPYOBJECT           ModuleFilenameObject,
    _Inout_  PPUNICODE_STRING    Path,
    _Inout_  PPSTRING            ModuleName,
    _In_     PALLOCATION_ROUTINE AllocationRoutine,
    _In_opt_ PVOID               AllocationContext,
    _In_     PFREE_ROUTINE       FreeRoutine,
    _In_opt_ PVOID               FreeContext
    )
{
    BOOL Success;
    BOOL Qualify = FALSE;

    SSIZE_T Length;
    USHORT  Width;
    PVOID   Buffer;

    ULARGE_INTEGER Size;
    ULARGE_INTEGER MaxSize = { MAX_USTRING - 2 };

    USHORT Count;
    PWSTR Dest;

    PUNICODE_STRING String;

    Success = GetPythonStringInformation(
        Python,
        ModuleFilenameObject,
        &Length,
        &Width,
        &Buffer
    );

    if (!Success) {
        return FALSE;
    }

    if (Width != sizeof(CHAR) && Width != sizeof(WCHAR)) {
        __debugbreak();
    }

    //
    // Calculate the size in bytes + trailing NUL.
    //

    Size.QuadPart = (Length * Width) + sizeof(WCHAR);

    //
    // Verify string size doesn't exceed our maximum unicode object size.
    //

    if (Size.HighPart != 0 || Size.LowPart > MaxSize.LowPart) {
        return FALSE;
    }

    //
    // Determine if we need to qualify the path.
    //

    if (Length >= 3) {

        if (Width == sizeof(CHAR)) {

            PSTR Buf = (PVOID)Buffer;

            Qualify = (Buf[1] != ':' || Buf[2] != '\\');

        } else {

            PWSTR Buf = (PWSTR)Buffer;

            Qualify = (Buf[1] != L':' || Buf[2] != '\\');

        }

    } else if (Length >= 2) {

        if (Width == sizeof(CHAR)) {

            PSTR Buf = (PSTR)Buffer;

            Qualify = (Buf[0] != '\\' || Buf[1] != '\\');

        } else {

            PWSTR Buf = (PWSTR)Buffer;

            Qualify = (Buf[0] != L'\\' || Buf[1] != L'\\');

        }
    }

    if (Qualify) {

        //
        // Get the length (in characters) of the current directory and verify it
        // fits within our unicode string size constraints.
        //

        ULONG CurDirLength;
        ULONG RequiredSizeInBytes;

        RequiredSizeInBytes = GetCurrentDirectoryW(0, NULL);
        if (RequiredSizeInBytes == 0) {
            return FALSE;
        }

        //
        // Update the allocation size with the current directory size plus
        // a joining backslash character.
        //

        Size.QuadPart = Size.LowPart + sizeof(WCHAR) + RequiredSizeInBytes;

        //
        // Verify it's within our limits.
        //

        if (Size.HighPart != 0 || Size.LowPart > MaxSize.LowPart) {
            return FALSE;
        }

        //
        // The size is within our max unicode string size; bump the size to
        // reflect the size of the UNICODE_STRING structure, and then proceed
        // with allocation.
        //

        Size.LowPart += sizeof(UNICODE_STRING);

        String = (PUNICODE_STRING)AllocationRoutine(AllocationContext, Size.LowPart);

        if (!String) {
            return FALSE;
        }

        //
        // Point the buffer to the memory immediately after the struct and
        // initialize length and maximum length.
        //

        String->Buffer = (PWSTR)RtlOffsetToPointer(String, sizeof(UNICODE_STRING));
        String->Length = (USHORT)Size.LowPart;
        String->MaximumLength = (USHORT)Size.LowPart;

        //
        // CurDirLength will represent the length (number of characters, not
        // bytes) of the string copied into our buffer.
        //

        CurDirLength = GetCurrentDirectoryW(RequiredSizeInBytes, String->Buffer);
        if (CurDirLength == 0) {
            return FALSE;
        }

        Dest = &String->Buffer[CurDirLength];

        //
        // Add the joining backslash.
        //

        *Dest++ = L'\\';


    } else {

        //
        // Path is already qualified; just allocate a new unicode string.
        //

        String = (PUNICODE_STRING)AllocationRoutine(AllocationContext, Size.LowPart);

        if (!String) {
            return FALSE;
        }

        //
        // Point the buffer to the memory immediately after the struct and
        // initialize length and maximum length.
        //

        String->Buffer = (PWSTR)RtlOffsetToPointer(String, sizeof(UNICODE_STRING));
        String->Length = (USHORT)Size.LowPart;
        String->MaximumLength = (USHORT)Size.LowPart;

        Dest = String->Buffer;

    }

    //
    // Copy the rest of the name over.
    //

    Count = ((USHORT)Length) + 1;

    if (Width == sizeof(CHAR)) {
        PSTR Source = (PSTR)Buffer;

        while (--Count) {
            *Dest++ = (WCHAR)*Source++;
        }

    } else {
        PWSTR Source = (PWSTR)Buffer;

        while (--Count) {
            *Dest++ = *Source++;
        }
    }

    //
    // Add terminating NUL.
    //

    *Dest++ = UNICODE_NULL;

    //
    // Update the user's path pointer.
    //

    *Path = String;

    //
    // We've handled the path, now construct the module name.
    // (XXX TODO.)
    //

    return FALSE;

}

BOOL
GetModuleFilenameStringObjectFromCodeObject(
    _In_    PPYTHON     Python,
    _In_    PPYOBJECT   CodeObject,
    _Inout_ PPPYOBJECT  ModuleFilenameStringObject
)
{
    if (!Python) {
        return FALSE;
    }

    if (!CodeObject) {
        return FALSE;
    }

    if (!ModuleFilenameStringObject) {
        return FALSE;
    }

    *ModuleFilenameStringObject = (PPYOBJECT)RtlOffsetToPointer(CodeObject, Python->PyCodeObjectOffsets->Filename);
    return TRUE;
}

BOOL
GetFunctionNameStringObjectAndLineNumberFromCodeObject(
    _In_    PPYTHON     Python,
    _In_    PPYOBJECT   CodeObject,
    _Inout_ PPPYOBJECT  FunctionNameStringObject,
    _Inout_ PDWORD      LineNumber
)
{
    if (!Python) {
        return FALSE;
    }

    if (!CodeObject) {
        return FALSE;
    }

    if (!FunctionNameStringObject) {
        return FALSE;
    }

    if (!LineNumber) {
        return FALSE;
    }

    *FunctionNameStringObject = *((PPPYOBJECT)RtlOffsetToPointer(CodeObject, Python->PyCodeObjectOffsets->Name));
    *LineNumber = *((PULONG)RtlOffsetToPointer(CodeObject, Python->PyCodeObjectOffsets->FirstLineNumber));

    return TRUE;
}

BOOL
ResolveFrameObjectDetailsFast(
    _In_    PPYTHON         Python,
    _In_    PPYFRAMEOBJECT  FrameObject,
    _Inout_ PPPYOBJECT      CodeObject,
    _Inout_ PPPYOBJECT      ModuleFilenameStringObject,
    _Inout_ PPPYOBJECT      FunctionNameStringObject,
    _Inout_ PULONG          LineNumber
)
{
    ResolveFrameObjectDetailsInline(
        Python,
        FrameObject,
        CodeObject,
        ModuleFilenameStringObject,
        FunctionNameStringObject,
        LineNumber
    );

    return TRUE;
}


BOOL
ResolveFrameObjectDetails(
    _In_    PPYTHON         Python,
    _In_    PPYFRAMEOBJECT  FrameObject,
    _Inout_ PPPYOBJECT      CodeObject,
    _Inout_ PPPYOBJECT      ModuleFilenameStringObject,
    _Inout_ PPPYOBJECT      FunctionNameStringObject,
    _Inout_ PULONG          LineNumber
)
{
    if (!Python) {
        return FALSE;
    }

    if (!CodeObject) {
        return FALSE;
    }

    if (!ModuleFilenameStringObject) {
        return FALSE;
    }

    if (!FunctionNameStringObject) {
        return FALSE;
    }

    if (!LineNumber) {
        return FALSE;
    }

    return ResolveFrameObjectDetailsFast(
        Python,
        FrameObject,
        CodeObject,
        ModuleFilenameStringObject,
        FunctionNameStringObject,
        LineNumber
    );
}

BOOL
GetClassNameStringObjectFromFrameObject(
    _In_    PPYTHON         Python,
    _In_    PPYFRAMEOBJECT  FrameObject,
    _In_    PPPYOBJECT      ClassNameStringObject
)
{
    return FALSE;
}


#ifdef __cpp
} // extern "C"
#endif
