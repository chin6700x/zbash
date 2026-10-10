#!/bin/bash
# ==============================================================================
# ADVANCE-TEST MODULE 05: DEVOPS, DOCKER WRAPPERS & HEALTHCHECKS (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

# Setup fixture
generate_docker_fixture
DOCKER_DIR="$FIXTURES_DIR/docker_env"
COMPOSE_FILE="$DOCKER_DIR/docker-compose.yml"
PS_FILE="$DOCKER_DIR/containers.ps"
LOG_FILE="$DOCKER_DIR/logs/app.log"
BACKUP_DIR="$DOCKER_DIR/backups"

echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}🚀 STARTING ADVANCE-TEST MODULE 05: DEVOPS, DOCKER & HEALTHCHECKS${NC}"
echo -e "   Target: zbash vs macOS /bin/bash (100 Differential Tests)"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

# ------------------------------------------------------------------------------
# 1. DOCKER COMPOSE & MANIFEST INSPECTIONS (TESTS #001 - #010)
# ------------------------------------------------------------------------------

run_adv_test "001" "Count Total Services Defined in Docker Compose" \
  "grep -E '^[ ]{2}[a-zA-Z0-9_-]+:' '$COMPOSE_FILE' | grep -v 'services:' | wc -l | tr -d ' '"

run_adv_test "002" "List All Service Names Sorted" \
  "grep -E '^[ ]{2}[a-zA-Z0-9_-]+:' '$COMPOSE_FILE' | tr -d ' :' | sort"

run_adv_test "003" "Extract Image Name for web Service" \
  "awk '/web:/,/image:/' '$COMPOSE_FILE' | grep 'image:' | awk '{print \$2}'"

run_adv_test "004" "Extract Image Name for api Service" \
  "awk '/api:/,/image:/' '$COMPOSE_FILE' | grep 'image:' | awk '{print \$2}'"

run_adv_test "005" "Extract Image Name for db Service" \
  "awk '/db:/,/image:/' '$COMPOSE_FILE' | grep 'image:' | awk '{print \$2}'"

run_adv_test "006" "Check If web Service Has restart: always Policy" \
  "awk '/web:/,/restart:/' '$COMPOSE_FILE' | grep -c 'restart: always'"

run_adv_test "007" "Verify Docker Compose Specification Version" \
  "grep '^version:' '$COMPOSE_FILE' | tr -d \"'\" | awk '{print \$2}'"

run_adv_test "008" "Check If Postgres Database Service is Configured" \
  "grep -c 'postgres:16-alpine' '$COMPOSE_FILE'"

run_adv_test "009" "Count Total Defined Container Names" \
  "grep -c 'container_name:' '$COMPOSE_FILE'"

run_adv_test "010" "Extract All container_name Values Sorted" \
  "grep 'container_name:' '$COMPOSE_FILE' | awk '{print \$2}' | sort"

# ------------------------------------------------------------------------------
# 2. CONTAINER STATUS & PS TABLE PARSING (TESTS #011 - #020)
# ------------------------------------------------------------------------------

run_adv_test "011" "Count Total Tracked Containers in PS Table" \
  "awk 'NR > 1' '$PS_FILE' | wc -l | tr -d ' '"

run_adv_test "012" "List All Container IDs (First Column)" \
  "awk 'NR > 1 {print \$1}' '$PS_FILE'"

run_adv_test "013" "Count Running Containers (Status Up)" \
  "grep -c ' Up ' '$PS_FILE'"

run_adv_test "014" "Count Exited Containers (Status Exited)" \
  "grep -c ' Exited ' '$PS_FILE'"

run_adv_test "015" "Extract Container ID for Redis" \
  "grep 'redis:' '$PS_FILE' | awk '{print \$1}'"

run_adv_test "016" "Extract Container ID for Nginx Web" \
  "grep 'nginx:' '$PS_FILE' | awk '{print \$1}'"

run_adv_test "017" "Check If Any Container is in Exited State" \
  "awk 'NR > 1 && \$3 == \"Exited\" {print \$1}' '$PS_FILE'"

run_adv_test "018" "Filter Containers Running > 1 Hour" \
  "grep 'Up ' '$PS_FILE' | awk '{print \$1, \$2}'"

run_adv_test "019" "Verify All Running Containers Have Non-empty ID" \
  "awk 'NR > 1 && length(\$1) != 12 {bad++} END {print bad+0}' '$PS_FILE'"

run_adv_test "020" "Summary Status Line of Container Fleet" \
  "up=\$(grep -c ' Up ' '$PS_FILE'); ex=\$(grep -c ' Exited ' '$PS_FILE'); echo \"Up:\$up Exited:\$ex\""

