#!/bin/bash
# ==============================================================================
# ADVANCE-TEST MODULE 06: SYSADMIN, SECURITY FORENSICS & AUDIT (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

# Setup fixture
generate_security_fixture
SEC_DIR="$FIXTURES_DIR/sysadmin"
AUTH_LOG="$SEC_DIR/auth.log"
CRON_FILE="$SEC_DIR/crontab.txt"
SOCK_FILE="$SEC_DIR/sockets.txt"
PROC_FILE="$SEC_DIR/processes.ps"

echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}🚀 STARTING ADVANCE-TEST MODULE 06: SYSADMIN & SECURITY FORENSICS${NC}"
echo -e "   Target: zbash vs macOS /bin/bash (100 Differential Tests)"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

# ------------------------------------------------------------------------------
# 1. SSH BRUTE FORCE & AUTH LOG ANALYSIS (TESTS #001 - #010)
# ------------------------------------------------------------------------------

run_adv_test "001" "Count Total Failed Password Attempts in auth.log" \
  "grep -c 'Failed password' '$AUTH_LOG'"

run_adv_test "002" "Detect Brute Force Source IP (> 5 failures)" \
  "grep 'Failed password' '$AUTH_LOG' | awk '{for(i=1;i<=NF;i++) if(\$i==\"from\") print \$(i+1)}' | sort | uniq -c | awk '\$1 >= 5 {print \$2}'"

run_adv_test "003" "List Unique Target Usernames Targeted by Attackers" \
  "grep 'Failed password' '$AUTH_LOG' | awk '{for(i=1;i<=NF;i++) if(\$i==\"for\") {if(\$(i+1)==\"invalid\") print \$(i+3); else print \$(i+1)}}' | sort -u"

run_adv_test "004" "Detect Probes with 'Invalid user'" \
  "grep -c 'Invalid user' '$AUTH_LOG'"

run_adv_test "005" "Extract Successful Publickey Logins" \
  "grep 'Accepted publickey' '$AUTH_LOG' | awk '{print \$9, \$11}'"

run_adv_test "006" "Count Successful Password Logins" \
  "grep -c 'Accepted password' '$AUTH_LOG'"

run_adv_test "007" "List Authorized Users Who Successfully Logged In" \
  "grep 'Accepted' '$AUTH_LOG' | awk '{print \$9}' | sort -u"

run_adv_test "008" "Extract Source IP for Successful Logins" \
  "grep 'Accepted' '$AUTH_LOG' | awk '{print \$11}' | sort -u"

run_adv_test "009" "Count Failed Logins from IP 192.0.2.15" \
  "grep -c 'from 192.0.2.15' '$AUTH_LOG'"

run_adv_test "010" "Count Total SSH Daemon Events" \
  "grep -c 'sshd\[' '$AUTH_LOG'"

# ------------------------------------------------------------------------------
# 2. SUDO PRIVILEGED EXECUTION AUDITS (TESTS #011 - #020)
# ------------------------------------------------------------------------------

run_adv_test "011" "Count Total Sudo Invocations in auth.log" \
  "grep -c 'sudo:' '$AUTH_LOG'"

run_adv_test "012" "Extract Commands Executed via Sudo" \
  "grep 'COMMAND=' '$AUTH_LOG' | sed 's/.*COMMAND=//' | sort"

run_adv_test "013" "Identify Users Invoking Sudo" \
  "grep 'sudo:' '$AUTH_LOG' | awk '{print \$6}' | sort -u"

run_adv_test "014" "Detect Sudo Commands Targeting Root" \
  "grep -c 'USER=root' '$AUTH_LOG'"

run_adv_test "015" "Filter Sudo Service Restarts" \
  "grep 'systemctl restart' '$AUTH_LOG' | sed 's/.*COMMAND=//'"

run_adv_test "016" "Extract TTY Terminal for Sudo Session" \
  "grep 'sudo:' '$AUTH_LOG' | grep -o 'TTY=[^ ]*' | sort -u | cut -d= -f2"

