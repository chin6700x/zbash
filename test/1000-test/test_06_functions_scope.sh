#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 06 - FUNCTIONS & SCOPE (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 06/10] FUNCTIONS, LOCAL VARIABLES & SCOPE (100 DIFFERENTIAL TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. Basic Function Definition & Calling (Tests 001 - 010) ---
run_diff_test 1   "simple function call"                           'hello() { echo "Hello World"; }; hello'
run_diff_test 2   "function with multiple echo statements"         'f() { echo "A"; echo "B"; }; f'
run_diff_test 3   "function keyword syntax"                        'function greet { echo "Greetings"; }; greet'
run_diff_test 4   "function keyword with parentheses syntax"       'function work() { echo "Working"; }; work'
run_diff_test 5   "function exiting with status 0"                 'f() { true; }; f; echo "exit:$?"'
run_diff_test 6   "function exiting with status 1"                 'f() { false; }; f; echo "exit:$?"'
run_diff_test 7   "empty colon function"                           'f() { :; }; f; echo "exit:$?"'
run_diff_test 8   "function invoked multiple times"                'f() { echo "done"; }; f; f'
run_diff_test 9   "function with three sequential statements"      'f() { echo "line1"; echo "line2"; echo "line3"; }; f'
run_diff_test 10  "function with echo -n"                          'f() { echo -n "no-nl "; }; f; echo "end"'

# --- 2. Positional Arguments $1..$9 & $# (Tests 011 - 020) ---
run_diff_test 11  "function argument $1"                           'f() { echo "arg1:$1"; }; f first'
run_diff_test 12  "function arguments $1 and $2"                   'f() { echo "1:$1 2:$2"; }; f one two'
run_diff_test 13  "function arguments $1 $2 $3"                    'f() { echo "1:$1 2:$2 3:$3"; }; f a b c'
run_diff_test 14  "function argument count 0"                      'f() { echo "count:$#"; }; f'
run_diff_test 15  "function argument count 5"                      'f() { echo "count:$#"; }; f 1 2 3 4 5'
run_diff_test 16  "function ninth argument $9"                     'f() { echo "9:$9"; }; f 1 2 3 4 5 6 7 8 9'
run_diff_test 17  "function missing argument expands empty"        'f() { echo "missing:[$2]"; }; f solo'
run_diff_test 18  "function empty string argument"                 'f() { echo "arg:[$1]"; }; f ""'
run_diff_test 19  "function quoted argument with spaces"           'f() { echo "arg:[$1]"; }; f "spaced argument"'
run_diff_test 20  "function arguments from variables"              'A="val1"; B="val2"; f() { echo "1:$1 2:$2"; }; f "$A" "$B"'

# --- 3. Special Parameters $* & $@ in Functions (Tests 021 - 030) ---
run_diff_test 21  "function parameter $* expansion"                'f() { echo "all:$*"; }; f a b c'
run_diff_test 22  "function parameter $@ expansion"                'f() { echo "all:$@"; }; f a b c'
run_diff_test 23  "for loop over $@ words"                         'f() { for x in "$@"; do echo "item:[$x]"; done; }; f one two three'
run_diff_test 24  "for loop over $@ preserving quoted spaces"      'f() { for x in "$@"; do echo "item:[$x]"; done; }; f "a b" "c d"'
run_diff_test 25  "for loop over $@ with 0 arguments"             'f() { for x in "$@"; do echo "item:[$x]"; done; }; f'
run_diff_test 26  "function count and $* combined"                 'f() { echo "cnt:$# all:$*"; }; f x y z'
run_diff_test 27  "function $* embedded in string"                 'f() { echo "pre:$*:post"; }; f 10 20'
run_diff_test 28  "function accumulation over $@"                  'f() { str=""; for x in "$@"; do str="${str}_$x"; done; echo "str:$str"; }; f A B C'
run_diff_test 29  "function $@ printed horizontally"               'f() { for x in "$@"; do echo -n "$x "; done; echo ""; }; f 1 2 3'
run_diff_test 30  "function $* single argument"                    'f() { echo "all:[$*] count:$#"; }; f "single item"'

