#!/bin/bash
# ==============================================================================
# ADVANCE-TEST MODULE 01: LOG ANALYTICS, WEB ACCESS PARSING & ETL PIPELINES
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

# Ensure fixture is generated
generate_nginx_fixture
LOG_FILE="$FIXTURES_DIR/nginx_access.log"

echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}${CYAN}📊 ADVANCE-TEST: MODULE 01 - REAL-WORLD LOG ANALYTICS & ETL PIPELINES (100 TESTS)${NC}"
echo -e "   Target: Nginx Combined Access Log Parsing, Field Extractions, Status Aggregations"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

# ------------------------------------------------------------------------------
# 1. FIELD EXTRACTION & SLICING (TESTS #001 - #010)
# ------------------------------------------------------------------------------

run_adv_test "001" "Extract Client IP Address (1st Column)" \
  "awk '{print \$1}' '$LOG_FILE' | head -n 5"

run_adv_test "002" "Extract Request Timestamps" \
  "awk '{print \$4}' '$LOG_FILE' | tr -d '[' | head -n 5"

run_adv_test "003" "Extract HTTP Method (GET/POST/DELETE)" \
  "awk '{print \$6}' '$LOG_FILE' | tr -d '\"' | head -n 6"

run_adv_test "004" "Extract Request URI Path" \
  "awk '{print \$7}' '$LOG_FILE' | head -n 6"

run_adv_test "005" "Extract HTTP Status Codes" \
  "awk '{print \$9}' '$LOG_FILE' | head -n 7"

run_adv_test "006" "Extract Response Size in Bytes" \
  "awk '{print \$10}' '$LOG_FILE' | head -n 7"

run_adv_test "007" "Extract Referer Headers" \
  "awk -F'\"' '{print \$4}' '$LOG_FILE' | head -n 6"

run_adv_test "008" "Extract User-Agent Strings" \
  "awk -F'\"' '{print \$6}' '$LOG_FILE' | head -n 5"

run_adv_test "009" "Extract Full HTTP Request Line" \
  "awk -F'\"' '{print \$2}' '$LOG_FILE' | head -n 5"

run_adv_test "010" "Combine IP + Method + Status Code" \
  "awk '{print \$1, \$6, \$9}' '$LOG_FILE' | tr -d '\"' | head -n 5"

# ------------------------------------------------------------------------------
# 2. HTTP STATUS CODE ANALYSIS & FILTERING (TESTS #011 - #020)
# ------------------------------------------------------------------------------

run_adv_test "011" "Filter all HTTP 200 OK Requests" \
  "awk '\$9 == 200 {print \$7}' '$LOG_FILE' | head -n 5"

run_adv_test "012" "Filter all HTTP 404 Not Found Requests" \
  "awk '\$9 == 404 {print \$7}' '$LOG_FILE' | head -n 5"

run_adv_test "013" "Filter all HTTP 5xx Server Errors (500, 502)" \
  "awk '\$9 ~ /^5/ {print \$7, \$9}' '$LOG_FILE' | head -n 5"

run_adv_test "014" "Count Total HTTP 200 Requests" \
  "grep -c ' 200 ' '$LOG_FILE'"

run_adv_test "015" "Count Total HTTP 404 Requests" \
  "grep -c ' 404 ' '$LOG_FILE'"

run_adv_test "016" "Count Total HTTP 5xx Server Errors" \
  "awk '\$9 ~ /^5/ {count++} END {print count+0}' '$LOG_FILE'"

run_adv_test "017" "Status Code Frequency Distribution" \
  "awk '{print \$9}' '$LOG_FILE' | sort | uniq -c | sort -nr"

run_adv_test "018" "Filter Non-200 Requests" \
  "awk '\$9 != 200 {print \$7, \$9}' '$LOG_FILE' | head -n 6"

