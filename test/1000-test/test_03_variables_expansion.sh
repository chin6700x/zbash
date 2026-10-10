#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 03 - VARIABLES & EXPANSIONS (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 03/10] VARIABLES & EXPANSIONS (100 DIFFERENTIAL PARITY TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. Basic Variable Assignment & Expansion (Tests 001 - 010) ---
run_diff_test 1   "plain string variable assignment"               'X="hello"; echo "X:$X"'
run_diff_test 2   "numeric string variable assignment"             'X=12345; echo "X:$X"'
run_diff_test 3   "empty string variable assignment"               'X=""; echo "X:[$X]"'
run_diff_test 4   "unset variable expands to empty"                'unset X; echo "X:[$X]"'
run_diff_test 5   "variable with underscore in name"               'MY_VAR_1="zephyr"; echo "V:$MY_VAR_1"'
run_diff_test 6   "variable with digits in name"                   'V2="apple"; echo "V:$V2"'
run_diff_test 7   "multiple variables in same string"              'A="foo"; B="bar"; echo "$A-$B"'
run_diff_test 8   "braced variable next to literal text"           'V="test"; echo "${V}ing"'
run_diff_test 9   "concatenation of two variables"                 'A="hello"; B="world"; echo "$A$B"'
run_diff_test 10  "variable re-assignment overwrites value"        'X="one"; X="two"; echo "X:$X"'

# --- 2. Default Values ${VAR:-default} & ${VAR-default} (Tests 011 - 020) ---
run_diff_test 11  "default value when variable unset"              'unset X; echo "[${X:-fallback}]"'
run_diff_test 12  "default value when variable empty string"       'X=""; echo "[${X:-fallback}]"'
run_diff_test 13  "default value ignored when variable has value"  'X="active"; echo "[${X:-fallback}]"'
run_diff_test 14  "no-colon default when variable unset"           'unset X; echo "[${X-fallback}]"'
run_diff_test 15  "no-colon default when variable empty string"    'X=""; echo "[${X-fallback}]"'
run_diff_test 16  "no-colon default when variable has value"       'X="active"; echo "[${X-fallback}]"'
run_diff_test 17  "default value referencing another variable"     'unset X; Y="ref"; echo "[${X:-$Y}]"'
run_diff_test 18  "default value containing spaces"                'unset X; echo "[${X:-default with spaces}]"'
run_diff_test 19  "multiple default expansions in one string"      'unset A B; echo "${A:-first} and ${B:-second}"'
run_diff_test 20  "default value with numbers and symbols"         'unset X; echo "${X:-val_123#45}"'

# --- 3. Assign Default Values ${VAR:=default} & ${VAR=default} (Tests 021 - 030) ---
run_diff_test 21  "assign default when variable unset"             'unset X; echo "[${X:=assigned}]"'
run_diff_test 22  "variable retains assigned value after :="       'unset X; echo "${X:=newval}"; echo "X:$X"'
run_diff_test 23  "assign default does not overwrite existing"     'X="keep"; echo "[${X:=assigned}]"'
run_diff_test 24  "variable retains existing value after :="       'X="keep"; echo "${X:=assigned}"; echo "X:$X"'
run_diff_test 25  "assign default when variable empty string"      'X=""; echo "[${X:=assigned}]"'
run_diff_test 26  "variable updated when previously empty"         'X=""; echo "${X:=assigned}"; echo "X:$X"'
run_diff_test 27  "assign default numeric value"                   'unset N; echo "${N:=999}"; echo "N:$N"'
run_diff_test 28  "two variables assigned with := in line"         'unset A B; echo "${A:=10}-${B:=20}"; echo "$A-$B"'
run_diff_test 29  "no-colon assign default when unset"             'unset X; echo "${X=val}"; echo "X:$X"'
run_diff_test 30  "no-colon assign default when empty string"      'X=""; echo "${X=val}"; echo "X:[$X]"'

