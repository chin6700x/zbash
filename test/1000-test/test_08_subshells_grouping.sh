#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 08 - SUBSHELLS & GROUPING (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 08/10] SUBSHELLS ( ... ) & COMMAND GROUPING { ...; } (100 TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. Basic Subshells ( ... ) & Exit Codes (Tests 001 - 010) ---
run_diff_test 1   "simple single command in subshell"                  '( echo "subshell hello" )'
run_diff_test 2   "multiple sequential commands in subshell"           '( echo "one"; echo "two" )'
run_diff_test 3   "subshell with colon builtin"                        '( : ); echo "rc:$?"'
run_diff_test 4   "subshell with true builtin"                         '( true ); echo "rc:$?"'
run_diff_test 5   "subshell with false builtin"                        '( false ); echo "rc:$?"'
run_diff_test 6   "subshell explicit exit 0"                           '( exit 0 ); echo "rc:$?"'
run_diff_test 7   "subshell explicit exit 1"                           '( exit 1 ); echo "rc:$?"'
run_diff_test 8   "subshell explicit exit 42"                          '( exit 42 ); echo "rc:$?"'
run_diff_test 9   "subshell explicit exit 127"                         '( exit 127 ); echo "rc:$?"'
run_diff_test 10  "subshell exit code of last command"                 '( echo "first"; false ); echo "rc:$?"'

# --- 2. Subshell Variable & Environment Isolation (Tests 011 - 020) ---
run_diff_test 11  "variable assigned in subshell is isolated"          '( X=100 ); echo "x:[${X:-empty}]"'
run_diff_test 12  "variable mutated in subshell leaves parent intact"  'X=10; ( X=20; echo "sub:$X" ); echo "parent:$X"'
run_diff_test 13  "variable unset in subshell leaves parent intact"    'X=orig; ( unset X; echo "sub:[${X:-empty}]" ); echo "parent:$X"'
run_diff_test 14  "subshell inherits existing parent variable"         'G="greeting"; ( echo "sub:$G" )'
run_diff_test 15  "subshell export does not leak to parent env"        '( export SUB_EXP=secret ); echo "parent:[${SUB_EXP:-empty}]"'
run_diff_test 16  "parent export inherited and modified in subshell"   'export P_EXP=initial; ( P_EXP=modified; echo "sub:$P_EXP" ); echo "parent:$P_EXP"'
run_diff_test 17  "arithmetic assignment isolated in subshell"         'X=5; ( (( X += 10 )); echo "sub:$X" ); echo "parent:$X"'
run_diff_test 18  "multiple variables assigned in subshell isolated"   '( A=1; B=2; C=3; echo "$A$B$C" ); echo "outer:[${A:-e}][${B:-e}][${C:-e}]"'
run_diff_test 19  "subshell reads empty string variable"               'V=""; ( echo "sub:[$V]" )'
run_diff_test 20  "subshell variable isolation with spaces"             'S="initial space"; ( S="new value with space"; echo "sub:$S" ); echo "parent:$S"'

# --- 3. Subshell Working Directory & State Isolation (Tests 021 - 030) ---
run_diff_test 21  "cd in subshell does not affect parent directory"    'ORIG=$(pwd); ( cd /tmp; echo "sub:$(pwd)" ); echo "parent:$( [ "$(pwd)" = "$ORIG" ] && echo same )"'
run_diff_test 22  "multiple cd inside subshell"                       'ORIG=$(pwd); ( cd /tmp; cd /; echo "sub:$(pwd)" ); echo "parent:$( [ "$(pwd)" = "$ORIG" ] && echo same )"'
run_diff_test 23  "cd nonexistent directory in subshell"              '( cd /nonexistent_zb_dir 2>/dev/null ); echo "rc:$?"'
run_diff_test 24  "function defined in subshell not visible in parent" '( f() { echo "sub_fn"; }; f ); f 2>/dev/null || echo "parent:undefined"'
run_diff_test 25  "function overridden in subshell preserves parent"   'f() { echo "parent_fn"; }; ( f() { echo "sub_fn"; }; f ); f'
run_diff_test 26  "$$ pid invariant in subshell"                       'P=$$; ( [ "$$" = "$P" ] && echo "same_pid" )'
run_diff_test 27  "exit terminates subshell only, parent proceeds"     '( echo "sub_start"; exit 9; echo "sub_end" ); echo "parent_rc:$?"'
run_diff_test 28  "exit 0 in subshell does not terminate parent"       '( exit 0 ); echo "parent_alive"'
run_diff_test 29  "subshell function return propagates status code"    'f() { return 5; }; ( f ); echo "parent_rc:$?"'
run_diff_test 30  "subshell function exit terminates subshell only"    'f() { echo "fn_enter"; exit 4; echo "fn_leave"; }; ( f ); echo "parent_rc:$?"'

