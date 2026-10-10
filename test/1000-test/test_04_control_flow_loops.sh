#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 04 - CONTROL FLOW & LOOPS (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 04/10] CONTROL FLOW & LOOPS (100 DIFFERENTIAL PARITY TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. Basic for ... in Words & Iteration (Tests 001 - 010) ---
run_diff_test 1   "for loop multiple words"                        'for x in a b c; do echo "item:$x"; done'
run_diff_test 2   "for loop numeric list"                          'for x in 1 2 3 4 5; do echo "n:$x"; done'
run_diff_test 3   "for loop quoted multi-word strings"             'for str in "apple pie" "banana split"; do echo "str:[$str]"; done'
run_diff_test 4   "for loop single item"                           'for x in single; do echo "x:$x"; done'
run_diff_test 5   "for loop empty list with space"                 'for x in ; do echo "x:$x"; done'
run_diff_test 6   "for loop empty list immediately"                'for x in; do echo "x:$x"; done'
run_diff_test 7   "for loop over unquoted variable words"          'X="foo bar baz"; for w in $X; do echo "w:$w"; done'
run_diff_test 8   "for loop inline echo without newline"           'for x in a b c; do echo -n "$x "; done; echo ""'
run_diff_test 9   "for loop accumulator sum"                       'sum=0; for n in 10 20 30; do (( sum += n )); done; echo "sum:$sum"'
run_diff_test 10  "for loop string concatenation"                  'out=""; for ch in X Y Z; do out="${out}${ch}"; done; echo "out:$out"'

# --- 2. Quoting, Spaces, and Word Splitting in for ... in (Tests 011 - 020) ---
run_diff_test 11  "for loop single quoted items"                   'for x in '"'one two'"' '"'three four'"'; do echo "[$x]"; done'
run_diff_test 12  "for loop multiple spaces preserved"             'for x in "hello  world" "good  day"; do echo "[$x]"; done'
run_diff_test 13  "for loop quoted variables with spaces"          'A="val 1"; B="val 2"; for x in "$A" "$B"; do echo "[$x]"; done'
run_diff_test 14  "for loop empty strings in list"                 'for x in "" "a" ""; do echo "item:[$x]"; done'
run_diff_test 15  "for loop mixed quoted and unquoted words"       'for x in "a b" c "d e"; do echo "[$x]"; done'
run_diff_test 16  "for loop middle empty string"                   'for x in 1 "" 2; do echo "[$x]"; done'
run_diff_test 17  "for loop variable interpolation in items"       'X="first"; for x in "${X}_1" "${X}_2"; do echo "[$x]"; done'
run_diff_test 18  "for loop special characters in items"           'for x in "*" "+" "-"; do echo "sym:[$x]"; done'
run_diff_test 19  "for loop literal escaped n in item"             'for x in "hello\nworld"; do echo "[$x]"; done'
run_diff_test 20  "for loop mixed embedded quotes"                 'for x in "a'"'"'b" '"'"'c"d'"'"'; do echo "[$x]"; done'

# --- 3. Range Expansion & Arrays in for ... in (Tests 021 - 030) ---
run_diff_test 21  "for loop brace range ascending"                 'for i in {1..5}; do echo -n "$i,"; done; echo ""'
run_diff_test 22  "for loop brace range descending"                'for i in {5..1}; do echo -n "$i,"; done; echo ""'
run_diff_test 23  "for loop brace range negative to positive"      'for i in {-2..2}; do echo -n "$i,"; done; echo ""'
run_diff_test 24  "for loop brace range descending into negative"  'for i in {3..-1}; do echo -n "$i,"; done; echo ""'
run_diff_test 25  "for loop brace range starting at 0"             'for i in {0..4}; do echo -n "$i"; done; echo ""'
run_diff_test 26  "for loop over array elements"                   'arr=(red green blue); for c in "${arr[@]}"; do echo "col:$c"; done'
run_diff_test 27  "for loop over numeric array"                    'arr=(10 20 30); for n in "${arr[@]}"; do echo "n:$n"; done'
run_diff_test 28  "for loop over array indices"                    'arr=(alpha beta gamma); for idx in "${!arr[@]}"; do echo "i:$idx"; done'
run_diff_test 29  "for loop over array slice"                      'arr=(a b c d e); for x in "${arr[@]:1:3}"; do echo "slice:$x"; done'
run_diff_test 30  "for loop array with quoted spaces"              'arr=(one "two three" four); for x in "${arr[@]}"; do echo "[$x]"; done'