run_adv_test "017" "Check If Sudo Executed in /home" \
  "grep -c 'PWD=/home' '$AUTH_LOG'"

run_adv_test "018" "Detect Package Manager (apt) Executions" \
  "grep -c '/usr/bin/apt' '$AUTH_LOG'"

run_adv_test "019" "Sudo Audit Log Formatter" \
  "grep 'sudo:' '$AUTH_LOG' | awk '{print \$1, \$2, \$3, \"USER:\"\$6, \"CMD:\"\$NF}' | head -n 1"

run_adv_test "020" "Verify All Sudo Events Have Non-Empty COMMAND" \
  "grep 'sudo:' '$AUTH_LOG' | grep -v 'COMMAND=' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 3. CRON SCHEDULE & SUSPICIOUS TASKS (TESTS #021 - #030)
# ------------------------------------------------------------------------------

run_adv_test "021" "Filter Active Cron Jobs (Exclude Comments and Blanks)" \
  "grep -v '^[#[:space:]]' '$CRON_FILE' | grep -v '^[A-Z]' | wc -l | tr -d ' '"

run_adv_test "022" "Detect Suspicious Cron Job (Contains curl / mine.sh)" \
  "grep 'curl' '$CRON_FILE' | awk '{print \$6, \$7, \$8}'"

run_adv_test "023" "List Users Scheduled in Crontab" \
  "grep -v '^[#[:space:]]' '$CRON_FILE' | grep -v '^[A-Z]' | awk '{print \$6}' | sort -u"

run_adv_test "024" "Check for Hourly Cron Job" \
  "grep -c 'cron.hourly' '$CRON_FILE'"

run_adv_test "025" "Check for Backup Cron Job" \
  "grep -c 'backup_db.sh' '$CRON_FILE'"

run_adv_test "026" "Extract Command Line for Nightly Backup" \
  "grep 'backup_db.sh' '$CRON_FILE' | awk '{print \$7}'"

run_adv_test "027" "Detect Jobs Running as 'hacker' User" \
  "awk '\$6 == \"hacker\" {print \$6, \$7}' '$CRON_FILE'"

run_adv_test "028" "Check If Crontab Has Valid SHELL Definition" \
  "grep '^SHELL=' '$CRON_FILE' | cut -d= -f2"

run_adv_test "029" "Extract PATH Setting from Crontab" \
  "grep '^PATH=' '$CRON_FILE' | cut -d= -f2 | tr ':' '\n' | head -n 2"

run_adv_test "030" "Total Cron Environment Variable Declarations" \
  "grep -E '^[A-Z]+=' '$CRON_FILE' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 4. LISTENING SOCKETS & NETWORK PORT AUDITING (TESTS #031 - #040)
# ------------------------------------------------------------------------------

run_adv_test "031" "Count Total Listening TCP Sockets" \
  "grep -c 'LISTEN' '$SOCK_FILE'"

run_adv_test "032" "List All Listening Port Numbers" \
  "grep 'LISTEN' '$SOCK_FILE' | awk '{print \$4}' | cut -d: -f2 | sort -n"

run_adv_test "033" "Detect Suspicious Port 4444" \
  "grep -c ':4444 ' '$SOCK_FILE'"

run_adv_test "034" "Identify Process Attached to Port 4444" \
  "grep ':4444 ' '$SOCK_FILE' | awk '{print \$7}'"

run_adv_test "035" "Verify SSH Service is Listening on Port 22" \
  "grep ':22 ' '$SOCK_FILE' | grep -c 'LISTEN'"

run_adv_test "036" "Verify Web Server Ports 80 and 443" \
  "grep -E ':80 |:443 ' '$SOCK_FILE' | wc -l | tr -d ' '"

run_adv_test "037" "Check If Database Port 5432 is Bound to Localhost Only" \
  "grep ':5432 ' '$SOCK_FILE' | grep -c '127.0.0.1:5432'"

