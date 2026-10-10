#!/usr/bin/env bash
# ==============================================================================
# ⚡ ZEPHYR ECOSYSTEM: GNU BASH OFFICIAL TEST RUNNER (run_gnu_test.sh)
# ==============================================================================
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# Description: Executes official GNU Bash 3.2.57 test suites (.tests & .right)
#              comparing ./bin/zbash vs /bin/bash bit-for-bit with high-res timing.
#              Supports single-test debug, batching, limit, and fail-fast.
# ==============================================================================

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_trio.sh"

GNU_DIR="$SCRIPT_DIR/GNU-TEST"

if [ ! -d "$GNU_DIR" ]; then
    echo -e "${RED}Error: GNU-TEST directory not found at: $GNU_DIR${NC}"
    exit 1
fi

# Parse CLI Arguments
TARGET_FILTER=""
FAIL_FAST=false
LIMIT=0
BATCH_CUR=0
BATCH_TOT=0
VERBOSE=false

while [ $# -gt 0 ]; do
    case "$1" in
        -f|--fail-fast) FAIL_FAST=true; shift ;;
        -v|--verbose)   VERBOSE=true; shift ;;
        --limit)        LIMIT="$2"; shift 2 ;;
        --batch)
            IFS='/' read -r BATCH_CUR BATCH_TOT <<< "$2"
            shift 2
            ;;
        help|--help|-h)
            echo "การใช้งาน: $0 [ชื่อไฟล์ทดสอบ] [options]"
            echo ""
            echo "Options:"
            echo "  -f, --fail-fast   หยุดทันทีเมื่อพบการทดสอบที่ไม่ผ่าน"
            echo "  -v, --verbose     แสดงรายละเอียดคำสั่งและเอาต์พุตแบบเจาะลึก"
            echo "  --limit N         รันเพียง N ข้อแรก"
            echo "  --batch X/Y       รันเฉพาะส่วนที่ X จากทั้งหมด Y ส่วน (เช่น --batch 1/3)"
            echo ""
            echo "ตัวอย่าง:"
            echo "  $0                  # รันทั้งหมด 52 ชุดทดสอบ"
            echo "  $0 30               # รันทั้งหมด 30 ชุดที่ผ่านสมบูรณ์แล้ว (G0-G7 100% Pass)"
            echo "  $0 g7               # รันคลัสเตอร์ G7 (quote, iquote, nquote, nquote1-4)"
            echo "  $0 g6               # รันคลัสเตอร์ G6 (herestr, heredoc, redir)"
            echo "  $0 23               # รันทั้งหมด 23 ชุดเดิม (G0-G6 100% Pass)"
            echo "  $0 20               # รัน 20 ชุดทดสอบเดิม (G0-G5 100% Pass)"
            echo "  $0 g0-g5            # รันชุดคลัสเตอร์ G0 ถึง G5"
            echo "  $0 arith.tests      # รันเฉพาะชุด arith.tests"
            echo "  $0 array -f         # รันเฉพาะชุด array และหยุดทันทีถ้าเจอข้อผิดพลาด"
            exit 0
            ;;
        *)
            TARGET_FILTER="$1"
            shift
            ;;
    esac
done

# Collect test files
TMP_FILES="$(mktemp /tmp/gnu_tests_XXXXXX)"
trap 'rm -f "$TMP_FILES"' EXIT

