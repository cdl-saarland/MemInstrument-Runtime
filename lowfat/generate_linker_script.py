#!/usr/bin/env python3
"""
Generate the linker script for the low-fat pointer.
"""
import argparse
import sys
import json
import math
from decimal import Decimal
from pathlib import Path

REQUIRED_DEFINTIONS = ["HEAP_REGION_SIZE",
                       "GLOBAL_REGION_SIZE",
                       "STACK_REGION_SIZE",
                       "MIN_ALLOC_SIZE",
                       "MAX_HEAP_ALLOC_SIZE",
                       "MAX_STACK_ALLOC_SIZE",
                       "MAX_GLOBAL_ALLOC_SIZE",
                       "STACK_SIZE"]


def verify_completeness(config, verbose):
    """
    Verify that the given config contains all required variables.
    """
    for variable in REQUIRED_DEFINTIONS:
        if not variable in config:
            print(f"Missing required variable in config file: {variable}")
            sys.exit(1)

    if verbose:
        print("All variables found.")


def parse_config_file(config_file, verbose):
    """
    Parse the given file into a dictionary.
    """
    json_content = None
    with open(config_file, 'r') as file_content:
        json_content = json.load(
            file_content, parse_int=Decimal, parse_float=Decimal)

    if not json_content:
        print(f"Could not parse json file '{config_file}'")
        sys.exit(1)

    if verbose:
        print(f"Successfully parsed '{config_file}'.")

    return json_content

def write_includes(out_file, _):
    """
    Write the necessary includes to the header file.
    """
    includes = ["stddef.h", "stdint.h", "sys/types.h"]
    with open(out_file, "w") as open_file:
        res = "#pragma once\n\n"
        for incl in includes:
            res += f"#include <{incl}>\n"
        res += "\n"
        open_file.write(res)


def write_defines(out_file, defines, verbose):
    """
    Use the given defines to generate a C file with them.
    """
    with open(out_file, "a") as open_file:
        for name, value in defines.items():
            define_line = f"#define {name} {value}"
            if value > 64:
                define_line += "ULL"
            else:
                define_line += "U"
            define_line += "\n"
            if verbose:
                print(define_line)
            open_file.write(define_line)


def decimal_log_2(number):
    """
    Compute log2 on a decimal. Ensure that the result is an integer.
    """
    result = Decimal(math.log2(number))
    return result.to_integral_exact()


def to_int(some_dec):
    """
    Convert the given decimal to an integer. Throw an error if it is not an
    integer but a floating point value.
    """
    return int(some_dec.to_integral_exact())


def get_array(name, content_type, content):
    """
    Create an array with the given name, type and content.
    """
    arr_string = f"static const {content_type} {name}[{len(content)}] = {{\n"
    for entry in content:
        arr_string += f"    {entry},\n"
    arr_string += "};\n"

    return arr_string


def get_values_for_index(min_size, max_size, func):
    """
    Compute the content of the array for the powers of two in the
    range [min_size, max_size]. Use the given function to transform the size.
    """
    result = []
    for i in range(0, 65):
        size_b = 2**i
        if size_b < min_size:
            result.append(func(min_size))
            continue
        if size_b > max_size:
            result.append(0)
            continue
        result.append(func(size_b))

    result.reverse()
    return result


def get_sizes(min_size, max_size):
    """
    Compute the sizes array. Min/Max size are given in bytes.
    """
    return get_values_for_index(min_size, max_size, lambda x: x)


def compute_mask(size):
    """
    Compute the mask value for the given size.
    """
    min_mask = (1 << 64) - 1 - (size - 1)
    return f"{hex(min_mask)}ULL"


def get_masks(min_size, max_size):
    """
    Compute the masks array. Min/Max size are given in bytes.
    """
    return get_values_for_index(min_size, max_size, compute_mask)


def compute_offset(size, number_of_regions, region_size):
    """
    Compute the offset value for the given size.
    """
    return (size - number_of_regions) * region_size


def get_offsets(min_size, max_size, number_of_regions, region_size):
    """
    Compute the offsets array. Min/Max size are given in bytes.
    """
    func = lambda x: compute_offset(decimal_log_2(x), number_of_regions, region_size)
    return get_values_for_index(min_size, max_size, func)