# --- 4. shift Builtin inside Functions (Tests 031 - 040) ---
run_diff_test 31  "function shift removes first argument"          'f() { shift; echo "1:$1 cnt:$#"; }; f a b c'
run_diff_test 32  "function shift 2 removes two arguments"         'f() { shift 2; echo "1:$1 cnt:$#"; }; f 1 2 3 4'
run_diff_test 33  "while loop consuming arguments via shift"       'f() { while [ $# -gt 0 ]; do echo -n "$1,"; shift; done; echo ""; }; f x y z'
run_diff_test 34  "head and tail separation via shift"             'f() { first=$1; shift; echo "head:$first tail:$*"; }; f red green blue'
run_diff_test 35  "shift on single argument results in count 0"    'f() { shift; echo "cnt:$#"; }; f solo'
run_diff_test 36  "math accumulator using shift"                   'f() { sum=0; while [ $# -gt 0 ]; do (( sum += $1 )); shift; done; echo "sum:$sum"; }; f 10 20 30'
run_diff_test 37  "shift 0 preserves arguments"                    'f() { shift 0; echo "$1"; }; f keep'
run_diff_test 38  "shift on zero arguments exit code"              'f() { shift 1; echo "cnt:$#"; }; f'
run_diff_test 39  "shift with quoted spaced arguments"             'f() { while [ $# -gt 0 ]; do echo "arg:$1"; shift; done; }; f a "b c"'
run_diff_test 40  "two sequential shifts"                          'f() { shift; shift; echo "rem:$*"; }; f 1 2 3 4 5'

# --- 5. return Builtin & Exit Code Propagation (Tests 041 - 050) ---
run_diff_test 41  "return 0 explicit code"                         'f() { return 0; }; f; echo "exit:$?"'
run_diff_test 42  "return 1 error code"                            'f() { return 1; }; f; echo "exit:$?"'
run_diff_test 43  "return 42 custom code"                          'f() { return 42; }; f; echo "exit:$?"'
run_diff_test 44  "return 255 high exit code"                      'f() { return 255; }; f; echo "exit:$?"'
run_diff_test 45  "return terminates function execution early"     'f() { echo "start"; return 5; echo "unreachable"; }; f; echo "rc:$?"'
run_diff_test 46  "bare return propagates false status"            'f() { false; return; }; f; echo "rc:$?"'
run_diff_test 47  "bare return propagates true status"             'f() { true; return; }; f; echo "rc:$?"'
run_diff_test 48  "conditional returns inside if branches"         'f() { if [ $1 -eq 0 ]; then return 10; else return 20; fi; }; f 0; echo "$?"; f 1; echo "$?"'
run_diff_test 49  "function used as if condition failing"          'f() { return 3; }; if f; then echo "ok"; else echo "failed:$?"; fi'
run_diff_test 50  "function used as if condition succeeding"        'f() { return 0; }; if f; then echo "success"; fi'

# --- 6. Local Variables Scope Isolation (Tests 051 - 060) ---
run_diff_test 51  "local variable restores global value"           'X="global"; f() { local X="local"; echo "in:$X"; }; f; echo "out:$X"'
run_diff_test 52  "local variable unsets if previously unset"      'unset X; f() { local X="temp"; echo "in:$X"; }; f; echo "out:[$X]"'
run_diff_test 53  "multiple local variables on one line"           'A="1"; B="2"; f() { local A="10" B="20"; echo "in:$A-$B"; }; f; echo "out:$A-$B"'
run_diff_test 54  "local declaration followed by assignment"       'X="orig"; f() { local X; X="modified"; echo "in:$X"; }; f; echo "out:$X"'
run_diff_test 55  "local variable math increment"                  'N=5; f() { local N=10; (( N += 5 )); echo "in:$N"; }; f; echo "out:$N"'
run_diff_test 56  "local outside function fails"                   'local X=1; echo "exit:$?"'
run_diff_test 57  "three-tier local variable scoping"              'V="outer"; f() { local V="mid"; g() { local V="inner"; echo "g:$V"; }; g; echo "f:$V"; }; f; echo "top:$V"'
run_diff_test 58  "re-declaring local in same scope"               'X="A"; f() { local X="B"; echo "1:$X"; local X="C"; echo "2:$X"; }; f; echo "3:$X"'
run_diff_test 59  "unsetting local variable inside function"       'A="keep"; f() { local A="temp"; unset A; echo "in:[$A]"; }; f; echo "out:$A"'
run_diff_test 60  "local variable initialized from outer value"    'X=100; f() { local X=$(( X + 50 )); echo "in:$X"; }; f; echo "out:$X"'

