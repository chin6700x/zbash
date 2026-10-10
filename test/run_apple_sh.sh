#!/usr/bin/env bash
# ==============================================================================
# ⚡ ZEPHYR ECOSYSTEM: APPLE DARWIN POSIX SH DIFFERENTIAL RUNNER (run_apple_sh.sh)
# ==============================================================================
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# Description: Executes Apple Darwin / FreeBSD POSIX sh tests (517 files)
#              comparing ./bin/zbash vs /bin/bash bit-for-bit with high-res timing.
#              Supports category filters, single-test debug, batching, fail-fast.
# ==============================================================================

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_trio.sh"

DARWIN_DIR="$SCRIPT_DIR/DARWIN-SH"

if [ ! -d "$DARWIN_DIR" ]; then
    echo -e "${RED}Error: DARWIN-SH directory not found at: $DARWIN_DIR${NC}"
    exit 1
fi

# Categories in Apple Darwin sh test suite
CATEGORIES=("builtins" "errors" "execution" "expansion" "invocation" "parameters" "parser" "set-e")

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
            echo "การใช้งาน: $0 [หมวดหมู่ | ไฟล์] [options]"
            echo ""
            echo "หมวดหมู่ที่รองรับ:"
            echo "  builtins, errors, execution, expansion, invocation, parameters, parser, set-e"
            echo ""
            echo "Options:"
            echo "  -f, --fail-fast   หยุดทันทีเมื่อพบการทดสอบที่ไม่ผ่าน"
            echo "  -v, --verbose     แสดงรายละเอียดคำสั่งและเอาต์พุตแบบเจาะลึก"
            echo "  --limit N         รันเพียง N ข้อแรก"
            echo "  --batch X/Y       รันเฉพาะส่วนที่ X จากทั้งหมด Y ส่วน (เช่น --batch 1/3)"
            echo ""
            echo "ตัวอย่าง:"
            echo "  $0                  # รันทั้งหมด 517 ข้อ"
            echo "  $0 builtins         # รันเฉพาะหมวด builtins (193 ข้อ)"
            echo "  $0 parser -f        # รันเฉพาะหมวด parser และหยุดทันทีถ้าเจอข้อผิดพลาด"
            echo "  $0 break1.0         # รันเฉพาะข้อ break1.0"
            exit 0
            ;;
        *)
            TARGET_FILTER="$1"
            shift
            ;;
    esac
done

# Collect test files
TMP_FILES="$(mktemp /tmp/apple_sh_tests_XXXXXX)"
trap 'rm -f "$TMP_FILES"' EXIT

if [ -n "$TARGET_FILTER" ]; then
    # Check if target is a category directory
    if [ -d "$DARWIN_DIR/$TARGET_FILTER" ]; then
        find "$DARWIN_DIR/$TARGET_FILTER" -type f ! -name "*.stdout" ! -name "*.stderr" ! -name "Makefile*" ! -name "*.sh" | sort > "$TMP_FILES"
    elif [ -f "$DARWIN_DIR/$TARGET_FILTER" ]; then
        echo "$DARWIN_DIR/$TARGET_FILTER" > "$TMP_FILES"
    else
        # Search by file basename pattern
        find "$DARWIN_DIR" -type f -name "*$TARGET_FILTER*" ! -name "*.stdout" ! -name "*.stderr" ! -name "Makefile*" ! -name "*.sh" | sort > "$TMP_FILES"
    fi
else
    # All tests across 8 categories
    find "$DARWIN_DIR" -type f ! -name "*.stdout" ! -name "*.stderr" ! -name "Makefile*" ! -name "*.sh" | sort > "$TMP_FILES"
fi

TOTAL_AVAILABLE=$(wc -l < "$TMP_FILES" | tr -d ' ')

if [ "$TOTAL_AVAILABLE" -eq 0 ]; then
    echo -e "${RED}ไม่พบข้อสอบที่ตรงกับเงื่อนไข: '$TARGET_FILTER'${NC}"
    exit 1