def generate_sizes_header_and_add_derived_vars(config_dict, out_file, verbose):
    """
    Generate the header file containing the sizes for the low fat regions and
    derived values.
    """
    verify_completeness(config_dict, verbose)

    # Define the offsets for heap/stack/globals within each region
    config_dict["HEAP_REGION_OFFSET"] = Decimal(0)
    config_dict["GLOBAL_REGION_OFFSET"] = config_dict["HEAP_REGION_OFFSET"] + \
        config_dict["HEAP_REGION_SIZE"]
    config_dict["STACK_REGION_OFFSET"] = config_dict["GLOBAL_REGION_OFFSET"] + \
        config_dict["GLOBAL_REGION_SIZE"]

    # Derive the size for each region from the sizes of the individual components
    config_dict["REGION_SIZE"] = config_dict["HEAP_REGION_SIZE"] + \
        config_dict["GLOBAL_REGION_SIZE"] + config_dict["STACK_REGION_SIZE"]
    config_dict["REGION_SIZE_LOG"] = decimal_log_2(config_dict["REGION_SIZE"])

    config_dict["MIN_ALLOC_SIZE_LOG"] = decimal_log_2(
        config_dict["MIN_ALLOC_SIZE"])

    # Generate powers of two from min to max
    lower = decimal_log_2(config_dict["MIN_ALLOC_SIZE"])
    upper = decimal_log_2(config_dict["MAX_HEAP_ALLOC_SIZE"])
    pows_two = [2**i for i in range(int(lower), int(upper) + 1)]
    number_of_alloc_sizes = len(pows_two)
    if verbose:
        print(
            f"Possible heap allocation sizes: {pows_two}\nNumber of sizes: {len(pows_two)}")

    config_dict["NUM_REGIONS"] = Decimal(number_of_alloc_sizes)

    # Place the stack after the other regions
    config_dict["BASE_STACK_REGION_NUM"] = config_dict["NUM_REGIONS"] + 1

    unsigned_type = "uint64_t"
    signed_type = "int64_t"
    min_size = to_int(config_dict["MIN_ALLOC_SIZE"])
    max_size = config_dict["MAX_STACK_ALLOC_SIZE"]

    sizes = get_array("STACK_SIZES", unsigned_type, get_sizes(min_size, max_size))
    masks = get_array("STACK_MASKS", unsigned_type,
                      get_masks(min_size, max_size))
    offsets = get_array("STACK_OFFSETS", signed_type, get_offsets(min_size,
                                                                  max_size,
                                                                  config_dict["NUM_REGIONS"] + 4,
                                                                  config_dict["REGION_SIZE"]))
    if verbose:
        print(f"Sizes array:\n{sizes}\n")
        print(f"Masks array:\n{masks}\n")
        print(f"Offsets array:\n{offsets}\n")

    write_includes(out_file, verbose)

    write_defines(out_file, config_dict, verbose)

    # Write the arrays
    with open(out_file, "a") as open_file:
        open_file.write(f"\n{sizes}\n{masks}\n{offsets}")



def bytes_to_gib(bytes_value):
    """
    Convert from B to GiB.
    """
    return bytes_value / Decimal(1024*1024*1024)


def get_section_definition(region, region_base, region_size, region_size_gib, is_read_only):
    """
    Generate a string that describes the section with the given properties.
    """
    base = hex(to_int(region_base))
    size = hex(to_int(region_size))
    if is_read_only:
        region = "read_only_" + region
    return (f'\t. = {base} + SIZEOF_HEADERS;\n'
            f'\tlf_section_{region} : {{KEEP(*(lf_section_{region}))}}\n'
            f'\tASSERT(. < {base} + SIZEOF_HEADERS + {size}, "LF region '
            f'for size {region} globals is too full (>{region_size_gib}GiB).")\n\n')


def generate_linker_script(config_dict, linker_script_name, verbose):
    """
    Generate the linker script from the information given by the config.
    """
    # Compute the region beginning for every region of globals
    offsets = []
    for i in range(0, to_int(config_dict["NUM_REGIONS"])):
        global_region_offset = (
            (i + 1) * config_dict["REGION_SIZE"] + config_dict["GLOBAL_REGION_OFFSET"])
        offsets.append(global_region_offset)

    if verbose:
        print(f"Generate {linker_script_name}")

    # Split the general region for globals in two parts for the constant and
    # non-constant globals of a size
    region_size = config_dict["GLOBAL_REGION_SIZE"] / Decimal(2)
    region_size_gib = bytes_to_gib(region_size)
    region_num = decimal_log_2(config_dict["MIN_ALLOC_SIZE"])

    # Generate the linker script file containing lowfat sections
    with open(linker_script_name, "w") as open_file:
        open_file.write("SECTIONS {\n")
        for offset in offsets:
            region = str(2**region_num)
            section_str = get_section_definition(
                region, offset, region_size, region_size_gib, False)
            section_str += get_section_definition(
                region, offset + region_size, region_size, region_size_gib, True)
            open_file.write(section_str)
            region_num += 1

        open_file.write("}\nINSERT AFTER .gnu.attributes;\n")


def generate_sizes_header_and_linker_script(config_file, sizes_path, script_path, verbose):
    """
    Generate the header file for C/C++ with the configured sizes.
    Additionally generate a linker script for correctly placing global variables
    given the config values.
    """
    config_dict = parse_config_file(config_file, verbose)

    generate_sizes_header_and_add_derived_vars(
        config_dict, sizes_path, verbose)

    generate_linker_script(config_dict, script_path, verbose)


def main():
    """See description below."""
    des = """Use the LowFat config file to generate
                1) a header file describing (heap-/stack-/global-) region sizes and derived sizes, and
                2) a linker script to place globals in sections according to the sizes."""
    parser = argparse.ArgumentParser(
        description=des, formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--config', type=Path,
                        help='Configuration for which the sizes header and the '
                             'linker script should be generated.',
                        default='lf_config.json')
    parser.add_argument('--sizes', type=Path,
                        help='The path of the generated file that describes the'
                             ' LowFat sizes.',
                        default='src/sizes.h')
    parser.add_argument('--scriptname', type=Path,
                        help='The path of the generated linker script file.',
                        default='build/lowfat.ld')
    parser.add_argument('-v', '--verbose',
                        action='store_true', help='Verbose output')
    args = parser.parse_args()

    assert args.config.exists()

    generate_sizes_header_and_linker_script(
        args.config, args.sizes, args.scriptname, args.verbose)


if __name__ == "__main__":
    main()