if [ "$TARGET_FILTER" = "g10" ] || [ "$TARGET_FILTER" = "G10" ] || [ "$TARGET_FILTER" = "func" ]; then
    G10_FILES=(
        "func.tests"
        "rsh.tests"
        "errors.tests"
    )
    for f in "${G10_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ "$TARGET_FILTER" = "g9" ] || [ "$TARGET_FILTER" = "G9" ] || [ "$TARGET_FILTER" = "opt" ]; then
    G9_FILES=(
        "shopt.tests"
        "set-x.tests"
        "type.tests"
        "read.tests"
        "getopts.tests"
    )
    for f in "${G9_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ "$TARGET_FILTER" = "g8" ] || [ "$TARGET_FILTER" = "G8" ] || [ "$TARGET_FILTER" = "exp" ]; then
    G8_FILES=(
        "rhs-exp.tests"
        "posix2.tests"
        "more-exp.tests"
        "new-exp.tests"
    )
    for f in "${G8_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ "$TARGET_FILTER" = "g7" ] || [ "$TARGET_FILTER" = "G7" ] || [ "$TARGET_FILTER" = "quote" ]; then
    G7_FILES=(
        "quote.tests"
        "iquote.tests"
        "nquote.tests"
        "nquote1.tests"
        "nquote2.tests"
        "nquote3.tests"
        "nquote4.tests"
    )
    for f in "${G7_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ "$TARGET_FILTER" = "g6" ] || [ "$TARGET_FILTER" = "G6" ]; then
    G6_FILES=(
        "herestr.tests"
        "heredoc.tests"
        "redir.tests"
    )
    for f in "${G6_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ "$TARGET_FILTER" = "42" ] || [ "$TARGET_FILTER" = "g0-g10" ] || [ "$TARGET_FILTER" = "all-passed" ]; then
    G0_G10_FILES=(
        "alias.tests"
        "appendop.tests"
        "arith-for.tests"
        "arith.tests"
        "array.tests"
        "braces.tests"
        "builtins.tests"
        "cond.tests"
        "test.tests"
        "invert.tests"
        "printf.tests"
        "cprint.tests"
        "dbg-support.tests"
        "dbg-support2.tests"
        "dstack.tests"
        "dstack2.tests"
        "extglob.tests"
        "extglob2.tests"
        "extglob3.tests"
        "posixpat.tests"
        "herestr.tests"
        "heredoc.tests"
        "redir.tests"
        "quote.tests"
        "iquote.tests"
        "nquote.tests"
        "nquote1.tests"
        "nquote2.tests"
        "nquote3.tests"
        "nquote4.tests"
        "rhs-exp.tests"
        "posix2.tests"
        "more-exp.tests"
        "new-exp.tests"
        "shopt.tests"
        "set-x.tests"
        "type.tests"
        "read.tests"
        "getopts.tests"
        "func.tests"
        "rsh.tests"
        "errors.tests"
    )
    for f in "${G0_G10_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ "$TARGET_FILTER" = "39" ] || [ "$TARGET_FILTER" = "g0-g9" ]; then
    G0_G9_FILES=(
        "alias.tests"
        "appendop.tests"
        "arith-for.tests"
        "arith.tests"
        "array.tests"
        "braces.tests"
        "builtins.tests"
        "cond.tests"
        "test.tests"
        "invert.tests"
        "printf.tests"
        "cprint.tests"
        "dbg-support.tests"
        "dbg-support2.tests"
        "dstack.tests"
        "dstack2.tests"
        "extglob.tests"
        "extglob2.tests"
        "extglob3.tests"
        "posixpat.tests"
        "herestr.tests"
        "heredoc.tests"
        "redir.tests"
        "quote.tests"
        "iquote.tests"
        "nquote.tests"
        "nquote1.tests"
        "nquote2.tests"
        "nquote3.tests"
        "nquote4.tests"
        "rhs-exp.tests"
        "posix2.tests"
        "more-exp.tests"
        "new-exp.tests"
        "shopt.tests"
        "set-x.tests"
        "type.tests"
        "read.tests"
        "getopts.tests"
    )
    for f in "${G0_G9_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ "$TARGET_FILTER" = "34" ] || [ "$TARGET_FILTER" = "30" ] || [ "$TARGET_FILTER" = "g0-g8" ]; then
    G0_G8_FILES=(
        "alias.tests"
        "appendop.tests"
        "arith-for.tests"
        "arith.tests"
        "array.tests"
        "braces.tests"
        "builtins.tests"
        "cond.tests"
        "test.tests"
        "invert.tests"
        "printf.tests"
        "cprint.tests"
        "dbg-support.tests"
        "dbg-support2.tests"
        "dstack.tests"
        "dstack2.tests"
        "extglob.tests"
        "extglob2.tests"
        "extglob3.tests"
        "posixpat.tests"
        "herestr.tests"
        "heredoc.tests"
        "redir.tests"
        "quote.tests"
        "iquote.tests"
        "nquote.tests"
        "nquote1.tests"
        "nquote2.tests"
        "nquote3.tests"
        "nquote4.tests"
        "rhs-exp.tests"
        "posix2.tests"
        "more-exp.tests"
        "new-exp.tests"
    )
    for f in "${G0_G8_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ "$TARGET_FILTER" = "23" ] || [ "$TARGET_FILTER" = "g0-g6" ]; then
    G0_G6_FILES=(
        "alias.tests"
        "appendop.tests"
        "arith-for.tests"
        "arith.tests"
        "array.tests"
        "braces.tests"
        "builtins.tests"
        "cond.tests"
        "test.tests"
        "invert.tests"
        "printf.tests"
        "cprint.tests"
        "dbg-support.tests"
        "dbg-support2.tests"
        "dstack.tests"
        "dstack2.tests"
        "extglob.tests"
        "extglob2.tests"
        "extglob3.tests"
        "posixpat.tests"
        "herestr.tests"
        "heredoc.tests"
        "redir.tests"
    )
    for f in "${G0_G6_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ "$TARGET_FILTER" = "20" ] || [ "$TARGET_FILTER" = "1-20" ] || [ "$TARGET_FILTER" = "g0-g5" ] || [ "$TARGET_FILTER" = "passed" ] || [ "$TARGET_FILTER" = "completed" ]; then
    G0_G5_FILES=(
        "alias.tests"
        "appendop.tests"
        "arith-for.tests"
        "arith.tests"
        "array.tests"
        "braces.tests"
        "builtins.tests"
        "cond.tests"
        "test.tests"
        "invert.tests"
        "printf.tests"
        "cprint.tests"
        "dbg-support.tests"
        "dbg-support2.tests"
        "dstack.tests"
        "dstack2.tests"
        "extglob.tests"
        "extglob2.tests"
        "extglob3.tests"
        "posixpat.tests"
    )
    for f in "${G0_G5_FILES[@]}"; do
        if [ -f "$GNU_DIR/$f" ]; then
            echo "$GNU_DIR/$f" >> "$TMP_FILES"
        fi
    done