fi

# Apply batching or limit
TMP_SELECTED="$(mktemp /tmp/apple_sh_selected_XXXXXX)"
trap 'rm -f "$TMP_FILES" "$TMP_SELECTED"' EXIT

if [ "$BATCH_TOT" -gt 0 ] && [ "$BATCH_CUR" -gt 0 ]; then
    awk -v cur="$BATCH_CUR" -v tot="$BATCH_TOT" '
    {
        idx = NR - 1;
        if (int(idx * tot / NR) == (cur - 1) || (NR % tot) == (cur % tot)) print $0;
    }' "$TMP_FILES" > "$TMP_SELECTED"
    # More uniform chunking:
    lines_per_batch=$(( (TOTAL_AVAILABLE + BATCH_TOT - 1) / BATCH_TOT ))
    start_line=$(( (BATCH_CUR - 1) * lines_per_batch + 1 ))
    sed -n "${start_line},$(( start_line + lines_per_batch - 1 ))p" "$TMP_FILES" > "$TMP_SELECTED"
elif [ "$LIMIT" -gt 0 ]; then
    head -n "$LIMIT" "$TMP_FILES" > "$TMP_SELECTED"
else
    cp "$TMP_FILES" "$TMP_SELECTED"
fi

RUN_COUNT=$(wc -l < "$TMP_SELECTED" | tr -d ' ')

print_test_banner "APPLE DARWIN POSIX SH SUITE (517 TESTS)" "$RUN_COUNT"

if [ -n "$TARGET_FILTER" ]; then
    echo -e "   🎯 Filter Selected: ${YELLOW}${TARGET_FILTER}${NC}"
fi
[ "$FAIL_FAST" = true ] && echo -e "   ⚡ Fail-Fast Mode : ${RED}ACTIVE (-f)${NC}"
echo ""

# Scoreboard accumulators
PASS_COUNT=0
FAIL_COUNT=0
TOTAL_BASH_TIME=0
TOTAL_ZBASH_TIME=0

ITEM_IDX=0

# Create sandbox for running tests safely
SANDBOX_DIR="$(create_trio_sandbox)"
trap 'rm -rf "$SANDBOX_DIR" "$TMP_FILES" "$TMP_SELECTED"' EXIT