# ------------------------------------------------------------------------------
# 3. PORT MAPPINGS & NETWORK TOPOLOGY (TESTS #021 - #030)
# ------------------------------------------------------------------------------

run_adv_test "021" "Extract Port Mapping for Port 80" \
  "grep -o '0.0.0.0:80->80/tcp' '$PS_FILE'"

run_adv_test "022" "Extract Port Mapping for Port 443" \
  "grep -o '0.0.0.0:443->443/tcp' '$PS_FILE'"

run_adv_test "023" "List All Exposed Internal Ports in Compose" \
  "grep -E '^[ ]+-[ ]+\"[0-9]+:[0-9]+\"' '$COMPOSE_FILE' | tr -d ' \"-' | sort"

run_adv_test "024" "Check If Port 5432 is Exposed in PS Table" \
  "grep -c '5432/tcp' '$PS_FILE'"

run_adv_test "025" "Check If Port 3000 is Exposed in PS Table" \
  "grep -c '3000/tcp' '$PS_FILE'"

run_adv_test "026" "Verify Both HTTP (80) and HTTPS (443) Are Exposed" \
  "grep -q '80:80' '$COMPOSE_FILE' && grep -q '443:443' '$COMPOSE_FILE'; echo \$?"

run_adv_test "027" "Extract Host Port from Mapping '8080:80'" \
  "mapping='8080:80'; echo \"\${mapping%%:*}\""

run_adv_test "028" "Extract Container Port from Mapping '8080:80'" \
  "mapping='8080:80'; echo \"\${mapping##*:}\""

run_adv_test "029" "Construct Docker Run -p Flag Argument in Shell" \
  "hp=8080; cp=80; echo \"-p \${hp}:\${cp}\""

run_adv_test "030" "Count Total Ports Listed in PS Table" \
  "grep -o '[0-9]*/tcp' '$PS_FILE' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 4. ENVIRONMENT VARIABLES & SECRET CONFIGURATION (TESTS #031 - #040)
# ------------------------------------------------------------------------------

run_adv_test "031" "Extract All Environment Keys from Compose File" \
  "grep -E '^[ ]+-[ ]+[A-Z_]+=' '$COMPOSE_FILE' | cut -d'=' -f1 | tr -d ' -' | sort -u"

run_adv_test "032" "Check If SECRET_KEY is Defined in Compose" \
  "grep -c 'SECRET_KEY=' '$COMPOSE_FILE'"

run_adv_test "033" "Extract Database Username" \
  "grep 'POSTGRES_USER=' '$COMPOSE_FILE' | cut -d'=' -f2"

run_adv_test "034" "Extract Database Name" \
  "grep 'POSTGRES_DB=' '$COMPOSE_FILE' | cut -d'=' -f2"

run_adv_test "035" "Extract Hostname from DB_URL Connection String" \
  "url='postgres://user:pass@db:5432/main'; host_port=\"\${url#*@}\"; echo \"\${host_port%%/*}\""

run_adv_test "036" "Extract Database Port from DB_URL" \
  "url='postgres://user:pass@db:5432/main'; hp=\"\${url#*@}\"; hp=\"\${hp%%/*}\"; echo \"\${hp#*:}\""

run_adv_test "037" "Mask Password Value with Asterisks in Shell" \
  "pw='dbpassword456'; echo \"\${pw:0:2}********\${pw: -2}\""

run_adv_test "038" "Check If Production Secrets Contain Default 'admin'" \
  "grep 'POSTGRES_USER=' '$COMPOSE_FILE' | grep -c 'admin'"

run_adv_test "039" "Count Total Environment Variables in Compose" \
  "grep -E -c '^[ ]+-[ ]+[A-Z_]+=' '$COMPOSE_FILE'"

run_adv_test "040" "Verify NGINX_HOST Value" \
  "grep 'NGINX_HOST=' '$COMPOSE_FILE' | cut -d'=' -f2"

# ------------------------------------------------------------------------------
# 5. LOG STREAMING & SECRET MASKING (TESTS #041 - #050)
# ------------------------------------------------------------------------------

run_adv_test "041" "Count Total Log Lines in Container Log" \
  "wc -l < '$LOG_FILE' | tr -d ' '"

run_adv_test "042" "Filter [INFO] Log Level Lines" \
  "grep -c '\[INFO\]' '$LOG_FILE'"

run_adv_test "043" "Filter [WARN] Log Level Lines" \
  "grep -c '\[WARN\]' '$LOG_FILE'"