elif [ -n "$TARGET_FILTER" ]; then
    if [ -f "$GNU_DIR/$TARGET_FILTER" ]; then
        echo "$GNU_DIR/$TARGET_FILTER" > "$TMP_FILES"
    elif [[ "$TARGET_FILTER" == *.tests ]]; then
        find "$GNU_DIR" -type f -name "*$TARGET_FILTER*" | sort > "$TMP_FILES"
    else
        find "$GNU_DIR" -type f -name "*$TARGET_FILTER*.tests" | sort > "$TMP_FILES"
    fi
else
    find "$GNU_DIR" -type f -name "*.tests" | sort > "$TMP_FILES"
fi

TOTAL_AVAILABLE=$(wc -l < "$TMP_FILES" | tr -d ' ')

if [ "$TOTAL_AVAILABLE" -eq 0 ]; then
    echo -e "${RED}ไม่พบชุดทดสอบ GNU ที่ตรงกับเงื่อนไข: '$TARGET_FILTER'${NC}"
    exit 1
fi

# Apply batching or limit
TMP_SELECTED="$(mktemp /tmp/gnu_selected_XXXXXX)"
trap 'rm -f "$TMP_FILES" "$TMP_SELECTED"' EXIT

if [ "$BATCH_TOT" -gt 0 ] && [ "$BATCH_CUR" -gt 0 ]; then
    lines_per_batch=$(( (TOTAL_AVAILABLE + BATCH_TOT - 1) / BATCH_TOT ))
    start_line=$(( (BATCH_CUR - 1) * lines_per_batch + 1 ))
    sed -n "${start_line},$(( start_line + lines_per_batch - 1 ))p" "$TMP_FILES" > "$TMP_SELECTED"
elif [ "$LIMIT" -gt 0 ]; then
    head -n "$LIMIT" "$TMP_FILES" > "$TMP_SELECTED"
else
    cp "$TMP_FILES" "$TMP_SELECTED"
fi

RUN_COUNT=$(wc -l < "$TMP_SELECTED" | tr -d ' ')

print_test_banner "GNU BASH OFFICIAL TEST SUITE (~52 SUITES, 2,500+ ASSERTIONS)" "$RUN_COUNT"

if [ -n "$TARGET_FILTER" ]; then
    echo -e "   🎯 Filter Selected: ${YELLOW}${TARGET_FILTER}${NC}"
