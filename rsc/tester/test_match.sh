#!/bin/bash


ROOT_DIR=$(pwd)

source ${ROOT_DIR}/rsc/sh/bash_log.sh

# Colors for OK and KO
BOLD_GREEN="\e[1;32m"
BOLD_RED="\e[1;31m"
BOLD_YELLOW="\e[1;33m"
BOLD_PURPLE="\e[1;35m"

OK="[${BOLD_GREEN}OK${RESET}]"
KO="[${BOLD_RED}KO${RESET}]"

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
    local regex="${1}"
    local test_str="${2}"

    create_lexer_file "${regex}"

    # log N "Testing regex: '${regex}' with input: '${test_str}'"

    local lex_output=$(${LEX} ${LEXER_FILE} "${test_str}")
    local lex_match=$(echo -e "${lex_output}" | grep "Match Rule" | awk '{ for (i=4; i<=NF; i++) printf "%s%s", $i, (i<NF ? OFS : ORS) }')

    local ft_lex_match=$(${FT_LEX_TEST} "${regex}" "${test_str}" | grep "Match Rule" | awk '{ for (i=4; i<=NF; i++) printf "%s%s", $i, (i<NF ? OFS : ORS) }')

    if [[ "${lex_match}" == "${ft_lex_match}" ]]; then
        log I "${OK}: ${BOLD_YELLOW}${regex}${RESET} with input: ${BOLD_PURPLE}${test_str}${RESET}"
        return 0
    else
        log E "${KO}: ${BOLD_YELLOW}${regex}${RESET} with input: ${BOLD_PURPLE}${test_str}${RESET}"
        log E "Expected: '${lex_match}', Got: '${ft_lex_match}'"
        return 1
    fi

}

make -s > /dev/null 2>&1

# Basic tests concat
test_regex "ab" "abbbKO a b ab aba"
test_regex "abc" "abbbKO a b ab aba abc abca abcb"
test_regex "abcd" "abbbKO a b ab aba abc abca abcb abcd abcde"

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

# Test alt |
test_regex "ab|ko" "abbbKO a ko ok KOko"
test_regex "(ab|ko)+" "abbbKO a ko ok KOko"

# Test alt | and +
test_regex "ab|ko|0+" "abbbKO a b ab aba 0000 00 0 000"
test_regex "(ab|ko|0+)+" "abbbKO a b ab aba 0000 00 0 000"

# Test all matched
test_regex ".*" "aXb1 ( aYb aZb a b ab"

# hard case
# test_regex "l?|ab" "al all lll aXb1 ( aYb aZb a b ab b bb bbb ab ab ab"
# test_regex "a.*b" "aXb1 ( aYb aZb a b ab"

# Test class []
# test_regex "[abc]+" "abbbKO a b ab aba 0000 00"


rm -f ${LEXER_FILE}