run_adv_test "044" "Filter [ERROR] Log Level Lines" \
  "grep -c '\[ERROR\]' '$LOG_FILE'"

run_adv_test "045" "Mask Secret Key in Log Line" \
  "grep 'secretKey=' '$LOG_FILE' | sed 's/secretKey=[^ ]*/secretKey=[REDACTED]/'"

run_adv_test "046" "Mask Authentication Token in Error Log" \
  "grep 'secret_token_' '$LOG_FILE' | sed 's/secret_token_[a-z]*/[TOKEN_MASKED]/'"

run_adv_test "047" "Extract Timestamp from First Log Entry" \
  "head -n 1 '$LOG_FILE' | awk '{print \$1}'"

run_adv_test "048" "Extract Memory Usage Percentage from Warning Log" \
  "grep 'High memory usage' '$LOG_FILE' | awk '{print \$NF}'"

run_adv_test "049" "Verify Healthcheck Status in Log" \
  "grep 'Healthcheck ok' '$LOG_FILE' | awk '{print \$3, \$4}'"

run_adv_test "050" "Count Unique Log Levels Encountered" \
  "grep -o '\[[A-Z]*\]' '$LOG_FILE' | sort -u | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 6. HEALTHCHECK POLLING & EVALUATION (TESTS #051 - #060)
# ------------------------------------------------------------------------------

run_adv_test "051" "Extract CPU Percentage from Healthcheck" \
  "grep 'CPU=' '$LOG_FILE' | grep -o 'CPU=[0-9]*%' | cut -d= -f2"

run_adv_test "052" "Extract Memory Percentage from Healthcheck" \
  "grep 'MEM=' '$LOG_FILE' | grep -o 'MEM=[0-9]*%' | cut -d= -f2"

run_adv_test "053" "Check If CPU Utilization is Below 80%" \
  "cpu=42; [ \"\$cpu\" -lt 80 ]; echo \$?"

run_adv_test "054" "Check If Memory Utilization Exceeded 80% Threshold" \
  "mem=82; [ \"\$mem\" -gt 80 ]; echo \$?"

run_adv_test "055" "Simulate Healthcheck Polling Success After 2 Tries" \
  "tries=0; while [ \$tries -lt 5 ]; do tries=\$((tries + 1)); if [ \$tries -ge 2 ]; then echo \"HealthyOn:\$tries\"; break; fi; done"

run_adv_test "056" "Healthcheck HTTP Status Code Evaluation (200 is healthy)" \
  "code=200; if [ \"\$code\" -eq 200 ]; then echo 'HEALTHY'; else echo 'UNHEALTHY'; fi"

run_adv_test "057" "Healthcheck HTTP 503 Evaluation (Unhealthy)" \
  "code=503; if [ \"\$code\" -eq 200 ]; then echo 'HEALTHY'; else echo 'UNHEALTHY'; fi"

run_adv_test "058" "Timeout Exceeded Flag Simulation" \
  "elapsed=35; max_t=30; if [ \"\$elapsed\" -gt \"\$max_t\" ]; then echo 'TIMEOUT'; else echo 'OK'; fi"

run_adv_test "059" "Calculate Remaining Timeout Budget" \
  "max_t=30; elapsed=12; echo \"Remaining:\$((max_t - elapsed))s\""

run_adv_test "060" "Simulate Healthy Container ID Lookup" \
  "cid='a1b2c3d4e5f6'; echo \"Container \${cid:0:8} is healthy\""

# ------------------------------------------------------------------------------
# 7. DATABASE BACKUP ROTATION & RETENTION (TESTS #061 - #070)
# ------------------------------------------------------------------------------

run_adv_test "061" "Count Total Database Backup Files Available" \
  "ls '$BACKUP_DIR' | wc -l | tr -d ' '"

run_adv_test "062" "List All Backups Sorted by Date Ascending" \
  "ls '$BACKUP_DIR' | sort"

run_adv_test "063" "Find Most Recent Database Backup" \
  "ls '$BACKUP_DIR' | sort | tail -n 1"

run_adv_test "064" "Find Oldest Database Backup" \
  "ls '$BACKUP_DIR' | sort | head -n 1"

run_adv_test "065" "Identify Backups to Retain (Keep Last 7 Days)" \
  "ls '$BACKUP_DIR' | sort | tail -n 7 | wc -l | tr -d ' '"

run_adv_test "066" "Identify Expired Backups to Prune (> 7 Days Old)" \
  "tot=\$(ls '$BACKUP_DIR' | wc -l | tr -d ' '); retain=7; echo \"ToPrune:\$((tot - retain))\""

