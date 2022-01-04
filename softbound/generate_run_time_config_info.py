#!/usr/bin/env python3
"""
Generates C++ header which contains information on the C-runtime config.
"""
import os
import argparse
import pathlib

from generate_available_wrapper import generate_header


def translate_to_CXX(feature_dict):
    """
    Translate the given dict to C++ defines.
    """
    result = ""
    for key, bool_value in feature_dict.items():
        key = key.strip()
        string_value = "0"
        if bool_value:
            string_value = "1"
        result += f"#define {key} {string_value}\n"

    return result


def compute_defines(feature_file, features, verbose):
    """
    Find all defined features and translate them to C++ defines.
    """
    feature_set = dict()
    with open(feature_file) as open_file:
        file_content = open_file.read()
        if verbose:
            print("Feature file content: ")
            print(file_content)

        for feature in features:
            if verbose:
                print("Check feature: " + feature)
                print("Used: " + str(feature in file_content))
            feature_set[feature] = feature in file_content

    return translate_to_CXX(feature_set)


def generate_config_info(feature_file, features, out_file_name, verbose):
    """
    Find the defined features and write them to the C++ header.
    """
    file_name = out_file_name.split("include/")[-1]
    defines = compute_defines(feature_file, features, verbose)

    include_guard = "SB_RT_CONFIG_INFO"
    namespace = ["meminstrument", "softbound"]
    short_description = "Run-Time Configuration"
    long_description = "Information on how the SoftBound run-time is configured."

    file_content = generate_header(
        include_guard, file_name, "", namespace, defines, short_description, long_description)
    if verbose:
        print("Header file content:")
        print(file_content)

    with open(out_file_name, "w") as generated_header:
        generated_header.write(file_content)

    print(f"'{out_file_name}' successfully generated.")


def main():
    """
    Automatically generate a C++ header file that contains information for which
    safety features the C run-time is configured.
    """
    parser = argparse.ArgumentParser(
        description="Generate a ready-to-include C++ file containing "
                    "information on SoftBound safety features configured in "
                    "the run-time.\n")
    parser.add_argument('-v', '--verbose', action='store_true',
                        help="Print verbose output")
    args = parser.parse_args()

    feature_file = "build/compile_commands.json"

    # Generate the directory where the auto-generated header is put
    out_dir = os.path.join("..", "include", "meminstrument-rt")
    pathlib.Path(out_dir).mkdir(parents=True, exist_ok=True)

    out_file_name = os.path.join(out_dir, "SBRTInfo.h")

    # Defines all features that should be communicated to the C++ Pass.
    # Extend this feature list to communicate more features.
    features = ["__SOFTBOUNDCETS_SPATIAL ",
                "__SOFTBOUNDCETS_TEMPORAL",
                "__SOFTBOUNDCETS_SPATIAL_TEMPORAL",
                "MIRT_STATISTICS"]

    generate_config_info(feature_file, features, out_file_name, args.verbose)


if __name__ == "__main__":
    main()
