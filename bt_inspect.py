#!/usr/bin/env python3

import argparse
import re
import sys
from subprocess import Popen, PIPE

EXEC_PAT = re.compile(r"> executable: (.+)")
ADDR_PAT = re.compile(r"\[(.+)\]")

INGORE_LIST = [
    re.compile(r"^__mi_"),
    re.compile(r"^__splay_"),
    re.compile(r"^_start"),
    # re.compile(r"^\?\? \?\?"),
]

START_IDENT = "meminstrument --- backtrace start"
END_IDENT = "meminstrument --- backtrace end"


def should_be_printed(text, print_ignored_functions):
    if len(text) == 0:
        return False
    if print_ignored_functions:
        return True
    for elem in INGORE_LIST:
        if elem.search(text):
            return False
    return True


def translate(binname, addr, no_demangle, print_ignored_functions):
    if not no_demangle:
        cmd = ['addr2line', '-e', binname, '-f', '-C', '-i', '-p', addr]
    else:
        cmd = ['addr2line', '-e', binname, '-f', '-i', '-p', addr]
    process = Popen(cmd, stdout=PIPE, stderr=PIPE)
    stdout, _ = process.communicate()
    out = stdout.decode("utf-8")
    if not should_be_printed(out, print_ignored_functions):
        return ''
    out = out[:-1]
    out = out.replace('\n', '\n  ')
    return out


def get_backtrace(input_file, given_exec, no_demangle, print_ignored_functions, in_place=False):
    lines = list(input_file.readlines())
    if len(lines) == 0:
        return ""

    exec_name = None
    if lines[0].startswith("TRACE "):
        exec_name = lines[0][6:-1]

    in_bt = False
    line_no = -1
    bt_string = ""
    for line in lines:
        if START_IDENT in line or line.startswith("  BACKTRACE("):
            assert not in_bt
            in_bt = True
            line_no = 0
            bt_string += "Start of backtrace:\n"
            continue

        if END_IDENT in line or line.startswith("  ENDBACKTRACE("):
            assert in_bt
            in_bt = False
            bt_string += "End of backtrace.\n"
            continue

        if in_bt:
            exec_match = EXEC_PAT.search(line)
            if exec_match:
                exec_name = exec_match.group(1)
                bt_string += f"Backtrace for executable {exec_name}:\n"
                continue

            if given_exec:
                exec_name = given_exec

            addr_match = ADDR_PAT.search(line)
            if addr_match:
                addr_name = addr_match.group(1)
                result = translate(exec_name, addr_name, no_demangle,
                                   print_ignored_functions)
                if result:
                    bt_string += f'#{line_no:2} {result}\n'
                    line_no += 1
        elif in_place:
            bt_string += line

    return bt_string


def main():

    parser = argparse.ArgumentParser(
        description='A small script for crunching backtraces')
    parser.add_argument('--executable', metavar='<binary file>',
                        help='the executable that produced the backtrace', default=None)
    parser.add_argument('--full', action='store_true',
                        help='show full backtrace without filtering')
    parser.add_argument('--nodemangle', action='store_true',
                        help='do not demangle C++ identifier names')
    parser.add_argument('input', nargs='*')
    args = parser.parse_args()

    found_backtrace = ""

    if len(args.input) == 0:
        found_backtrace = get_backtrace(
            sys.stdin, args.executable, args.nodemangle, args.full, in_place=True)
        if found_backtrace:
            print(found_backtrace)
    else:
        for file_name in args.input:
            with open(file_name, "r") as input_file:
                found_backtrace = get_backtrace(
                    input_file, args.executable, args.nodemangle, args.full)
            if found_backtrace:
                print(found_backtrace)

    if not found_backtrace:
        print("Backtrace inspector called on input without backtraces!")
        print("Most likely, you intended to pipe things from stderr here.")
        print("You can do so by first redirecting stderr to stdout:")
        print("    <command that produces a backtrace> 2>&1 | {}".format(__file__))


if __name__ == "__main__":
    main()