# --- 4. Positional Parameter For Loop (for var; do) (Tests 031 - 040) ---
run_diff_test 31  "positional for loop inside function"            'func() { for x; do echo "arg:$x"; done; }; func one two three'
run_diff_test 32  "positional for loop inline formatted"           'func() { for x; do echo -n "[$x]"; done; echo ""; }; func a b'
run_diff_test 33  "positional for loop argument counter"           'func() { count=0; for x; do (( count++ )); done; echo "cnt:$count"; }; func 10 20 30 40'
run_diff_test 34  "positional for loop single argument"            'func() { for arg; do echo "arg:$arg"; done; }; func solo'
run_diff_test 35  "positional for loop zero arguments"             'func() { for x; do echo "x:$x"; done; }; func'
run_diff_test 36  "positional for loop quoted space arguments"     'func() { for x; do echo "x:[$x]"; done; }; func "with spaces" "quoted"'
run_diff_test 37  "positional for loop accumulation"               'func() { res=""; for x; do res="${res}:${x}"; done; echo "res:$res"; }; func a b c'
run_diff_test 38  "positional for loop with break"                 'func() { for x; do if [ $x = "stop" ]; then break; fi; echo "v:$x"; done; }; func a b stop c'
run_diff_test 39  "positional for loop with continue"              'func() { for x; do if [ $x = "skip" ]; then continue; fi; echo "v:$x"; done; }; func 1 skip 2'
run_diff_test 40  "positional for loop multiple invocations"       'func() { for x; do echo "f:$x"; done; }; func x y; func 1 2'

# --- 5. Nested For Loops & Scope (Tests 041 - 050) ---
run_diff_test 41  "nested for loops basic pairs"                   'for i in 1 2; do for j in A B; do echo "$i-$j"; done; done'
run_diff_test 42  "nested for loops product accumulation"          'sum=0; for i in 1 2 3; do for j in 10 20; do (( sum += i * j )); done; done; echo "sum:$sum"'
run_diff_test 43  "triple nested for loops"                        'for x in 1 2; do for y in 3 4; do for z in 5 6; do echo -n "$x$y$z "; done; done; done; echo ""'
run_diff_test 44  "variable modified in inner loop"                'for i in a b; do v="outer"; for j in 1 2; do v="inner"; done; echo "v:$v"; done'
run_diff_test 45  "string accumulation in nested loop"             'out=""; for i in X Y; do for j in 1 2; do out="${out}${i}${j}"; done; done; echo "out:$out"'
run_diff_test 46  "nested loops with range expansions"             'for i in {1..2}; do for j in {3..4}; do echo "$i*$j=$(( i * j ))"; done; done'
run_diff_test 47  "inner loop math accumulator"                    'for a in 1 2; do b=10; for c in 100 200; do (( b += c )); done; echo "b:$b"; done'
run_diff_test 48  "nested loop inner string reset"                 'for i in 1 2; do str=""; for j in a b c; do str="${str}$j"; done; echo "str:$str"; done'
run_diff_test 49  "nested loop 3x3 iteration count"                'cnt=0; for i in {1..3}; do for j in {1..3}; do (( cnt++ )); done; done; echo "cnt:$cnt"'
run_diff_test 50  "nested loop formatted pair printing"           'for i in A B; do for j in 1 2; do echo "pair:($i,$j)"; done; done'

