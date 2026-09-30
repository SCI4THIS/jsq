#!/bin/bash

mkdir -p build
rm build/*

SANITIZE="-fsanitize=address -g"
INCLUDES="-Isrc/mmap_file/src -Isrc/json -Isrc -Ibuild"

# Default output is lex.yy.c
flex -o build/flex.out.c src/json/json.l

# -d will output a header file containing tokens to build/bison.out.h
bison -d -o build/bison.out.c src/json/json.y

cp src/mmap_file/build/mmap_file_nix.c.o    build/mmap_file_nix.c.o
for FILE in build/flex.out.c build/bison.out.c src/json/json.c src/json/parser.c src/json/schema.c src/json/harness.c src/main.c src/args.c
do
  BASENAME=$(basename ${FILE})
  gcc ${SANITIZE} ${INCLUDES} -o build/${BASENAME}.o -c ${FILE}
  echo "BASENAME: ${BASENAME}"
done

gcc ${SANITIZE} -o build/main                           build/*\.o
