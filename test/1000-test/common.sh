#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST MASTER HARNESS (common.sh)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

ZBASH_BIN="/Volumes/Mac Backup/zero-tools/zbash/bin/zbash"
BASH_BIN="/bin/bash"

TOTAL_TESTS=0
PASS_TESTS=0
FAIL_TESTS=0

# Colors
CLR_RESET="\033[0m"
CLR_RED="\033[31m"
CLR_GREEN="\033[32m"
CLR_YELLOW="\033[33m"
CLR_CYAN="\033[36m"
CLR_BOLD="\033[1m"

normalize_stderr() {
    # Normalize program name prefixes: "/bin/bash: line 0: " -> "" and "zbash: line 0: " -> ""
    sed -E 's|^/bin/bash: (line [0-9]+: )?||; s|^bash: (line [0-9]+: )?||; s|^zbash: (line [0-9]+: )?||; s|^\./bin/zbash: (line [0-9]+: )?||'
}

run_diff_test() {
    local test_num="$1"
    local desc="$2"
    local cmd_script="$3"

    if [ -n "$FILTER_TEST" ] && [ "$test_num" -ne "$FILTER_TEST" ]; then
        return 0
    fi

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    # 1. Run Baseline (macOS BSD /bin/bash)
    local bash_stdout bash_stderr bash_exit
    local tmp_bash_out="/tmp/diff_bash_out_$$.tmp"
    local tmp_bash_err="/tmp/diff_bash_err_$$.tmp"

    "$BASH_BIN" -c "$cmd_script" >"$tmp_bash_out" 2>"$tmp_bash_err"
    bash_exit=$?
    bash_stdout=$(cat "$tmp_bash_out")
    bash_stderr=$(normalize_stderr < "$tmp_bash_err")

    # 2. Run Zephyr Engine (zbash)
    local zbash_stdout zbash_stderr zbash_exit
    local tmp_zbash_out="/tmp/diff_zbash_out_$$.tmp"
    local tmp_zbash_err="/tmp/diff_zbash_err_$$.tmp"

    "$ZBASH_BIN" -c "$cmd_script" >"$tmp_zbash_out" 2>"$tmp_zbash_err"
    zbash_exit=$?
    zbash_stdout=$(cat "$tmp_zbash_out")
    zbash_stderr=$(normalize_stderr < "$tmp_zbash_err")

    # Clean up temp files
    rm -f "$tmp_bash_out" "$tmp_bash_err" "$tmp_zbash_out" "$tmp_zbash_err"

    # 3. Compare Parity
    local pass=true
    local reason=""

    if [ "$bash_exit" -ne "$zbash_exit" ]; then
        pass=false
        reason="Exit code mismatch: bash=$bash_exit vs zbash=$zbash_exit"
    elif [ "$bash_stdout" != "$zbash_stdout" ]; then
        pass=false
        reason="Stdout mismatch"
    elif [ "$bash_stderr" != "$zbash_stderr" ]; then
        pass=false
        reason="Stderr mismatch"
    fi

    # 4. Report
    if [ "$pass" = true ]; then
        PASS_TESTS=$((PASS_TESTS + 1))
        printf "────────────────────────────────────────────────────────────────────────────────\n"
        printf "${CLR_BOLD}#%03d ${CLR_GREEN}[PASS]${CLR_RESET} %s\n" "$test_num" "$desc"
        printf "  ${CLR_CYAN}\$ bash  -c${CLR_RESET} '%s'\n" "$cmd_script"
        if [ -n "$bash_stdout" ]; then
            printf "    ↳ ${CLR_BOLD}[stdout]:${CLR_RESET} %s\n" "$bash_stdout"
        else
            printf "    ↳ ${CLR_BOLD}[stdout]:${CLR_RESET} <empty>\n"
        fi
        if [ -n "$bash_stderr" ]; then
            printf "    ↳ ${CLR_YELLOW}[stderr]:${CLR_RESET} %s\n" "$bash_stderr"
        fi
        printf "    ↳ ${CLR_BOLD}[exit]:${CLR_RESET}   %d\n" "$bash_exit"
        printf "  ${CLR_CYAN}\$ zbash -c${CLR_RESET} '%s'\n" "$cmd_script"
        if [ -n "$zbash_stdout" ]; then
            printf "    ↳ ${CLR_BOLD}[stdout]:${CLR_RESET} %s\n" "$zbash_stdout"
        else
            printf "    ↳ ${CLR_BOLD}[stdout]:${CLR_RESET} <empty>\n"
        fi
        if [ -n "$zbash_stderr" ]; then
            printf "    ↳ ${CLR_YELLOW}[stderr]:${CLR_RESET} %s\n" "$zbash_stderr"
        fi
        printf "    ↳ ${CLR_BOLD}[exit]:${CLR_RESET}   %d\n" "$zbash_exit"
        printf "  ${CLR_GREEN}✔ PARITY: EXACT BIT-FOR-BIT MATCH${CLR_RESET}\n"
    else
        FAIL_TESTS=$((FAIL_TESTS + 1))
        printf "────────────────────────────────────────────────────────────────────────────────\n"
        printf "${CLR_BOLD}#%03d ${CLR_RED}[FAIL]${CLR_RESET} %s (${CLR_YELLOW}%s${CLR_RESET})\n" "$test_num" "$desc" "$reason"
        printf "  ${CLR_CYAN}\$ bash  -c${CLR_RESET} '%s'\n" "$cmd_script"
        printf "    ↳ ${CLR_BOLD}[stdout]:${CLR_RESET} '%s'\n" "$bash_stdout"
        printf "    ↳ ${CLR_YELLOW}[stderr]:${CLR_RESET} '%s'\n" "$bash_stderr"
        printf "    ↳ ${CLR_BOLD}[exit]:${CLR_RESET}   %d\n" "$bash_exit"
        printf "  ${CLR_CYAN}\$ zbash -c${CLR_RESET} '%s'\n" "$cmd_script"
        printf "    ↳ ${CLR_BOLD}[stdout]:${CLR_RESET} '%s'\n" "$zbash_stdout"
        printf "    ↳ ${CLR_YELLOW}[stderr]:${CLR_RESET} '%s'\n" "$zbash_stderr"
        printf "    ↳ ${CLR_BOLD}[exit]:${CLR_RESET}   %d\n" "$zbash_exit"
        printf "  ${CLR_RED}✘ PARITY: MISMATCH DETECTED${CLR_RESET}\n"
    fi
}

print_summary() {
    local suite_name="$1"
    echo "--------------------------------------------------------------------------------"
    if [ "$FAIL_TESTS" -eq 0 ]; then
        printf "${CLR_GREEN}${CLR_BOLD}🎉 100%% BIT-EXACT PARITY PASS! [%s] : %d / %d PASS (100.0%%)${CLR_RESET}\n" "$suite_name" "$PASS_TESTS" "$TOTAL_TESTS"
    else
        printf "${CLR_RED}${CLR_BOLD}⚠️ PARITY FAILED! [%s] : %d / %d PASS (Failed: %d)${CLR_RESET}\n" "$suite_name" "$PASS_TESTS" "$TOTAL_TESTS" "$FAIL_TESTS"
    fi
    echo "--------------------------------------------------------------------------------"
}