# --- 7. Nested Function Calls (Chained Invocations) (Tests 061 - 070) ---
run_diff_test 61  "function calling another function"              'f1() { echo "1"; f2; }; f2() { echo "2"; }; f1'
run_diff_test 62  "three-level call chain c -> b -> a"             'a() { b; echo "a"; }; b() { c; echo "b"; }; c() { echo "c"; }; a'
run_diff_test 63  "function forwarding argument to another"        'f1() { echo "f1:$1"; f2 "$1-sub"; }; f2() { echo "f2:$1"; }; f1 test'
run_diff_test 64  "nested function returning status code"          'inner() { echo "inner:$1"; return 3; }; outer() { inner "$1"; return $?; }; outer val; echo "rc:$?"'
run_diff_test 65  "function output captured by caller"             'add() { echo $(( $1 + $2 )); }; calc() { res=$(add $1 $2); echo "res:$res"; }; calc 15 25'
run_diff_test 66  "four-level chained invocations"                 'f1() { f2; }; f2() { f3; }; f3() { echo "deep"; }; f1'
run_diff_test 67  "math helper function squaring"                  'mul() { echo $(( $1 * $2 )); }; square() { mul $1 $1; }; square 7'
run_diff_test 68  "nested function parameter passthrough"          'f() { g $1; }; g() { h $1; }; h() { echo "h:$1"; }; f hello'
run_diff_test 69  "caller and callee separate argument counts"     'caller() { callee 1 2 3; echo "caller_cnt:$#"; }; callee() { echo "callee_cnt:$#"; }; caller a b'
run_diff_test 70  "callee error return code propagates"            'f() { g; echo "post-g"; }; g() { return 1; }; f; echo "exit:$?"'

# --- 8. Recursive Functions (Tests 071 - 080) ---
run_diff_test 71  "recursive factorial of 5"                      'fact() { if [ $1 -le 1 ]; then echo 1; else prev=$(fact $(( $1 - 1 ))); echo $(( $1 * prev )); fi; }; fact 5'
run_diff_test 72  "recursive factorial of 1"                      'fact() { if [ $1 -le 1 ]; then echo 1; else prev=$(fact $(( $1 - 1 ))); echo $(( $1 * prev )); fi; }; fact 1'
run_diff_test 73  "recursive factorial of 0"                      'fact() { if [ $1 -le 1 ]; then echo 1; else prev=$(fact $(( $1 - 1 ))); echo $(( $1 * prev )); fi; }; fact 0'
run_diff_test 74  "recursive fibonacci of 6"                      'fib() { if [ $1 -le 0 ]; then echo 0; elif [ $1 -eq 1 ]; then echo 1; else a=$(fib $(( $1 - 1 ))); b=$(fib $(( $1 - 2 ))); echo $(( a + b )); fi; }; fib 6'
run_diff_test 75  "recursive fibonacci of 1"                      'fib() { if [ $1 -le 0 ]; then echo 0; elif [ $1 -eq 1 ]; then echo 1; else a=$(fib $(( $1 - 1 ))); b=$(fib $(( $1 - 2 ))); echo $(( a + b )); fi; }; fib 1'
run_diff_test 76  "recursive countdown timer"                      'countdown() { if [ $1 -le 0 ]; then echo "blastoff"; else echo -n "$1 "; countdown $(( $1 - 1 )); fi; }; countdown 3'
run_diff_test 77  "recursive sum of 1 to N"                        'sum_n() { if [ $1 -le 0 ]; then echo 0; else prev=$(sum_n $(( $1 - 1 ))); echo $(( $1 + prev )); fi; }; sum_n 5'
run_diff_test 78  "recursive integer power computation"           'pow() { if [ $2 -le 0 ]; then echo 1; else prev=$(pow $1 $(( $2 - 1 ))); echo $(( $1 * prev )); fi; }; pow 2 4'
run_diff_test 79  "recursive depth tracer unwind"                  'rec_depth() { if [ $1 -gt 0 ]; then rec_depth $(( $1 - 1 )); fi; echo -n "$1 "; }; rec_depth 3; echo ""'
run_diff_test 80  "recursive euclidean greatest common divisor"   'gcd() { if [ $2 -eq 0 ]; then echo $1; else gcd $2 $(( $1 % $2 )); fi; }; gcd 48 18'