run_adv_test "019" "Calculate Success Ratio (HTTP 200 Count vs Total)" \
  "tot=\$(wc -l < '$LOG_FILE' | tr -d ' '); ok=\$(grep -c ' 200 ' '$LOG_FILE'); echo \"OK:\$ok Total:\$tot\""

run_adv_test "020" "Check if 503 Service Unavailable Occurred" \
  "grep -q ' 503 ' '$LOG_FILE'; echo \$?"

# ------------------------------------------------------------------------------
# 3. IP ADDRESS ANALYTICS & CLIENT TRACKING (TESTS #021 - #030)
# ------------------------------------------------------------------------------

run_adv_test "021" "Count Unique Client IP Addresses" \
  "awk '{print \$1}' '$LOG_FILE' | sort -u | wc -l | tr -d ' '"

run_adv_test "022" "Top 3 Most Active Client IPs" \
  "awk '{print \$1}' '$LOG_FILE' | sort | uniq -c | sort -nr | head -n 3"

run_adv_test "023" "Filter Requests from Local Subnet 192.168.1.x" \
  "grep '^192\.168\.1\.' '$LOG_FILE' | awk '{print \$1, \$7}' | head -n 5"

run_adv_test "024" "Filter Public Client IPs (Non-192.168)" \
  "grep -v '^192\.168\.' '$LOG_FILE' | awk '{print \$1}' | head -n 5"

run_adv_test "025" "Find IPs Generating HTTP 404 Errors" \
  "awk '\$9 == 404 {print \$1}' '$LOG_FILE' | sort | uniq -c | sort -nr"

run_adv_test "026" "Count Total Requests per Unique IP" \
  "awk '{print \$1}' '$LOG_FILE' | sort | uniq -c | head -n 5"

run_adv_test "027" "Detect IPs Probing Admin Endpoints" \
  "grep -E 'wp-admin|phpmyadmin' '$LOG_FILE' | awk '{print \$1, \$7}' | sort -u"

run_adv_test "028" "Sort Client IPs in Descending Frequency" \
  "awk '{print \$1}' '$LOG_FILE' | sort | uniq -c | sort -nr | head -n 4"

run_adv_test "029" "List IPs that Sent POST or DELETE Requests" \
  "awk '\$6 ~ /POST|DELETE/ {print \$1, \$6}' '$LOG_FILE' | tr -d '\"' | sort -u"

run_adv_test "030" "Calculate Total Bandwidth for IP 192.168.1.15" \
  "awk '\$1 == \"192.168.1.15\" {sum += \$10} END {print sum}' '$LOG_FILE'"

# ------------------------------------------------------------------------------
# 4. REQUEST METHOD & PROTOCOL ANALYSIS (TESTS #031 - #040)
# ------------------------------------------------------------------------------

run_adv_test "031" "Count Total GET Requests" \
  "grep -c '\"GET ' '$LOG_FILE'"

run_adv_test "032" "Count Total POST Requests" \
  "grep -c '\"POST ' '$LOG_FILE'"

run_adv_test "033" "Count Total DELETE Requests" \
  "grep -c '\"DELETE ' '$LOG_FILE'"

run_adv_test "034" "HTTP Method Frequency Distribution" \
  "awk '{print \$6}' '$LOG_FILE' | tr -d '\"' | sort | uniq -c | sort -nr"

run_adv_test "035" "Verify All Requests Use HTTP/1.1 Protocol" \
  "awk '{print \$8}' '$LOG_FILE' | tr -d '\"' | sort -u"

run_adv_test "036" "Filter Write Operations (POST and DELETE)" \
  "awk '\$6 ~ /POST|DELETE/ {print \$6, \$7, \$9}' '$LOG_FILE' | tr -d '\"' | head -n 5"

run_adv_test "037" "Compare GET vs Write Operation Counts" \
  "r=\$(grep -c '\"GET ' '$LOG_FILE'); w=\$(grep -E -c '\"POST|\"DELETE' '$LOG_FILE'); echo \"Read:\$r Write:\$w\""

