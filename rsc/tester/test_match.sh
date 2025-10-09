#!/bin/bash


ROOT_DIR=$(pwd)

source ${ROOT_DIR}/rsc/sh/bash_log.sh


BOLD_YELLOW="\e[1;33m"
BOLD_PURPLE="\e[1;35m"

LEX=${ROOT_DIR}/rsc/run_lex.sh

LEXER_FILE="test_match.l"
FT_LEX_TEST="./ft_lex"


function create_lexer_file() {
    local regex="${1}"

    # log N "Creating rule: ${regex}"

    cat << EOF > ${LEXER_FILE}
%%

${regex} printf("\nMatch Rule: ${regex} %s\\n", yytext);

EOF
}

function test_regex() {
    local regex=${1}
    local test_str=${2}

    create_lexer_file "${regex}"

    # log N "Testing regex: '${regex}' with input: '${test_str}'"

    # echo "TEST REGEX: ${regex}"
    # echo "TEST STR: ${test_str}"

    local lex_output=$(${LEX} ${LEXER_FILE} ${test_str})
    local lex_match=$(echo -e "${lex_output}" | grep "Match Rule" | cut -d ' ' -f 4-)
    local ft_lex_match=$(${FT_LEX_TEST} "${regex}" ${test_str} | grep "Match Rule" | cut -d ' ' -f 4- )

    if [[ "${lex_match}" == "${ft_lex_match}" ]]; then
        log OK "${BOLD_YELLOW}${regex}${RESET} with input: ${BOLD_PURPLE}${test_str}${RESET}"
        return 0
    else
        log KO "${BOLD_YELLOW}${regex}${RESET} with input: ${BOLD_PURPLE}${test_str}${RESET}"
        log E "Expected:\n\n${lex_match}\n\nGot:\n${ft_lex_match}"
        return 1
    fi

}

make -s > /dev/null 2>&1

function test_class() {
    test_regex '[a-z*]' '*aXb1 * TE'
    test_regex '[0-38-9^@A-B*]' '*aXb1  ( aYb aZb a b ab ko KOko 456 9 ^ @ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z'
    test_regex '[a-z]' 'aXb1 ( aYb aZb a b ab ko KOko'
}

function full_test() {
    # Basic tests concat
    test_regex "ab" "abbbKO a b ab aba"
    test_regex "abcd" "abbbKO a b ab aba abc abca abcb abcd abcde"
    test_regex "abcd|ab" "aXkob abcdasYb aZb a b ab adbabkoskkpokkod"
    test_regex "ab|abcd" "aXkabcdaboab abcdasYb aZbabcd a b ab adbabkoskkpokkod"

    # Basic tests +
    test_regex "ab+" "abbbKO a b ab aba"
    test_regex "a+b" "abbbKO a b ab aba"
    test_regex "a+b+" "abbbKO a b ab aba"

    # Test parenthese and +
    test_regex "(ab)+" "abbbKO a b ab aba aabb abbba"
    test_regex "(ab)+b" "abbbKO a b ab aba aabb abbba"
    test_regex "ko|(ab)+" "aXb aYb aZb a b ab abbbbb"
   
    # Test *
    test_regex "a*b" "aXb aYb aZb a b ab"
    test_regex "ab*" "aXb aYb aZb a b ab"

    # Test .
    test_regex "." "aXb1 ( aYb aZb a b ab"
    test_regex "a.b" "aXb aYb aZb a b ab"
    test_regex "a.*" "aXb1 ( aYb aZb a b ab"


    # Test optional ?
    test_regex "ab?" "aXb aYb aZb a b ab"
    test_regex "a?b" "aXb aYb aZb a b ab"
    test_regex "a?b?" "aXb aYb aZb a b ab"
    test_regex "(ab?)|b?" "dsaabababababbbabbaa"

    # Test alt |
    test_regex "ab|ko" "abbbKO a ko ok KOko"
    test_regex "(ab|ko)+" "abbbKO a ko ok KOko"

    # Test alt | and +
    test_regex "ab|ko|0+" "abbbKO a b ab aba 0000 00 0 000"
    test_regex "(ab|ko|0+)+" "abbbKO a b ab aba 0000 00 0 000"

    # Test all matched
    test_regex ".*" "aXb1 ( aYb aZb a b ab"

    # hard case
    test_regex "a.*b" "aXb1 ( aYb aZb a b ab"
    test_regex "l?|ab" "al all lll aXb1 ( aYb aZb a b ab b bb bbb ab ab ab"

    # test_regex "[a-z]" "aXb1 ( aYb aZb a b ab ko KOko"

}

function no_op_test {
    test_regex "ab" "aXb aYb aZb a b ab"
    test_regex "ab|ko" "aXkob aYb aZb a b ab adbabkoskkpokkod"
    test_regex "aZb|ab|ko" "aXkob aYb aZb a b ab adbabkoskkpokkod"
    test_regex "abcd|ab" "aXkob abcdasYb aZb a b ab adbabkoskkpokkod"
    test_regex "ab|abcd" "aXkabcdaboab abcdasYb aZbabcd a b ab adbabkoskkpokkod"
}

# no_op_test

full_test
test_class


rm -f ${LEXER_FILE}