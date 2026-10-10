#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 09 - ARRAYS & QUOTING (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 09/10] ARRAYS & QUOTING MECHANICS (100 TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. Basic Array Initialization & Element Indexing (Tests 001 - 010) ---
run_diff_test 1   "array first element [0]"                            'arr=( one two three ); echo "${arr[0]}"'
run_diff_test 2   "array second element [1]"                           'arr=( one two three ); echo "${arr[1]}"'
run_diff_test 3   "array third element [2]"                            'arr=( one two three ); echo "${arr[2]}"'
run_diff_test 4   "single element array"                               'arr=( single ); echo "${arr[0]}"'
run_diff_test 5   "numeric elements in array"                          'arr=( 10 20 30 ); echo "${arr[1]}"'
run_diff_test 6   "mixed types in array"                               'arr=( 1 "two" 3 "four" ); echo "${arr[3]}"'
run_diff_test 7   "empty array length"                                 'arr=(); echo "len:${#arr[@]}"'
run_diff_test 8   "out of bounds index returns empty"                  'arr=( a b c ); echo "out:[${arr[99]}]"'
run_diff_test 9   "scalar $arr access returns first element"           'arr=( first second ); echo "default:$arr"'
run_diff_test 10  "scalar ${arr} access returns first element"         'arr=( first second ); echo "default_brace:${arr}"'

# --- 2. Array Expansions: @ and * (Tests 011 - 020) ---
run_diff_test 11  "array expand all with @"                            'arr=( apple banana cherry ); echo "${arr[@]}"'
run_diff_test 12  "array expand all with *"                            'arr=( apple banana cherry ); echo "${arr[*]}"'
run_diff_test 13  "array elements with spaces inside quotes"           'arr=( "hello world" "foo bar" ); echo "${arr[@]}"'
run_diff_test 14  "array expand with leading and trailing text"         'arr=( a b c ); echo "items:${arr[@]}:end"'
run_diff_test 15  "array expand with star in text"                      'arr=( x y z ); echo "pre:${arr[*]}:post"'
run_diff_test 16  "array elements piped to filter"                      'arr=( 1 2 3 ); echo "${arr[@]}" | tr " " "-"'
run_diff_test 17  "printf multiple array elements"                     'arr=( alpha beta ); printf "[%s]\n" "${arr[@]}"'
run_diff_test 18  "printf array elements with spaces"                  'arr=( "spaced one" "spaced two" ); printf "<%s>\n" "${arr[@]}"'
run_diff_test 19  "word count of array elements"                       'arr=( 1 2 3 ); echo "${arr[*]}" | wc -w | tr -d " "'
run_diff_test 20  "array with empty string elements"                   'arr=( "" "val" "" ); echo "c:${#arr[@]}"'

# --- 3. Array Length & Metadata Operations (Tests 021 - 030) ---
run_diff_test 21  "array length using ${#arr[@]}"                      'arr=( 10 20 30 40 50 ); echo "len:${#arr[@]}"'
run_diff_test 22  "array length using ${#arr[*]}"                      'arr=( a b c d ); echo "len_star:${#arr[*]}"'
run_diff_test 23  "empty array length zero"                            'arr=(); echo "empty_len:${#arr[@]}"'
run_diff_test 24  "element string lengths"                             'arr=( "hello" "world" ); echo "l0:${#arr[0]} l1:${#arr[1]}"'
run_diff_test 25  "long element string length"                         'arr=( "supercalifragilistic" ); echo "elem_len:${#arr[0]}"'
run_diff_test 26  "array indices expansion ${!arr[@]}"                 'arr=( a b c ); echo "indices:${!arr[@]}"'
run_diff_test 27  "array indices with 4 elements"                      'arr=( 10 20 30 40 ); echo "idx:${!arr[@]}"'
run_diff_test 28  "single element array index"                         'arr=( single ); echo "single_idx:${!arr[@]}"'
run_diff_test 29  "empty string element length is zero"                'arr=( "" ); echo "empty_str_len:${#arr[0]}"'
run_diff_test 30  "multiple element lengths"                           'arr=( "a" "bc" "def" ); echo "${#arr[0]} ${#arr[1]} ${#arr[2]}"'

