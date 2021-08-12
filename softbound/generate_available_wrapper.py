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


def generate_header(header_name, file_path, includes, namespaces, content,
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

    namespaces_begin = ""
    namespaces_end = ""
    for namespace in namespaces:
        namespaces_begin += textwrap.dedent(
            f"""namespace {namespace} {{

            """)
        namespaces_end = textwrap.dedent(
            f"""
                    }} // end namespace {namespace}
            """) + namespaces_end

    long_description_lines = textwrap.wrap(long_description, 76)
    long_description_wrapped = ""
    for entry in long_description_lines:
        long_description_wrapped += "/// " + entry + "\n"

    header_text = (textwrap.dedent(f"""\
    //==={minus_front} {file_path} - {short_description} {minus_back}*- C++ -*-===//
    //
    //                     The LLVM Compiler Infrastructure
    //
    // This file is distributed under the University of Illinois Open Source
    // License. See LICENSE.TXT for details.
    //
    //===----------------------------------------------------------------------===//
    ///
    """) + long_description_wrapped + textwrap.dedent(f"""\
    /// THIS FILE IS AUTO-GENERATED. DO NOT UPDATE IT MANUALLY.
    ///
    //===----------------------------------------------------------------------===//

    #ifndef {header_name}
    #define {header_name}

    """) +
                   includes +
                   namespaces_begin +
                   content +
                   namespaces_end +
                   textwrap.dedent(
                       f"""
                       #endif // {header_name}
                       """))
    return header_text

def surround_with_header_stuff(text, number_of_functions, file_path):
    """
    Specify the header content and use it to instantiate the LLVM header template.
    """
    header_name = "SB_WRAPPER_H"
    namespaces = ["meminstrument", "softbound"]
    short_description = "Wrapper Functions"
    long_description = "Contains all (standard library) wrapper functions available in the SoftBound runtime."
    includes = '#include "llvm/ADT/SmallVector.h"\n\n'
    content = "static llvm::SmallVector<std::string, " + \
        str(number_of_functions) + "> availableWrappers({\n" + text + "});\n"

    return generate_header(header_name, file_path, includes, namespaces, content,
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
            if not line.startswith("softboundcets"):
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
        print("Ctags is necessary for this program to work. Please install it "
              "(http://ctags.sourceforge.net/ or "
              "https://github.com/universal-ctags/ctags).")
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

    list_of_source_files = ["src/softboundcets-wrappers.c"]

    # Generate the directory where the auto-generated header is put
    out_dir = os.path.join("..", "include", "meminstrument-rt")
    pathlib.Path(out_dir).mkdir(parents=True, exist_ok=True)

    out_file_name = os.path.join(out_dir, "SBWrapper.h")

    generate_file(list_of_source_files, out_file_name, args.verbose)

if __name__ == "__main__":
    main()
