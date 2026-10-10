#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 02 - ARITHMETIC & COMPOUND (( ... )) (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 02/10] ARITHMETIC & COMPOUND EXPRESSIONS (( ... )) (100 TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. Basic Arithmetic Operators (Tests 001 - 010) ---
run_diff_test 1   "simple addition"                                '(( X = 1 + 2 )); echo "X:$X"'
run_diff_test 2   "simple subtraction"                             '(( X = 100 - 37 )); echo "X:$X"'
run_diff_test 3   "multiplication"                                 '(( X = 12 * 12 )); echo "X:$X"'
run_diff_test 4   "integer division"                               '(( X = 100 / 4 )); echo "X:$X"'
run_diff_test 5   "modulo operator"                                '(( X = 105 % 10 )); echo "X:$X"'
run_diff_test 6   "negative number addition"                       '(( X = -5 + 20 )); echo "X:$X"'
run_diff_test 7   "unary plus operator"                            '(( X = +42 )); echo "X:$X"'
run_diff_test 8   "unary double negation"                          '(( X = -(-100) )); echo "X:$X"'
run_diff_test 9   "zero addition"                                  '(( X = 0 + 0 )); echo "X:$X"'
run_diff_test 10  "multiplication by zero"                         '(( X = 1000 * 0 )); echo "X:$X"'

# --- 2. Power Operator & Precedence (Tests 011 - 020) ---
run_diff_test 11  "power operator 2 ** 3"                          '(( X = 2 ** 3 )); echo "X:$X"'
run_diff_test 12  "power operator 3 ** 4"                          '(( X = 3 ** 4 )); echo "X:$X"'
run_diff_test 13  "power operator 2 ** 10"                         '(( X = 2 ** 10 )); echo "X:$X"'
run_diff_test 14  "precedence addition and multiplication"         '(( X = 2 + 3 * 4 )); echo "X:$X"'
run_diff_test 15  "parentheses overriding precedence"             '(( X = (2 + 3) * 4 )); echo "X:$X"'
run_diff_test 16  "mixed precedence operations"                    '(( X = 10 - 2 * 3 + 4 )); echo "X:$X"'
run_diff_test 17  "grouped mixed precedence operations"            '(( X = (10 - 2) * (3 + 4) )); echo "X:$X"'
run_diff_test 18  "power precedence over multiplication"           '(( X = 2 * 3 ** 2 )); echo "X:$X"'
run_diff_test 19  "parenthesized power precedence"                 '(( X = (2 * 3) ** 2 )); echo "X:$X"'
run_diff_test 20  "left-to-right division chaining"                '(( X = 100 / 10 / 2 )); echo "X:$X"'

# --- 3. Bitwise Operators (Tests 021 - 030) ---
run_diff_test 21  "bitwise AND"                                    '(( X = 5 & 3 )); echo "X:$X"'
run_diff_test 22  "bitwise OR"                                     '(( X = 5 | 3 )); echo "X:$X"'
run_diff_test 23  "bitwise XOR"                                    '(( X = 5 ^ 3 )); echo "X:$X"'
run_diff_test 24  "bitwise NOT of zero"                            '(( X = ~0 )); echo "X:$X"'
run_diff_test 25  "bitwise NOT of positive integer"                '(( X = ~5 )); echo "X:$X"'
run_diff_test 26  "bitwise left shift"                             '(( X = 1 << 4 )); echo "X:$X"'
run_diff_test 27  "bitwise right shift"                            '(( X = 64 >> 2 )); echo "X:$X"'
run_diff_test 28  "shift with subtraction"                         '(( X = (1 << 8) - 1 )); echo "X:$X"'
run_diff_test 29  "hexadecimal bitwise AND"                        '(( X = 0xFF & 0x0F )); echo "X:$X"'
run_diff_test 30  "hexadecimal bitwise XOR self"                   '(( X = 0x12 ^ 0x12 )); echo "X:$X"'