run_adv_test "038" "Check If Redis Port 6379 is Bound to Localhost Only" \
  "grep ':6379 ' '$SOCK_FILE' | grep -c '127.0.0.1:6379'"

run_adv_test "039" "Extract Foreign IP of Established Connection" \
  "grep 'ESTABLISHED' '$SOCK_FILE' | awk '{print \$5}' | cut -d: -f1"

run_adv_test "040" "Total Network Sockets in Audit Table" \
  "awk 'NR > 1' '$SOCK_FILE' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 5. PROCESS TABLE AUDITING & CRYPTOMINER HUNTING (TESTS #041 - #050)
# ------------------------------------------------------------------------------

run_adv_test "041" "Find Process with Highest CPU Utilization" \
  "awk 'NR > 1 {print \$3, \$1, \$NF}' '$PROC_FILE' | sort -nr | head -n 1"

run_adv_test "042" "Detect Malicious Cryptominer Process (xmrig)" \
  "grep -c 'xmrig' '$PROC_FILE'"

run_adv_test "043" "Extract PID of High CPU Cryptominer" \
  "grep 'xmrig' '$PROC_FILE' | awk '{print \$2}'"

run_adv_test "044" "Extract User Running Cryptominer" \
  "grep 'xmrig' '$PROC_FILE' | awk '{print \$1}'"

run_adv_test "045" "Check If Cryptominer CPU Exceeds 90%" \
  "cpu=\$(grep 'xmrig' '$PROC_FILE' | awk '{print int(\$3)}'); [ \"\$cpu\" -gt 90 ]; echo \$?"

run_adv_test "046" "Extract Cryptominer Working Memory (RSS)" \
  "grep 'xmrig' '$PROC_FILE' | awk '{print \$6}'"

run_adv_test "047" "Verify Init / Systemd Process is PID 1" \
  "awk '\$2 == 1 {print \$1, \$NF}' '$PROC_FILE'"

run_adv_test "048" "Count Total Active System Processes in Audit" \
  "awk 'NR > 1' '$PROC_FILE' | wc -l | tr -d ' '"

run_adv_test "049" "List Processes Running as root User" \
  "awk 'NR > 1 && \$1 == \"root\" {print \$2, \$NF}' '$PROC_FILE'"

run_adv_test "050" "Find Process with TTY Attached" \
  "awk 'NR > 1 && \$7 != \"?\" {print \$1, \$2, \$7}' '$PROC_FILE'"

# ------------------------------------------------------------------------------
# 6. FILE PERMISSIONS & INTEGRITY AUDITING (TESTS #051 - #060)
# ------------------------------------------------------------------------------

run_adv_test "051" "Simulate Permission Check for 0600 Private Key" \
  "mode=600; if [ \"\$mode\" -eq 600 ]; then echo 'SECURE'; else echo 'INSECURE'; fi"

run_adv_test "052" "Detect World-Writable Flag in Permission String" \
  "perms='-rw-rw-rw-'; if [ \"\${perms: -1}\" = 'w' ] || [ \"\${perms:8:1}\" = 'w' ]; then echo 'WORLD_WRITABLE'; else echo 'SAFE'; fi"

run_adv_test "053" "Check If Standard File is Not World-Writable" \
  "perms='-rw-r--r--'; if [ \"\${perms:8:1}\" = 'w' ]; then echo 'WORLD_WRITABLE'; else echo 'SAFE'; fi"

run_adv_test "054" "Simulate SUID Bit Detection in Perm String" \
  "perms='-rwsr-xr-x'; if [ \"\${perms:3:1}\" = 's' ]; then echo 'SUID_SET'; else echo 'NORMAL'; fi"

run_adv_test "055" "Simulate SGID Bit Detection in Perm String" \
  "perms='-rwxr-sr-x'; if [ \"\${perms:6:1}\" = 's' ]; then echo 'SGID_SET'; else echo 'NORMAL'; fi"