run_adv_test "038" "Identify Endpoints Accessed via POST" \
  "awk '\$6 == \"\\\"POST\" {print \$7}' '$LOG_FILE' | sort -u"

run_adv_test "039" "Find Client IPs Using DELETE Method" \
  "awk '\$6 == \"\\\"DELETE\" {print \$1, \$7}' '$LOG_FILE' | sort -u"

run_adv_test "040" "Check if OPTIONS or PATCH Occurred" \
  "grep -E -q '\"OPTIONS|\"PATCH' '$LOG_FILE'; echo \$?"

# ------------------------------------------------------------------------------
# 5. BANDWIDTH & DATA TRANSFER AGGREGATION (TESTS #041 - #050)
# ------------------------------------------------------------------------------

run_adv_test "041" "Calculate Total Bytes Transferred Across All Logs" \
  "awk '{sum += \$10} END {print sum}' '$LOG_FILE'"

run_adv_test "042" "Find Largest Single Response Payload (Max Bytes)" \
  "awk '{if (\$10 > max) max = \$10} END {print max}' '$LOG_FILE'"

run_adv_test "043" "Find Smallest Non-Zero Payload (Min Bytes)" \
  "awk '\$10 > 0 {if (min == \"\" || \$10 < min) min = \$10} END {print min}' '$LOG_FILE'"

run_adv_test "044" "Calculate Average Payload Size in Bytes" \
  "awk '{sum += \$10; count++} END {print int(sum/count)}' '$LOG_FILE'"

run_adv_test "045" "Total Bandwidth Consumed by HTTP 200 OK" \
  "awk '\$9 == 200 {sum += \$10} END {print sum}' '$LOG_FILE'"

run_adv_test "046" "Total Bandwidth for Image Assets (.jpg, .png)" \
  "awk '\$7 ~ /\.(jpg|png)$/ {sum += \$10} END {print sum+0}' '$LOG_FILE'"

run_adv_test "047" "Bandwidth Consumed by Prometheus Scraper" \
  "grep 'Prometheus' '$LOG_FILE' | awk '{sum += \$10} END {print sum}'"

run_adv_test "048" "Filter Requests Exceeding 10 KB Payload" \
  "awk '\$10 > 10240 {print \$7, \$10}' '$LOG_FILE' | head -n 5"

run_adv_test "049" "Count Zero-Byte Responses" \
  "awk '\$10 == 0 {count++} END {print count+0}' '$LOG_FILE'"

run_adv_test "050" "Total Payload Transferred to Googlebot" \
  "grep 'Googlebot' '$LOG_FILE' | awk '{sum += \$10} END {print sum}'"

# ------------------------------------------------------------------------------
# 6. ENDPOINT POPULARITY & URL PATH ANALYTICS (TESTS #051 - #060)
# ------------------------------------------------------------------------------

run_adv_test "051" "Top 5 Most Frequently Requested URLs" \
  "awk '{print \$7}' '$LOG_FILE' | sort | uniq -c | sort -nr | head -n 5"

run_adv_test "052" "Count Requests to API Endpoints (/api/*)" \
  "grep -c ' /api/' '$LOG_FILE'"

run_adv_test "053" "Count Requests to Static Assets (css, js, images)" \
  "grep -E -c ' /css/| /js/| /images/' '$LOG_FILE'"

run_adv_test "054" "Strip Query Strings from Search URLs" \
  "awk '\$7 ~ /\\?/ {split(\$7, a, \"?\"); print a[1]}' '$LOG_FILE' | head -n 4"

run_adv_test "055" "Top 3 API Routes by Request Volume" \
  "awk '\$7 ~ /^\/api/ {print \$7}' '$LOG_FILE' | sort | uniq -c | sort -nr | head -n 3"

run_adv_test "056" "Find URLs Returning HTTP 500 Server Error" \
  "awk '\$9 == 500 {print \$7}' '$LOG_FILE' | sort -u"

