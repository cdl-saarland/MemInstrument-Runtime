#!/usr/bin/env python3

import fileinput

import re

from subprocess import Popen, PIPE

exec_pat = re.compile(r"> executable: (.+)")

def translate(binname, addr):
    process = Popen(['addr2line', '-e' , binname, '-f', '-C', '-i', '-p', addr], stdout=PIPE, stderr=PIPE)
    stdout, stderr = process.communicate()
    print(stdout)

def main():

    in_bt = False

    exec_name = None

    for line in fileinput.input():
        if "#################### meminstrument --- backtrace start ####################" in line:
            assert not in_bt
            in_bt = True
            continue

        if "#################### meminstrument --- backtrace end ######################" in line:
            assert in_bt
            in_bt = False
            exec_name = None
            continue

        if in_bt:
            exec_match = exec_pat.search(line)
            if exec_match:
                exec_name = ratio_match.group(1)
                print("Backtrace for executable " + exec_name +":")
                continue

            if exec_name:
                translate(exec_name, line)


if __name__ == "__main__":
    main()
