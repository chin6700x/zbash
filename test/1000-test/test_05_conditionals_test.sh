#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 05 - CONDITIONALS & TEST (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 05/10] CONDITIONALS & TEST [ ... ] (100 DIFFERENTIAL PARITY TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. Basic if ... then ... fi & Single-Line Syntax (Tests 001 - 010) ---
run_diff_test 1   "if true branch executed"                        'if true; then echo "yes"; fi'
run_diff_test 2   "if false else branch executed"                  'if false; then echo "yes"; else echo "no"; fi'
run_diff_test 3   "if true with else prints then branch"           'if true; then echo "branch-true"; else echo "branch-false"; fi'
run_diff_test 4   "if false without else continues script"         'if false; then echo "yes"; fi; echo "after"'
run_diff_test 5   "if test integers equal"                         'if [ 1 -eq 1 ]; then echo "equal"; fi'
run_diff_test 6   "if test integers not equal else branch"         'if [ 1 -eq 2 ]; then echo "equal"; else echo "not equal"; fi'
run_diff_test 7   "if string equality match"                       'X="match"; if [ "$X" = "match" ]; then echo "matched"; fi'
run_diff_test 8   "if string inequality mismatch else branch"      'X="diff"; if [ "$X" = "match" ]; then echo "matched"; else echo "mismatched"; fi'
run_diff_test 9   "if multiple statements inside then block"       'if true; then echo "A"; echo "B"; fi'
run_diff_test 10  "if multiple statements inside else block"       'if false; then echo "A"; else echo "B"; echo "C"; fi'

# --- 2. elif Chaining & Multi-Branch Decisions (Tests 011 - 020) ---
run_diff_test 11  "elif chain first branch matches"                'X=1; if [ $X -eq 1 ]; then echo "one"; elif [ $X -eq 2 ]; then echo "two"; else echo "other"; fi'
run_diff_test 12  "elif chain second branch matches"               'X=2; if [ $X -eq 1 ]; then echo "one"; elif [ $X -eq 2 ]; then echo "two"; else echo "other"; fi'
run_diff_test 13  "elif chain falls through to else"               'X=3; if [ $X -eq 1 ]; then echo "one"; elif [ $X -eq 2 ]; then echo "two"; else echo "other"; fi'
run_diff_test 14  "elif chain matches without else block"          'X=2; if [ $X -eq 1 ]; then echo "one"; elif [ $X -eq 2 ]; then echo "two"; fi'
run_diff_test 15  "multiple elif branches fallback to else"        'X=5; if [ $X -eq 1 ]; then echo "one"; elif [ $X -eq 2 ]; then echo "two"; elif [ $X -eq 3 ]; then echo "three"; else echo "fallback"; fi'
run_diff_test 16  "multiple elif string comparisons"               'MODE="prod"; if [ "$MODE" = "dev" ]; then echo "dev"; elif [ "$MODE" = "stage" ]; then echo "stage"; elif [ "$MODE" = "prod" ]; then echo "prod"; fi'
run_diff_test 17  "elif range check with -lt"                      'NUM=15; if [ $NUM -lt 10 ]; then echo "small"; elif [ $NUM -lt 20 ]; then echo "medium"; else echo "large"; fi'
run_diff_test 18  "elif chain third branch true"                   'if false; then echo "1"; elif false; then echo "2"; elif true; then echo "3"; fi'
run_diff_test 19  "elif chain first true branch wins"              'if false; then echo "1"; elif true; then echo "2"; elif true; then echo "3"; fi'
run_diff_test 20  "elif zero positive negative classification"     'VAL=0; if [ $VAL -eq 0 ]; then echo "zero"; elif [ $VAL -gt 0 ]; then echo "pos"; else echo "neg"; fi'

