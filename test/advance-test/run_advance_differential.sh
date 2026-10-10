#!/bin/bash
# ==============================================================================
# ADVANCE-TEST MASTER RUNNER: EMPIRICAL DIFFERENTIAL SUITE (1,000 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

MODULES=(
    "test_adv_01_log_analytics_etl.sh"
    "test_adv_02_json_csv_data.sh"
    "test_adv_03_git_developer_flow.sh"
    "test_adv_04_cicd_build_tools.sh"
    "test_adv_05_devops_cloud_docker.sh"
    "test_adv_06_sysadmin_security.sh"
    "test_adv_07_api_stream_curl.sh"
    "test_adv_08_string_algo_parsing.sh"
    "test_adv_09_process_supervision.sh"
    "test_adv_10_extreme_stress_thai.sh"
)

TOTAL_SUITE_PASS=0
TOTAL_SUITE_FAIL=0

echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}🏛️  ZEPHYR ADVANCE-TEST SUITE: BATCH EXECUTION RUNNER${NC}"
echo -e "   Target Binary: ${ZBASH_BIN}"
echo -e "   Golden Truth:  /bin/bash (GNU bash 3.2.57)"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

for mod in "${MODULES[@]}"; do
    if [ -f "$SCRIPT_DIR/$mod" ]; then
        echo -e "\n${BOLD}${YELLOW}▶ EXECUTING MODULE: $mod${NC}"
        bash "$SCRIPT_DIR/$mod"
        RC=$?
        if [ $RC -ne 0 ]; then
            echo -e "${RED}Module $mod finished with failures!${NC}"
            exit $RC
        fi
    fi
done

echo -e "\n${BOLD}${GREEN}================================================================================${NC}"
echo -e "${BOLD}🏆 ALL COMPLETED MODULES PASSED WITH 100% BIT-FOR-BIT PARITY! ⚡${NC}"
echo -e "${BOLD}${GREEN}================================================================================${NC}"