run_adv_test "057" "Identify All Static Image Requests" \
  "awk '\$7 ~ /\.(jpg|png|gif|ico)$/ {print \$7}' '$LOG_FILE' | sort -u"

run_adv_test "058" "Count Total Unique URL Paths" \
  "awk '{print \$7}' '$LOG_FILE' | sort -u | wc -l | tr -d ' '"

run_adv_test "059" "Extract Search Query Parameters (q=...)" \
  "awk -F'[?&]' '/\?q=/ {for(i=1;i<=NF;i++) if (\$i ~ /^q=/) print \$i}' '$LOG_FILE' | head -n 3"

run_adv_test "060" "Filter Favicon and Robots.txt Traffic" \
  "grep -E 'favicon\.ico|robots\.txt' '$LOG_FILE' | awk '{print \$7, \$9}' | head -n 4"

# ------------------------------------------------------------------------------
# 7. TIME WINDOWING & HOURLY GROUPING (TESTS #061 - #070)
# ------------------------------------------------------------------------------

run_adv_test "061" "Filter Traffic for Date 09/Oct/2026" \
  "grep -c '09/Oct/2026' '$LOG_FILE'"

run_adv_test "062" "Group Requests by Hour" \
  "awk '{print \$4}' '$LOG_FILE' | cut -d: -f2 | sort | uniq -c"

run_adv_test "063" "Group Requests by Minute (HH:MM)" \
  "awk '{print \$4}' '$LOG_FILE' | cut -d: -f2,3 | sort | uniq -c | head -n 5"

run_adv_test "064" "Identify Peak Traffic Minute" \
  "awk '{print \$4}' '$LOG_FILE' | cut -d: -f2,3 | sort | uniq -c | sort -nr | head -n 1"

run_adv_test "065" "Extract Timestamps without Timezone Offset" \
  "awk '{print \$4}' '$LOG_FILE' | tr -d '[' | head -n 5"

run_adv_test "066" "Count Requests in First 30 Seconds of Each Minute" \
  "awk '{split(\$4, a, \":\"); sec=substr(a[4], 1, 2); if (sec < 30) count++} END {print count}' '$LOG_FILE'"

run_adv_test "067" "Find Timestamp of Earliest Log Entry" \
  "head -n 1 '$LOG_FILE' | awk '{print \$4}' | tr -d '['"

run_adv_test "068" "Find Timestamp of Latest Log Entry" \
  "tail -n 1 '$LOG_FILE' | awk '{print \$4}' | tr -d '['"

run_adv_test "069" "Count Total Logged Minutes" \
  "awk '{print \$4}' '$LOG_FILE' | cut -d: -f2,3 | sort -u | wc -l | tr -d ' '"

run_adv_test "070" "Filter Requests Occurring at Exact Minute :15" \
  "grep ':15:' '$LOG_FILE' | awk '{print \$7}' | head -n 4"

# ------------------------------------------------------------------------------
# 8. MULTI-STAGE ETL & DATA TRANSFORMATION (TESTS #071 - #080)
# ------------------------------------------------------------------------------

run_adv_test "071" "Transform Log Line to CSV Format (IP,Method,URI,Status,Bytes)" \
  "awk '{gsub(/\"/, \"\", \$6); print \$1\",\"\$6\",\"\$7\",\"\$9\",\"\$10}' '$LOG_FILE' | head -n 5"

run_adv_test "072" "Transform Log Line to TSV Format" \
  "awk -v OFS='\t' '{gsub(/\"/, \"\", \$6); print \$1, \$6, \$7, \$9, \$10}' '$LOG_FILE' | head -n 5"

run_adv_test "073" "ETL Pipeline: Extract 404s -> URL -> Sort -> Count" \
  "awk '\$9 == 404 {print \$7}' '$LOG_FILE' | sort | uniq -c | sort -nr | head -n 4"

