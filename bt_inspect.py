#!/usr/bin/env python3

import argparse
import re
import sys
from subprocess import Popen, PIPE

exec_pat = re.compile(r"> executable: (.+)")
addr_pat = re.compile(r"\[(.+)\]")

blacklist = [
        re.compile(r"^__mi_"),
        re.compile(r"^__splay_"),
        re.compile(r"^_start"),
        # re.compile(r"^\?\? \?\?"),
        ]

parser = argparse.ArgumentParser(description='A small script for crunching backtraces')
parser.add_argument('--executable', metavar='<binary file>',
                    help='the executable that produced the backtrace', default=None)
parser.add_argument('--full', action='store_true',
                    help='show full backtrace without filtering')
parser.add_argument('--nodemangle', action='store_true',
                    help='do not demangle C++ identifier names')
parser.add_argument('input', nargs='*')
args = parser.parse_args()


def shouldBePrinted(s):
    if len(s) == 0:
        return False
    if args.full:
        return True
    for x in blacklist:
        if x.search(s):
            return False
    return True

n = 0
def translate(binname, addr):
    if not args.nodemangle:
        cmd = ['addr2line', '-e' , binname, '-f', '-C', '-i', '-p', addr]
    else:
        cmd = ['addr2line', '-e' , binname, '-f', '-i', '-p', addr]
    process = Popen(cmd, stdout=PIPE, stderr=PIPE)
    stdout, stderr = process.communicate()
    out = stdout.decode("utf-8")
    if not shouldBePrinted(out):
        return
    out = out[:-1]
    out = out.replace('\n','\n  ')
    global n
    print('#{:2} {}'.format(n, out))
    n += 1


def main():
    def crunchFile(infile, inplace = False):
        in_bt = False

        exec_name = None

        lines = list(infile.readlines())

        if len(lines) == 0:
            return

        if lines[0].startswith("TRACE "):
            exec_name = lines[0][6:-1]

        for line in lines:
            if "#################### meminstrument --- backtrace start ####################" in line or line.startswith("  BACKTRACE("):
                assert not in_bt
                in_bt = True
                print("Start of backtrace:")
                continue

            if "#################### meminstrument --- backtrace end ######################" in line or line.startswith("  ENDBACKTRACE("):
                assert in_bt
                in_bt = False
                print("End of backtrace.")
                continue

            if in_bt:
                exec_match = exec_pat.search(line)
                if exec_match:
                    exec_name = exec_match.group(1)
                    print("Backtrace for executable " + exec_name +":")
                    continue

                addr_match = addr_pat.search(line)
                if addr_match:
                    addr_name = addr_match.group(1)
                    if args.executable:
                        translate(args.executable, addr_name)
                    else:
                        translate(exec_name, addr_name)
            elif inplace:
                print(line, end="")

    if len(args.input) == 0:
        crunchFile(sys.stdin, inplace=True, )
    else:
        for fname in args.input:
            with open(fname, "r") as infile:
                crunchFile(infile)


if __name__ == "__main__":
    main()
