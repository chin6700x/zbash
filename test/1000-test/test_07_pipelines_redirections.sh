#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 07 - PIPELINES & REDIRECTIONS (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 07/10] PIPELINES, REDIRECTIONS & HERE-STRINGS (100 DIFFERENTIAL TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. Basic Anonymous Pipelines & Standard Filters (Tests 001 - 010) ---
run_diff_test 1   "simple two-command pipe"                        'echo "hello" | cat'
run_diff_test 2   "pipe to grep filter"                            'printf "apple\nbanana\ncherry\n" | grep banana'
run_diff_test 3   "pipe with wc line count"                        'printf "one\ntwo\nthree\n" | wc -l | tr -d " "'
run_diff_test 4   "pipe with tr uppercase"                         'echo "lowercase text" | tr "a-z" "A-Z"'
run_diff_test 5   "pipe with head"                                 'printf "1\n2\n3\n4\n5\n" | head -n 2'
run_diff_test 6   "pipe with tail"                                 'printf "1\n2\n3\n4\n5\n" | tail -n 2'
run_diff_test 7   "pipe with cut delimiter"                        'echo "foo:bar:baz" | cut -d: -f2'
run_diff_test 8   "pipe multiple fields cut"                       'echo "a,b,c,d" | cut -d, -f1,3'
run_diff_test 9   "pipe into read in subshell child"               'echo "secret" | read line; echo "parent:[$line]"'
run_diff_test 10  "pipe empty input"                               'echo -n "" | cat'

# --- 2. Pipelines with Builtins & Expansions (Tests 011 - 020) ---
run_diff_test 11  "pipe newline only with wc"                      'echo "" | wc -l | tr -d " "'
run_diff_test 12  "pipe with arithmetic evaluation"                '(( X = 5 * 2 )); echo $X | cat'
run_diff_test 13  "pipe with variable expansion"                   'MSG="hello world"; echo "$MSG" | tr " " "_"'
run_diff_test 14  "pipe exit code of successful pipeline"          'echo "ok" | cat; echo "rc:$?"'
run_diff_test 15  "pipe with tr char substitution"                 'echo "foo-bar" | tr "-" "/"'
run_diff_test 16  "three stage pipe sort and head"                 'printf "c\na\nb\n" | sort | head -n 2'
run_diff_test 17  "three stage pipe grep and wc count"             'printf "foo\nbar\nfoo\n" | grep foo | wc -l | tr -d " "'
run_diff_test 18  "three stage pipe tr sort head"                  'echo "banana apple cherry" | tr " " "\n" | sort | head -n 1'
run_diff_test 19  "three stage pipe numeric sort"                  'printf "3\n1\n2\n" | sort -n | tr "\n" "," ; echo ""'
run_diff_test 20  "three stage pipe tr sort tail"                  'echo "10:20:30:40" | tr ":" "\n" | sort -n | tail -n 2'

# --- 3. Multi-Stage Pipelines & Ordering (Tests 021 - 030) ---
run_diff_test 21  "five stage pipeline data flow"                  'echo "a b c d e" | tr " " "\n" | sort -r | head -n 3 | tr "\n" " "; echo ""'
run_diff_test 22  "multi-stage pipe with uniq"                     'printf "a\na\nb\nb\nc\n" | uniq | tr "\n" "-"; echo ""'
run_diff_test 23  "multi-stage pipe with sort -u"                  'printf "b\na\nb\na\n" | sort -u | tr "\n" " "; echo ""'
run_diff_test 24  "multi-stage pipe with numbers"                  'printf "10\n2\n30\n" | sort -n | head -n 2'
run_diff_test 25  "multi-stage pipe with reverse sort"             'printf "apple\nbanana\ncherry\n" | sort -r | head -n 1'
run_diff_test 26  "multi-stage pipeline squashing spaces"          'echo "  spaced  text  " | cat | tr -s " "'
run_diff_test 27  "multi-stage pipe with wc word count"            'echo "one two three four" | wc -w | tr -d " "'
run_diff_test 28  "multi-stage pipe with wc char count"            'echo -n "12345" | wc -c | tr -d " "'
run_diff_test 29  "multi-stage pipe leading command failure"       'false | cat | cat; echo "rc:$?"'
run_diff_test 30  "multi-stage pipe middle command failure"        'echo "hi" | false | cat; echo "rc:$?"'

