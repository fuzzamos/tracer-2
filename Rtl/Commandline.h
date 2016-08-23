#pragma once

#include "stdafx.h"

#include "Memory.h"

#pragma comment(linker, "shell32")

typedef PPWSTR (COMMAND_LINE_TO_ARGVW)(
  _In_  PWSTR  CommandLine,
  _Out_ PLONG  NumberOfArgs
);
typedef COMMAND_LINE_TO_ARGVW *PCOMMAND_LINE_TO_ARGVW;

typedef PWSTR (WINAPI GET_COMMAND_LINE)(VOID);
typedef GET_COMMAND_LINE *PGET_COMMAND_LINE;

typedef PPSTR *PPPSTR;

typedef
_Success_(return != 0)
BOOLEAN
(ARGVW_TO_ARGVA)(
    _In_ PPWSTR ArgvW,
    _In_ ULONG NumberOfArguments,
    _Out_ PPPSTR ArgvA,
    _In_opt_ PSTRING Argv0,
    _In_ PALLOCATOR Allocator
    );
typedef ARGVW_TO_ARGVA *PARGVW_TO_ARGVA;

ARGVW_TO_ARGVA ArgvWToArgvA;

#define MAX_ARGV 100

_Use_decl_annotations_
BOOLEAN
ArgvWToArgvA(
    PPWSTR ArgvW,
    ULONG NumberOfArguments,
    PPPSTR ArgvAPointer,
    PSTR Argv0,
    PALLOCATOR Allocator
    )
/*++

Routine Description:

    Converts a wide string argv to a utf-8 format one.

Arguments:

    ArgvW - Supplies a pointer to the source argv, obtained from
        CommandLineToArgvW().

    NumberOfArguments - Supplies the number of arguments in the ArgvW
        array.

    ArgvA - Supplies a pointer to an argv array that will receive the
        address of the UTF-8 converted arguments.

    Argv0 - Supplies a pointer to an optional character array that
        will be used for the first (0-index) argv item in the ArgvA
        array.  (If this is NULL, a NULL pointer is placed in the
        first element instead.)

    Allocator - Supplies a pointer to the memory allocator to use.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    USHORT Index;
    USHORT AllocSize;
    PPSTR AnsiArgv;
    PSTR AnsiArg;
    PWSTR UnicodeArg;

    //
    // Validate arguments.
    //

    if (!ARGUMENT_PRESENT(ArgvW)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(ArgvAPointer)) {
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(Allocator)) {
        return FALSE;
    }

    if (NumberOfArguments == 0 || NumberOfArguments > MAX_ARGV) {
        return FALSE;
    }

    //
    // Calculate required space for the argv array.
    //

    AllocSize = (sizeof(PSTR) * NumberOfArguments) + 1;
    __try {
        AnsiArgv = (PPSTR)(
            Allocator->Calloc(
                Allocator->Context,
                NumberOfArguments + 1,
                sizeof(PSTR)
            )
        );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        AnsiArgv = NULL;
        goto Error;
    }

    if (!AnsiArgv) {
        goto Error;
    }

    //
    // Loop through the arguments, get the utf-8 multi-byte size,
    // allocate memory and do the utf-8 conversion into the newly
    // allocated buffer.
    //

    for (Index = 0; Index < NumberOfArguments; Index++) {
        INT Size;

        UnicodeArg = ArgvW[Index];

        if (Index == 0) {
            AnsiArgv[Index] = Argv0;
            continue;
        }

        //
        // Get the number of bytes necessary to hold the string.
        //

        Size = WideCharToMultiByte(
            CP_UTF8,
            0,
            UnicodeArg,
            -1,
            NULL,
            0,
            NULL,
            0
        );

        if (Size <= 0) {
            goto Error;
        }

        //
        // Allocate zeroed memory.
        //

        __try {
            AnsiArg = (PSTR)(
                Allocator->Calloc(Allocator->Context, 1, Size)
            );
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            AnsiArg = NULL;
            goto Error;
        }

        if (!AnsiArg) {
            goto Error;
        }

        //
        // As soon as we've got a valid pointer, assign it to the argv array
        // so that it can be free'd if we encounter an error after this point.
        //

        AnsiArgv[Index] = AnsiArg;

        //
        // Now do the actual conversion against the newly allocated buffer.
        //

        Size = WideCharToMultiByte(
            CP_UTF8,
            0,
            UnicodeArg,
            -1,
            AnsiArg,
            Size,
            NULL,
            0
        );

        if (Size <= 0) {
            goto Error;
        }

    }

    //
    // Success, everything was converted.
    //

    return TRUE;

Error:

    //
    // If the AnsiArgv array exists, loop through and free any pointers
    // we find.
    //

    if (AnsiArgv) {

        //
        // Note that we start the index at 1 here in order to skip the
        // argv[0], which we don't own.
        //

        for (Index = 1; Index < NumberOfArguments; Index++) {

            AnsiArg = AnsiArgv[Index];

            __try {
                Allocator->Free(Allocator->Context, AnsiArg);
                AnsiArg = NULL;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                AnsiArg = NULL;
            }
        }

        __try {
            Allocator->Free(Allocator->Context, AnsiArgv);
            AnsiArgv = NULL;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            AnsiArgv = NULL;
        }
    }

    return FALSE;
}
