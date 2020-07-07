#!/usr/bin/env python3

import os
import argparse
import subprocess

from shutil import which
from shutil import rmtree

def is_available(program_name):
    return which(program_name) is not None

def surround_with_C_header_stuff(text, number_of_functions):
    header_name = "SBMI_WRAPPER_H"
    header_text =  "//===-------- SBMI/Wrapper.h - Supported Wrapper Functions ------*- C++ -*-===//" + "\n"
    header_text += "//" + "\n"
    header_text += "//                     The LLVM Compiler Infrastructure" + "\n"
    header_text += "//" + "\n"
    header_text += "// This file is distributed under the University of Illinois Open Source" + "\n"
    header_text += "// License. See LICENSE.TXT for details." + "\n"
    header_text += "//" + "\n"
    header_text += "//===----------------------------------------------------------------------===//" + "\n"
    header_text += "///" + "\n"
    header_text += "/// \\file" + "\n"
    header_text += "/// Contains all wrapper functions available in the SBMI runtime." + "\n"
    header_text += "///" + "\n"
    header_text += "//===----------------------------------------------------------------------===//" + "\n\n"
    header_text += "#ifndef " + header_name + "\n"
    header_text += "#define " + header_name + "\n\n"
    header_text += '#include "llvm/ADT/SmallVector.h"\n\n'
    header_text += "namespace sbmi {\n\n"
    header_text += 'llvm::SmallVector<String, ' + str(number_of_functions) + '> available_wrappers = {\n'
    header_text += text + "}\n\n"
    header_text += "} // end namespace sbmi\n\n"
    header_text += "#endif // " + header_name + "\n"

    return header_text

def process_file(file_name, verbose):

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

    result = surround_with_C_header_stuff(result, number_of_functions)
    return result

def generate_file(list_of_source_files, verbose):

    tmp_file_name = "ctags.out"
    if not is_available("ctags"):
        print("Ctags is necessary for this program to work. Please install it (you can find it here: http://ctags.sourceforge.net/).")
        exit(1)

    result = subprocess.run(["ctags", "-o", tmp_file_name, "--c-types=f"] + list_of_source_files, check=True)
    c_file_content = process_file(tmp_file_name, verbose)

    # Delete the temporarily generated file
    os.remove(tmp_file_name)

    if verbose:
        print("Header file content:")
        print(c_file_content)

    out_file_name = "Wrapper.h"
    with open(out_file_name, "w") as generated_header:
        generated_header.write(c_file_content)

    print("'" + out_file_name + "' successfully generated.")

def main():
    parser = argparse.ArgumentParser(description="Generate a file containing all available wrappers of the SoftBound runtime.\n")
    parser.add_argument('-v', '--verbose', action='store_true', help="Print verbose output")
    args = parser.parse_args()

    list_of_source_files = ["softboundcets-wrappers.c", "softboundcets.c", "softboundcets.h"]

    generate_file(list_of_source_files, args.verbose)

if __name__ == "__main__":
    main()