run_adv_test "074" "ETL Pipeline: Sum Bandwidth by HTTP Method" \
  "awk '{m=\$6; gsub(/\"/, \"\", m); sum[m] += \$10} END {for(k in sum) print k, sum[k]}' '$LOG_FILE' | sort"

run_adv_test "075" "ETL Pipeline: Top User-Agents by Request Volume" \
  "awk -F'\"' '{print \$6}' '$LOG_FILE' | sort | uniq -c | sort -nr | head -n 4"

run_adv_test "076" "ETL Pipeline: Top Referrers Generating Traffic" \
  "awk -F'\"' '{print \$4}' '$LOG_FILE' | grep -v '^-' | sort | uniq -c | sort -nr | head -n 4"

run_adv_test "077" "Count Direct Traffic (No Referer Present)" \
  "awk -F'\"' '\$4 == \"-\" {count++} END {print count}' '$LOG_FILE'"

run_adv_test "078" "Extract Top Client for Highest Bandwidth Endpoint" \
  "awk '\$7 == \"/images/item101.jpg\" {print \$1}' '$LOG_FILE' | sort | uniq -c"

run_adv_test "079" "Generate Markdown Summary Header and Total Lines" \
  "echo \"| Metric | Value |\"; echo \"|---|---|\"; echo \"| Total Lines | \$(wc -l < '$LOG_FILE' | tr -d ' ') |\""

run_adv_test "080" "Transform Status Distribution into JSON Payload" \
  "awk '{count[\$9]++} END {printf \"{\"; for(s in count) printf \"\\\"%s\\\":%d,\", s, count[s]; print \"\\\"ok\\\":1}\"}' '$LOG_FILE'"

# ------------------------------------------------------------------------------
# 9. SECURITY AUDITS & SUSPICIOUS PROBES (TESTS #081 - #090)
# ------------------------------------------------------------------------------

run_adv_test "081" "Detect Directory Traversal and Sensitive File Probes" \
  "grep -E '\.sql|\.env|\.tar\.gz' '$LOG_FILE' | awk '{print \$1, \$7, \$9}'"

run_adv_test "082" "Detect WordPress Admin Scanner Bots" \
  "grep -E 'wp-login|wp-admin' '$LOG_FILE' | awk '{print \$1, \$7}' | sort -u"

run_adv_test "083" "Detect Database Admin Probes (phpMyAdmin/pma)" \
  "grep -E '/phpmyadmin/|/pma/' '$LOG_FILE' | awk '{print \$1, \$7}' | sort -u"

run_adv_test "084" "Identify Requests Using python-requests User-Agent" \
  "grep 'python-requests' '$LOG_FILE' | awk '{print \$1, \$7, \$9}' | head -n 3"

run_adv_test "085" "Identify Search Engine Crawler Requests (Googlebot)" \
  "grep 'Googlebot' '$LOG_FILE' | awk '{print \$1, \$7}' | sort -u"

run_adv_test "086" "Filter Requests Returning 403 Forbidden" \
  "awk '\$9 == 403 {print \$1, \$7}' '$LOG_FILE'"

run_adv_test "087" "Count Total Suspicious Scanner Probe Attempts" \
  "grep -E -c 'wp-|phpmyadmin|pma|\.sql|\.env' '$LOG_FILE'"

run_adv_test "088" "List Suspicious Attacker IPs with Probe Count" \
  "grep -E 'wp-|phpmyadmin|pma|\.sql|\.env' '$LOG_FILE' | awk '{print \$1}' | sort | uniq -c"

run_adv_test "089" "Generate Security Alert Line for 403 Probes" \
  "awk '\$9 == 403 {print \"ALERT: Forbidden probe on\", \$7, \"from\", \$1}' '$LOG_FILE' | head -n 2"

run_adv_test "090" "Verify No Successful Access to Admin Dumps (Must be 403/404)" \
  "awk '\$7 ~ /dump\.sql/ {print \$9}' '$LOG_FILE' | sort -u"

