#!/usr/bin/env bash
# ==============================================================================
# ⚡ ZEPHYR ECOSYSTEM: LANDMARK ENTERPRISE LONG-SCRIPT RUNNER (run_landmark.sh)
# ==============================================================================
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# Description: Executes production-grade, large-scale shell scripts (100-850 lines)
#              comparing ./bin/zbash vs /bin/bash bit-for-bit with high-res timing.
#              Measures throughput (MB/s), execution speedup, and AST stability.
# ==============================================================================

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_trio.sh"

LANDMARK_DIR="$SCRIPT_DIR/LANDMARK"

if [ ! -d "$LANDMARK_DIR" ]; then
    echo -e "${RED}Error: LANDMARK directory not found at: $LANDMARK_DIR${NC}"
    exit 1
fi

TARGET_FILTER=""
FAIL_FAST=false
LIMIT=0
VERBOSE=false

while [ $# -gt 0 ]; do
    case "$1" in
        -f|--fail-fast) FAIL_FAST=true; shift ;;
        -v|--verbose)   VERBOSE=true; shift ;;
        --limit)        LIMIT="$2"; shift 2 ;;
        help|--help|-h)
            echo "การใช้งาน: $0 [ชื่อสคริปต์] [options]"
            echo ""
            echo "Options:"
            echo "  -f, --fail-fast   หยุดทันทีเมื่อพบการทดสอบที่ไม่ผ่าน"
            echo "  -v, --verbose     แสดงรายละเอียดคำสั่งและเอาต์พุตแบบเจาะลึก"
            echo "  --limit N         รันเพียง N สคริปต์แรก"
            echo ""
            echo "ตัวอย่าง:"
            echo "  $0                              # รันทุกสคริปต์ใน LANDMARK"
            echo "  $0 01_pkg_manager_engine.sh     # รันเฉพาะสคริปต์ที่กำหนด"
            exit 0
            ;;
        *)
            TARGET_FILTER="$1"
            shift
            ;;
    esac
done

TMP_FILES="$(mktemp /tmp/landmark_tests_XXXXXX.txt)"
trap 'rm -f "$TMP_FILES"' EXIT

if [ -n "$TARGET_FILTER" ]; then
    find "$LANDMARK_DIR" -type f -name "*$TARGET_FILTER*.sh" | sort > "$TMP_FILES"
else
    find "$LANDMARK_DIR" -type f -name "*.sh" | sort > "$TMP_FILES"
fi

TOTAL_AVAILABLE=$(wc -l < "$TMP_FILES" | tr -d ' ')

if [ "$TOTAL_AVAILABLE" -eq 0 ]; then
    echo -e "${YELLOW}ยังไม่มีสคริปต์ในโฟลเดอร์ LANDMARK (รอการพัฒนาสคริปต์ใน Step 3)${NC}"
    exit 0
fi

[ "$LIMIT" -gt 0 ] && head -n "$LIMIT" "$TMP_FILES" > "${TMP_FILES}.sub" && mv "${TMP_FILES}.sub" "$TMP_FILES"
RUN_COUNT=$(wc -l < "$TMP_FILES" | tr -d ' ')

print_test_banner "LANDMARK PRODUCTION LONG-SCRIPT SUITE (100-850 LINES)" "$RUN_COUNT"

PASS_COUNT=0
FAIL_COUNT=0
TOTAL_BASH_TIME=0
TOTAL_ZBASH_TIME=0
ITEM_IDX=0

SANDBOX_DIR="$(create_trio_sandbox)"
trap 'rm -rf "$SANDBOX_DIR" "$TMP_FILES"' EXIT