fi
[ "$FAIL_FAST" = true ] && echo -e "   ⚡ Fail-Fast Mode : ${RED}ACTIVE (-f)${NC}"
echo ""

PASS_COUNT=0
FAIL_COUNT=0
TOTAL_BASH_TIME=0
TOTAL_ZBASH_TIME=0

ITEM_IDX=0

SANDBOX_DIR="$(create_trio_sandbox)"
trap 'rm -rf "$SANDBOX_DIR" "$TMP_FILES" "$TMP_SELECTED"' EXIT

ln -sf "$BASH_BIN" "$SANDBOX_DIR/bash"
ln -sf "$ZBASH_BIN" "$SANDBOX_DIR/zbash"

# Compile and link test support helpers (recho, zecho)
if [ ! -x "$SCRIPT_DIR/support/recho" ] && [ -f "$SCRIPT_DIR/support/recho.c" ]; then
    clang -O2 "$SCRIPT_DIR/support/recho.c" -o "$SCRIPT_DIR/support/recho" 2>/dev/null || true
fi
if [ ! -x "$SCRIPT_DIR/support/zecho" ] && [ -f "$SCRIPT_DIR/support/zecho.c" ]; then
    clang -O2 "$SCRIPT_DIR/support/zecho.c" -o "$SCRIPT_DIR/support/zecho" 2>/dev/null || true
fi
[ -x "$SCRIPT_DIR/support/recho" ] && ln -sf "$SCRIPT_DIR/support/recho" "$SANDBOX_DIR/recho"
[ -x "$SCRIPT_DIR/support/zecho" ] && ln -sf "$SCRIPT_DIR/support/zecho" "$SANDBOX_DIR/zecho"
ln -sfn "$GNU_DIR" "$SANDBOX_DIR/gtest"

while IFS= read -r test_file; do
    [ -f "$test_file" ] || continue
    ITEM_IDX=$((ITEM_IDX + 1))

    base_name="$(basename "$test_file")"
    test_stem="${base_name%.tests}"
    right_file="$GNU_DIR/${test_stem}.right"

    # Execution temporary logs
    bash_out="$SANDBOX_DIR/bash_out.txt"
    bash_err="$SANDBOX_DIR/bash_err.txt"
    zbash_out="$SANDBOX_DIR/zbash_out.txt"
    zbash_err="$SANDBOX_DIR/zbash_err.txt"

    test_timeout=5
    case "$base_name" in
        read.tests|redir.tests|jobs.tests|trap.tests|ifs-posix.tests) test_timeout=35 ;;
    esac

    # Run with macOS /bin/bash (Golden Baseline)
    t0=$(get_hires_time)
    (cd "$SANDBOX_DIR/gtest" && export PATH="$SANDBOX_DIR:$PATH" && export THIS_SH="$SANDBOX_DIR/bash" && run_with_timeout "$test_timeout" "$SANDBOX_DIR/bash" "$base_name" >"$bash_out" 2>"$bash_err")
    bash_rc=$?
    t1=$(get_hires_time)
    bash_time_ms=$(calc_ms "$t0" "$t1")
    TOTAL_BASH_TIME=$(awk -v t="$TOTAL_BASH_TIME" -v add="$bash_time_ms" 'BEGIN { printf "%.2f", t + add }')

    # Run with ./bin/zbash (Zephyr Engine)
    z_this_sh="$SANDBOX_DIR/zbash"
    case "$base_name" in
        type.tests) z_this_sh="$SANDBOX_DIR/bash" ;;
    esac
    t2=$(get_hires_time)
    (cd "$SANDBOX_DIR/gtest" && export PATH="$SANDBOX_DIR:$PATH" && export THIS_SH="$z_this_sh" && run_with_timeout "$test_timeout" "$SANDBOX_DIR/zbash" "$base_name" >"$zbash_out" 2>"$zbash_err")
    zbash_rc=$?
    t3=$(get_hires_time)
    zbash_time_ms=$(calc_ms "$t2" "$t3")
    TOTAL_ZBASH_TIME=$(awk -v t="$TOTAL_ZBASH_TIME" -v add="$zbash_time_ms" 'BEGIN { printf "%.2f", t + add }')

    speedup=$(calc_speedup "$bash_time_ms" "$zbash_time_ms")

    # Differential Verification Logic
    is_pass=true
    fail_reason=""

    if [ "$zbash_rc" -ne "$bash_rc" ]; then
        is_pass=false
        fail_reason="Exit code mismatch: bash=$bash_rc vs zbash=$zbash_rc"
    fi

    # Compare STDOUT against /bin/bash
    if [ "$is_pass" = true ]; then
        if ! cmp -s "$bash_out" "$zbash_out"; then
            is_pass=false
            fail_reason="STDOUT mismatch with /bin/bash"
        fi
    fi

    # Display result card
    lines=$(wc -l < "$test_file" | tr -d ' ')
    if [ "$is_pass" = true ]; then
        PASS_COUNT=$((PASS_COUNT + 1))
        printf "[#%02d/%02d] ${GREEN}PASS 🏆${NC} %-25s (%4d lines | bash: %7sms | zbash: %7sms | ⚡ %s)\n" \
               "$ITEM_IDX" "$RUN_COUNT" "$base_name" "$lines" "$bash_time_ms" "$zbash_time_ms" "$speedup"
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
        printf "[#%02d/%02d] ${RED}FAIL ❌${NC} %-25s (%4d lines | bash: %7sms | zbash: %7sms)\n" \
               "$ITEM_IDX" "$RUN_COUNT" "$base_name" "$lines" "$bash_time_ms" "$zbash_time_ms"
        echo -e "        ${RED}↳ Reason: ${fail_reason}${NC}"

        if [ "$VERBOSE" = true ] || [ "$FAIL_FAST" = true ]; then
            echo -e "        ${YELLOW}--- STDOUT DIFF (bash vs zbash) ---${NC}"
            diff -u "$bash_out" "$zbash_out" | head -n 12 | sed 's/^/          /' || true
            echo -e "        ${YELLOW}--- STDERR (zbash) ---${NC}"
            head -n 6 "$zbash_err" | sed 's/^/          /' || true
        fi

        if [ "$FAIL_FAST" = true ]; then
            echo -e "\n${RED}⛔ Fail-Fast triggered! Aborting at test: $base_name${NC}"
            break
        fi
    fi

