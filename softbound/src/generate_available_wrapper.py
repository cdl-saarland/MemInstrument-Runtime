#!/usr/bin/env python3

import os
import argparse
import subprocess
import math
import textwrap
import sys
import shutil
import pathlib

def is_available(program_name):
    """Check if a given program is available."""
    return shutil.which(program_name) is not None


def generate_header(header_name, file_path, includes, namespace, content,
                    short_description, long_description):
    """
    Generate the content of an LLVM C++ header, given all non-generic details as arguments.
    """
    # Calculate the line filler of the first line
    first_line_length = len(
        f"//=== {file_path} - {short_description} *- C++ -*-===//")
    fill_num = 80 - first_line_length
    if fill_num <= 0:
        print(
            f"File path and short description are too long (by {abs(fill_num)} characters)")
        sys.exit(1)
    minus_front = "-" * math.floor(fill_num / 2.0)
    minus_back = "-" * math.ceil(fill_num / 2.0)

    header_text = textwrap.dedent(f"""\
    //==={minus_front} {file_path} - {short_description} {minus_back}*- C++ -*-===//
    //
    //                     The LLVM Compiler Infrastructure
    //
    // This file is distributed under the University of Illinois Open Source
    // License. See LICENSE.TXT for details.
    //
    //===----------------------------------------------------------------------===//
    ///
    /// \\file
    /// {long_description}
    /// THIS FILE IS AUTO-GENERATED. DO NOT UPDATE IT MANUALLY.
    ///
    //===----------------------------------------------------------------------===//

    #ifndef {header_name}
    #define {header_name}
    """) + includes + textwrap.dedent(
    f"""
    namespace {namespace} {{

    """) + content + textwrap.dedent(
        f"""
        }} // end namespace {namespace}

        #endif // {header_name}
        """)
    return header_text

def surround_with_header_stuff(text, number_of_functions, file_path):
    """
    Specify the header content and use it to instantiate the LLVM header template.
    """
    header_name = "SB_WRAPPER_H"
    namespace = "sbmi"
    short_description = "Supported Wrapper Functions"
    long_description = "Contains all wrapper functions available in the SBMI runtime."
    includes = '\n#include "llvm/ADT/SmallVector.h"\n'
    content = "llvm::SmallVector<String, " + \
        str(number_of_functions) + "> available_wrappers = {\n" + text + "}"

    return generate_header(header_name, file_path, includes, namespace, content,
                           short_description, long_description)

def process_file(file_name, out_file_name, verbose):
    """
    Filter all SoftBound function from the given ctags file and generate the C++ header.
    """
    number_of_functions = 0
    result = ''
    with open(file_name, "r") as open_file:
        for line in open_file:

            # ignore ctags metadata
            if line.startswith("!"):
                if verbose:
                    print("Ignore ctag metadata:\t" + line)
                continue

            # ignore helper functions
            if (not line.startswith("softboundcets")) and (not line.startswith("__softboundcets")):
                if verbose:
                    print("Ignore softbound helper function:\t" + line)
                continue

            splitted = line.split("\t")
            function_name = splitted[0]
            result += '\t"' + function_name + '",\n'
            number_of_functions += 1

    result = surround_with_header_stuff(result, number_of_functions, out_file_name)
    return result


def generate_file(list_of_source_files, out_file_name, verbose):
    """
    Find SoftBound function in the given source files and generate a C++ header
    that lists the functions.
    """
    tmp_file_name = "ctags.out"
    if not is_available("ctags"):
        print("Ctags is necessary for this program to work."
              "Please install it (you can find it here: http://ctags.sourceforge.net/).")
        sys.exit(1)

    subprocess.run(["ctags", "-o", tmp_file_name, "--c-types=f"] +
                   list_of_source_files, check=True)
    file_name = out_file_name.split("include/")[-1]
    c_file_content = process_file(tmp_file_name, file_name, verbose)

    # Delete the temporarily generated file
    os.remove(tmp_file_name)

    if verbose:
        print("Header file content:")
        print(c_file_content)

    with open(out_file_name, "w") as generated_header:
        generated_header.write(c_file_content)

    print("'" + out_file_name + "' successfully generated.")

def main():
    """
    Automatically generate a C++ header file that contains information on
    available SoftBound wrappers defined by the run-time library.
    """
    parser = argparse.ArgumentParser(
        description="Generate a file containing all available wrappers of the SoftBound runtime.\n")
    parser.add_argument('-v', '--verbose', action='store_true',
                        help="Print verbose output")
    args = parser.parse_args()

    list_of_source_files = ["softboundcets-wrappers.c",
                            "softboundcets.c", "softboundcets.h"]

    # Generate the directory where the auto-generated header is put
    # TODO change this out_dir when moving this to the meminstrument run-times
    out_dir = os.path.join("include", "SBMI")
    pathlib.Path(out_dir).mkdir(parents=True, exist_ok=True)

    out_file_name = os.path.join(out_dir, "SBWrapper.h")

    generate_file(list_of_source_files, out_file_name, args.verbose)

if __name__ == "__main__":
    main()