# --- 4. Alternate Values ${VAR:+alternate} & ${VAR+alternate} (Tests 031 - 040) ---
run_diff_test 31  "alternate value when variable has value"        'X="valid"; echo "[${X:+alternate}]"'
run_diff_test 32  "alternate value when variable unset"            'unset X; echo "[${X:+alternate}]"'
run_diff_test 33  "alternate value when variable empty string"     'X=""; echo "[${X:+alternate}]"'
run_diff_test 34  "original variable retains value after :+"       'X="original"; echo "${X:+alt}"; echo "X:$X"'
run_diff_test 35  "no-colon alternate when variable has value"     'X="valid"; echo "[${X+alternate}]"'
run_diff_test 36  "no-colon alternate when variable empty string"  'X=""; echo "[${X+alternate}]"'
run_diff_test 37  "no-colon alternate when variable unset"         'unset X; echo "[${X+alternate}]"'
run_diff_test 38  "alternate value with spaces"                    'X="yes"; echo "[${X:+has alternate text}]"'
run_diff_test 39  "alternate value referencing another variable"   'X="on"; Y="extra"; echo "[${X:+$Y}]"'
run_diff_test 40  "multiple alternate expansions in one line"      'A="1"; unset B; echo "${A:+A_set}-${B:+B_set}"'

# --- 5. String Length ${#VAR} (Tests 041 - 050) ---
run_diff_test 41  "string length of regular word"                  'X="hello"; echo "len:${#X}"'
run_diff_test 42  "string length of empty string"                  'X=""; echo "len:${#X}"'
run_diff_test 43  "string length of unset variable"                 'unset X; echo "len:${#X}"'
run_diff_test 44  "string length of string with spaces"            'X="one two three"; echo "len:${#X}"'
run_diff_test 45  "string length of numeric string"                'X="1234567890"; echo "len:${#X}"'
run_diff_test 46  "string length of long sentence"                 'X="the quick brown fox jumps over the lazy dog"; echo "len:${#X}"'
run_diff_test 47  "string length in arithmetic test"               'X="apple"; (( ${#X} == 5 )); echo "exit:$?"'
run_diff_test 48  "string length arithmetic sum"                   'A="foo"; B="barbaz"; echo "$(( ${#A} + ${#B} ))"'
run_diff_test 49  "string length of single character"              'X="Z"; echo "len:${#X}"'
run_diff_test 50  "string length of punctuation string"            'X="!@#$%^&*()"; echo "len:${#X}"'

# --- 6. Substring Slicing ${VAR:offset} & ${VAR:offset:len} (Tests 051 - 060) ---
run_diff_test 51  "slice from offset to end"                       'S="hello world"; echo "[${S:6}]"'
run_diff_test 52  "slice with offset and length"                   'S="hello world"; echo "[${S:0:5}]"'
run_diff_test 53  "slice middle segment"                           'S="abcdefghij"; echo "[${S:3:4}]"'
run_diff_test 54  "slice single character"                         'S="zephyr"; echo "[${S:2:1}]"'
run_diff_test 55  "slice beyond length of string"                  'S="short"; echo "[${S:20}]"'
run_diff_test 56  "slice length exceeds remainder"                 'S="abcdef"; echo "[${S:4:10}]"'
run_diff_test 57  "slice from offset 0 entire string"              'S="complete"; echo "[${S:0:8}]"'
run_diff_test 58  "slice length 0 returns empty"                   'S="sample"; echo "[${S:2:0}]"'
run_diff_test 59  "concatenation of two slices"                    'S="0123456789"; echo "${S:0:3}-${S:7:3}"'
run_diff_test 60  "slice on numeric string"                        'N="20261008"; echo "Year:${N:0:4} Month:${N:4:2} Day:${N:6:2}"'

# --- 7. Pattern Substitution ${VAR/pat/rep} & ${VAR//pat/rep} (Tests 061 - 070) ---
run_diff_test 61  "replace first occurrence"                       'S="banana"; echo "${S/a/o}"'
run_diff_test 62  "replace all occurrences"                        'S="banana"; echo "${S//a/o}"'
run_diff_test 63  "replace first word"                             'S="cat and cat"; echo "${S/cat/dog}"'
run_diff_test 64  "replace all words"                              'S="cat and cat"; echo "${S//cat/dog}"'
run_diff_test 65  "replace pattern not found returns unchanged"    'S="zephyr"; echo "${S/xyz/abc}"'
run_diff_test 66  "delete first occurrence"                        'S="banana"; echo "${S/a}"'
run_diff_test 67  "delete all occurrences"                         'S="banana"; echo "${S//a}"'
run_diff_test 68  "replace spaces with hyphens"                    'S="one two three"; echo "${S// /-}"'
run_diff_test 69  "replace numbers in text"                        'S="version 1.0.1"; echo "${S//1/2}"'
run_diff_test 70  "pattern replacement on empty string"            'S=""; echo "[${S/a/b}]"'