run_adv_test "056" "Verify File Owner is Root in Audit" \
  "owner='root'; [ \"\$owner\" = 'root' ]; echo \$?"

run_adv_test "057" "Verify Directory Has 0755 Permissions" \
  "dmode=755; [ \"\$dmode\" -eq 755 ]; echo \$?"

run_adv_test "058" "Detect Hidden Directory Prefix (.hidden)" \
  "path='/tmp/.hidden/xmrig'; if [ \"\${path#*/.}\" != \"\$path\" ]; then echo 'HIDDEN_PATH'; else echo 'VISIBLE'; fi"

run_adv_test "059" "Calculate File Hash for Audit Baseline" \
  "echo 'zephyr_integrity_baseline' | sha256sum 2>/dev/null || shasum -a 256 <<< 'zephyr_integrity_baseline' | awk '{print \$1}'"

run_adv_test "060" "Simulate Audit Pass Flag on Known Hash" \
  "h1='abc'; h2='abc'; [ \"\$h1\" = \"\$h2\" ]; echo \$?"

# ------------------------------------------------------------------------------
# 7. SECURITY INCIDENT ALERTING & TRIAGE (TESTS #061 - #070)
# ------------------------------------------------------------------------------

run_adv_test "061" "Generate Security Alert Line for Brute Force IP" \
  "ip='192.0.2.15'; cnt=6; echo \"CRITICAL: SSH Brute-Force from \$ip (\$cnt failed attempts)\""

run_adv_test "062" "Generate High CPU Miner Alert" \
  "pid=31337; cpu=99.8; echo \"ALERT: Process \$pid consuming \${cpu}% CPU\""

run_adv_test "063" "Format Security Incident Severity Level" \
  "score=95; if [ \$score -ge 90 ]; then echo 'CRITICAL'; elif [ \$score -ge 70 ]; then echo 'HIGH'; else echo 'LOW'; fi"

run_adv_test "064" "Check If Failed Logins Exceed 5 Threshold" \
  "cnt=\$(grep -c 'Failed password' '$AUTH_LOG'); [ \$cnt -ge 5 ]; echo \$?"

run_adv_test "065" "Group Failed Logins by Hour Window (04:00)" \
  "grep 'Failed password' '$AUTH_LOG' | awk '{print \$1, \$2, substr(\$3, 1, 2)\":00\"}' | sort | uniq -c | tr -s ' ' | sed 's/^[ ]*//'"

run_adv_test "066" "Extract Unique SSH Ports Targeted" \
  "grep 'Failed password' '$AUTH_LOG' | grep -o 'port [0-9]*' | awk '{print \$2}' | head -n 3"

run_adv_test "067" "Verify All Failed Logins Used SSH2 Protocol" \
  "grep 'Failed password' '$AUTH_LOG' | awk '{print \$NF}' | sort -u"

run_adv_test "068" "Construct Security Advisory Markdown Header" \
  "date='2026-10-09'; echo \"# Security Incident Report - \$date\""

run_adv_test "069" "Count Total IP Addresses Flagged as Malicious" \
  "echo '192.0.2.15 198.51.100.88' | wc -w | tr -d ' '"

run_adv_test "070" "Incident Severity Score Calculation" \
  "f_cnt=8; s_cnt=1; score=\$((f_cnt * 10 + s_cnt * 20)); echo \"IncidentScore:\$score\""

# ------------------------------------------------------------------------------
# 8. USER ACCOUNT & SHADOW FILE SIMULATION (TESTS #071 - #080)
# ------------------------------------------------------------------------------

run_adv_test "071" "Parse Mock User Account Entry: Root Shell" \
  "line='root:x:0:0:root:/root:/bin/bash'; echo \"\${line##*:}\""

run_adv_test "072" "Extract UID from User Account Entry" \
  "line='root:x:0:0:root:/root:/bin/bash'; uid=\"\${line#*:*:}\"; echo \"\${uid%%:*}\""

