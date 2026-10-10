#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 10 - EDGE CASES, UNICODE & STRESS (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 10/10] EDGE CASES, UNICODE, STRESS & RECURSION (100 TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. Unicode, UTF-8 & Thai Language (Tests 001 - 010) ---
run_diff_test 1   "thai simple greeting"                               'echo "สวัสดีชาวโลก"'
run_diff_test 2   "thai variable expansion"                            'MSG="ภาษาไทยและ Apple Silicon"; echo "$MSG"'
run_diff_test 3   "thai array elements indexing"                       'arr=( "หนึ่ง" "สอง" "สาม" ); echo "${arr[0]} ${arr[1]} ${arr[2]}"'
run_diff_test 4   "thai loop iteration"                                'for t in "กล้วย" "ส้ม" "มะละกอ"; do echo "ผลไม้:$t"; done'
run_diff_test 5   "thai function argument"                             'f() { echo "สวัสดีคุณ: $1"; }; f "วิโรจน์"'
run_diff_test 6   "emoji echo string"                                  'echo "Emojis: ⚡ 🚀 💎 🍏"'
run_diff_test 7   "emoji array elements"                               'arr=( "⚡" "🚀" "💎" ); echo "${arr[@]} count:${#arr[@]}"'
run_diff_test 8   "mixed unicode language text"                        'echo "Mixed: English ไทย 123 !@#"'
run_diff_test 9   "thai in subshell"                                   '( echo "ใน Subshell: ผ่านฉลุย" )'
run_diff_test 10  "thai in brace grouping"                              '{ echo "ใน Brace: ผ่านฉลุย"; }'

# --- 2. Advanced Parameter Substrings & Slicing (Tests 011 - 020) ---
run_diff_test 11  "substring slice offset and length"                  'S="abcdef"; echo "${S:0:3}"'
run_diff_test 12  "substring middle slice"                             'S="abcdef"; echo "${S:2:2}"'
run_diff_test 13  "substring slice from offset to end"                 'S="hello world"; echo "${S:6}"'
run_diff_test 14  "numeric string slice"                               'S="0123456789"; echo "${S:5:4}"'
run_diff_test 15  "variable length parameter ${#VAR}"                  'S="apple"; echo "${#S}"'
run_diff_test 16  "empty variable length"                              'S=""; echo "len:${#S}"'
run_diff_test 17  "string with spaces length"                          'S="spaced string"; echo "${#S}"'
run_diff_test 18  "ascii slice of mixed string"                        'S="test_1234"; echo "${S:0:4}"'
run_diff_test 19  "substring slice with variable offset and len"       'S="abcdef"; offset=2; len=3; echo "${S:$offset:$len}"'
run_diff_test 20  "substring slice beyond string length"               'S="short"; echo "res:[${S:10:2}]"'

# --- 3. Pattern Removal & Replacement Expansion (Tests 021 - 030) ---
run_diff_test 21  "suffix removal literal extension"                   'P="document.pdf"; echo "${P%.pdf}"'
run_diff_test 22  "suffix removal second case"                         'P="archive.tar.gz"; echo "${P%.gz}"'
run_diff_test 23  "prefix removal literal prefix"                      'P="prefix_sample"; echo "${P#prefix_}"'
run_diff_test 24  "prefix removal second case"                         'P="v1.2.3"; echo "${P#v}"'
run_diff_test 25  "pattern replace first occurrence"                   'S="apple banana apple"; echo "${S/apple/orange}"'
run_diff_test 26  "pattern replace all occurrences"                    'S="apple banana apple"; echo "${S//apple/orange}"'
run_diff_test 27  "pattern replace hyphen with underscore"             'S="foo-bar-baz"; echo "${S//-/_}"'
run_diff_test 28  "pattern replace single char"                        'S="10:20:30"; echo "${S//:/,}"'
run_diff_test 29  "pattern replace non-matching leaves intact"         'S="hello world"; echo "${S/nomatch/rep}"'
run_diff_test 30  "pattern replace delete match"                       'S="clean_me_up"; echo "${S//_me/}"'

# --- 4. Parameter Defaults & Conditional Expansions (Tests 031 - 040) ---
run_diff_test 31  "parameter default when unset"                       'unset V; echo "${V:-default_val}"'
run_diff_test 32  "parameter default when set"                         'V="existing"; echo "${V:-default_val}"'
run_diff_test 33  "parameter default when empty with colon"            'V=""; echo "${V:-fallback}"'
run_diff_test 34  "parameter default without colon on empty"           'V=""; echo "${V-fallback}"'
run_diff_test 35  "parameter assign default when unset"                'unset V; echo "${V:=assigned}"; echo "v:$V"'
run_diff_test 36  "parameter assign default when set"                  'V="set"; echo "${V:=not_assigned}"; echo "v:$V"'
run_diff_test 37  "parameter alternate when set"                       'V="defined"; echo "${V:+alternate}"'
run_diff_test 38  "parameter alternate when unset"                     'unset V; echo "alt:[${V:+alternate}]"'
run_diff_test 39  "parameter alternate when empty with colon"          'V=""; echo "alt:[${V:+alt}]"'
run_diff_test 40  "parameter alternate without colon on empty"         'V=""; echo "alt:[${V+alt}]"'