# --- 4. Nested Subshells ( ( ... ) ) (Tests 031 - 040) ---
run_diff_test 31  "double nested subshell"                             '( ( echo "nested 2" ) )'
run_diff_test 32  "triple nested subshell"                             '( ( ( echo "nested 3" ) ) )'
run_diff_test 33  "quad nested subshell"                               '( ( ( ( echo "nested 4" ) ) ) )'
run_diff_test 34  "nested subshell variable isolation levels"          'X=lvl0; ( X=lvl1; ( X=lvl2; echo "in2:$X" ); echo "in1:$X" ); echo "in0:$X"'
run_diff_test 35  "nested subshell exit propagation from level 3"      '( ( ( exit 77 ) ) ); echo "rc:$?"'
run_diff_test 36  "nested subshell exit 0"                             '( ( ( exit 0 ) ) ); echo "rc:$?"'
run_diff_test 37  "nested subshells with sequential commands"          '( echo "a"; ( echo "b"; ( echo "c" ); echo "d" ); echo "e" )'
run_diff_test 38  "nested subshell cd isolation"                       'ORIG=$(pwd); ( cd /tmp; ( cd /; echo "lvl2:$(pwd)" ); echo "lvl1:$(pwd)" ); echo "lvl0:$( [ "$(pwd)" = "$ORIG" ] && echo same )"'
run_diff_test 39  "nested subshell arithmetic mutation"                'N=1; ( (( N+=1 )); ( (( N+=10 )); echo "l2:$N" ); echo "l1:$N" ); echo "l0:$N"'
run_diff_test 40  "nested subshells with boolean chaining"             '( ( true && echo "ok1" ); ( false || echo "ok2" ) )'

# --- 5. Subshells in Pipelines & Redirections (Tests 041 - 050) ---
run_diff_test 41  "subshell piping to filter"                          '( echo "apple"; echo "banana" ) | tr "a-z" "A-Z"'
run_diff_test 42  "subshell piping to grep"                            '( printf "one\ntwo\nthree\n" ) | grep two'
run_diff_test 43  "pipe input into subshell read"                      'echo "piped_input" | ( read val; echo "val:$val" )'
run_diff_test 44  "pipe input into subshell with multiple reads"       'printf "line1\nline2\n" | ( read a; read b; echo "$b-$a" )'
run_diff_test 45  "subshell to subshell pipe"                          '( echo "foo"; echo "bar" ) | ( tr "a-z" "A-Z" )'
run_diff_test 46  "subshell output redirection to file"                 '( echo "sub_file_out" ) > /tmp/zb_ss1.txt; cat /tmp/zb_ss1.txt; rm -f /tmp/zb_ss1.txt'
run_diff_test 47  "subshell append redirection to file"                 'echo "base" > /tmp/zb_ss2.txt; ( echo "app1"; echo "app2" ) >> /tmp/zb_ss2.txt; cat /tmp/zb_ss2.txt; rm -f /tmp/zb_ss2.txt'
run_diff_test 48  "subshell input redirection from file"               'echo "from_file" > /tmp/zb_ss3.txt; ( read line; echo "got:$line" ) < /tmp/zb_ss3.txt; rm -f /tmp/zb_ss3.txt'
run_diff_test 49  "subshell error redirection"                         '( echo "err_msg" >&2 ) 2> /tmp/zb_ss4.txt; cat /tmp/zb_ss4.txt; rm -f /tmp/zb_ss4.txt'
run_diff_test 50  "subshell pipeline exit code propagation"            '( exit 10 ) | ( exit 25 ); echo "rc:$?"'