run_adv_test "073" "Extract Home Directory from User Account" \
  "line='chinchin:x:1000:1000:Chin:/home/chinchin:/bin/zbash'; home=\"\${line%:*}\"; echo \"\${home##*:}\""

run_adv_test "074" "Identify Interactive Shell vs nologin" \
  "sh='/bin/bash'; if [ \"\$sh\" = '/sbin/nologin' ]; then echo 'SYSTEM'; else echo 'INTERACTIVE'; fi"

run_adv_test "075" "Identify System Account Shell (/sbin/nologin)" \
  "sh='/sbin/nologin'; if [ \"\$sh\" = '/sbin/nologin' ]; then echo 'SYSTEM'; else echo 'INTERACTIVE'; fi"

run_adv_test "076" "Verify UID 0 is Exclusively Root" \
  "user='root'; uid=0; if [ \$uid -eq 0 ] && [ \"\$user\" = 'root' ]; then echo 'VALID_ROOT'; else echo 'ALERT'; fi"

run_adv_test "077" "Check If Account Password is Locked (!)" \
  "hash='!$6$rounds=5000$saltsalt$xyz'; if [ \"\${hash:0:1}\" = '!' ]; then echo 'LOCKED'; else echo 'ACTIVE'; fi"

run_adv_test "078" "Check If Account Password is Active ($6$)" \
  "hash='$6$rounds=5000$saltsalt$xyz'; if [ \"\${hash:0:1}\" = '!' ]; then echo 'LOCKED'; else echo 'ACTIVE'; fi"

run_adv_test "079" "Verify Sudoers Line Syntax Format" \
  "sudo_line='%wheel ALL=(ALL:ALL) ALL'; echo \"\${sudo_line%% *}:privileged\""

run_adv_test "080" "Count Formatted User Entries in Array" \
  "users='root chinchin deployer'; echo \$users | wc -w | tr -d ' '"

# ------------------------------------------------------------------------------
# 9. IP REPUTATION & CIDR BLOCKLIST (TESTS #081 - #090)
# ------------------------------------------------------------------------------

run_adv_test "081" "Check If Attacker IP 192.0.2.15 is in Blocklist" \
  "blocklist='192.0.2.15 198.51.100.88 203.0.113.77'; if echo \"\$blocklist\" | grep -q '192.0.2.15'; then echo 'BLOCKED'; else echo 'ALLOWED'; fi"

run_adv_test "082" "Check If Whitelisted IP 10.0.0.50 is Permitted" \
  "whitelist='10.0.0.50 10.0.0.99'; if echo \"\$whitelist\" | grep -q '10.0.0.50'; then echo 'PERMITTED'; else echo 'REJECTED'; fi"

run_adv_test "083" "Extract First Octet of Attacker IP" \
  "ip='192.0.2.15'; echo \"\${ip%%.*}\""

run_adv_test "084" "Extract Subnet Prefix /24 from IP" \
  "ip='192.0.2.15'; echo \"\${ip%.*}.0/24\""

run_adv_test "085" "Extract Last Octet Host Address" \
  "ip='192.0.2.15'; echo \"\${ip##*.}\""

run_adv_test "086" "Check Private RFC 1918 Range 10.x.x.x" \
  "ip='10.0.0.50'; if [ \"\${ip%%.*}\" -eq 10 ]; then echo 'RFC1918_PRIVATE'; else echo 'PUBLIC'; fi"

run_adv_test "087" "Check Public IP 198.51.100.88" \
  "ip='198.51.100.88'; if [ \"\${ip%%.*}\" -eq 10 ]; then echo 'RFC1918_PRIVATE'; else echo 'PUBLIC'; fi"

run_adv_test "088" "Construct Iptables Drop Rule in Shell" \
  "bad_ip='192.0.2.15'; echo \"iptables -A INPUT -s \${bad_ip} -j DROP\""