# --- 5. Deeply Nested Control Flow & Complex Arithmetic (Tests 041 - 050) ---
run_diff_test 41  "triple nested for loops"                            'for i in 1 2; do for j in 10 20; do for k in 100 200; do echo "$i-$j-$k"; done; done; done'
run_diff_test 42  "complex compound arithmetic expression"             '(( r = ((2 + 3) * 4 - 2) / 3 )); echo "r:$r"'
run_diff_test 43  "comma operator in arithmetic evaluation"            '(( a = 1, b = 2, c = 3, d = a + b * c )); echo "d:$d"'
run_diff_test 44  "bitwise operations in arithmetic"                   '(( bit = (1 << 4) | (1 << 2) & ~1 )); echo "bit:$bit"'
run_diff_test 45  "power exponentiation in arithmetic"                 '(( pow = 2 ** 8 )); echo "pow:$pow"'
run_diff_test 46  "modulo operator in arithmetic"                      '(( x = 10 % 3 )); echo "mod:$x"'
run_diff_test 47  "ternary true condition in arithmetic"               '(( t = (5 > 2) ? 100 : 200 )); echo "t:$t"'
run_diff_test 48  "ternary false condition in arithmetic"              '(( t = (1 == 0) ? 100 : 200 )); echo "t:$t"'
run_diff_test 49  "nested C-style for loops"                           'for (( i=1; i<=3; i++ )); do for (( j=1; j<=2; j++ )); do echo "$i,$j"; done; done'
run_diff_test 50  "while loop with continue and condition"             'c=0; while [ $c -lt 3 ]; do (( c++ )); [ $c -eq 2 ] && continue; echo "c:$c"; done'

# --- 6. Deep Pipelines & Multi-Stage Processing (Tests 051 - 060) ---
run_diff_test 51  "four-stage pipeline sort, tr, head"                 'printf "c\na\nb\n" | sort | tr "a-z" "A-Z" | head -n 2'
run_diff_test 52  "pipeline with tr, grep filter, tr"                  'echo "hello world test" | tr " " "\n" | grep -v "world" | tr "\n" " "'
run_diff_test 53  "pipeline into while read with arithmetic"           'printf "10\n20\n30\n" | ( while read n; do (( d = n * 2 )); echo "d:$d"; done )'
run_diff_test 54  "cut with tr pipeline"                               'echo "a:b:c:d" | cut -d: -f2,3 | tr ":" ","'
run_diff_test 55  "sort pipeline with tr output"                       'printf "3\n1\n2\n" | sort | tr "\n" " "; echo ""'
run_diff_test 56  "multiple cat passthroughs in pipe"                  'echo "line1" | cat | cat | cat | cat'
run_diff_test 57  "nested subshells in pipeline"                       '( ( ( echo "deep pipe" ) | cat ) | tr "a-z" "A-Z" )'
run_diff_test 58  "pipeline with three subshells"                      'echo "multi" | ( tr "a-z" "A-Z" ) | ( cat ) | ( tr "M" "W" )'
run_diff_test 59  "printf line count pipeline"                         'printf "one\ntwo\n" | wc -l | tr -d " "'
run_diff_test 60  "printf grep tr pipeline"                            'printf "apple\nbanana\n" | grep banana | tr "a-z" "A-Z"'

# --- 7. Extensive Quoting & Escaping Stress (Tests 061 - 070) ---
run_diff_test 61  "mixed quotes in single argument"                     'printf "%s\n" "line with single and \"double\" quotes"'
run_diff_test 62  "escaped backslash and quote"                        'echo "back\\slash and \"quote\""'
run_diff_test 63  "escaped dollar does not expand"                     'echo "dollar \$VAR not expanded"'
run_diff_test 64  "nested command substitution quoting"                'VAR="outer"; echo "nested: $(echo inner: $VAR)"'
run_diff_test 65  "tab escape in printf"                               'printf "%s\t%s\n" "col1" "col2"'
run_diff_test 66  "concatenated triple quote segments"                 'echo "quote1"'\''quote2'\''"quote3"'
run_diff_test 67  "function with three quoted arguments"               'f() { printf "[%s] [%s] [%s]\n" "$1" "$2" "$3"; }; f "a b" "c d" "e f"'
run_diff_test 68  "function argument count with empty strings"         'f() { echo "args:$#"; }; f "" "" ""'
run_diff_test 69  "quoted variable with trailing spaces"               'V="text   "; printf "[%s]\n" "$V"'
run_diff_test 70  "multiple empty strings separated by spaces"         'echo "" "" "" | tr -d "\n"; echo "end"'

