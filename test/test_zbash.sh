#!/usr/bin/env bash
#
# X-Series Test Suite: zbash (100 Automated Parity Tests vs macOS bash)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SOURCE_DIR/common.sh"
setup_fixtures

ORIG_BASH="/bin/bash"
START_TIME=$(perl -MTime::HiRes=time -e 'printf "%.6f\n", time')

# 1. Echo and Builtins (20 tests)
echo_tests=(
    "echo hello world"
    "echo 'single quoted string'"
    "echo \"double quoted string\""
    "echo -n 'no newline test'"
    "echo -e 'line1\nline2'"
    "echo -e 'col1\tcol2'"
    "echo 123456789"
    "echo ''"
    "echo '   spaces inside   '"
    "echo 'Thai Unicode: สวัสดีชาวโลก'"
    "pwd"
    "true"
    "false"
    ":"
    "type cd"
    "type pwd"
    "type echo"
    "type exit"
    "echo a b c d e f"
    "echo 'DONE'"
)

for i in "${!echo_tests[@]}"; do
    c="${echo_tests[$i]}"
    assert_parity "zbash builtin $i" \
        "\"$ORIG_BASH\" -c \"$c\"" \
        "\"$BIN_DIR/zbash\" -c \"$c\""
done

# 2. Pipelines (20 tests)
pipe_tests=(
    "echo 'alpha beta gamma' | tr ' ' '\n'"
    "echo 'zebra apple banana' | tr ' ' '\n' | sort"
    "cat '$TMP_BASE/simple_10.txt' | head -n 5"
    "cat '$TMP_BASE/simple_10.txt' | tail -n 4"
    "cat '$TMP_BASE/simple_10.txt' | wc -l"
    "cat '$TMP_BASE/simple_10.txt' | grep apple"
    "cat '$TMP_BASE/medium_100.txt' | grep 'Record 042'"
    "cat '$TMP_BASE/medium_100.txt' | head -n 20 | tail -n 5"
    "cat '$TMP_BASE/medium_100.txt' | wc -l"
    "cat '$TMP_BASE/large_1000.txt' | head -n 10"
    "cat '$TMP_BASE/large_1000.txt' | tail -n 10"
    "cat '$TMP_BASE/large_1000.txt' | wc -l"
    "cat '$TMP_BASE/thai_unicode.txt' | wc -l"
    "cat '$TMP_BASE/empty.txt' | wc -l"
    "cat '$TMP_BASE/single_char.txt' | wc -c"
    "echo 'foo bar' | tr a-z A-Z | tr ' ' '_'"
    "echo '10 20 30' | tr ' ' '\n' | wc -l"
    "cat '$TMP_BASE/no_nl_eof.txt' | wc -w"
    "cat '$TMP_BASE/simple_10.txt' | grep -v apple | wc -l"
    "echo 'pipeline finished' | cat"
)

for i in "${!pipe_tests[@]}"; do
    c="${pipe_tests[$i]}"
    assert_parity "zbash pipeline $i" \
        "\"$ORIG_BASH\" -c \"$c\"" \
        "\"$BIN_DIR/zbash\" -c \"$c\""
done

# 3. File Redirections (20 tests)
for i in {0..19}; do
    out_file="$TMP_BASE/redir_test_${i}.tmp"
    assert_parity "zbash redirection $i" \
        "\"$ORIG_BASH\" -c \"echo 'data_$i' > '$out_file' && echo 'more_$i' >> '$out_file' && cat '$out_file'\"" \
        "\"$BIN_DIR/zbash\" -c \"echo 'data_$i' > '$out_file' && echo 'more_$i' >> '$out_file' && cat '$out_file'\""
done

# 4. Conditionals & Sequence (&&, ||, ;) (20 tests)
cond_tests=(
    "true && echo ok"
    "false && echo ok || echo failed"
    "true || echo skip"
    "false || echo recovered"
    "echo step1 ; echo step2"
    "true && echo a ; echo b"
    "false && echo a ; echo b"
    "echo 1 ; echo 2 ; echo 3"
    "test -f '$TMP_BASE/simple_10.txt' && echo exists"
    "test -f '$TMP_BASE/nonexistent_xyz.txt' || echo missing"
    "test -d '$TMP_BASE' && echo is_dir"
    "test -z '' && echo empty"
    "test -n 'hello' && echo not_empty"
    "test 'abc' = 'abc' && echo equal"
    "test 'abc' != 'xyz' && echo not_equal"
    "test 10 -eq 10 && echo eq"
    "test 10 -lt 20 && echo lt"
    "test 30 -gt 20 && echo gt"
    "true && true && true && echo all_true"
    "true && false || echo fallback"
)

for i in "${!cond_tests[@]}"; do
    c="${cond_tests[$i]}"
    assert_parity "zbash conditional $i" \
        "\"$ORIG_BASH\" -c \"$c\"" \
        "\"$BIN_DIR/zbash\" -c \"$c\""
done

# 5. Variables and Expansion (10 tests)
var_tests=(
    "VAR=Zephyr ; echo \$VAR"
    "A=Hello ; B=World ; echo \$A \$B"
    "NUM=42 ; echo Value is \${NUM}"
    "true ; echo \$?"
    "false ; echo \$?"
    "echo \"Literal with spaces and text\""
    "NAME=Silicon ; echo \"Platform: \$NAME\""
    "X=1 ; Y=2 ; echo \$X ; echo \$Y"
    "MSG=Testing ; echo \$MSG"
    "STATUS=OK ; echo status=\$STATUS"
)

for i in "${!var_tests[@]}"; do
    c="${var_tests[$i]}"
    assert_parity "zbash var expansion $i" \
        "\"$ORIG_BASH\" -c \"$c\"" \
        "\"$BIN_DIR/zbash\" -c \"$c\""
done

# 6. Script Execution and Exit Status (10 tests)
exit_cases=(0 1 2 5 10 42 77 99 125 127)
for i in "${!exit_cases[@]}"; do
    code="${exit_cases[$i]}"
    assert_parity "zbash exit status $code" \
        "\"$ORIG_BASH\" -c 'exit $code'" \
        "\"$BIN_DIR/zbash\" -c 'exit $code'" 0 1
done

END_TIME=$(perl -MTime::HiRes=time -e 'printf "%.6f\n", time')
ELAPSED_MS=$(perl -e "printf('%.1f', ($END_TIME - $START_TIME) * 1000)")

report_tool_results "zbash" 100 "$ELAPSED_MS"