# --- 6. C-Style for (( init; cond; step )) Basic (Tests 051 - 060) ---
run_diff_test 51  "C-style for loop basic increment"               'for (( i=0; i<5; i++ )); do echo -n "$i "; done; echo ""'
run_diff_test 52  "C-style for loop <= condition"                  'for (( i=1; i<=4; i++ )); do echo "i:$i"; done'
run_diff_test 53  "C-style for loop decrement"                     'for (( i=5; i>0; i-- )); do echo -n "$i "; done; echo ""'
run_diff_test 54  "C-style for loop step +2"                       'for (( i=0; i<=10; i+=2 )); do echo -n "$i "; done; echo ""'
run_diff_test 55  "C-style for loop step -3"                       'for (( i=10; i>=0; i-=3 )); do echo -n "$i "; done; echo ""'
run_diff_test 56  "C-style for loop sum accumulator"              'sum=0; for (( i=1; i<=5; i++ )); do (( sum += i )); done; echo "sum:$sum"'
run_diff_test 57  "C-style for loop product accumulator"          'prod=1; for (( i=1; i<=4; i++ )); do (( prod *= i )); done; echo "prod:$prod"'
run_diff_test 58  "C-style for loop 0 iterations false cond"       'for (( i=0; i<0; i++ )); do echo "never"; done; echo "done"'
run_diff_test 59  "C-style for loop 0 iterations exit status"      'for (( i=10; i<10; i++ )); do echo "never"; done; echo "exit:$?"'
run_diff_test 60  "C-style for loop pre-increment step"            'for (( i=0; i<3; ++i )); do echo "pre:$i"; done'

# --- 7. C-Style for (( ... )) Advanced Expressions (Tests 061 - 070) ---
run_diff_test 61  "C-style for loop two variables comma init"      'for (( i=0, j=10; i<5; i++, j-- )); do echo "i:$i j:$j"; done'
run_diff_test 62  "C-style for loop multi-var step calculation"    'for (( a=1, b=1; a<=10; a+=b, b++ )); do echo "a:$a b:$b"; done'
run_diff_test 63  "C-style for loop quadratic condition"           'for (( i=1; i*i<=25; i++ )); do echo "sq:$(( i * i ))"; done'
run_diff_test 64  "C-style for loop power of two step"             'for (( i=1; i<=16; i*=2 )); do echo -n "$i "; done; echo ""'
run_diff_test 65  "C-style for loop using external variable limit" 'limit=4; for (( i=0; i<limit; i++ )); do echo "i:$i"; done'
run_diff_test 66  "nested C-style for loops"                       'for (( i=0; i<3; i++ )); do for (( j=0; j<2; j++ )); do echo "($i,$j)"; done; done'
run_diff_test 67  "C-style for loop complex body formula"          'val=0; for (( i=0; i<5; i++ )); do (( val = (val + i) * 2 )); done; echo "val:$val"'
run_diff_test 68  "C-style for loop with modulo check inside"      'for (( i=0; i<6; i++ )); do if (( i % 2 == 0 )); then echo "even:$i"; fi; done'
run_diff_test 69  "C-style for loop early break"                   'for (( i=0; i<5; i++ )); do if (( i == 3 )); then break; fi; echo "i:$i"; done'
run_diff_test 70  "C-style for loop continue condition"            'for (( i=0; i<5; i++ )); do if (( i == 2 )); then continue; fi; echo "i:$i"; done'

# --- 8. while Loops (Tests 071 - 080) ---
run_diff_test 71  "while loop with test brackets -lt"              'i=0; while [ $i -lt 4 ]; do echo "i:$i"; (( i++ )); done'
run_diff_test 72  "while loop countdown -gt"                       'i=5; while [ $i -gt 0 ]; do echo -n "$i "; (( i-- )); done; echo ""'
run_diff_test 73  "while loop with compound arithmetic cond"       'i=0; while (( i < 3 )); do echo "i:$i"; (( i++ )); done'
run_diff_test 74  "while loop sum accumulator"                     'sum=0; i=1; while [ $i -le 4 ]; do (( sum += i, i++ )); done; echo "sum:$sum"'
run_diff_test 75  "while false 0 iterations exit status"           'while false; do echo "never"; done; echo "exit:$?"'
run_diff_test 76  "while loop false condition exit status"         'i=0; while [ $i -lt 0 ]; do echo "never"; done; echo "exit:$?"'
run_diff_test 77  "while loop doubling step"                       'i=1; while (( i <= 8 )); do echo -n "$i "; (( i *= 2 )); done; echo ""'
run_diff_test 78  "while loop with break statement"                'i=0; while [ $i -lt 5 ]; do if [ $i -eq 2 ]; then break; fi; echo "i:$i"; (( i++ )); done'
run_diff_test 79  "while loop with continue statement"             'i=0; while [ $i -lt 4 ]; do (( i++ )); if [ $i -eq 2 ]; then continue; fi; echo "i:$i"; done'
run_diff_test 80  "nested while loops"                             'i=0; while [ $i -lt 2 ]; do j=0; while [ $j -lt 2 ]; do echo "w:$i-$j"; (( j++ )); done; (( i++ )); done'