while IFS= read -r test_file; do
    [ -f "$test_file" ] || continue
    ITEM_IDX=$((ITEM_IDX + 1))

    rel_name="${test_file#$DARWIN_DIR/}"
    base_name="$(basename "$test_file")"
    file_dir="$(dirname "$test_file")"

    # Expected exit code from file suffix (e.g., .0, .2, .4)
    expected_rc=0
    suffix="${base_name##*.}"
    if [[ "$suffix" =~ ^[0-9]+$ ]]; then
        expected_rc="$suffix"
    fi

    out_file="${test_file}.stdout"
    err_file="${test_file}.stderr"

    # Execution temporary logs
    bash_out="$SANDBOX_DIR/bash_out.txt"
    bash_err="$SANDBOX_DIR/bash_err.txt"
    zbash_out="$SANDBOX_DIR/zbash_out.txt"
    zbash_err="$SANDBOX_DIR/zbash_err.txt"

    # Run with macOS /bin/bash (Golden Baseline)
    t0=$(get_hires_time)
    (cd "$file_dir" && run_with_timeout 5 "$BASH_BIN" "$test_file" >"$bash_out" 2>"$bash_err")
    bash_rc=$?
    t1=$(get_hires_time)
    bash_time_ms=$(calc_ms "$t0" "$t1")
    TOTAL_BASH_TIME=$(awk -v t="$TOTAL_BASH_TIME" -v add="$bash_time_ms" 'BEGIN { printf "%.2f", t + add }')

    # Run with ./bin/zbash (Zephyr Engine)
    t2=$(get_hires_time)
    (cd "$file_dir" && run_with_timeout 5 "$ZBASH_BIN" "$test_file" >"$zbash_out" 2>"$zbash_err")
    zbash_rc=$?
    t3=$(get_hires_time)
    zbash_time_ms=$(calc_ms "$t2" "$t3")
    TOTAL_ZBASH_TIME=$(awk -v t="$TOTAL_ZBASH_TIME" -v add="$zbash_time_ms" 'BEGIN { printf "%.2f", t + add }')

    speedup=$(calc_speedup "$bash_time_ms" "$zbash_time_ms")

    # Differential Verification Logic
    is_pass=true
    fail_reason=""

    # 1. Compare Exit Code against bash
    if [ "$zbash_rc" -ne "$bash_rc" ]; then
        is_pass=false
        fail_reason="Exit code mismatch: bash=$bash_rc vs zbash=$zbash_rc (Expected: $expected_rc)"
    fi

    # 2. Compare STDOUT (normalize whitespace or exact cmp)
    if [ "$is_pass" = true ]; then
        if ! cmp -s "$bash_out" "$zbash_out"; then
            is_pass=false
            fail_reason="STDOUT mismatch with /bin/bash"
        fi
    fi

    # 3. Check against expected .stdout file if present
    if [ "$is_pass" = true ] && [ -f "$out_file" ]; then
        if ! cmp -s "$out_file" "$zbash_out"; then
            is_pass=false
            fail_reason="STDOUT mismatch with golden ${base_name}.stdout"
        fi
    fi

    # Display result card
    if [ "$is_pass" = true ]; then
        PASS_COUNT=$((PASS_COUNT + 1))
        printf "[#%03d/%03d] ${GREEN}PASS 🏆${NC} %-35s (bash: %6sms | zbash: %6sms | ⚡ %s)\n" \
               "$ITEM_IDX" "$RUN_COUNT" "$rel_name" "$bash_time_ms" "$zbash_time_ms" "$speedup"
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
        printf "[#%03d/%03d] ${RED}FAIL ❌${NC} %-35s (bash: %6sms | zbash: %6sms)\n" \
               "$ITEM_IDX" "$RUN_COUNT" "$rel_name" "$bash_time_ms" "$zbash_time_ms"
        echo -e "         ${RED}↳ Reason: ${fail_reason}${NC}"

        if [ "$VERBOSE" = true ] || [ "$FAIL_FAST" = true ]; then
            echo -e "         ${YELLOW}--- STDOUT DIFF (bash vs zbash) ---${NC}"
            diff -u "$bash_out" "$zbash_out" | head -n 10 | sed 's/^/           /' || true
            echo -e "         ${YELLOW}--- STDERR (zbash) ---${NC}"
            head -n 5 "$zbash_err" | sed 's/^/           /' || true
        fi

        if [ "$FAIL_FAST" = true ]; then
            echo -e "\n${RED}⛔ Fail-Fast triggered! Aborting at test: $rel_name${NC}"
            break
        fi
    fi

done < "$TMP_SELECTED"

# Summary Scoreboard
echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "📊 ${BOLD}APPLE DARWIN POSIX SH: SCOREBOARD SUMMARY${NC}"
echo -e "--------------------------------------------------------------------------------"
echo -e "  • ทั้งหมดที่รัน (Executed)     : ${BOLD}${ITEM_IDX}${NC} / ${RUN_COUNT} ข้อ"
echo -e "  • ผ่านสมบูรณ์ (Passed)        : ${GREEN}${PASS_COUNT}${NC} ข้อ"
echo -e "  • ไม่ผ่าน (Failed)            : ${RED}${FAIL_COUNT}${NC} ข้อ"
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
    echo -e "🎉 ${GREEN}${BOLD}ALL TESTS PASSED 100% BIT-FOR-BIT DARWIN POSIX SH PARITY! 💎${NC}\n"
    exit 0
else
    echo -e "⚠️  ${RED}${BOLD}SOME TESTS FAILED - INSPECT LOGS ABOVE${NC}\n"
    exit 1
fi
