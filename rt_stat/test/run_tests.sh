#! /bin/bash

clang -L../build -lrt_stat test1.c test2.c -o test_bin

LD_LIBRARY_PATH=../build ./test_bin