# --- 4. Functions in Pipelines: Producer & Consumer (Tests 031 - 040) ---
run_diff_test 31  "function as pipe producer"                      'prod() { echo "produced"; }; prod | tr a-z A-Z'
run_diff_test 32  "function producing multi-line output"           'gen() { printf "1\n2\n3\n"; }; gen | head -n 2'
run_diff_test 33  "function as pipe consumer"                      'consume() { read val; echo "got:[$val]"; }; echo "data" | consume'
run_diff_test 34  "function consumer while read loop"              'reader() { while read line; do echo "line:$line"; done; }; printf "a\nb\n" | reader'
run_diff_test 35  "function in pipe middle stage"                  'mid() { read x; echo "mid:$x"; }; echo "start" | mid | tr a-z A-Z'
run_diff_test 36  "two functions connected by pipe"                'p1() { echo "stage1"; }; p2() { read s; echo "stage2:$s"; }; p1 | p2'
run_diff_test 37  "function with local variable in pipe"           'fn() { local L="local_val"; echo "$L"; }; fn | cat'
run_diff_test 38  "function in pipe does not mutate parent state"  'f() { count=99; }; count=10; f | cat; echo "count:$count"'
run_diff_test 39  "function returning non-zero in pipe"            'f() { return 42; }; f | cat; echo "rc:$?"'
run_diff_test 40  "function receiving pipe input with arguments"   'fn() { read x; echo "$1:$x"; }; echo "val" | fn "prefix"'

# --- 5. Functions & Compounds in Pipelines (Tests 041 - 050) ---
run_diff_test 41  "function pipe with arithmetic mutation"         'f() { read x; (( res = x * 2 )); echo "res:$res"; }; echo "21" | f'
run_diff_test 42  "function pipe with conditional logic"           'f() { read x; if [ "$x" = "pass" ]; then echo "OK"; else echo "FAIL"; fi; }; echo "pass" | f'
run_diff_test 43  "recursive factorial in pipe"                    'fact() { if [ $1 -le 1 ]; then echo 1; else prev=$(fact $(( $1 - 1 ))); echo $(( $1 * prev )); fi; }; fact 5 | cat'
run_diff_test 44  "function pipe with arguments and shift"         'f() { shift; echo "$1"; }; f a b c | cat'
run_diff_test 45  "subshell command in pipe"                       '( echo "sub_pipe" ) | tr a-z A-Z'
run_diff_test 46  "brace group command in pipe"                    '{ echo "br_pipe"; } | tr a-z A-Z'
run_diff_test 47  "pipe input into brace group"                    'echo "in" | { read x; echo "got:$x"; }'
run_diff_test 48  "pipe input into subshell"                       'echo "in" | ( read x; echo "got:$x" )'
run_diff_test 49  "arithmetic before pipe"                         '(( X = 40 + 2 )); echo $X | tr 4 5'
run_diff_test 50  "function with for loop in pipe"                 'f() { for x in 1 2 3; do echo $x; done; }; f | wc -l | tr -d " "'

# --- 6. Output Redirections: > and >> (Tests 051 - 060) ---
run_diff_test 51  "overwrite redirection creates file"             'echo "hello" > /tmp/zb_t1.txt; cat /tmp/zb_t1.txt; rm -f /tmp/zb_t1.txt'
run_diff_test 52  "overwrite redirection overwrites existing"      'echo "first" > /tmp/zb_t2.txt; echo "second" > /tmp/zb_t2.txt; cat /tmp/zb_t2.txt; rm -f /tmp/zb_t2.txt'
run_diff_test 53  "append redirection appends to existing"         'echo "line1" > /tmp/zb_t3.txt; echo "line2" >> /tmp/zb_t3.txt; cat /tmp/zb_t3.txt; rm -f /tmp/zb_t3.txt'
run_diff_test 54  "append redirection creates missing file"        'rm -f /tmp/zb_t4.txt; echo "created" >> /tmp/zb_t4.txt; cat /tmp/zb_t4.txt; rm -f /tmp/zb_t4.txt'
run_diff_test 55  "multiple sequential appends"                    'rm -f /tmp/zb_t5.txt; echo 1 >> /tmp/zb_t5.txt; echo 2 >> /tmp/zb_t5.txt; echo 3 >> /tmp/zb_t5.txt; tr "\n" " " < /tmp/zb_t5.txt; echo ""; rm -f /tmp/zb_t5.txt'
run_diff_test 56  "overwrite redirection with empty string"        'echo -n "" > /tmp/zb_t6.txt; wc -c < /tmp/zb_t6.txt | tr -d " "; rm -f /tmp/zb_t6.txt'
run_diff_test 57  "overwrite truncates larger existing file"       'printf "1234567890\n" > /tmp/zb_t7.txt; echo "abc" > /tmp/zb_t7.txt; cat /tmp/zb_t7.txt; rm -f /tmp/zb_t7.txt'
run_diff_test 58  "echo -n output redirection"                     'echo -n "no-newline" > /tmp/zb_t8.txt; cat /tmp/zb_t8.txt; rm -f /tmp/zb_t8.txt'
run_diff_test 59  "pwd builtin output redirection non-empty"       'pwd > /tmp/zb_t9.txt; test -s /tmp/zb_t9.txt && echo "non-empty"; rm -f /tmp/zb_t9.txt'
run_diff_test 60  "subshell output redirection"                    '( echo "sub1"; echo "sub2" ) > /tmp/zb_t10.txt; cat /tmp/zb_t10.txt; rm -f /tmp/zb_t10.txt'