# --- 8. Prefix and Suffix Trimming ${VAR#pat} & ${VAR%pat} (Tests 071 - 080) ---
run_diff_test 71  "remove matching prefix word"                    'S="hello world"; echo "[${S#hello }]"'
run_diff_test 72  "remove non-matching prefix unchanged"           'S="hello world"; echo "[${S#bye}]"'
run_diff_test 73  "remove matching suffix word"                    'S="hello world"; echo "[${S% world}]"'
run_diff_test 74  "remove non-matching suffix unchanged"           'S="hello world"; echo "[${S%bye}]"'
run_diff_test 75  "remove single leading character"                'S="Xhello"; echo "[${S#X}]"'
run_diff_test 76  "remove single trailing character"               'S="helloZ"; echo "[${S%Z}]"'
run_diff_test 77  "remove file extension suffix"                   'F="archive.tar.gz"; echo "[${F%.gz}]"'
run_diff_test 78  "remove path directory prefix"                   'P="prefix_name"; echo "[${P#prefix_}]"'
run_diff_test 79  "remove fixed prefix directory"                  'P="/usr/local/bin"; echo "[${P#/usr}]"'
run_diff_test 80  "prefix and suffix trimming combined"            'S="--flag--"; echo "[${S#--}]-[${S%--}]"'

# --- 9. Positional and Special Parameters (Tests 081 - 090) ---
run_diff_test 81  "positional parameters expansion"                'set -- apple banana cherry; echo "$1 $2 $3"'
run_diff_test 82  "positional parameter count $#"                  'set -- one two three four five; echo "$#"'
run_diff_test 83  "positional parameters all $*"                   'set -- a b c; echo "$*"'
run_diff_test 84  "positional parameters with internal spaces"     'set -- "first arg" "second arg"; echo "$1 | $2"'
run_diff_test 85  "positional parameter in braces ${1}"            'set -- alpha beta; echo "${1} and ${2}"'
run_diff_test 86  "positional parameter beyond count is empty"     'set -- solo; echo "1:[$1] 2:[$2]"'
run_diff_test 87  "exit code variable $? on success"               'true; echo "Status:$?"'
run_diff_test 88  "exit code variable $? on failure"               'false; echo "Status:$?"'
run_diff_test 89  "exit code variable $? from arithmetic"          '(( 10 > 20 )); echo "Status:$?"'
run_diff_test 90  "positional parameters after shift"              'set -- a b c; shift; echo "$1 cnt:$#"'

# --- 10. Quoting & Complex Expansions (Tests 091 - 100) ---
run_diff_test 91  "double quotes preserves whitespace"             'A="  spaced   words  "; echo "[$A]"'
run_diff_test 92  "single quotes prevents variable expansion"      'A="raw"; echo '\''$A'\'''
run_diff_test 93  "mixed quotes in single argument"                 'A="middle"; echo "prefix "'\''$A'\''" suffix"'
run_diff_test 94  "escaped dollar sign prints literal"             'echo "\$NOT_A_VAR"'
run_diff_test 95  "variable expansion inside double quotes"        'NAME="Zephyr"; echo "Welcome to $NAME shell"'
run_diff_test 96  "variable expansion adjacent to double quotes"   'V="core"; echo "prefix"${V}"suffix"'
run_diff_test 97  "variable expansion in path string"              'A="foo"; B="bar"; echo "$A/$B.log"'
run_diff_test 98  "variable containing equals sign"                'X="key=value"; echo "$X"'
run_diff_test 99  "expansion inside command option argument"       'VAL="debug"; echo "--mode=$VAL"'
run_diff_test 100 "mixed variable defaults and math"               'A=10; B=20; echo "Math: $(( A + B )) Var: ${C:-default}"'

print_summary "MODULE 03 - VARIABLES & EXPANSIONS"