# --- 3. Nested If Statements (Tests 021 - 030) ---
run_diff_test 21  "nested if both conditions true"                 'if true; then if true; then echo "both true"; fi; fi'
run_diff_test 22  "nested if inner condition false else taken"     'if true; then if false; then echo "inner"; else echo "inner else"; fi; fi'
run_diff_test 23  "nested if outer else contains if"               'if false; then echo "outer"; else if true; then echo "outer else inner"; fi; fi'
run_diff_test 24  "nested if comparing two variables"              'A=1; B=2; if [ $A -eq 1 ]; then if [ $B -eq 2 ]; then echo "A1-B2"; fi; fi'
run_diff_test 25  "nested if second variable mismatch"             'A=1; B=3; if [ $A -eq 1 ]; then if [ $B -eq 2 ]; then echo "A1-B2"; else echo "A1-Bnot2"; fi; fi'
run_diff_test 26  "triple nested if statements"                    'if true; then if true; then if true; then echo "level 3"; fi; fi; fi'
run_diff_test 27  "nested if with variable assignment inside"      'X="yes"; if [ "$X" = "yes" ]; then Y="ok"; if [ "$Y" = "ok" ]; then echo "nested ok"; fi; fi'
run_diff_test 28  "nested if numeric comparisons"                  'if [ 10 -gt 5 ]; then if [ 20 -gt 15 ]; then echo "both gt"; fi; fi'
run_diff_test 29  "nested if surrounded by sibling statements"     'if true; then echo "pre"; if false; then echo "no"; else echo "yes"; fi; echo "post"; fi'
run_diff_test 30  "nested if string accumulator"                   'res=""; if true; then res="A"; if true; then res="${res}B"; fi; fi; echo "res:$res"'

# --- 4. Negated Conditions (!) in If & Tests (Tests 031 - 040) ---
run_diff_test 31  "if ! false evaluates to true"                   'if ! false; then echo "not false is true"; fi'
run_diff_test 32  "if ! true evaluates to false"                   'if ! true; then echo "yes"; else echo "not true is false"; fi'
run_diff_test 33  "if ! [ 1 -eq 2 ] evaluates to true"             'if ! [ 1 -eq 2 ]; then echo "1 not eq 2"; fi'
run_diff_test 34  "if ! string equality test"                      'if ! [ "foo" = "bar" ]; then echo "strings differ"; fi'
run_diff_test 35  "test bracket not empty string with !"           '[ ! -z "hello" ]; echo "exit:$?"'
run_diff_test 36  "test bracket not nonempty string with !"        '[ ! -n "" ]; echo "exit:$?"'
run_diff_test 37  "test bracket not 5 eq 5 exit status"            '[ ! 5 -eq 5 ]; echo "exit:$?"'
run_diff_test 38  "test bracket not 5 eq 6 exit status"            '[ ! 5 -eq 6 ]; echo "exit:$?"'
run_diff_test 39  "test command negated nonexistent dir"           'test ! -d "/nonexistent_dir_12345"; echo "exit:$?"'
run_diff_test 40  "if ! nonexistent file check"                    'if ! [ -f "/nonexistent_file_xyz" ]; then echo "file absent"; fi'

# --- 5. String Comparisons with [ and test (Tests 041 - 050) ---
run_diff_test 41  "test bracket -z with empty string"              '[ -z "" ]; echo "exit:$?"'
run_diff_test 42  "test bracket -z with non-empty string"          '[ -z "not empty" ]; echo "exit:$?"'
run_diff_test 43  "test bracket -n with non-empty string"          '[ -n "hello" ]; echo "exit:$?"'
run_diff_test 44  "test bracket -n with empty string"              '[ -n "" ]; echo "exit:$?"'
run_diff_test 45  "test bracket string equality true"              '[ "abc" = "abc" ]; echo "exit:$?"'
run_diff_test 46  "test bracket string equality false"             '[ "abc" = "xyz" ]; echo "exit:$?"'
run_diff_test 47  "test bracket string inequality true"            '[ "abc" != "xyz" ]; echo "exit:$?"'
run_diff_test 48  "test bracket double equals string match"        '[ "abc" == "abc" ]; echo "exit:$?"'
run_diff_test 49  "test bracket single non-empty word argument"    '[ "nonempty" ]; echo "exit:$?"'
run_diff_test 50  "test bracket single empty word argument"        '[ "" ]; echo "exit:$?"'