# --- 8. Recursive Functions & Deep Call Stacks (Tests 071 - 080) ---
run_diff_test 71  "recursive factorial function"                       'fact() { if [ $1 -le 1 ]; then echo 1; else local prev=$(fact $(( $1 - 1 ))); echo $(( $1 * prev )); fi; }; fact 5'
run_diff_test 72  "recursive fibonacci function"                       'fib() { if [ $1 -le 1 ]; then echo $1; else local a=$(fib $(( $1 - 1 ))); local b=$(fib $(( $1 - 2 ))); echo $(( a + b )); fi; }; fib 6'
run_diff_test 73  "recursive countdown function"                       'cdwn() { printf "%s " "$1"; if [ $1 -gt 1 ]; then cdwn $(( $1 - 1 )); fi; }; cdwn 5; echo ""'
run_diff_test 74  "recursive depth nesting function"                   'nest() { local d=$1; if [ $d -lt 4 ]; then nest $(( d + 1 )); fi; printf "%s " "$d"; }; nest 1; echo ""'
run_diff_test 75  "chain of three functions calling each other"        'f1() { f2; }; f2() { f3; }; f3() { echo "deep call ok"; }; f1'
run_diff_test 76  "recursive function printing unwound stack"          'f() { local x=$1; if [ $x -gt 0 ]; then f $(( x - 1 )); fi; printf "%s " "$x"; }; f 3; echo ""'
run_diff_test 77  "recursive summation function"                       'sum_rec() { if [ $1 -le 0 ]; then echo 0; else local p=$(sum_rec $(( $1 - 1 ))); echo $(( $1 + p )); fi; }; sum_rec 5'
run_diff_test 78  "function returning custom status codes"             'f() { return $1; }; f 0; echo "rc0:$?"; f 42; echo "rc42:$?"'
run_diff_test 79  "conditional return branches"                        'f() { [ $1 -eq 0 ] && return 10 || return 20; }; f 0; echo "r:$?"; f 1; echo "r:$?"'
run_diff_test 80  "function defining inner function"                   'outer() { inner() { echo "inner_ok"; }; inner; }; outer; inner'

# --- 9. Exit Code & Logic Status Parity (Tests 081 - 090) ---
run_diff_test 81  "subshell exit code 0"                               '( exit 0 ); echo "rc:$?"'
run_diff_test 82  "subshell exit code 1"                               '( exit 1 ); echo "rc:$?"'
run_diff_test 83  "subshell exit code 2"                               '( exit 2 ); echo "rc:$?"'
run_diff_test 84  "subshell exit code 126"                             '( exit 126 ); echo "rc:$?"'
run_diff_test 85  "subshell exit code 127"                             '( exit 127 ); echo "rc:$?"'
run_diff_test 86  "subshell exit code 255"                             '( exit 255 ); echo "rc:$?"'
run_diff_test 87  "boolean operator chaining status"                   'true && false || true; echo "status:$?"'
run_diff_test 88  "boolean recovery chaining status"                   'false || false || true; echo "status:$?"'
run_diff_test 89  "boolean failure chaining status"                    'true && true && false; echo "status:$?"'
run_diff_test 90  "logical negation operator on false"                 '! false; echo "not_false:$?"'

# --- 10. Real-World Shell Script Patterns & Milestone (Tests 091 - 100) ---
run_diff_test 91  "array loop with item processing"                    'FILES=( "a.txt" "b.txt" "c.txt" ); for f in "${FILES[@]}"; do echo "processing: $f"; done'
run_diff_test 92  "directory test commands"                            'DIR="/tmp/zb_test_dir"; mkdir -p "$DIR"; test -d "$DIR" && echo "dir_exists"; rmdir "$DIR"'
run_diff_test 93  "arithmetic calculator function"                     'calc() { (( res = $1 * $2 + $3 )); echo "calc:$res"; }; calc 5 4 3'
run_diff_test 94  "logging function formatting"                        'log() { echo "[$1] $2"; }; log "INFO" "Starting build"; log "SUCCESS" "Build completed"'
run_diff_test 95  "array accumulation in loop"                         'items=( 10 20 30 ); total=0; for x in "${items[@]}"; do (( total += x )); done; echo "total:$total"'
run_diff_test 96  "string non-empty check function"                    'check() { [ -n "$1" ] && echo "non-empty: $1" || echo "empty"; }; check "hello"; check ""'
run_diff_test 97  "pipeline tr and cut integration"                    'echo "USER:1000:ADMIN" | cut -d: -f1,3 | tr ":" "="; echo "rc:$?"'
run_diff_test 98  "nested variable state isolation"                    'A="alpha"; B="beta"; ( A="sub_a"; echo "$A-$B" ); echo "$A-$B"'
run_diff_test 99  "sequence of brace and subshell"                     'echo "start"; { echo "in_brace"; }; ( echo "in_subshell" ); echo "done"'
run_diff_test 100 "grand milestone 1000 parity statement"             'echo "🎉 1000-TEST MASTER PARITY COMPLETED SUCCESSFULLY!"'

print_summary "MODULE 10 - EDGE CASES, UNICODE & STRESS"