# --- 7. Compounds & Functions Output Redirections (Tests 061 - 070) ---
run_diff_test 61  "subshell append redirection"                    'echo "init" > /tmp/zb_t11.txt; ( echo "s1"; echo "s2" ) >> /tmp/zb_t11.txt; cat /tmp/zb_t11.txt; rm -f /tmp/zb_t11.txt'
run_diff_test 62  "brace grouping output redirection"              '{ echo "b1"; echo "b2"; } > /tmp/zb_t12.txt; cat /tmp/zb_t12.txt; rm -f /tmp/zb_t12.txt'
run_diff_test 63  "brace grouping append redirection"              'echo "top" > /tmp/zb_t13.txt; { echo "mid"; echo "bot"; } >> /tmp/zb_t13.txt; cat /tmp/zb_t13.txt; rm -f /tmp/zb_t13.txt'
run_diff_test 64  "function output redirection"                    'f() { echo "fn_out"; }; f > /tmp/zb_t14.txt; cat /tmp/zb_t14.txt; rm -f /tmp/zb_t14.txt'
run_diff_test 65  "function append redirection"                    'echo "base" > /tmp/zb_t15.txt; f() { echo "added"; }; f >> /tmp/zb_t15.txt; cat /tmp/zb_t15.txt; rm -f /tmp/zb_t15.txt'
run_diff_test 66  "input redirection < file to cat"                'echo "source text" > /tmp/zb_i1.txt; cat < /tmp/zb_i1.txt; rm -f /tmp/zb_i1.txt'
run_diff_test 67  "input redirection to wc -l"                     'printf "a\nb\nc\n" > /tmp/zb_i2.txt; wc -l < /tmp/zb_i2.txt | tr -d " "; rm -f /tmp/zb_i2.txt'
run_diff_test 68  "input redirection to read builtin"              'echo "hello read" > /tmp/zb_i3.txt; read x < /tmp/zb_i3.txt; echo "x:[$x]"; rm -f /tmp/zb_i3.txt'
run_diff_test 69  "input redirection to tr translation"            'echo "sample" > /tmp/zb_i4.txt; tr "a-z" "A-Z" < /tmp/zb_i4.txt; rm -f /tmp/zb_i4.txt'
run_diff_test 70  "input redirection to head"                      'printf "1\n2\n3\n" > /tmp/zb_i5.txt; head -n 1 < /tmp/zb_i5.txt; rm -f /tmp/zb_i5.txt'

# --- 8. Input Redirections & Pipeline Interactions (Tests 071 - 080) ---
run_diff_test 71  "input redirection to tail"                      'printf "1\n2\n3\n" > /tmp/zb_i6.txt; tail -n 1 < /tmp/zb_i6.txt; rm -f /tmp/zb_i6.txt'
run_diff_test 72  "input and output redirection simultaneously"     'echo "input_data" > /tmp/zb_in.txt; cat < /tmp/zb_in.txt > /tmp/zb_out.txt; cat /tmp/zb_out.txt; rm -f /tmp/zb_in.txt /tmp/zb_out.txt'
run_diff_test 73  "input and append redirection simultaneously"     'echo "base" > /tmp/zb_out2.txt; echo "append_in" > /tmp/zb_in2.txt; cat < /tmp/zb_in2.txt >> /tmp/zb_out2.txt; cat /tmp/zb_out2.txt; rm -f /tmp/zb_in2.txt /tmp/zb_out2.txt'
run_diff_test 74  "input redirection into pipeline"                'echo "pipe_in" > /tmp/zb_i9.txt; cat < /tmp/zb_i9.txt | tr a-z A-Z; rm -f /tmp/zb_i9.txt'
run_diff_test 75  "input redirection overrides pipe stdin"         'echo "override" > /tmp/zb_i10.txt; echo "ignored" | cat < /tmp/zb_i10.txt; rm -f /tmp/zb_i10.txt'
run_diff_test 76  "input redirection into function"                'f() { read w; echo "fn:$w"; }; echo "arg" > /tmp/zb_i11.txt; f < /tmp/zb_i11.txt; rm -f /tmp/zb_i11.txt'
run_diff_test 77  "input redirection into while read loop"         'printf "x\ny\n" > /tmp/zb_i12.txt; while read l; do echo "l:$l"; done < /tmp/zb_i12.txt; rm -f /tmp/zb_i12.txt'
run_diff_test 78  "input redirection with empty file"              'touch /tmp/zb_i13.txt; cat < /tmp/zb_i13.txt; echo "done:$?"; rm -f /tmp/zb_i13.txt'
run_diff_test 79  "input redirection failure status code"          'cat < /tmp/zb_nonexistent_xyz 2>/dev/null; echo "rc:$?"'
run_diff_test 80  "builtin input redirection failure status code"  'read x < /tmp/zb_nonexistent_xyz 2>/dev/null; echo "rc:$?"'