# --- 4. Relational & Comparison Operators (Tests 031 - 040) ---
run_diff_test 31  "less than true"                                 '(( X = 5 < 10 )); echo "X:$X"'
run_diff_test 32  "less than false"                                '(( X = 10 < 5 )); echo "X:$X"'
run_diff_test 33  "less than or equal equal case"                  '(( X = 5 <= 5 )); echo "X:$X"'
run_diff_test 34  "less than or equal false case"                  '(( X = 5 <= 4 )); echo "X:$X"'
run_diff_test 35  "greater than true"                              '(( X = 10 > 5 )); echo "X:$X"'
run_diff_test 36  "greater than false"                             '(( X = 5 > 10 )); echo "X:$X"'
run_diff_test 37  "greater than or equal equal case"               '(( X = 10 >= 10 )); echo "X:$X"'
run_diff_test 38  "greater than or equal false case"               '(( X = 9 >= 10 )); echo "X:$X"'
run_diff_test 39  "equality operator true"                         '(( X = 42 == 42 )); echo "X:$X"'
run_diff_test 40  "inequality operator false"                      '(( X = 42 != 42 )); echo "X:$X"'

# --- 5. Logical Operators (Tests 041 - 050) ---
run_diff_test 41  "logical AND true"                               '(( X = 1 && 1 )); echo "X:$X"'
run_diff_test 42  "logical AND false"                              '(( X = 1 && 0 )); echo "X:$X"'
run_diff_test 43  "logical OR true"                                '(( X = 0 || 1 )); echo "X:$X"'
run_diff_test 44  "logical OR false"                               '(( X = 0 || 0 )); echo "X:$X"'
run_diff_test 45  "logical NOT of zero"                            '(( X = !0 )); echo "X:$X"'
run_diff_test 46  "logical NOT of one"                             '(( X = !1 )); echo "X:$X"'
run_diff_test 47  "logical NOT of positive number"                 '(( X = !42 )); echo "X:$X"'
run_diff_test 48  "compound logical AND with comparison"           '(( X = (5 > 2) && (10 < 20) )); echo "X:$X"'
run_diff_test 49  "compound logical OR with comparison"            '(( X = (5 < 2) || (10 == 10) )); echo "X:$X"'
run_diff_test 50  "negated compound logical expression"            '(( X = !((5 < 2) || (10 != 10)) )); echo "X:$X"'

# --- 6. Pre- and Post-Increment / Decrement (Tests 051 - 060) ---
run_diff_test 51  "pre-increment assignment"                       '(( A = 5, B = ++A )); echo "A:$A B:$B"'
run_diff_test 52  "post-increment assignment"                      '(( A = 5, B = A++ )); echo "A:$A B:$B"'
run_diff_test 53  "pre-decrement assignment"                       '(( A = 5, B = --A )); echo "A:$A B:$B"'
run_diff_test 54  "post-decrement assignment"                      '(( A = 5, B = A-- )); echo "A:$A B:$B"'
run_diff_test 55  "multiple post-increments"                       '(( A = 10, A++, A++, A++ )); echo "A:$A"'
run_diff_test 56  "multiple pre-decrements"                        '(( A = 10, --A, --A )); echo "A:$A"'
run_diff_test 57  "post-increment in addition"                     '(( A = 5, B = A++ + 10 )); echo "A:$A B:$B"'
run_diff_test 58  "pre-increment in addition"                      '(( A = 5, B = ++A + 10 )); echo "A:$A B:$B"'
run_diff_test 59  "post-decrement in subtraction"                  '(( A = 20, B = A-- - 5 )); echo "A:$A B:$B"'
run_diff_test 60  "pre-decrement in subtraction"                   '(( A = 20, B = --A - 5 )); echo "A:$A B:$B"'

# --- 7. Compound Assignments (Tests 061 - 070) ---
run_diff_test 61  "plus-equals assignment"                         '(( A = 10, A += 5 )); echo "A:$A"'
run_diff_test 62  "minus-equals assignment"                        '(( A = 10, A -= 4 )); echo "A:$A"'
run_diff_test 63  "times-equals assignment"                        '(( A = 10, A *= 3 )); echo "A:$A"'
run_diff_test 64  "divide-equals assignment"                       '(( A = 20, A /= 4 )); echo "A:$A"'
run_diff_test 65  "modulo-equals assignment"                       '(( A = 23, A %= 5 )); echo "A:$A"'
run_diff_test 66  "left-shift-equals assignment"                   '(( A = 1, A <<= 4 )); echo "A:$A"'
run_diff_test 67  "right-shift-equals assignment"                  '(( A = 32, A >>= 2 )); echo "A:$A"'
run_diff_test 68  "bitwise-and-equals assignment"                  '(( A = 7, A &= 2 )); echo "A:$A"'
run_diff_test 69  "bitwise-xor-equals assignment"                  '(( A = 5, A ^= 3 )); echo "A:$A"'
run_diff_test 70  "bitwise-or-equals assignment"                   '(( A = 4, A |= 2 )); echo "A:$A"'