# --- 9. Overwriting & Redefining Functions (Tests 081 - 090) ---
run_diff_test 81  "function redefinition overwrites body"          'f() { echo "v1"; }; f; f() { echo "v2"; }; f'
run_diff_test 82  "triple function redefinition"                   'greet() { echo "Hi"; }; greet; greet() { echo "Hello"; }; greet; greet() { echo "Hey"; }; greet'
run_diff_test 83  "redefined function with arguments"              'f() { echo "old:$1"; }; f A; f() { echo "new:$1"; }; f B'
run_diff_test 84  "redefined function changes return code"         'f() { return 1; }; f; echo "$?"; f() { return 0; }; f; echo "$?"'
run_diff_test 85  "redefinition from keyword to standard syntax"   'function f { echo "A"; }; f; f() { echo "B"; }; f'
run_diff_test 86  "redefinition from standard to keyword syntax"   'f() { echo "A"; }; function f { echo "B"; }; f'
run_diff_test 87  "redefined function multiple statements"         'f() { echo "X"; }; f; f() { echo "Y"; echo "Z"; }; f'
run_diff_test 88  "redefined function with empty body"             'f() { echo "1"; }; f; f() { :; }; f; echo "empty"'
run_diff_test 89  "redefined function updates global state"        'f() { (( count = 1 )); }; f; echo "c:$count"; f() { (( count = 2 )); }; f; echo "c:$count"'
run_diff_test 90  "redefined function with different signature"    'f() { echo "$#"; }; f 1 2; f() { echo "fixed"; }; f 1 2'

# --- 10. Functions with Control Flow, Arrays & Complex Bodies (Tests 091 - 100) ---
run_diff_test 91  "function iterating over $@ with for in"         'f() { for x in "$@"; do echo "item:$x"; done; }; f apple banana cherry'
run_diff_test 92  "function containing if else condition"          'f() { if [ $1 -gt 10 ]; then echo "gt10"; else echo "le10"; fi; }; f 15; f 5'
run_diff_test 93  "function with while and continue statement"     'f() { while [ $# -gt 0 ]; do if [ $1 = "skip" ]; then shift; continue; fi; echo "v:$1"; shift; done; }; f a skip b'
run_diff_test 94  "function with for loop and break"               'f() { for x; do if [ $x = "stop" ]; then break; fi; echo "x:$x"; done; }; f 1 2 stop 3'
run_diff_test 95  "function with arithmetic mutation"              'f() { case_x=$1; (( case_x *= 2 )); echo "res:$case_x"; }; f 21'
run_diff_test 96  "function using array and iteration"             'f() { arr=(a b c); for elem in "${arr[@]}"; do echo "e:$elem"; done; }; f'
run_diff_test 97  "function multi-argument arithmetic calculation" 'f() { (( res = $1 * $2 + $3 )); echo "calc:$res"; }; f 3 4 5'
run_diff_test 98  "function definition with semicolon"             'f() { echo "output"; }; f'
run_diff_test 99  "command substitution capturing function output" 'f() { echo "$1"; return 0; }; res=$(f "captured"); echo "got:$res"'
run_diff_test 100 "function with C-style for loop accumulator"     'f() { local sum=0; for (( i=1; i<=$1; i++ )); do (( sum += i )); done; echo "sum:$sum"; }; f 10'

print_summary "MODULE 06 - FUNCTIONS & SCOPE"