run_adv_test "067" "List Names of Expired Backups to Prune" \
  "ls '$BACKUP_DIR' | sort | head -n 2"

run_adv_test "068" "Extract Date from Backup Filename via Parameter Expansion" \
  "fname='backup_2026-10-09.sql.gz'; d=\"\${fname#backup_}\"; echo \"\${d%%.sql.gz}\""

run_adv_test "069" "Verify All Backups Have .sql.gz Extension" \
  "ls '$BACKUP_DIR' | grep -v '\.sql\.gz\$' | wc -l | tr -d ' '"

run_adv_test "070" "Construct S3 Backup Upload Destination Path" \
  "bucket='s3://zephyr-cloud-backups'; date='2026-10-09'; echo \"\${bucket}/postgres/\${date}/dump.sql.gz\""

# ------------------------------------------------------------------------------
# 8. VOLUME MANAGEMENT & PERSISTENCE (TESTS #071 - #080)
# ------------------------------------------------------------------------------

run_adv_test "071" "Extract Top-level Named Volumes from Compose" \
  "awk '/^volumes:/,0' '$COMPOSE_FILE' | grep -E '^[ ]{2}[a-zA-Z0-9_-]+:' | tr -d ' :'"

run_adv_test "072" "Verify pgdata Volume is Defined" \
  "grep -c 'pgdata:' '$COMPOSE_FILE'"

run_adv_test "073" "Extract Volume Mount Path inside Postgres Container" \
  "grep -E '^[ ]+-[ ]+pgdata:' '$COMPOSE_FILE' | cut -d':' -f2"

run_adv_test "074" "Check If Volume Name Matches Source Mapping" \
  "vmap='pgdata:/var/lib/postgresql/data'; echo \"\${vmap%%:*}\""

run_adv_test "075" "Check Volume Target Mount Directory" \
  "vmap='pgdata:/var/lib/postgresql/data'; echo \"\${vmap#*:}\""

run_adv_test "076" "Check If Read-Only Volume Flag (:ro) is Present" \
  "vmap='pgdata:/data:ro'; if [ \"\$vmap\" != \"\${vmap%:ro}\" ]; then echo 'ReadOnly'; else echo 'ReadWrite'; fi"

run_adv_test "077" "Check If Standard Volume is ReadWrite" \
  "vmap='pgdata:/data'; if [ \"\$vmap\" != \"\${vmap%:ro}\" ]; then echo 'ReadOnly'; else echo 'ReadWrite'; fi"

run_adv_test "078" "Simulate Docker Volume Inspect JSON Format" \
  "vname='pgdata'; echo \"{\\\"Name\\\":\\\"\$vname\\\",\\\"Driver\\\":\\\"local\\\"}\""

run_adv_test "079" "Count Total Volume Declarations across Services" \
  "grep -c 'volumes:' '$COMPOSE_FILE'"

run_adv_test "080" "Verify Host Volume Prefix Starts with Character" \
  "v='pgdata'; if [ \"\${v:0:1}\" != '/' ]; then echo 'NamedVolume'; else echo 'BindMount'; fi"

# ------------------------------------------------------------------------------
# 9. SERVICE DEPENDENCY GRAPH (DEPENDS_ON) (TESTS #081 - #090)
# ------------------------------------------------------------------------------

run_adv_test "081" "Check If api Service depends_on db" \
  "awk '/api:/,/db:/' '$COMPOSE_FILE' | grep -A 2 'depends_on:' | grep -c 'db'"

run_adv_test "082" "Extract All Services with depends_on Declarations" \
  "awk '/^[ ]{2}[a-z]+:/{svc=\$1} /depends_on:/{print svc}' '$COMPOSE_FILE' | tr -d ':'"

run_adv_test "083" "Topological Start Order: DB Must Precede API" \
  "order='db api web'; echo \$order | awk '{if (index(\$0, \"db\") < index(\$0, \"api\")) print \"VALID\"; else print \"INVALID\"}'"

run_adv_test "084" "Check If web Depends on Any Service" \
  "awk '/web:/,/api:/' '$COMPOSE_FILE' | grep -c 'depends_on:'"

run_adv_test "085" "Simulate Dependency Ready Check Loop" \
  "dep='db'; ready=0; for t in 1 2 3; do if [ \$t -ge 2 ]; then ready=1; break; fi; done; echo \"\${dep}_ready:\$ready\""

