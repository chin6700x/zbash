#!/usr/bin/env bash
# ==============================================================================
# ⚡ ZEPHYR ECOSYSTEM: MASTER TRI-PILLAR TEST ORCHESTRATOR (run_trio_test.sh)
# ==============================================================================
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# Description: Master orchestrator delegating to the 3 dedicated runners:
#                1. run_gnu_test.sh   (GNU Bash Official Test Suite)
#                2. run_apple_sh.sh   (Apple Darwin POSIX sh Test Suite)
#                3. run_landmark.sh   (Landmark Enterprise Long-Scripts)
# ==============================================================================

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_trio.sh"

TARGET_MODE="${1:-all}"
shift || true

echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}${WHITE}⚡ ZEPHYR TRI-PILLAR DIFFERENTIAL TEST MATRIX: MASTER ORCHESTRATOR${NC}"
echo -e "   Target Engine  : ${CYAN}${ZBASH_BIN}${NC}"
echo -e "   Golden Baseline: ${YELLOW}${BASH_BIN}${NC}"
echo -e "   Timestamp      : $(date '+%Y-%m-%d %H:%M:%S %Z')"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

case "$TARGET_MODE" in
    gnu)
        "$SCRIPT_DIR/run_gnu_test.sh" "$@"
        ;;

    apple|darwin|sh)
        "$SCRIPT_DIR/run_apple_sh.sh" "$@"
        ;;

    landmark)
        "$SCRIPT_DIR/run_landmark.sh" "$@"
        ;;

    all|"")
        echo -e "${BOLD}โหมดการทำงาน: รันชุดทดสอบครบทั้ง 3 เสาหลัก (GNU -> Apple Darwin -> Landmark)${NC}\n"

        echo -e "${BOLD}${YELLOW}▶ 1/3 EXECUTING GNU BASH OFFICIAL TEST SUITE...${NC}"
        "$SCRIPT_DIR/run_gnu_test.sh" "$@" || true

        echo -e "\n${BOLD}${YELLOW}▶ 2/3 EXECUTING APPLE DARWIN POSIX SH SUITE...${NC}"
        "$SCRIPT_DIR/run_apple_sh.sh" "$@" || true

        echo -e "\n${BOLD}${YELLOW}▶ 3/3 EXECUTING LANDMARK ENTERPRISE SUITE...${NC}"
        "$SCRIPT_DIR/run_landmark.sh" "$@" || true

        echo -e "\n${BOLD}${GREEN}================================================================================${NC}"
        echo -e "🏁 ${BOLD}การรันชุดทดสอบทั้ง 3 เสาหลักเสร็จสิ้นเรียบร้อยแล้ว${NC}"
        echo -e "${BOLD}${GREEN}================================================================================${NC}"
        ;;

    help|--help|-h)
        echo "การใช้งาน: $0 [all | gnu | apple | landmark] [runner_options...]"
        echo ""
        echo "คำสั่งย่อย (Subcommands):"
        echo "  $0 all              # รันครบทั้ง 3 เสาหลัก"
        echo "  $0 gnu [args]       # รันเฉพาะ GNU Bash Official Test Suite"
        echo "  $0 apple [args]     # รันเฉพาะ Apple Darwin POSIX sh Test Suite"
        echo "  $0 landmark [args]  # รันเฉพาะ Landmark Enterprise Suite"
        ;;

    *)
        echo -e "${RED}Error: คำสั่งไม่ถูกต้อง '$TARGET_MODE'${NC}"
        echo "เรียกดูความช่วยเหลือ: $0 --help"
        exit 1
        ;;
esac
