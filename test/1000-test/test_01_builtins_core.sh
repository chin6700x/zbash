#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MODULE 01 - CORE BUILTINS (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

echo "================================================================================"
echo "🧪 [SUITE 01/10] CORE BUILTIN COMMANDS (100 DIFFERENTIAL PARITY TESTS)"
echo "Baseline: /bin/bash vs Engine: zbash"
echo "================================================================================"

# --- 1. echo Builtin (Tests 001 - 020) ---
run_diff_test 1  "echo plain string"                         'echo Hello World'
run_diff_test 2  "echo multiple arguments"                   'echo one two three four'
run_diff_test 3  "echo empty call"                           'echo'
run_diff_test 4  "echo single quotes"                        'echo '\''Single Quoted String'\'
run_diff_test 5  "echo double quotes"                        'echo "Double Quoted String"'
run_diff_test 6  "echo -n without newline"                   'echo -n "No Newline"'
run_diff_test 7  "echo -n with trailing spaces"              'echo -n "trailing spaces   "'
run_diff_test 8  "echo -e with newline escape"               'echo -e "Line1\nLine2"'
run_diff_test 9  "echo -e with tab escape"                   'echo -e "Col1\tCol2"'
run_diff_test 10 "echo -e with backslash escape"             'echo -e "Backslash: \\\\"'
run_diff_test 11 "echo -ne combined flags"                   'echo -ne "Tab:\tDone\n"'
run_diff_test 12 "echo with escaped asterisks and question"  'echo "\* and \?"'
run_diff_test 13 "echo with semicolon in quotes"             'echo "First; Second"'
run_diff_test 14 "echo numbers and negative values"          'echo 0 42 -100 999999'
run_diff_test 15 "echo multiple lines in quotes"             'echo "Line A
Line B"'
run_diff_test 16 "echo preserved multiple spaces"            'echo "A    B    C"'
run_diff_test 17 "echo variable evaluation"                  'V="Zephyr"; echo "Engine: $V"'
run_diff_test 18 "echo empty variable"                       'EMPTY=""; echo "Val:[$EMPTY]"'
run_diff_test 19 "echo -E suppress escapes"                  'echo -E "No\tEscape"'
run_diff_test 20 "echo with trailing slash"                  'echo "/path/to/dir/"'

# --- 2. pwd and Path Reporting (Tests 021 - 030) ---
run_diff_test 21 "pwd basic output"                          'pwd'
run_diff_test 22 "pwd matches PWD environment variable"      'test "$(pwd)" = "$PWD" && echo "PWD_MATCH"'
run_diff_test 23 "pwd after subshell cd"                     '( cd /tmp ); pwd'
run_diff_test 24 "pwd in subdirectory"                       'cd /tmp; pwd; cd /Volumes'
run_diff_test 25 "pwd extra args ignored"                    'pwd foo bar'
run_diff_test 26 "echo OLDPWD existence"                     'cd /tmp; cd - >/dev/null; test -n "$OLDPWD" && echo "OLDPWD_SET"'
run_diff_test 27 "cd and verify pwd output"                  'cd /tmp; pwd'
run_diff_test 28 "cd - prints previous directory"            'cd /tmp; cd /var; cd - >/dev/null; pwd'
run_diff_test 29 "cd /tmp then echo PWD"                     'cd /tmp; echo "$PWD"'
run_diff_test 30 "cd / then pwd"                             'cd /; pwd'

# --- 3. cd Builtin Navigation and Errors (Tests 031 - 045) ---
run_diff_test 31 "cd /tmp root path"                         'cd /tmp && echo "CD_OK"'
run_diff_test 32 "cd ."                                      'cd . && echo "CD_DOT_OK"'
run_diff_test 33 "cd .."                                     'cd /tmp && cd .. && echo "CD_DOTDOT_OK"'
run_diff_test 34 "cd /"                                      'cd / && echo "CD_ROOT_OK"'
run_diff_test 35 "cd /var/tmp"                               'cd /var/tmp && echo "CD_VAR_OK"'
run_diff_test 36 "cd non-existent directory exit code"       'cd /non_existent_dir_998877 2>/dev/null; echo "Exit: $?"'
run_diff_test 37 "cd non-existent directory error output"    'cd /non_existent_dir_998877 2>&1 | grep -q "not found\|No such file" && echo "ERR_REPORTED"'
run_diff_test 38 "cd with extra arguments"                   'cd /tmp /var 2>/dev/null; echo "Exit: $?"'
run_diff_test 39 "cd - updates OLDPWD correctly"             'cd /tmp; cd /; cd - >/dev/null; echo "$PWD"'
run_diff_test 40 "cd chaining with semicolons"               'cd /tmp; pwd; cd /; pwd'
run_diff_test 41 "cd to HOME directory via variable"         'cd "$HOME" && test "$PWD" = "$HOME" && echo "HOME_OK"'
run_diff_test 42 "cd without arguments defaults to HOME"     'cd && test "$PWD" = "$HOME" && echo "CD_HOME_DEFAULT_OK"'
run_diff_test 43 "cd to relative path"                       'cd /tmp && cd .. && pwd'
run_diff_test 44 "cd /tmp and subfolder check"               'cd /tmp; test -d . && echo "DIR_EXISTS"'
run_diff_test 45 "cd failure does not change PWD"            'ORIG="$PWD"; cd /bad_path_xyz 2>/dev/null; test "$PWD" = "$ORIG" && echo "PWD_PRESERVED"'