# ------------------------------------------------------------------------------
# 10. ADVANCED IN-SHELL ANALYTICS & RECURSIVE LOOPS (TESTS #091 - #100)
# ------------------------------------------------------------------------------

run_adv_test "091" "Pure Shell While-Read Loop: Count 200 vs 404 in Shell" \
  "c200=0; c404=0; while read -r ip d1 d2 dt m u p s b rest; do if [ \"\$s\" = \"200\" ]; then c200=\$((c200+1)); elif [ \"\$s\" = \"404\" ]; then c404=\$((c404+1)); fi; done < '$LOG_FILE'; echo \"200:\$c200 404:\$c404\""

run_adv_test "092" "Pure Shell Running Total of Bandwidth" \
  "tot=0; cnt=0; while read -r ip d1 d2 dt m u p s b rest; do if [ \"\$s\" = \"200\" ]; then tot=\$((tot + b)); cnt=\$((cnt+1)); fi; if [ \$cnt -ge 10 ]; then break; fi; done < '$LOG_FILE'; echo \"RunningSum10:\$tot\""

run_adv_test "093" "Shell Parameter Expansion: Extract Endpoint Category" \
  "while read -r ip d1 d2 dt m u p s b rest; do cat=\"\${u#/*/}\"; echo \"\${cat%%/*}\"; break; done < '$LOG_FILE'"

run_adv_test "094" "Shell Loop Filter: Find Requests Over 40,000 Bytes" \
  "while read -r ip d1 d2 dt m u p s b rest; do if [ \"\$b\" -gt 40000 ] 2>/dev/null; then echo \"\$ip -> \$b bytes\"; fi; done < '$LOG_FILE' | head -n 3"

run_adv_test "095" "Generate JSON Array of Unique IPs in Shell" \
  "ips=\$(awk '{print \$1}' '$LOG_FILE' | sort -u | head -n 3); out=\"[\"; for ip in \$ips; do out=\"\$out\\\"\$ip\\\",\"; done; echo \"\${out%,}]\""

run_adv_test "096" "Categorize Response Sizes into Buckets (<1KB, 1-10KB, >10KB)" \
  "awk '{if (\$10 < 1024) s++; else if (\$10 <= 10240) m++; else l++} END {print \"Small:\"s, \"Med:\"m, \"Large:\"l}' '$LOG_FILE'"

run_adv_test "097" "Shell-Based Deduplication of Consecutive IPs" \
  "prev=\"\"; count=0; while read -r ip rest; do if [ \"\$ip\" != \"\$prev\" ]; then count=\$((count+1)); prev=\"\$ip\"; fi; done < '$LOG_FILE'; echo \"Changes:\$count\""

run_adv_test "098" "Construct Structured CSV Header + Data Stream" \
  "echo \"ip,uri,status\"; awk '{gsub(/\"/, \"\", \$6); print \$1\",\"\$7\",\"\$9}' '$LOG_FILE' | head -n 3"

run_adv_test "099" "Calculate Error Rate Percentage (4xx+5xx / Total * 100)" \
  "tot=\$(wc -l < '$LOG_FILE' | tr -d ' '); err=\$(awk '\$9 >= 400 {count++} END {print count+0}' '$LOG_FILE'); echo \"\$(( err * 100 / tot ))%\""

run_adv_test "100" "Complete ETL Pipeline Integrity Check (Non-zero Output & Zero Exit Code)" \
  "awk '\$9 == 200 {sum += \$10; cnt++} END {print cnt, sum}' '$LOG_FILE' >/dev/null; echo \$?"

# ------------------------------------------------------------------------------
# FINAL MODULE SUMMARY REPORT
# ------------------------------------------------------------------------------

echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}📊 MODULE 01 EXECUTION SUMMARY (LOG ANALYTICS & ETL PIPELINES):${NC}"
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
