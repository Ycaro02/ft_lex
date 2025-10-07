#!/bin/bash

ROOT_DIR=$(pwd)

source ${ROOT_DIR}/rsc/sh/bash_log.sh

L_FILE=${1}

ARGS="${2}"


if [ -z "${L_FILE}" ]; then
    log E "Usage: $0 <flex_file> [args_to_lexer]"
    exit 1
fi

log I "Using flex file: ${L_FILE}"

log I "With args: ${ARGS}"

lex ${L_FILE}

clang lex.yy.c -o lex.yy -ll

echo -n ${ARGS} | ./lex.yy