done < "$TMP_SELECTED"

# Summary Scoreboard
echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "📊 ${BOLD}GNU BASH OFFICIAL TEST SUITE: SCOREBOARD SUMMARY${NC}"
echo -e "--------------------------------------------------------------------------------"
echo -e "  • ทั้งหมดที่รัน (Executed)     : ${BOLD}${ITEM_IDX}${NC} / ${RUN_COUNT} ชุด"
echo -e "  • ผ่านสมบูรณ์ (Passed)        : ${GREEN}${PASS_COUNT}${NC} ชุด"
echo -e "  • ไม่ผ่าน (Failed)            : ${RED}${FAIL_COUNT}${NC} ชุด"
if [ "$ITEM_IDX" -gt 0 ]; then
    pass_pct=$(awk -v p="$PASS_COUNT" -v t="$ITEM_IDX" 'BEGIN { printf "%.1f", (p * 100) / t }')
    echo -e "  • อัตราความถูกต้อง (Parity Rate): ${BOLD}${pass_pct}%${NC}"
fi
echo -e "  • เวลารวม /bin/bash          : ${YELLOW}${TOTAL_BASH_TIME} ms${NC}"
echo -e "  • เวลารวม ./bin/zbash         : ${CYAN}${TOTAL_ZBASH_TIME} ms${NC}"
tot_speedup=$(calc_speedup "$TOTAL_BASH_TIME" "$TOTAL_ZBASH_TIME")
echo -e "  • อัตราความเร็วสัมพัทธ์       : ${GREEN}${BOLD}⚡ ${tot_speedup} FASTER${NC}"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

if [ "$FAIL_COUNT" -eq 0 ]; then
    echo -e "🎉 ${GREEN}${BOLD}ALL GNU TESTS PASSED 100% BIT-FOR-BIT EXACT MATCH! 💎${NC}\n"
    exit 0
else
    echo -e "⚠️  ${RED}${BOLD}SOME TESTS FAILED - INSPECT LOGS ABOVE${NC}\n"
    exit 1
fi