# --- 6. Basic Brace Grouping { ...; } & Output (Tests 051 - 060) ---
run_diff_test 51  "simple single command in brace"                     '{ echo "brace hello"; }'
run_diff_test 52  "multiple sequential commands in brace"              '{ echo "line 1"; echo "line 2"; }'
run_diff_test 53  "empty colon in brace"                               '{ :; }; echo "rc:$?"'
run_diff_test 54  "brace with true builtin"                            '{ true; }; echo "rc:$?"'
run_diff_test 55  "brace with false builtin"                           '{ false; }; echo "rc:$?"'
run_diff_test 56  "brace exit status of last command"                  '{ false; true; }; echo "rc:$?"'
run_diff_test 57  "brace exit status on last failure"                  '{ true; false; }; echo "rc:$?"'
run_diff_test 58  "brace with printf and multiple strings"             '{ printf "a:%s b:%s\n" "1" "2"; }'
run_diff_test 59  "brace with arithmetic evaluation"                   '{ (( X = 7 * 6 )); echo "math:$X"; }'
run_diff_test 60  "brace with multi-line statements in single string"  '{ echo "start"; (( i = 100 )); echo "i:$i"; }'

# --- 7. Brace Grouping State Persistence (In-Process) (Tests 061 - 070) ---
run_diff_test 61  "brace variable assignment persists in parent"       '{ B_VAR=persisted; }; echo "b:$B_VAR"'
run_diff_test 62  "brace variable mutation persists in parent"         'VAL=10; { VAL=20; }; echo "val:$VAL"'
run_diff_test 63  "brace variable unset unsets in parent"               'VAL=exists; { unset VAL; }; echo "val:[${VAL:-empty}]"'
run_diff_test 64  "brace export exports to parent environment"         '{ export B_EXP=exported; }; echo "exp:$B_EXP"'
run_diff_test 65  "brace loop modifies parent accumulator"             'SUM=0; { for i in 1 2 3 4; do (( SUM += i )); done; }; echo "sum:$SUM"'
run_diff_test 66  "brace while loop modifies parent variable"          'CNT=0; { while [ $CNT -lt 3 ]; do (( CNT++ )); done; }; echo "cnt:$CNT"'
run_diff_test 67  "function defined in brace accessible in parent"     '{ fn_in_brace() { echo "brace_defined"; }; }; fn_in_brace'
run_diff_test 68  "brace multiple arithmetic mutations persist"        'A=1; B=2; { (( A += 10, B += 20 )); }; echo "A:$A B:$B"'
run_diff_test 69  "brace cd changes parent directory"                  'ORIG=$(pwd); { cd /tmp; }; CUR=$(pwd); [ "$CUR" = "/tmp" ] && echo "in_tmp:yes"; cd "$ORIG"'
run_diff_test 70  "brace exit terminates entire shell process"         '{ exit 33; echo "unreachable"; }; echo "unreachable_parent"'

# --- 8. Brace Grouping with I/O Redirection & Pipelines (Tests 071 - 080) ---
run_diff_test 71  "brace output redirection to file"                   '{ echo "b1"; echo "b2"; } > /tmp/zb_br1.txt; cat /tmp/zb_br1.txt; rm -f /tmp/zb_br1.txt'
run_diff_test 72  "brace append redirection to file"                   'echo "head" > /tmp/zb_br2.txt; { echo "b3"; echo "b4"; } >> /tmp/zb_br2.txt; cat /tmp/zb_br2.txt; rm -f /tmp/zb_br2.txt'
run_diff_test 73  "brace input redirection from file"                  'printf "alpha\nbeta\n" > /tmp/zb_br3.txt; { read w1; read w2; echo "$w2-$w1"; } < /tmp/zb_br3.txt; rm -f /tmp/zb_br3.txt'
run_diff_test 74  "brace error redirection 2> file"                    '{ echo "b_err" >&2; } 2> /tmp/zb_br4.txt; cat /tmp/zb_br4.txt; rm -f /tmp/zb_br4.txt'
run_diff_test 75  "brace error redirection 2>&1 into pipe"             '{ echo "std"; echo "err" >&2; } 2>&1 | tr "a-z" "A-Z"'
run_diff_test 76  "brace in pipeline left side"                        '{ echo "foo"; echo "bar"; } | grep foo'
run_diff_test 77  "brace in pipeline right side"                       'printf "val1\nval2\n" | { read r1; read r2; echo "got:$r1 and $r2"; }'
run_diff_test 78  "brace piping to sort and tr"                        '{ echo 3; echo 1; echo 2; } | sort | tr "\n" " "; echo ""'
run_diff_test 79  "brace redirection with variable mutation"           '{ echo "mutating"; M_VAR=changed; } > /dev/null; echo "m:$M_VAR"'
run_diff_test 80  "brace here-string input redirection"                '{ read h1; read h2; echo "h1:$h1 h2:$h2"; } <<< "hello world"'