# --- 9. Error Redirections: 2>, 2>>, 2>&1, >&2 (Tests 081 - 090) ---
run_diff_test 81  "error redirection 2> writes to file"            'f() { echo "err" >&2; }; f 2> /tmp/zb_err1.txt; cat /tmp/zb_err1.txt; rm -f /tmp/zb_err1.txt'
run_diff_test 82  "error redirection 2> suppresses stderr"         'f() { echo "err" >&2; }; f 2> /dev/null; echo "suppressed:$?"'
run_diff_test 83  "error append redirection 2>>"                   'f() { echo "e1" >&2; }; g() { echo "e2" >&2; }; f 2> /tmp/zb_err2.txt; g 2>> /tmp/zb_err2.txt; cat /tmp/zb_err2.txt; rm -f /tmp/zb_err2.txt'
run_diff_test 84  "redirect stderr to stdout with 2>&1"            'f() { echo "msg" >&2; }; f 2>&1 | tr a-z A-Z'
run_diff_test 85  "redirect stdout to file and stderr to stdout"   'f() { echo "normal"; echo "err" >&2; }; f > /tmp/zb_err3.txt 2>&1; cat /tmp/zb_err3.txt; rm -f /tmp/zb_err3.txt'
run_diff_test 86  "redirect stdout to stderr with >&2"             'echo "err_msg" 2> /tmp/zb_e1.txt >&2; cat /tmp/zb_e1.txt; rm -f /tmp/zb_e1.txt'
run_diff_test 87  "redirect stdout to stderr with 1>&2"            'echo "err_msg2" 2> /tmp/zb_e2.txt 1>&2; cat /tmp/zb_e2.txt; rm -f /tmp/zb_e2.txt'
run_diff_test 88  "function writing to stderr redirected to stdout" 'f() { echo "fn_err" >&2; }; f 2>&1 | cat'
run_diff_test 89  "builtin stdout suppressed but stderr captured"  'f() { echo "norm"; echo "warn" >&2; }; f >/dev/null 2> /tmp/zb_w.txt; cat /tmp/zb_w.txt; rm -f /tmp/zb_w.txt'
run_diff_test 90  "separate stdout and stderr redirections"        'echo "ok" > /tmp/zb_o.txt 2> /tmp/zb_e.txt; cat /tmp/zb_o.txt; test -s /tmp/zb_e.txt || echo "err_empty"; rm -f /tmp/zb_o.txt /tmp/zb_e.txt'

# --- 10. Combined Redirections, Here-Strings & Exit Codes (Tests 091 - 100) ---
run_diff_test 91  "combined &> creates output file"                'echo "both" &> /tmp/zb_cb1.txt; cat /tmp/zb_cb1.txt; rm -f /tmp/zb_cb1.txt'
run_diff_test 92  "combined &> overwrites existing file"           'echo "first" &> /tmp/zb_cb2.txt; echo "second" &> /tmp/zb_cb2.txt; cat /tmp/zb_cb2.txt; rm -f /tmp/zb_cb2.txt'
run_diff_test 93  "combined append with 2>&1"                      'echo "l1" &> /tmp/zb_cb3.txt; echo "l2" >> /tmp/zb_cb3.txt 2>&1; cat /tmp/zb_cb3.txt; rm -f /tmp/zb_cb3.txt'
run_diff_test 94  "combined &> on subshell grouping"               '( echo "s_out" ) &> /tmp/zb_cb6.txt; cat /tmp/zb_cb6.txt; rm -f /tmp/zb_cb6.txt'
run_diff_test 95  "combined &> on brace grouping"                  '{ echo "b_out"; } &> /tmp/zb_cb7.txt; cat /tmp/zb_cb7.txt; rm -f /tmp/zb_cb7.txt'
run_diff_test 96  "here-string <<< simple word"                    'cat <<< "hello world"'
run_diff_test 97  "here-string <<< with variable expansion"        'NAME="Alice"; cat <<< "Hello, $NAME!"'
run_diff_test 98  "here-string <<< into read builtin"              'read val <<< "captured"; echo "val:$val"'
run_diff_test 99  "here-string <<< into tr filter"                 'tr "a-z" "A-Z" <<< "lowercase"'
run_diff_test 100 "pipeline exit code propagation last command"    '(exit 10) | (exit 20) | (exit 42); echo "rc:$?"'

print_summary "MODULE 07 - PIPELINES & REDIRECTIONS"
