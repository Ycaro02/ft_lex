#!/bin/bash

L_FILE=${1}


ARGS="${2}"

if [ -z "$L_FILE" ]; then
    echo "Usage: $0 <flex_file.l>"
    exit 1
fi

echo "Using flex file: ${L_FILE}"
echo "With args: ${ARGS}"

lex ${L_FILE}

clang lex.yy.c -o lex.yy -ll

echo -n ${ARGS} | ./lex.yy