# --- 6. Integer Comparisons with [ and test (Tests 051 - 060) ---
run_diff_test 51  "test bracket -eq true"                          '[ 10 -eq 10 ]; echo "exit:$?"'
run_diff_test 52  "test bracket -eq false"                         '[ 10 -eq 20 ]; echo "exit:$?"'
run_diff_test 53  "test bracket -ne true"                          '[ 10 -ne 20 ]; echo "exit:$?"'
run_diff_test 54  "test bracket -ne false"                         '[ 10 -ne 10 ]; echo "exit:$?"'
run_diff_test 55  "test bracket -lt true"                          '[ 5 -lt 10 ]; echo "exit:$?"'
run_diff_test 56  "test bracket -lt false"                         '[ 10 -lt 5 ]; echo "exit:$?"'
run_diff_test 57  "test bracket -le boundary true"                 '[ 5 -le 5 ]; echo "exit:$?"'
run_diff_test 58  "test bracket -gt true"                          '[ 10 -gt 5 ]; echo "exit:$?"'
run_diff_test 59  "test bracket -ge boundary true"                 '[ 5 -ge 5 ]; echo "exit:$?"'
run_diff_test 60  "test bracket negative number comparison"        '[ -5 -lt 0 ]; echo "exit:$?"'

# --- 7. 64-Bit Integers & Command test Syntax (Tests 061 - 070) ---
run_diff_test 61  "test command 64-bit integer -gt"                'test 10000000000 -gt 5000000000; echo "exit:$?"'
run_diff_test 62  "test command -z empty string"                   'test -z ""; echo "exit:$?"'
run_diff_test 63  "test command -n non-empty string"               'test -n "hello"; echo "exit:$?"'
run_diff_test 64  "test command string equality"                   'test "alpha" = "alpha"; echo "exit:$?"'
run_diff_test 65  "test command string inequality"                 'test "alpha" != "beta"; echo "exit:$?"'
run_diff_test 66  "test command integer -eq"                       'test 42 -eq 42; echo "exit:$?"'
run_diff_test 67  "test command integer -ne"                       'test 42 -ne 99; echo "exit:$?"'
run_diff_test 68  "test command two negative integers -lt"         'test -10 -lt -5; echo "exit:$?"'
run_diff_test 69  "test command two negative integers -gt"         'test -5 -gt -10; echo "exit:$?"'
run_diff_test 70  "test command zero equals zero"                  'test 0 -eq 0; echo "exit:$?"'

# --- 8. File System Tests (-e, -f, -d, -s, -r, -w, -x, -c) (Tests 071 - 080) ---
run_diff_test 71  "test bracket -e file exists"                    '[ -e "/bin/sh" ]; echo "exit:$?"'
run_diff_test 72  "test bracket -f regular file"                   '[ -f "/bin/sh" ]; echo "exit:$?"'
run_diff_test 73  "test bracket -d directory"                      '[ -d "/tmp" ]; echo "exit:$?"'
run_diff_test 74  "test bracket -s non-zero file size"             '[ -s "/bin/sh" ]; echo "exit:$?"'
run_diff_test 75  "test bracket -r readable file"                  '[ -r "/bin/sh" ]; echo "exit:$?"'
run_diff_test 76  "test bracket -x executable file"                '[ -x "/bin/sh" ]; echo "exit:$?"'
run_diff_test 77  "test bracket -e device node exists"             '[ -e "/dev/null" ]; echo "exit:$?"'
run_diff_test 78  "test bracket -c character device"               '[ -c "/dev/null" ]; echo "exit:$?"'
run_diff_test 79  "test bracket -e nonexistent path false"         '[ -e "/nonexistent_path_xyz_999" ]; echo "exit:$?"'
run_diff_test 80  "test bracket -f directory is false"             '[ -f "/tmp" ]; echo "exit:$?"'