run_adv_test "086" "Check If Redis Container is Stopped" \
  "grep 'redis:' '$PS_FILE' | grep -q 'Exited'; echo \$?"

run_adv_test "087" "Check If Node API Container is Up" \
  "grep 'node:' '$PS_FILE' | grep -q 'Up'; echo \$?"

run_adv_test "088" "Count Dependent Services for db" \
  "grep -A 2 'depends_on:' '$COMPOSE_FILE' | grep -c 'db'"

run_adv_test "089" "Construct Startup Sequence String" \
  "echo 'Step 1: db -> Step 2: api -> Step 3: web'"

run_adv_test "090" "Verify All Services Have Unique Container Names" \
  "names=\$(grep 'container_name:' '$COMPOSE_FILE' | awk '{print \$2}'); tot=\$(echo \"\$names\" | wc -l | tr -d ' '); uniq=\$(echo \"\$names\" | sort -u | wc -l | tr -d ' '); [ \"\$tot\" -eq \"\$uniq\" ]; echo \$?"

# ------------------------------------------------------------------------------
# 10. IN-SHELL DEVOPS AUTOMATION & WATCHDOG (TESTS #091 - #100)
# ------------------------------------------------------------------------------

run_adv_test "091" "Pure Shell Container Watchdog: Count Dead Services" \
  "dead=0; while read -r id img st rest; do if [ \"\$st\" = \"Exited\" ]; then dead=\$((dead + 1)); fi; done <<< \"\$(awk 'NR > 1' '$PS_FILE')\"; echo \"DeadContainers:\$dead\""

run_adv_test "092" "Pure Shell Secret Scrubber Function" \
  "scrub_log() { sed -E 's/(secretKey|secret_token|password)=[a-zA-Z0-9_]*/\1=[MASKED]/g'; }; head -n 2 '$LOG_FILE' | scrub_log"

run_adv_test "093" "Pure Shell Backup Rotation Pruning Simulator" \
  "kept=0; pruned=0; while read -r f; do if [ \$kept -lt 7 ]; then kept=\$((kept + 1)); else pruned=\$((pruned + 1)); fi; done <<< \"\$(ls '$BACKUP_DIR' | sort -r)\"; echo \"Kept:\$kept Pruned:\$pruned\""

run_adv_test "094" "Pure Shell Port Conflict Detector" \
  "ports='80 443 3000 5432'; conflict=0; for p in \$ports; do if [ \"\$p\" = \"80\" ]; then conflict=\$((conflict + 1)); fi; done; echo \"Occurrences:\$conflict\""

run_adv_test "095" "Docker Service Health Badge Generator" \
  "up=3; total=4; if [ \$up -eq \$total ]; then echo 'fleet:healthy'; else echo 'fleet:degraded'; fi"

run_adv_test "096" "Extract First 8 Chars of All Container IDs in Loop" \
  "out=\"\"; for id in \$(awk 'NR > 1 {print \$1}' '$PS_FILE'); do out=\"\$out \${id:0:8}\"; done; echo \"\${out# }\""

run_adv_test "097" "Memory Usage Warning Generator in Shell" \
  "mem=82; if [ \$mem -ge 80 ]; then echo \"WARNING: Memory at \${mem}%\"; fi"

run_adv_test "098" "Construct Prometheus Node Exporter Metric Line" \
  "echo 'container_memory_usage_percent{container=\"zephyr_api\"} 82'"

run_adv_test "099" "Check Clean Docker Environment Setup" \
  "[ -f '$COMPOSE_FILE' ] && [ -f '$PS_FILE' ]; echo \$?"

run_adv_test "100" "Complete Module 5 Parity & Integrity Proof" \
  "grep 'container_name:' '$COMPOSE_FILE' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# FINAL MODULE SUMMARY REPORT
# ------------------------------------------------------------------------------

echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}📊 MODULE 05 EXECUTION SUMMARY (DEVOPS, DOCKER & HEALTHCHECKS):${NC}"
echo -e "   TOTAL TESTS: 100"
echo -e "   PASSED:      ${GREEN}${PASS_COUNT}${NC}"
echo -e "   FAILED:      ${RED}${FAIL_COUNT}${NC}"
if [ "$FAIL_COUNT" -eq 0 ]; then
    echo -e "   ${BOLD}${GREEN}🎉 100% PERFECT BIT-FOR-BIT MATCH (100 / 100 PASS) ⚡${NC}"
else
    echo -e "   ${BOLD}${RED}❌ PARITY FAILURES DETECTED!${NC}"
    exit 1
fi
echo -e "${BOLD}${CYAN}================================================================================${NC}"