run_adv_test "089" "Construct macOS PF Firewall Block Rule" \
  "bad_ip='192.0.2.15'; echo \"block drop in quick from \${bad_ip} to any\""

run_adv_test "090" "Sort List of Threat IPs Numerically" \
  "ips='198.51.100.88 192.0.2.15 203.0.113.77'; echo \"\$ips\" | tr ' ' '\n' | sort -t . -k 1,1n -k 2,2n -k 3,3n -k 4,4n | head -n 1"

# ------------------------------------------------------------------------------
# 10. IN-SHELL SECURITY FORENSICS ORCHESTRATION (TESTS #091 - #100)
# ------------------------------------------------------------------------------

run_adv_test "091" "Pure Shell While-Read Loop over Auth Log" \
  "cnt=0; while read -r line; do cnt=\$((cnt + 1)); done < '$AUTH_LOG'; echo \"AuthEvents:\$cnt\""

run_adv_test "092" "Pure Shell Brute-Force Detector in Function" \
  "check_bf() { local ip=\"\$1\"; local n=\$(grep -c \"from \$ip\" '$AUTH_LOG'); if [ \$n -ge 5 ]; then echo \"\$ip:THREAT\"; else echo \"\$ip:OK\"; fi; }; check_bf '192.0.2.15'"

run_adv_test "093" "Check Whitelisted IP in Function" \
  "check_bf() { local ip=\"\$1\"; local n=\$(grep -c \"from \$ip\" '$AUTH_LOG'); if [ \$n -ge 5 ]; then echo \"\$ip:THREAT\"; else echo \"\$ip:OK\"; fi; }; check_bf '10.0.0.50'"

run_adv_test "094" "Process CPU Threshold Watcher Function" \
  "watch_cpu() { local pid=\"\$1\"; local u=\$(grep \" \$pid \" '$PROC_FILE' | awk '{print int(\$3)}'); if [ \$u -gt 90 ]; then echo \"\$pid:OVERHEAT\"; else echo \"\$pid:NORMAL\"; fi; }; watch_cpu '31337'"

run_adv_test "095" "Process Normal CPU Check" \
  "watch_cpu() { local pid=\"\$1\"; local u=\$(grep \" \$pid \" '$PROC_FILE' | awk '{print int(\$3)}'); if [ \$u -gt 90 ]; then echo \"\$pid:OVERHEAT\"; else echo \"\$pid:NORMAL\"; fi; }; watch_cpu '980'"

run_adv_test "096" "Firewall Rule Generator in Shell Loop" \
  "rules=\"\"; for ip in 192.0.2.15 198.51.100.88; do rules=\"\$rules [DROP \$ip]\"; done; echo \"\${rules# }\""

run_adv_test "097" "Crontab Malware Scanner in Pure Shell" \
  "mal=0; while read -r line; do if echo \"\$line\" | grep -q 'malicious\.cc'; then mal=\$((mal + 1)); fi; done < '$CRON_FILE'; echo \"MaliciousJobs:\$mal\""

run_adv_test "098" "Construct Structured Security Audit JSON" \
  "threat_ip='192.0.2.15'; miner_pid='31337'; echo \"{\\\"threat_ip\\\":\\\"\$threat_ip\\\",\\\"miner_pid\\\":\$miner_pid}\""

run_adv_test "099" "Clean Security Environment Verification" \
  "[ -f '$AUTH_LOG' ] && [ -f '$CRON_FILE' ] && [ -f '$SOCK_FILE' ]; echo \$?"

run_adv_test "100" "Complete Module 6 Parity & Integrity Proof" \
  "wc -l < '$AUTH_LOG' | tr -d ' '"

# ------------------------------------------------------------------------------
# FINAL MODULE SUMMARY REPORT
# ------------------------------------------------------------------------------

echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}📊 MODULE 06 EXECUTION SUMMARY (SYSADMIN & SECURITY FORENSICS):${NC}"
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
