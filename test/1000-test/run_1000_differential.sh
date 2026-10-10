#!/bin/bash
# ==============================================================================
# ZEPHYR 1000-TEST SUITE: MASTER DIFFERENTIAL PARITY ORCHESTRATOR
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Ensure zbash binary is up to date
if [ -f "$SCRIPT_DIR/../../bin/zbash" ]; then
    cp "$SCRIPT_DIR/../../bin/zbash" "$SCRIPT_DIR/zbash"
fi
chmod +x "$SCRIPT_DIR/zbash"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}${CYAN}🚀 ZEPHYR ZBASH: 1,000 DIFFERENTIAL PARITY TEST MASTER SUITE${NC}"
echo -e "   Bare-Metal ARM64 Silicon vs macOS BSD /bin/bash (Bit-for-Bit Exact Proof)"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

MODULES=(
    "test_01_builtins_core.sh:Core Builtins & Basic Commands"
    "test_02_arithmetic_compound.sh:Compound Math & Arithmetic (( ... ))"
    "test_03_variables_expansion.sh:Variable Expansions & Parameter Math"
    "test_04_control_flow_loops.sh:Loops & Control Flow (for, while, until)"
    "test_05_conditionals_test.sh:Conditionals & Test Operators ([ ... ])"
    "test_06_functions_scope.sh:Functions, Local Scope & Call Stack"
    "test_07_pipelines_redirections.sh:Pipelines, Redirections & Here-Strings"
    "test_08_subshells_grouping.sh:Subshells ( ... ) & Command Grouping { ...; }"
    "test_09_arrays_quoting.sh:Arrays & Quoting Mechanics"
    "test_10_edge_cases_stress.sh:Edge Cases, Unicode (Thai), Stress & Recursion"
)

TOTAL_PASS=0
TOTAL_FAIL=0
START_TIME=$(perl -MTime::HiRes=time -e 'printf "%.6f", time')

declare -a MODULE_RESULTS

for ITEM in "${MODULES[@]}"; do
    FILE="${ITEM%%:*}"
    DESC="${ITEM#*:}"

    echo ""
    echo -e "${BOLD}${YELLOW}▶ RUNNING MODULE: $FILE ($DESC)...${NC}"
    
    # Run the module script and extract pass/fail
    OUTPUT=$(bash "$SCRIPT_DIR/$FILE" 2>&1)
    
    # Check if module passed 100%
    if echo "$OUTPUT" | grep -q "100% BIT-EXACT PARITY PASS"; then
        echo -e "  ${GREEN}✔ [PASS] 100 / 100 PASS (100.0%) - Bit-for-bit exact match${NC}"
        TOTAL_PASS=$((TOTAL_PASS + 100))
        MODULE_RESULTS+=("$FILE | $DESC | 100/100 | PASS 🏆")
    else
        FAIL_COUNT=$(echo "$OUTPUT" | grep -c "MISMATCH DETECTED" || true)
        PASS_COUNT=$((100 - FAIL_COUNT))
        echo -e "  ${RED}✘ [FAIL] $PASS_COUNT / 100 PASS (Failed: $FAIL_COUNT)${NC}"
        TOTAL_PASS=$((TOTAL_PASS + PASS_COUNT))
        TOTAL_FAIL=$((TOTAL_FAIL + FAIL_COUNT))
        MODULE_RESULTS+=("$FILE | $DESC | $PASS_COUNT/100 | FAIL ⚠️")
    fi
done

END_TIME=$(perl -MTime::HiRes=time -e 'printf "%.6f", time')
DURATION=$(perl -e "printf \"%.3f\", $END_TIME - $START_TIME")

echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}${CYAN}📊 1,000-TEST MASTER SCOREBOARD SUMMARY${NC}"
echo -e "${BOLD}${CYAN}================================================================================${NC}"
printf "%-35s | %-42s | %-10s | %s\n" "Test Suite File" "Module Description" "Score" "Status"
echo "------------------------------------------------------------------------------------------------------"
for RES in "${MODULE_RESULTS[@]}"; do
    IFS='|' read -r R_FILE R_DESC R_SCORE R_STAT <<< "$RES"
    printf "%-35s | %-42s | %-10s | %s\n" "$(echo "$R_FILE" | xargs)" "$(echo "$R_DESC" | xargs)" "$(echo "$R_SCORE" | xargs)" "$(echo "$R_STAT" | xargs)"
done
echo "------------------------------------------------------------------------------------------------------"
TOTAL_TESTS=$((TOTAL_PASS + TOTAL_FAIL))
PERCENT=$(perl -e "printf \"%.1f\", ($TOTAL_PASS / $TOTAL_TESTS) * 100")
printf "${BOLD}%-35s | %-42s | %-10s | %s${NC}\n" "GRAND TOTAL (10 MODULES)" "Complete Differential Parity Verification" "$TOTAL_PASS / $TOTAL_TESTS" "$PERCENT% PASS"
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "⏱️ Total Execution Time: ${BOLD}${DURATION} seconds${NC}"

if [ "$TOTAL_FAIL" -eq 0 ]; then
    echo ""
    echo -e "${BOLD}${GREEN}🎉 1,000 / 1,000 TESTS PASSED (100.0%)! PERFECT BIT-FOR-BIT DARWIN BASH PARITY ACHIEVED! 🏆💎${NC}"
    echo ""
    exit 0
else
    echo ""
    echo -e "${BOLD}${RED}⚠️ $TOTAL_FAIL TESTS FAILED OUT OF $TOTAL_TESTS!${NC}"
    echo ""
    exit 1
fi