# --- 9. Logical Operators -a (AND) and -o (OR) (Tests 081 - 090) ---
run_diff_test 81  "test bracket -a true and true"                  '[ 1 -eq 1 -a 2 -eq 2 ]; echo "exit:$?"'
run_diff_test 82  "test bracket -a true and false"                 '[ 1 -eq 1 -a 2 -eq 3 ]; echo "exit:$?"'
run_diff_test 83  "test bracket -a false and true"                 '[ 1 -eq 2 -a 2 -eq 2 ]; echo "exit:$?"'
run_diff_test 84  "test bracket -o true or false"                  '[ 1 -eq 1 -o 2 -eq 3 ]; echo "exit:$?"'
run_diff_test 85  "test bracket -o false or true"                  '[ 1 -eq 2 -o 2 -eq 2 ]; echo "exit:$?"'
run_diff_test 86  "test bracket -o false or false"                 '[ 1 -eq 2 -o 2 -eq 3 ]; echo "exit:$?"'
run_diff_test 87  "test bracket -a with string tests"              '[ -n "foo" -a -z "" ]; echo "exit:$?"'
run_diff_test 88  "test bracket -o with string tests"              '[ -z "foo" -o -n "bar" ]; echo "exit:$?"'
run_diff_test 89  "test bracket chained -a expressions"            '[ 1 -eq 1 -a 2 -eq 2 -a 3 -eq 3 ]; echo "exit:$?"'
run_diff_test 90  "test bracket chained -o expressions"            '[ 1 -eq 2 -o 2 -eq 3 -o 3 -eq 3 ]; echo "exit:$?"'

# --- 10. Precedence, Grouping \( ... \) & Complex Ifs (Tests 091 - 100) ---
run_diff_test 91  "operator precedence -a over -o"                 '[ 1 -eq 1 -o 2 -eq 2 -a 3 -eq 4 ]; echo "exit:$?"'
run_diff_test 92  "operator precedence -a evaluated before -o"     '[ 1 -eq 2 -a 2 -eq 2 -o 3 -eq 3 ]; echo "exit:$?"'
run_diff_test 93  "parentheses grouping simple true"               '[ \( 1 -eq 1 \) ]; echo "exit:$?"'
run_diff_test 94  "parentheses grouping simple false"              '[ \( 1 -eq 2 \) ]; echo "exit:$?"'
run_diff_test 95  "parentheses grouping with -a"                   '[ \( 1 -eq 1 \) -a \( 2 -eq 2 \) ]; echo "exit:$?"'
run_diff_test 96  "parentheses grouping overriding precedence"     '[ \( 1 -eq 1 -o 1 -eq 2 \) -a 2 -eq 2 ]; echo "exit:$?"'
run_diff_test 97  "negated parenthesized expression"               '[ ! \( 1 -eq 2 \) ]; echo "exit:$?"'
run_diff_test 98  "if condition with compound test brackets"       'if [ 5 -gt 3 -a "a" = "a" ]; then echo "compound ok"; fi'
run_diff_test 99  "if condition with filesystem and string tests"  'if [ -d "/tmp" -a -f "/bin/sh" ]; then echo "files ok"; fi'
run_diff_test 100 "complex multi-branch if with parenthesized test" 'if [ 10 -gt 20 ]; then echo "A"; elif [ 5 -lt 3 ]; then echo "B"; elif [ \( 1 -eq 1 \) -a \( 2 -eq 2 \) ]; then echo "C"; else echo "D"; fi'

print_summary "MODULE 05 - CONDITIONALS & TEST [ ... ]"
