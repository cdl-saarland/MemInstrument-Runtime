#!/usr/bin/env python3
import os
import argparse

from filter_unwrapped import get_unwrapped_functions, pretty_print_unwrapped

def get_binary_names(lnt_dir, errors, verbose):

    assert(os.path.exists(lnt_dir))

    # Search for the subfolder with the testresults
    test_folder = ""
    subfolders = [subfolder for subfolder in os.listdir(lnt_dir) if os.path.isdir(os.path.join(lnt_dir, subfolder))]
    for folder in subfolders:
        if folder.startswith("test"):
            test_folder = os.path.join(lnt_dir, folder)

    # Make sure a folder with the expected name was found
    assert(test_folder)
    # ... and that it contains a MultiSource sub-folder
    assert(os.path.exists(os.path.join(test_folder, "MultiSource")))

    binary_names = []
    with open(errors, "r") as error_file:
        for line in error_file:
            if line.startswith("MultiSource"):
                if verbose:
                    print("Work on item: " + line)
                parts = line.split(os.path.sep)
                bin_name = parts[-1].strip() + ".simple"
                new_parts = parts[:-1] + ["Output"] + [bin_name]
                result = os.path.join(*new_parts)
                full_path = os.path.join(test_folder, result)
                if verbose:
                    print("Full path: " + full_path)
                assert(os.path.exists(full_path))
                binary_names.append(full_path)
    return binary_names

def get_unwrapped_for_binary(binary, verbose):
    return get_unwrapped_functions(binary, True, verbose)

def find_all_unwrapped(lnt_dir, errors, verbose):
    unwrapped_per_binary = dict()

    binaries = get_binary_names(lnt_dir, errors, verbose)
    for binary_file in binaries:
        unwrapped_funs = get_unwrapped_for_binary(binary_file, verbose)
        unwrapped_per_binary[binary_file] = unwrapped_funs

    return unwrapped_per_binary


def main():
    """See description below."""
    des = """
    Find unwrapped functions for all MultiSource failures.
    """

    parser = argparse.ArgumentParser(
        description=des, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('error_file', metavar='FILE', help='File containing failing tests')
    parser.add_argument('lnt_dir', metavar='RUNDIR', help='Folder with LNT results')
    parser.add_argument('-v', '--verbose',
                        action='store_true', help='Verbose output')
    args = parser.parse_args()

    unwrapped_per_binary = find_all_unwrapped(args.lnt_dir, args.error_file, args.verbose)
    for binary, unwrapped in unwrapped_per_binary.items():
        print(binary)
        pretty_print_unwrapped(unwrapped, "Number unwrapped in " + binary.split(os.path.sep)[-1])
        print("\n\n")

if __name__ == "__main__":
    main()
