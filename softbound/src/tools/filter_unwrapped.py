#!/usr/bin/env python3
"""
Script to find library functions for which SoftBoundCETS does not provide a
wrapper. It checks binaries built with the instrumentation enabled for
undefined symbols using `nm`.
"""

import argparse
import os
import subprocess
import shutil
import sys
from collections import defaultdict

# Function names that contain the WrappedMarker are considered wrapped, they
# will not be classified as unwrapped.
WrappedMarker = "softboundcets"

# The functions in the IgnoreFunction list are assumed to be ok to ignore, they
# will not be classified as unwrapped.
IgnoredFunction = ["__libc_start_main", "__gmon_start__"]

# List of libraries for which wrappers exist. If a function is encountered that
# is neither defined within the binary nor in any of the known libraries, the
# wrappers are missing for sure.
KnownLibs = ["GLIBC", "OPENSSL", "UUID"]


def is_available(program_name):
    """Check if a given program is available."""
    return shutil.which(program_name) is not None


def split_into_lib_and_name(symbol):
    """
    Split the given string into the library name and the function name.
    The library name will be empty if the function is not defined in a library.
    """
    lib_identifier = "@@"
    if not lib_identifier in symbol:
        return (symbol.strip(), None)

    trimmed = symbol.split(lib_identifier)
    assert len(trimmed) == 2

    return (trimmed[0].strip(), trimmed[1].strip())


def get_symbols(binary_file, verbose):
    """
    Get all function symbols from the given binary file that are not defined
    within the binary itself.
    """
    result = subprocess.run(["nm", "-u", binary_file], check=False,
                            stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    if result.returncode:
        if verbose:
            print("'" + binary_file + "' is not in a readable format for nm.")
        return []
    test = result.stdout.decode("utf-8")
    test = test.split("\n")
    return [sym.strip()[2:] for sym in test if sym.strip()]


def get_undefined_symbols(execs, verbose):
    """
    Get all function symbols from the given binary file or all binary files in
    the folder, that are not defined within the binaries themselves.
    """
    to_check = []
    if os.path.isdir(execs):
        for root, _, files in os.walk(execs):
            for file in files:
                full_path = os.path.join(root, file)
                to_check.append(full_path)
    else:
        to_check.append(execs)

    undefined_symbols_per_file = dict()
    for file in to_check:
        symbol_list = get_symbols(file, verbose)

        # If the file cannot be red by nm, ignore it
        if len(symbol_list) == 0:
            continue
        undefined_symbols_per_file[file] = symbol_list
        if verbose:
            print("'" + file + "' has " +
                  str(len(symbol_list)) + " undefined symbols.")

    return undefined_symbols_per_file


def filter_known_lib_functions(undefined_symbols):
    """
    Generate a list of all functions in libraries and a mapping from library to
    all encountered functions that belong to that library.
    """
    known_lib_functions = set()
    known_functions_per_lib = defaultdict(list)

    for symbols in undefined_symbols.values():
        for symbol in symbols:
            fun_name, lib_name = split_into_lib_and_name(symbol)
            if not lib_name:
                continue
            for known_lib in KnownLibs:
                if known_lib in lib_name:
                    known_lib_functions.add(fun_name)
                    known_functions_per_lib[known_lib].append(fun_name)

    return known_lib_functions, known_functions_per_lib


def filter_ignored_functions(to_filter, verbose):
    """Filter functions that should be ignored"""
    filtered_functions = []
    for unwrapped_function in to_filter:
        if unwrapped_function in IgnoredFunction:
            if verbose:
                print("Ignore function: " + unwrapped_function)
            continue
        filtered_functions.append(unwrapped_function)

    return filtered_functions


def check_for_unknown_libs(undefined_symbols):
    """Find unknown libraries"""
    unknown_libs_used = set()
    for symbols in undefined_symbols.values():
        for symbol in symbols:
            _, lib_name = split_into_lib_and_name(symbol)
            if not lib_name:
                continue

            if not any([library in lib_name for library in KnownLibs]):
                unknown_libs_used.add(lib_name)

    return unknown_libs_used


def add_lib_name_to_functions(unwrapped_functions, known_functions_per_lib):
    """Add the library name to the function that are not wrapped."""
    unwrapped_functions_with_lib_names = set()
    for unwrapped_function in unwrapped_functions:
        for lib in KnownLibs:
            if unwrapped_function in known_functions_per_lib[lib]:
                unwrapped_function = lib + ":" + unwrapped_function
                unwrapped_functions_with_lib_names.add(unwrapped_function)
    unwrapped_functions = unwrapped_functions_with_lib_names
    return unwrapped_functions


def get_unwrapped_functions(execs, show_lib_names, verbose):
    """
    Compute the unwrapped function for all given executables, remove duplicates
    and filter those that should be ignored.
    Abort in case that a unknown library is used.
    """
    undefined_symbols = get_undefined_symbols(execs, verbose)

    unknown_lib_prefixes = check_for_unknown_libs(undefined_symbols)
    if len(unknown_lib_prefixes) != 0:
        print("Libraries not listed as known library: " +
              " ".join(unknown_lib_prefixes))
        sys.exit(1)

    unwrapped_functions, known_functions_per_lib = filter_known_lib_functions(
        undefined_symbols)
    unwrapped_functions = filter_ignored_functions(
        unwrapped_functions, verbose)

    if show_lib_names:
        unwrapped_functions = add_lib_name_to_functions(
            unwrapped_functions, known_functions_per_lib)
    return unwrapped_functions


def pretty_print_unwrapped(names, info_str):
    """Pretty print function for unwrapped function names"""
    if len(names) > 0:
        print("Library functions:")
        for name in sorted(names):
            print("\t" + name)
    print(info_str + ": " + str(len(names)))


def main():
    """See description below."""
    des = """
    Find library functions that SoftBundCETS did not wrap in a given binary
    (or folder with binaries).

    This script can be used if your instrumented binary produces memory safety
    violation errors when executed although you are sure that there should be
    none. This can be happen due to missing library wrappers, which this script
    can find for you.
    """

    parser = argparse.ArgumentParser(
        description=des, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument(
        'BIN', help='The binary file (or a folder containing them) to filter the unwrapped functions from')
    parser.add_argument('--show-lib-names', action='store_true',
                        help='Show the library where the functions comes from')
    # parser.add_argument('--all', action='store_true',
    #                     help='Filter all functions, not only those that involve pointers')
    parser.add_argument('-v', '--verbose',
                        action='store_true', help='Verbose output')
    args = parser.parse_args()

    if not is_available("nm"):
        print("""
        This script relies on nm (GNU binary utilities) to find undefined symbols in binaries.
        nm is a required dependency but cannot be found on your system.
        """)
        sys.exit(1)

    unwrapped_functions = get_unwrapped_functions(
        args.BIN, args.show_lib_names, args.verbose)
    pretty_print_unwrapped(unwrapped_functions,
                           "Number of functions that are unwrapped")


if __name__ == "__main__":
    main()