# --- 8. Ternary Conditional Operator (Tests 071 - 080) ---
run_diff_test 71  "ternary true branch literal"                    '(( X = 1 ? 10 : 20 )); echo "X:$X"'
run_diff_test 72  "ternary false branch literal"                   '(( X = 0 ? 10 : 20 )); echo "X:$X"'
run_diff_test 73  "ternary with variable comparison true"          '(( A = 5, X = A > 3 ? 100 : 200 )); echo "X:$X"'
run_diff_test 74  "ternary with variable comparison false"         '(( A = 2, X = A > 3 ? 100 : 200 )); echo "X:$X"'
run_diff_test 75  "ternary min function true branch"               '(( A = 10, B = 20, X = A < B ? A : B )); echo "X:$X"'
run_diff_test 76  "ternary min function false branch"              '(( A = 30, B = 20, X = A < B ? A : B )); echo "X:$X"'
run_diff_test 77  "ternary with equality expression true"          '(( X = (10 + 2 == 12) ? 1 : 0 )); echo "X:$X"'
run_diff_test 78  "ternary with inequality expression false"        '(( X = (10 + 2 != 12) ? 1 : 0 )); echo "X:$X"'
run_diff_test 79  "nested ternary branch 1"                        '(( A = 1, X = A == 0 ? 10 : (A == 1 ? 20 : 30) )); echo "X:$X"'
run_diff_test 80  "nested ternary branch 2"                        '(( A = 2, X = A == 0 ? 10 : (A == 1 ? 20 : 30) )); echo "X:$X"'

# --- 9. Inline Arithmetic Expansion $(( ... )) (Tests 081 - 090) ---
run_diff_test 81  "inline arithmetic addition"                     'echo "Sum: $(( 10 + 20 ))"'
run_diff_test 82  "inline arithmetic multiplication"               'echo "Product: $(( 5 * 5 ))"'
run_diff_test 83  "inline arithmetic hex constants"                'echo "Hex: $(( 0x10 + 0x20 ))"'
run_diff_test 84  "inline arithmetic octal constants"              'echo "Octal: $(( 010 + 010 ))"'
run_diff_test 85  "inline arithmetic assignment to var"            'X=$(( 100 / 4 )); echo "X=$X"'
run_diff_test 86  "inline arithmetic with parentheses"             'echo "Nested: $(( (2 + 3) * 4 ))"'
run_diff_test 87  "inline arithmetic bit shift"                    'echo "Shift: $(( 1 << 10 ))"'
run_diff_test 88  "inline arithmetic power"                        'echo "Pow: $(( 2 ** 10 ))"'
run_diff_test 89  "multiple inline arithmetics on line"            'echo "A=$(( 1 + 2 )) B=$(( 3 + 4 ))"'
run_diff_test 90  "inline arithmetic ternary"                      'echo "Ternary: $(( 10 > 5 ? 1 : 0 ))"'

# --- 10. Multi-expressions, Boundaries & Exit Codes (Tests 091 - 100) ---
run_diff_test 91  "comma operator multiple assignments"            '(( A = 1, B = 2, C = A + B )); echo "C:$C"'
run_diff_test 92  "chained comma calculations"                     '(( A = 10, B = 20, C = 30, D = A * B + C )); echo "D:$D"'
run_diff_test 93  "variables initialized outside arithmetic"       'A=15; B=25; (( C = A + B )); echo "C:$C"'
run_diff_test 94  "(( 1 )) exit code zero"                         '(( 1 )); echo "Exit:$?"'
run_diff_test 95  "(( 0 )) exit code one"                          '(( 0 )); echo "Exit:$?"'
run_diff_test 96  "(( 10 - 10 )) zero result exit code one"        '(( 10 - 10 )); echo "Exit:$?"'
run_diff_test 97  "(( 5 + 5 )) non-zero result exit code zero"     '(( 5 + 5 )); echo "Exit:$?"'
run_diff_test 98  "(( X = 0 )) assign zero exit code one"          '(( X = 0 )); echo "Exit:$? X:$X"'
run_diff_test 99  "64-bit integer multiplication"                  '(( X = 1000000000 * 3 )); echo "X:$X"'
run_diff_test 100 "large bitwise left shift (1 << 30)"             '(( X = 1 << 30 )); echo "X:$X"'

print_summary "MODULE 02 - ARITHMETIC & COMPOUND (( ... ))"