# --- 9. Interactions Between Subshells, Braces & Control Flow (Tests 081 - 090) ---
run_diff_test 81  "subshell inside brace grouping"                     '{ ( S_VAR=inner; echo "s:$S_VAR" ); echo "outer:[${S_VAR:-empty}]"; }'
run_diff_test 82  "brace grouping inside subshell"                     '( { B_IN=inner; echo "b:$B_IN"; }; echo "sub:$B_IN" ); echo "outer:[${B_IN:-empty}]"'
run_diff_test 83  "if condition with subshell true"                    'if ( true ); then echo "if_true"; else echo "if_false"; fi'
run_diff_test 84  "if condition with subshell false"                   'if ( false ); then echo "if_true"; else echo "if_false"; fi'
run_diff_test 85  "if condition with brace true"                       'if { true; }; then echo "if_brace_true"; else echo "if_brace_false"; fi'
run_diff_test 86  "if condition with brace false"                      'if { false; }; then echo "if_brace_true"; else echo "if_brace_false"; fi'
run_diff_test 87  "logical AND with subshell"                          '( true ) && echo "and_ok" || echo "and_fail"'
run_diff_test 88  "logical AND with failing subshell"                  '( false ) && echo "and_ok" || echo "and_fail"'
run_diff_test 89  "logical OR with subshell"                           '( exit 1 ) || echo "or_recovered"'
run_diff_test 90  "logical chaining with brace"                         '{ false; } && echo "no" || echo "brace_or_ok"'

# --- 10. Advanced Grouping, Substitutions & Edge Cases (Tests 091 - 100) ---
run_diff_test 91  "subshell inside command substitution"               'OUT=$( ( echo "sub_cmd_sub" ) ); echo "out:[$OUT]"'
run_diff_test 92  "brace inside command substitution"                  'OUT=$( { echo "brace_cmd_sub"; } ); echo "out:[$OUT]"'
run_diff_test 93  "nested command substitution in subshell"            '( echo "res:$( echo inner_val )" )'
run_diff_test 94  "subshell with loop and break"                       '( for i in 1 2 3; do [ $i -eq 2 ] && break; echo "i:$i"; done )'
run_diff_test 95  "subshell with loop and continue"                    '( for i in 1 2 3; do [ $i -eq 2 ] && continue; echo "i:$i"; done )'
run_diff_test 96  "function called in subshell with local variable"    'f() { local loc=local_val; echo "fn:$loc"; }; ( f ); echo "parent:[${loc:-empty}]"'
run_diff_test 97  "function internally using subshell"                 'f() { ( X=sub_only; echo "f_sub:$X" ); }; f; echo "parent:[${X:-empty}]"'
run_diff_test 98  "subshell with spaces in arguments preserved"        '( printf "[%s] [%s]\n" "arg 1" "arg 2" )'
run_diff_test 99  "inverted subshell in conditional"                   '! ( false ) && echo "inverted_true" || echo "inverted_false"'
run_diff_test 100 "pipeline between nested subshells"                  '( ( echo "hello" ) ) | ( ( tr "a-z" "A-Z" ) )'

print_summary "MODULE 08 - SUBSHELLS & GROUPING"
