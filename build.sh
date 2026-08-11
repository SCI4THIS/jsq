#!/bin/bash

rm build/*

INCLUDES="-g -Isrc/mmap_file/src -Isrc/json"

# Default output is lex.yy.c
flex -o build/flex.out.c src/json/json.l

# -d will output a header file containing tokens to build/bison.out.h
bison -d -o build/bison.out.c src/json/json.y

cp src/mmap_file/build/mmap_file_nix.c.o    build/mmap_file_nix.c.o
gcc ${INCLUDES} -o build/flex.out.c.o    -c build/flex.out.c
gcc ${INCLUDES} -o build/bison.out.c.o   -c build/bison.out.c -Ibuild
gcc ${INCLUDES} -o build/json.c.o        -c src/json/json.c
gcc ${INCLUDES} -o build/json_parser.c.o -c src/json/json_parser.c
gcc ${INCLUDES} -o build/main.c.o        -c src/main.c
gcc -o build/main                           build/*\.o