# --- 4. Dynamic & Computed Array Indices (Tests 031 - 040) ---
run_diff_test 31  "variable index with dollar ${arr[$idx]}"            'arr=( zero one two three ); idx=2; echo "${arr[$idx]}"'
run_diff_test 32  "variable index without dollar ${arr[idx]}"          'arr=( zero one two three ); idx=1; echo "${arr[idx]}"'
run_diff_test 33  "computed index variable"                            'arr=( a b c d ); i=3; echo "${arr[$i]}"'
run_diff_test 34  "arithmetic addition in index ${arr[1+1]}"           'arr=( 10 20 30 40 ); echo "${arr[1+1]}"'
run_diff_test 35  "arithmetic subtraction in index ${arr[2-1]}"        'arr=( 10 20 30 40 ); echo "${arr[2-1]}"'
run_diff_test 36  "arithmetic multiplication in index ${arr[0*2]}"     'arr=( 10 20 30 40 ); echo "${arr[0*2]}"'
run_diff_test 37  "variable plus constant in index"                    'arr=( zero one two three ); i=1; echo "${arr[i+2]}"'
run_diff_test 38  "variable minus constant in index"                   'arr=( zero one two three ); i=2; echo "${arr[i-1]}"'
run_diff_test 39  "indexed loop accessing array elements"              'arr=( a b c ); for i in 0 1 2; do echo "idx:$i val:${arr[$i]}"; done'
run_diff_test 40  "loop over ${!arr[@]} accessing elements"            'arr=( x y z ); for i in ${!arr[@]}; do echo "i:$i v:${arr[i]}"; done'

# --- 5. Array Element Mutability & Manipulation (Tests 041 - 050) ---
run_diff_test 41  "modify middle array element"                        'arr=( a b c ); arr[1]="modified"; echo "${arr[@]}"'
run_diff_test 42  "modify first array element"                         'arr=( a b c ); arr[0]="first_mod"; echo "${arr[@]}"'
run_diff_test 43  "modify last array element"                          'arr=( a b c ); arr[2]="last_mod"; echo "${arr[@]}"'
run_diff_test 44  "append element via next index"                      'arr=( a b ); arr[2]="appended"; echo "${arr[@]} count:${#arr[@]}"'
run_diff_test 45  "modify element with number"                         'arr=( 1 2 3 ); arr[1]=20; echo "${arr[1]}"'
run_diff_test 46  "modify element with space in string"                'arr=( "old name" ); arr[0]="new name"; echo "${arr[0]}"'
run_diff_test 47  "modify element with arithmetic index"               'arr=( a b ); arr[1+0]="updated"; echo "${arr[1]}"'
run_diff_test 48  "modify element with variable index"                 'arr=( a b c ); i=1; arr[i]="by_var"; echo "${arr[@]}"'
run_diff_test 49  "set element to empty string"                        'arr=( x y ); arr[0]=""; echo "empty0:[${arr[0]}] len:${#arr[@]}"'
run_diff_test 50  "sequential element modifications"                   'arr=( 10 20 ); arr[0]="100"; arr[1]="200"; echo "${arr[@]}"'

# --- 6. Array Iteration & Control Flow (Tests 051 - 060) ---
run_diff_test 51  "for in array expansion"                             'arr=( apple orange banana ); for f in "${arr[@]}"; do echo "fruit:$f"; done'
run_diff_test 52  "array elements arithmetic accumulation"             'arr=( 1 2 3 4 ); sum=0; for n in "${arr[@]}"; do (( sum += n )); done; echo "sum:$sum"'
run_diff_test 53  "array modified in subshell is isolated"             'arr=( a b c ); ( arr[0]="sub_mod"; echo "sub:${arr[0]}" ); echo "parent:${arr[0]}"'
run_diff_test 54  "array modified in brace persists"                   'arr=( a b c ); { arr[0]="brace_mod"; }; echo "parent:${arr[0]}"'
run_diff_test 55  "array defined inside function"                      'f() { arr=( in_fn_1 in_fn_2 ); echo "${arr[@]}"; }; f'
run_diff_test 56  "while loop over array by length"                    'arr=( 10 20 30 ); i=0; while [ $i -lt ${#arr[@]} ]; do echo "${arr[$i]}"; (( i++ )); done'
run_diff_test 57  "for loop over array with break"                     'arr=( a b c ); for x in "${arr[@]}"; do [ "$x" = "b" ] && break; echo "$x"; done'
run_diff_test 58  "for loop over array with continue"                  'arr=( a b c ); for x in "${arr[@]}"; do [ "$x" = "b" ] && continue; echo "$x"; done'
run_diff_test 59  "command substitution of array expansion"            'arr=( foo bar ); res=$( echo "${arr[@]}" ); echo "res:$res"'
run_diff_test 60  "array expansion in pipeline"                        'arr=( line1 line2 ); printf "%s\n" "${arr[@]}" | tr "a-z" "A-Z"'