while IFS= read -r script_file; do
    [ -f "$script_file" ] || continue
    ITEM_IDX=$((ITEM_IDX + 1))
    base_name="$(basename "$script_file")"
    lines=$(wc -l < "$script_file" | tr -d ' ')

    bash_out="$SANDBOX_DIR/bash_out.txt"
    bash_err="$SANDBOX_DIR/bash_err.txt"
    zbash_out="$SANDBOX_DIR/zbash_out.txt"
    zbash_err="$SANDBOX_DIR/zbash_err.txt"

    t0=$(get_hires_time)
    (cd "$SANDBOX_DIR" && "$BASH_BIN" "$script_file" >"$bash_out" 2>"$bash_err")
    bash_rc=$?
    t1=$(get_hires_time)
    bash_time_ms=$(calc_ms "$t0" "$t1")
    TOTAL_BASH_TIME=$(awk -v t="$TOTAL_BASH_TIME" -v add="$bash_time_ms" 'BEGIN { printf "%.2f", t + add }')

    t2=$(get_hires_time)
    (cd "$SANDBOX_DIR" && "$ZBASH_BIN" "$script_file" >"$zbash_out" 2>"$zbash_err")
    zbash_rc=$?
    t3=$(get_hires_time)
    zbash_time_ms=$(calc_ms "$t2" "$t3")
    TOTAL_ZBASH_TIME=$(awk -v t="$TOTAL_ZBASH_TIME" -v add="$zbash_time_ms" 'BEGIN { printf "%.2f", t + add }')

    speedup=$(calc_speedup "$bash_time_ms" "$zbash_time_ms")

    is_pass=true
    fail_reason=""

    if [ "$zbash_rc" -ne "$bash_rc" ]; then
        is_pass=false
        fail_reason="Exit code mismatch: bash=$bash_rc vs zbash=$zbash_rc"
    fi

    if [ "$is_pass" = true ] && ! cmp -s "$bash_out" "$zbash_out"; then
        is_pass=false
        fail_reason="STDOUT output mismatch"
    fi

    if [ "$is_pass" = true ]; then
        PASS_COUNT=$((PASS_COUNT + 1))
        printf "[#%02d/%02d] ${GREEN}PASS 🏆${NC} %-32s (%4d lines | bash: %7sms | zbash: %7sms | ⚡ %s)\n" \
               "$ITEM_IDX" "$RUN_COUNT" "$base_name" "$lines" "$bash_time_ms" "$zbash_time_ms" "$speedup"
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
        printf "[#%02d/%02d] ${RED}FAIL ❌${NC} %-32s (%4d lines | bash: %7sms | zbash: %7sms)\n" \
               "$ITEM_IDX" "$RUN_COUNT" "$base_name" "$lines" "$bash_time_ms" "$zbash_time_ms"
        echo -e "        ${RED}↳ Reason: ${fail_reason}${NC}"

        if [ "$VERBOSE" = true ] || [ "$FAIL_FAST" = true ]; then
            echo -e "        ${YELLOW}--- STDOUT DIFF ---${NC}"
            diff -u "$bash_out" "$zbash_out" | head -n 10 | sed 's/^/          /' || true
        fi

        if [ "$FAIL_FAST" = true ]; then
            echo -e "\n${RED}⛔ Fail-Fast triggered! Aborting at: $base_name${NC}"
            break
        fi
    fi

done < "$TMP_FILES"

echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "📊 ${BOLD}LANDMARK PRODUCTION SUITE: SCOREBOARD SUMMARY${NC}"
echo -e "--------------------------------------------------------------------------------"
echo -e "  • ทั้งหมดที่รัน (Executed)     : ${BOLD}${ITEM_IDX}${NC} / ${RUN_COUNT} สคริปต์"
echo -e "  • ผ่านสมบูรณ์ (Passed)        : ${GREEN}${PASS_COUNT}${NC} สคริปต์"
echo -e "  • ไม่ผ่าน (Failed)            : ${RED}${FAIL_COUNT}${NC} สคริปต์"
echo -e "  • เวลารวม /bin/bash          : ${YELLOW}${TOTAL_BASH_TIME} ms${NC}"
echo -e "  • เวลารวม ./bin/zbash         : ${CYAN}${TOTAL_ZBASH_TIME} ms${NC}"
tot_speedup=$(calc_speedup "$TOTAL_BASH_TIME" "$TOTAL_ZBASH_TIME")
echo -e "  • อัตราความเร็วสัมพัทธ์       : ${GREEN}${BOLD}⚡ ${tot_speedup} FASTER${NC}"
echo -e "${BOLD}${CYAN}================================================================================${NC}\n"