# --- 4. true and false Builtins (Tests 046 - 055) ---
run_diff_test 46 "true exit code 0"                          'true; echo "Exit: $?"'
run_diff_test 47 "false exit code 1"                         'false; echo "Exit: $?"'
run_diff_test 48 "true with arguments ignored"               'true foo bar; echo "Exit: $?"'
run_diff_test 49 "false with arguments ignored"              'false foo bar; echo "Exit: $?"'
run_diff_test 50 "true in logical AND"                       'true && echo "TRUE_AND_OK"'
run_diff_test 51 "false in logical OR fallback"              'false || echo "FALSE_OR_OK"'
run_diff_test 52 "true in logical OR does not run second"    'true || echo "SHOULD_NOT_PRINT"'
run_diff_test 53 "false in logical AND does not run second"  'false && echo "SHOULD_NOT_PRINT"'
run_diff_test 54 "negated true ! true"                       '! true; echo "Exit: $?"'
run_diff_test 55 "negated false ! false"                     '! false; echo "Exit: $?"'

# --- 5. type Builtin (Tests 056 - 070) ---
run_diff_test 56 "type echo identification"                  'type echo'
run_diff_test 57 "type cd identification"                    'type cd'
run_diff_test 58 "type pwd identification"                   'type pwd'
run_diff_test 59 "type true identification"                  'type true'
run_diff_test 60 "type false identification"                 'type false'
run_diff_test 61 "type export identification"                'type export'
run_diff_test 62 "type unset identification"                 'type unset'
run_diff_test 63 "type exit identification"                  'type exit'
run_diff_test 64 "type type identification"                  'type type'
run_diff_test 65 "type read identification"                  'type read'
run_diff_test 66 "type shift identification"                 'type shift'
run_diff_test 67 "type non-existent command exit code"       'type nonexistent_foo_123 2>/dev/null; echo "Exit: $?"'
run_diff_test 68 "type external binary ls"                   'type ls | grep -q "ls is" && echo "LS_FOUND"'
run_diff_test 69 "type multiple builtins"                    'type cd pwd true'
run_diff_test 70 "type mixed builtins and non-existent"      'type echo bad_cmd_xyz 2>/dev/null; echo "Exit: $?"'

# --- 6. export and unset Builtins (Tests 071 - 085) ---
run_diff_test 71 "export single variable"                    'export MY_VAR=hello; echo "$MY_VAR"'
run_diff_test 72 "export numeric value"                      'export NUM=98765; echo "$NUM"'
run_diff_test 73 "export empty value"                        'export EMPTY=""; echo "Val:[$EMPTY]"'
run_diff_test 74 "export overwrite existing value"           'export V=1; export V=2; echo "$V"'
run_diff_test 75 "export multiple variables on one line"     'export A=alpha B=beta; echo "$A:$B"'
run_diff_test 76 "export with spaces in quoted value"        'export GREET="Hello Zephyr World"; echo "$GREET"'
run_diff_test 77 "export with hyphen in value"               'export FLAG="--opt-flag"; echo "$FLAG"'
run_diff_test 78 "unset single variable"                     'export X=10; unset X; echo "X:[$X]"'
run_diff_test 79 "unset non-existent variable exit code"     'unset NON_EXISTENT_VAR; echo "Exit: $?"'
run_diff_test 80 "unset one of multiple variables"           'export P=1 Q=2; unset P; echo "P:[$P] Q:[$Q]"'
run_diff_test 81 "unset multiple variables on one line"      'export M=10 N=20; unset M N; echo "M:[$M] N:[$N]"'
run_diff_test 82 "export with equal sign in value"           'export EQ_VAR="key=value"; echo "$EQ_VAR"'
run_diff_test 83 "export variable with digits in name"       'export VAR123="numeric_name"; echo "$VAR123"'
run_diff_test 84 "export variable with underscore prefix"    'export _PRIVATE="secret"; echo "$_PRIVATE"'
run_diff_test 85 "export invalid identifier returns error"   'export 1BAD=val 2>/dev/null; echo "Exit: $?"'

# --- 7. shift and Argument Shifting (Tests 086 - 090) ---
run_diff_test 86 "set positional arguments and print"        'set -- one two three; echo "$1 $2 $3"'
run_diff_test 87 "shift default by 1"                        'set -- one two three; shift; echo "$1 $2"'
run_diff_test 88 "shift by 2"                                'set -- one two three four; shift 2; echo "$1 $2"'
run_diff_test 89 "shift all arguments to empty"              'set -- only_one; shift; echo "Arg:[$1]"'
run_diff_test 90 "shift updates argument count $#"           'set -- a b c; echo "$#"; shift; echo "$#"'

# --- 8. read Builtin (Tests 091 - 095) ---
run_diff_test 91 "read multiple words into separate vars"    'echo "apple banana" | { read A B; echo "A:$A B:$B"; }'
run_diff_test 92 "read single line into variable"            'echo "SingleWord" | { read W; echo "W:$W"; }'
run_diff_test 93 "read more words than variables"            'echo "one two three four" | { read X Y; echo "X:$X Y:$Y"; }'
run_diff_test 94 "read empty input line"                     'echo "" | { read E; echo "E:[$E]"; }'
run_diff_test 95 "read with leading whitespace trimmed"      'echo "   padded   " | { read STR; echo "[$STR]"; }'

# --- 9. exit Status Code Propagation (Tests 096 - 100) ---
run_diff_test 96 "exit 0 explicit success"                   'exit 0'
run_diff_test 97 "exit 1 explicit general error"             'exit 1; echo "Should not print"'
run_diff_test 98 "exit 42 custom status"                     'exit 42'
run_diff_test 99 "exit 255 max single byte status"           'exit 255'
run_diff_test 100 "subshell exit status caught by parent"    '( exit 77 ); echo "Caught: $?"'

# --- Print Final Suite Summary ---
print_summary "MODULE 01: CORE BUILTINS"

# Return 0 if all 100 passed, 1 otherwise
[ "$FAIL_TESTS" -eq 0 ]