# --- 7. Basic Quoting: Single Quotes '...' (Tests 061 - 070) ---
run_diff_test 61  "single quotes prevent variable expansion"           'echo '\''literal $VAR string'\'''
run_diff_test 62  "single quotes preserve glob characters"             'echo '\''special characters: * ? [ ] { }'\'''
run_diff_test 63  "single quotes preserve pipe and redirection symbols" 'echo '\''pipes and redirects: | > < >> 2>&1'\'''
run_diff_test 64  "single quotes preserve consecutive spaces"          'echo '\''spaces   and     tabs'\'''
run_diff_test 65  "single quotes preserve logic operators"             'echo '\''logical operators: && || ;'\'''
run_diff_test 66  "empty single quotes string"                         'echo '\'''\'''
run_diff_test 67  "concatenated single quoted strings"                 'echo '\''foo'\'''\''bar'\'''
run_diff_test 68  "three concatenated single quoted strings"           'echo '\''a'\'''\''b'\'''\''c'\'''
run_diff_test 69  "single quoted word with no spaces"                  'echo '\''plaintext'\'''
run_diff_test 70  "single quoted parens"                               'echo '\''parentheses: ( ) (( ))'\'''

# --- 8. Double Quotes "..." & Variable Expansion (Tests 071 - 080) ---
run_diff_test 71  "double quotes expand simple variable"               'NAME="Alice"; echo "Hello, $NAME!"'
run_diff_test 72  "double quotes expand braced variable"               'NAME="Bob"; echo "Hello, ${NAME}!"'
run_diff_test 73  "double quotes preserve leading and trailing spaces" 'MSG="  spaces   "; echo "[$MSG]"'
run_diff_test 74  "escaped double quote inside double quotes"          'echo "escaped quote: \"inside\""'
run_diff_test 75  "escaped dollar prevents expansion"                  'echo "escaped dollar: \$NOT_EXPANDED"'
run_diff_test 76  "escaped backslash in double quotes"                 'echo "escaped backslash: \\"'
run_diff_test 77  "arithmetic expansion inside double quotes"          'X=5; Y=3; echo "sum:$(( X + Y ))"'
run_diff_test 78  "command substitution inside double quotes"          'echo "cmd:$(echo nested)"'
run_diff_test 79  "status parameter inside double quotes"               'echo "status:$?"'
run_diff_test 80  "empty variable inside quotes"                       'EMPTY=""; echo "empty:\"$EMPTY\""'

# --- 9. Mixing & Nesting Quotes (Tests 081 - 090) ---
run_diff_test 81  "single quotes inside double quotes"                 'echo "single inside double: '\''hello'\''"'
run_diff_test 82  "double quotes inside single quotes"                 'echo '\''double inside single: "world"'\'''
run_diff_test 83  "mixed quotes in single string"                      'echo "mixed: '\''single'\'' and \"double\""'
run_diff_test 84  "double quotes and literal dollar in single quotes"  'echo '\''mixed: "double" and literal $X'\'''
run_diff_test 85  "alternating quote blocks"                           'echo "part1" '\''part2'\'' "part3"'
run_diff_test 86  "adjacent double and single quote"                   'echo "a"'\'''\''"b"'
run_diff_test 87  "adjacent single and double quote"                   'echo '\''a'\''""'\''b'\'''
run_diff_test 88  "double quote followed by single quote"              'echo "hello "'\''world'\'''
run_diff_test 89  "single quote followed by double quote"              'echo '\''hello '\''"world"'
run_diff_test 90  "empty double quotes"                                'echo ""'

# --- 10. Quoting with Commands, Arguments & Edge Cases (Tests 091 - 100) ---
run_diff_test 91  "multiple arguments with spaces"                     'echo "first arg" "second arg"'
run_diff_test 92  "empty argument between non-empty arguments"         'echo "" "non-empty" ""'
run_diff_test 93  "printf quoted arguments"                            'printf "[%s]\n" "a b" "c d"'
run_diff_test 94  "variable with spaces passed to printf"              'VAR="with spaces"; printf "<%s>\n" "$VAR"'
run_diff_test 95  "array elements with spaces separated by pipe"       'arr=( "first space" "second space" ); echo "${arr[0]} | ${arr[1]}"'
run_diff_test 96  "function with quoted arguments containing spaces"   'f() { echo "arg1:[$1] arg2:[$2]"; }; f "hello world" "foo bar"'
run_diff_test 97  "function argument count with empty quoted strings"  'f() { echo "count:$#"; }; f "a" "" "b"'
run_diff_test 98  "semicolon inside quoted string"                     'echo "semi;colon inside quotes"'
run_diff_test 99  "pipe inside quoted string"                          'echo "pipe|line inside quotes"'
run_diff_test 100 "ampersand inside quoted string"                     'echo "ampersand&and inside quotes"'

print_summary "MODULE 09 - ARRAYS & QUOTING"