# --- 9. until Loops (Tests 081 - 090) ---
run_diff_test 81  "until loop with test brackets -ge"              'i=0; until [ $i -ge 4 ]; do echo "i:$i"; (( i++ )); done'
run_diff_test 82  "until loop countdown -le"                       'i=5; until [ $i -le 0 ]; do echo -n "$i "; (( i-- )); done; echo ""'
run_diff_test 83  "until loop with compound arithmetic cond"      'i=0; until (( i >= 3 )); do echo "i:$i"; (( i++ )); done'
run_diff_test 84  "until true 0 iterations exit status"            'until true; do echo "never"; done; echo "exit:$?"'
run_diff_test 85  "until loop sum accumulator"                    'i=1; sum=0; until [ $i -gt 4 ]; do (( sum += i, i++ )); done; echo "sum:$sum"'
run_diff_test 86  "until loop with break statement"               'i=0; until [ $i -ge 5 ]; do if [ $i -eq 3 ]; then break; fi; echo "i:$i"; (( i++ )); done'
run_diff_test 87  "until loop with continue statement"            'i=0; until [ $i -ge 4 ]; do (( i++ )); if [ $i -eq 2 ]; then continue; fi; echo "i:$i"; done'
run_diff_test 88  "until loop doubling step"                      'i=1; until (( i > 8 )); do echo -n "$i "; (( i *= 2 )); done; echo ""'
run_diff_test 89  "nested until loops"                            'i=0; until [ $i -ge 2 ]; do j=0; until [ $j -ge 2 ]; do echo "u:$i-$j"; (( j++ )); done; (( i++ )); done'
run_diff_test 90  "until loop subtraction step"                   'x=10; until [ $x -lt 5 ]; do (( x -= 2 )); echo "x:$x"; done'

# --- 10. break, continue, Multi-Level & Exit Codes (Tests 091 - 100) ---
run_diff_test 91  "break immediately on first iteration"          'for i in 1 2 3; do break; echo "never"; done; echo "exit:$?"'
run_diff_test 92  "continue immediately on every iteration"       'for i in 1 2 3; do continue; echo "never"; done; echo "exit:$?"'
run_diff_test 93  "break 2 from nested for loops"                 'for i in 1 2 3; do for j in A B C; do if [ $j = "B" ]; then break 2; fi; echo "$i-$j"; done; done'
run_diff_test 94  "continue 2 from nested for loops"              'for i in 1 2; do for j in A B C; do if [ $j = "B" ]; then continue 2; fi; echo "$i-$j"; done; done'
run_diff_test 95  "break 2 from nested while loops"                'i=0; while [ $i -lt 3 ]; do j=0; while [ $j -lt 3 ]; do if [ $j -eq 1 ]; then break 2; fi; echo "$i-$j"; (( j++ )); done; (( i++ )); done'
run_diff_test 96  "break 2 from nested C-style for loops"          'for (( i=0; i<3; i++ )); do for (( j=0; j<3; j++ )); do if (( j == 1 )); then break 2; fi; echo "$i-$j"; done; done'
run_diff_test 97  "break 3 from triple nested loops"              'for i in 1 2; do for j in 10 20; do for k in 100 200; do if [ $k -eq 100 ]; then break 3; fi; echo "$i-$j-$k"; done; done; done; echo "broken"'
run_diff_test 98  "exit status after false and break"              'for x in a b; do false; break; done; echo "status:$?"'
run_diff_test 99  "for loop iterating keyword in as item"         'for x in in; do echo "item:$x"; done'
run_diff_test 100 "even numbers sum with continue filter"         'res=0; for (( i=1; i<=10; i++ )); do if (( i % 2 == 1 )); then continue; fi; (( res += i )); done; echo "even_sum:$res"'

print_summary "MODULE 04 - CONTROL FLOW & LOOPS"
