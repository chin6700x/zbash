#!/usr/bin/env bash
#
# X-Series Test Suite: Common Test Harness
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# Clean ANSI Formatting without Emojis for Universal Terminal/OS Parity
#

export LC_ALL=C
export PATH="/usr/bin:/bin:$PATH"

# Clean ANSI Colors
C_RESET="\033[0m"
C_BOLD="\033[1m"
C_DIM="\033[2m"
C_GREEN="\033[38;2;46;213;115m"
C_RED="\033[38;2;255;71;87m"
C_BLUE="\033[38;2;77;142;255m"
C_GRAY="\033[38;2;140;150;170m"
C_AMBER="\033[38;2;255;165;2m"

# Locate Binary Directory (zephyr-x-series/bin)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN_DIR="$REPO_DIR/bin"

TMP_BASE="/tmp/x_suite_100"
TMP_OUT_O="/tmp/x_test_out_o.tmp"
TMP_OUT_X="/tmp/x_test_out_x.tmp"
TMP_ERR_O="/tmp/x_test_err_o.tmp"
TMP_ERR_X="/tmp/x_test_err_x.tmp"

PASS_COUNT=0
FAIL_COUNT=0
TOTAL_COUNT=0

setup_fixtures() {
    rm -rf "$TMP_BASE"
    mkdir -p "$TMP_BASE"

    # Basic files
    : > "$TMP_BASE/empty.txt"
    printf "a" > "$TMP_BASE/single_char.txt"
    printf "\n" > "$TMP_BASE/single_nl.txt"
    printf "alpha beta gamma" > "$TMP_BASE/no_nl_eof.txt"

    # simple_10.txt
    for i in {1..10}; do
        echo "Line $i apple banana cherry date" >> "$TMP_BASE/simple_10.txt"
    done

    # medium_100.txt
    for i in {1..100}; do
        printf "Record %03d: Apple Silicon ARM64 SIMD hardware acceleration core #%d\n" "$i" "$((i % 8))" >> "$TMP_BASE/medium_100.txt"
    done

    # large_1000.txt
    for i in {1..1000}; do
        printf "Entry %04d system kernel stream data value=%d status=OK\n" "$i" "$((i * 7))" >> "$TMP_BASE/large_1000.txt"
    done

    # long_line.txt
    printf "HEADER " > "$TMP_BASE/long_line.txt"
    for i in {1..2000}; do
        printf "word " >> "$TMP_BASE/long_line.txt"
    done
    printf "TAIL\n" >> "$TMP_BASE/long_line.txt"

    # thai_unicode.txt
    cat << 'EOF' > "$TMP_BASE/thai_unicode.txt"
สวัสดีชาวโลก ทดสอบระบบ X-Series Darwin 2026
การประมวลผลข้อความภาษาไทยและเวกเตอร์ SIMD
ความเร็วสูงระดับฮาร์ดแวร์โดยตรง
EOF

    # spaces in name.txt
    printf "file with spaces inside filename\nline 2 with data\n" > "$TMP_BASE/spaces in name.txt"

    # binary.bin
    perl -e 'print(pack("C*", 0x00, 0x1F, 0x20, 0x7F, 0x80, 0xFF, 0x0A, 0x00, 0x42, 0x69, 0x6E) x 256)' > "$TMP_BASE/binary.bin"

    # Nested tree
    local curr="$TMP_BASE/tree"
    for lvl in {1..5}; do
        curr="$curr/level_$lvl"
        mkdir -p "$curr"
        echo "data in level $lvl" > "$curr/file_$lvl.txt"
        echo "int func_$lvl() { return $lvl; }" > "$curr/code_$lvl.c"
    done

    # flat_files
    mkdir -p "$TMP_BASE/flat_files"
    for i in {1..50}; do
        printf "log item %d\n" "$i" > "$TMP_BASE/flat_files/item_$(printf "%03d" "$i").log"
    done

    # hidden_dir
    mkdir -p "$TMP_BASE/hidden_dir"
    echo "key=value" > "$TMP_BASE/hidden_dir/.config"
    echo "ENV=prod" > "$TMP_BASE/hidden_dir/.env"
    echo "visible" > "$TMP_BASE/hidden_dir/visible.txt"
}

# assert_parity <desc> <cmd_orig> <cmd_x> [normalize_ws] [check_exit_only]
assert_parity() {
    local desc="$1"
    local cmd_o="$2"
    local cmd_x="$3"
    local norm_ws="${4:-0}"
    local exit_only="${5:-0}"

    ((TOTAL_COUNT++))

    eval "$cmd_o" > "$TMP_OUT_O" 2> "$TMP_ERR_O"
    local code_o=$?

    eval "$cmd_x" > "$TMP_OUT_X" 2> "$TMP_ERR_X"
    local code_x=$?

    local ok=1
    if [ "$exit_only" -eq 1 ]; then
        if [ "$code_o" -ne "$code_x" ]; then
            ok=0
        fi
    elif [ "$norm_ws" -eq 1 ]; then
        # Compare whitespace-normalized output and exit codes
        local clean_o clean_x
        clean_o=$(tr -s '[:space:]' ' ' < "$TMP_OUT_O" | sed 's/^ //;s/ $//')
        clean_x=$(tr -s '[:space:]' ' ' < "$TMP_OUT_X" | sed 's/^ //;s/ $//')
        if [ "$clean_o" != "$clean_x" ] || [ "$code_o" -ne "$code_x" ]; then
            ok=0
        fi
    else
        # Exact byte match and exit code
        if ! cmp -s "$TMP_OUT_O" "$TMP_OUT_X" || [ "$code_o" -ne "$code_x" ]; then
            ok=0
        fi
    fi

    if [ "$ok" -eq 1 ]; then
        ((PASS_COUNT++))
    else
        ((FAIL_COUNT++))
        printf "  ${C_RED}[FAIL] Test %d FAILED:${C_RESET} %s\n" "$TOTAL_COUNT" "$desc"
        printf "     BSD cmd   (%d): %s\n" "$code_o" "$cmd_o"
        printf "     X-Series  (%d): %s\n" "$code_x" "$cmd_x"
        if [ "$exit_only" -eq 0 ]; then
            printf "     Diff:\n"
            diff -u "$TMP_OUT_O" "$TMP_OUT_X" | head -n 10
        fi
    fi
}

report_tool_results() {
    local tool_name="$1"
    local target_tests="$2"
    local elapsed_ms="$3"

    if [ "$FAIL_COUNT" -eq 0 ] && [ "$PASS_COUNT" -ge "$target_tests" ]; then
        printf "  Testing %-8s ... [ ${C_GREEN}%d / %d PASS (100.0%%)${C_RESET} ] (%s ms)\n" "$tool_name" "$PASS_COUNT" "$target_tests" "$elapsed_ms"
        return 0
    else
        printf "  Testing %-8s ... [ ${C_RED}%d / %d PASS, %d FAIL${C_RESET} ] (%s ms)\n" "$tool_name" "$PASS_COUNT" "$target_tests" "$FAIL_COUNT" "$elapsed_ms"
        return 1
    fi
}
