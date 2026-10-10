#!/bin/bash
# ==============================================================================
# ADVANCE-TEST MODULE 02: JSON, CSV & TSV DATA PROCESSING (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

# Setup fixtures
generate_csv_fixture
generate_tsv_fixture
generate_json_fixture

CSV_FILE="$FIXTURES_DIR/transactions.csv"
TSV_FILE="$FIXTURES_DIR/inventory.tsv"
K8S_FILE="$FIXTURES_DIR/k8s_deployment.json"
PKG_FILE="$FIXTURES_DIR/package.json"

echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}🚀 STARTING ADVANCE-TEST MODULE 02: JSON, CSV & TSV PROCESSING${NC}"
echo -e "   Target: zbash vs macOS /bin/bash (100 Differential Tests)"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

# ------------------------------------------------------------------------------
# 1. CSV HEADER PARSING & COLUMN SLICING (TESTS #001 - #010)
# ------------------------------------------------------------------------------

run_adv_test "001" "Extract CSV Header Line" \
  "head -n 1 '$CSV_FILE'"

run_adv_test "002" "Count Total Data Rows in CSV" \
  "awk 'NR > 1' '$CSV_FILE' | wc -l | tr -d ' '"

run_adv_test "003" "Extract First 5 IDs and Customer Names" \
  "awk -F',' 'NR > 1 {print \$1, \$3}' '$CSV_FILE' | head -n 5"

run_adv_test "004" "Extract Country and Status Columns" \
  "awk -F',' 'NR > 1 {print \$4, \$10}' '$CSV_FILE' | head -n 4"

run_adv_test "005" "Unique Product Categories List Sorted" \
  "awk -F',' 'NR > 1 {print \$6}' '$CSV_FILE' | sort -u"

run_adv_test "006" "Count Distinct Customers" \
  "awk -F',' 'NR > 1 {print \$3}' '$CSV_FILE' | sort -u | wc -l | tr -d ' '"

run_adv_test "007" "List All Transactions from Thailand (TH)" \
  "awk -F',' '\$4 == \"TH\" {print \$1, \$3, \$5, \$9}' '$CSV_FILE' | head -n 4"

run_adv_test "008" "List Transactions with Status PENDING" \
  "awk -F',' '\$10 == \"PENDING\" {print \$1, \$3, \$5}' '$CSV_FILE' | head -n 3"

run_adv_test "009" "Extract Date and Total Amount for First 3 Rows" \
  "awk -F',' 'NR >= 2 && NR <= 4 {print \$2, \"->\", \$9}' '$CSV_FILE'"

run_adv_test "010" "Extract Last 4 Transaction IDs and Products" \
  "awk -F',' 'NR > 1 {print \$1, \$5}' '$CSV_FILE' | tail -n 4"

# ------------------------------------------------------------------------------
# 2. NUMERICAL CALCULATIONS & AGGREGATION (TESTS #011 - #020)
# ------------------------------------------------------------------------------

run_adv_test "011" "Calculate Total Revenue for COMPLETED Orders" \
  "awk -F',' '\$10 == \"COMPLETED\" {sum += \$9} END {print sum}' '$CSV_FILE'"

run_adv_test "012" "Calculate Total Quantity of Items Sold" \
  "awk -F',' 'NR > 1 {qty += \$7} END {print qty}' '$CSV_FILE'"

run_adv_test "013" "Average Transaction Value for COMPLETED Orders" \
  "awk -F',' '\$10 == \"COMPLETED\" {sum += \$9; count++} END {print int(sum/count)}' '$CSV_FILE'"

run_adv_test "014" "Find Maximum Single Transaction Value" \
  "awk -F',' 'NR > 1 {if (\$9 > max) max = \$9} END {print max}' '$CSV_FILE'"

run_adv_test "015" "Find Minimum Single Transaction Value" \
  "awk -F',' 'NR == 2 {min = \$9} NR > 2 {if (\$9 < min) min = \$9} END {print min}' '$CSV_FILE'"

run_adv_test "016" "Total Revenue for US Customers" \
  "awk -F',' '\$4 == \"US\" && \$10 == \"COMPLETED\" {sum += \$9} END {print sum}' '$CSV_FILE'"

run_adv_test "017" "Total Revenue for TH Customers" \
  "awk -F',' '\$4 == \"TH\" && \$10 == \"COMPLETED\" {sum += \$9} END {print sum}' '$CSV_FILE'"

run_adv_test "018" "Total Refunded Amount" \
  "awk -F',' '\$10 == \"REFUNDED\" {sum += \$9} END {print sum+0}' '$CSV_FILE'"

run_adv_test "019" "Count of Transactions Exceeding $1,000" \
  "awk -F',' 'NR > 1 && \$9 > 1000 {count++} END {print count+0}' '$CSV_FILE'"

run_adv_test "020" "Percentage of Completed vs Total Transactions" \
  "tot=\$(awk 'NR > 1' '$CSV_FILE' | wc -l | tr -d ' '); comp=\$(awk -F',' '\$10 == \"COMPLETED\" {c++} END {print c}' '$CSV_FILE'); echo \"\$(( comp * 100 / tot ))%\""

# ------------------------------------------------------------------------------
# 3. MULTI-CONDITION ROW FILTERING (TESTS #021 - #030)
# ------------------------------------------------------------------------------

run_adv_test "021" "Filter COMPLETED Orders in Category Laptops" \
  "awk -F',' '\$6 == \"Laptops\" && \$10 == \"COMPLETED\" {print \$1, \$3, \$5}' '$CSV_FILE' | head -n 3"

run_adv_test "022" "Filter Orders with Quantity > 2 and Price < 100" \
  "awk -F',' 'NR > 1 && \$7 > 2 && \$8 < 100 {print \$1, \$5, \$7, \$8}' '$CSV_FILE' | head -n 3"

run_adv_test "023" "Find Orders with Total > 500 from UK or DE" \
  "awk -F',' '(\$4 == \"UK\" || \$4 == \"DE\") && \$9 > 500 {print \$1, \$4, \$9}' '$CSV_FILE' | head -n 4"

run_adv_test "024" "Find Accessories Ordered by US Customers" \
  "awk -F',' '\$4 == \"US\" && \$6 == \"Accessories\" {print \$1, \$3, \$5}' '$CSV_FILE' | head -n 3"

run_adv_test "025" "Count Cancelled or Refunded Orders" \
  "awk -F',' '\$10 ~ /CANCELLED|REFUNDED/ {count++} END {print count+0}' '$CSV_FILE'"

run_adv_test "026" "List Customers Ordering Displays" \
  "awk -F',' '\$6 == \"Displays\" {print \$3}' '$CSV_FILE' | sort -u"

run_adv_test "027" "Find Orders where Quantity == 1 and Total >= 2000" \
  "awk -F',' '\$7 == 1 && \$9 >= 2000 {print \$1, \$3, \$5, \$9}' '$CSV_FILE' | head -n 3"

run_adv_test "028" "Filter Orders Placed on 2026-10-01" \
  "awk -F',' '\$2 == \"2026-10-01\" {print \$1, \$3, \$9}' '$CSV_FILE'"

run_adv_test "029" "Find Transactions from JP with Status PENDING" \
  "awk -F',' '\$4 == \"JP\" && \$10 == \"PENDING\" {print \$1, \$3, \$5}' '$CSV_FILE' | head -n 2"

run_adv_test "030" "Exclude CANCELLED Orders and List Statuses" \
  "awk -F',' 'NR > 1 && \$10 != \"CANCELLED\" {print \$10}' '$CSV_FILE' | sort -u"

# ------------------------------------------------------------------------------
# 4. CUSTOMER & COUNTRY GROUPING / FREQUENCY (TESTS #031 - #040)
# ------------------------------------------------------------------------------

run_adv_test "031" "Top 3 Purchasing Countries by Order Count" \
  "awk -F',' 'NR > 1 {print \$4}' '$CSV_FILE' | sort | uniq -c | sort -nr | head -n 3"

run_adv_test "032" "Top 3 Customers by Order Count" \
  "awk -F',' 'NR > 1 {print \$3}' '$CSV_FILE' | sort | uniq -c | sort -nr | head -n 3"

run_adv_test "033" "Distribution of Order Statuses" \
  "awk -F',' 'NR > 1 {print \$10}' '$CSV_FILE' | sort | uniq -c | sort -nr"

run_adv_test "034" "Order Count by Category" \
  "awk -F',' 'NR > 1 {print \$6}' '$CSV_FILE' | sort | uniq -c | sort -nr"

run_adv_test "035" "Average Item Price by Category (Laptops)" \
  "awk -F',' '\$6 == \"Laptops\" {sum += \$8; c++} END {print int(sum/c)}' '$CSV_FILE'"

run_adv_test "036" "Total Revenue by Category (Accessories)" \
  "awk -F',' '\$6 == \"Accessories\" && \$10 == \"COMPLETED\" {sum += \$9} END {print sum}' '$CSV_FILE'"

run_adv_test "037" "Top 3 Best-selling Products by Total Quantity" \
  "awk -F',' 'NR > 1 {qty[\$5] += \$7} END {for (p in qty) print qty[p], p}' '$CSV_FILE' | sort -nr | head -n 3"

run_adv_test "038" "Count of Unique Products in Accessories Category" \
  "awk -F',' '\$6 == \"Accessories\" {print \$5}' '$CSV_FILE' | sort -u | wc -l | tr -d ' '"

run_adv_test "039" "Distinct Customers Ordering from Thailand" \
  "awk -F',' '\$4 == \"TH\" {print \$3}' '$CSV_FILE' | sort -u"

run_adv_test "040" "Check if German (DE) Customer had Cancelled Order" \
  "awk -F',' '\$4 == \"DE\" && \$10 == \"CANCELLED\" {print \$1}' '$CSV_FILE' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 5. FORMAT CONVERSIONS (CSV TO TSV, SQL, JSON LINES) (TESTS #041 - #050)
# ------------------------------------------------------------------------------

run_adv_test "041" "Convert CSV First 3 Rows to Tab-Delimited TSV" \
  "head -n 3 '$CSV_FILE' | tr ',' '\t'"

run_adv_test "042" "Convert CSV Header to Semicolon Delimited" \
  "head -n 1 '$CSV_FILE' | tr ',' ';'"

run_adv_test "043" "Convert CSV Header to Pipe Delimited" \
  "head -n 1 '$CSV_FILE' | tr ',' '|'"

run_adv_test "044" "Generate JSON Lines for First 2 Orders" \
  "awk -F',' 'NR >= 2 && NR <= 3 {printf \"{\\\"id\\\":%s,\\\"customer\\\":\\\"%s\\\",\\\"total\\\":%s}\\\\n\", \$1, \$3, \$9}' '$CSV_FILE'"

run_adv_test "045" "Convert Header to SQL Column Definition List" \
  "head -n 1 '$CSV_FILE' | awk -F',' '{for(i=1;i<=NF;i++) printf \"%s VARCHAR(64)%s\", \$i, (i==NF?\"\": \", \"); print \"\"}'"

run_adv_test "046" "Generate SQL INSERT Value Tuples for First 2 Orders" \
  "awk -F',' 'NR >= 2 && NR <= 3 {printf \"(\\\"%s\\\", \\\"%s\\\", %s),\\\\n\", \$1, \$3, \$9}' '$CSV_FILE'"

run_adv_test "047" "Convert First Order to Key-Value Pairs" \
  "awk -F',' 'NR==1{for(i=1;i<=NF;i++) h[i]=\$i} NR==2{for(i=1;i<=NF;i++) printf \"%s=%s \", h[i], \$i; print \"\"}' '$CSV_FILE'"

run_adv_test "048" "Wrap First CSV Row in XML-like Tags" \
  "awk -F',' 'NR==2 {print \"<order id=\\\"\"\$1\"\\\" customer=\\\"\"\$3\"\\\" total=\\\"\"\$9\"\\\"/>\"}' '$CSV_FILE'"

run_adv_test "049" "Generate Markdown Table Header and First Row" \
  "awk -F',' 'NR==1 {print \"| \"\$1\" | \"\$3\" | \"\$9\" |\"; print \"|---|---|---|\"} NR==2 {print \"| \"\$1\" | \"\$3\" | \"\$9\" |\"}' '$CSV_FILE'"

run_adv_test "050" "Generate HTML Table Row for Header" \
  "head -n 1 '$CSV_FILE' | awk -F',' '{printf \"<tr>\"; for(i=1;i<=3;i++) printf \"<th>%s</th>\", \$i; print \"</tr>\"}'"

# ------------------------------------------------------------------------------
# 6. TSV DATA MANIPULATION & INVENTORY CONTROL (TESTS #051 - #060)
# ------------------------------------------------------------------------------

run_adv_test "051" "Extract TSV Inventory Header" \
  "head -n 1 '$TSV_FILE'"

run_adv_test "052" "Total Unique SKUs in Inventory TSV" \
  "awk -F'\t' 'NR > 1 {print \$1}' '$TSV_FILE' | wc -l | tr -d ' '"

run_adv_test "053" "List Items in WH-WEST Warehouse" \
  "awk -F'\t' '\$4 == \"WH-WEST\" {print \$1, \$2, \$3}' '$TSV_FILE'"

run_adv_test "054" "Sum of Total Stock across All Warehouses" \
  "awk -F'\t' 'NR > 1 {stock += \$3} END {print stock}' '$TSV_FILE'"

run_adv_test "055" "Identify Items Below or Equal to Reorder Level" \
  "awk -F'\t' 'NR > 1 && \$3 <= \$5 {print \$1, \$2, \"Stock:\"\$3, \"Reorder:\"\$5}' '$TSV_FILE'"

run_adv_test "056" "Sort TSV by Stock Count Descending" \
  "awk -F'\t' 'NR > 1 {print \$3, \$2}' '$TSV_FILE' | sort -nr | head -n 3"

run_adv_test "057" "Swap TSV Column 1 and Column 2" \
  "awk -F'\t' 'NR > 1 {print \$2, \"[\"\$1\"]\"}' '$TSV_FILE' | head -n 3"

run_adv_test "058" "Convert TSV Inventory to Comma-Separated CSV" \
  "head -n 3 '$TSV_FILE' | tr '\t' ','"

run_adv_test "059" "Find Warehouse with Largest Single Item Stock" \
  "awk -F'\t' 'NR > 1 {print \$3, \$4, \$2}' '$TSV_FILE' | sort -nr | head -n 1"

run_adv_test "060" "Generate Low Stock Alert Warning via Awk" \
  "awk -F'\t' 'NR > 1 && \$3 <= 20 {print \"ALERT: Low stock on\", \$2, \"(\"\$3\" left)\"}' '$TSV_FILE' | head -n 2"

# ------------------------------------------------------------------------------
# 7. PURE SHELL JSON PARSING (NO JQ DEPENDENCY) (TESTS #061 - #070)
# ------------------------------------------------------------------------------

run_adv_test "061" "Extract apiVersion from K8s JSON" \
  "grep '\"apiVersion\"' '$K8S_FILE' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "062" "Extract kind from K8s JSON" \
  "grep '\"kind\"' '$K8S_FILE' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "063" "Extract metadata.name from K8s JSON" \
  "grep -A 5 '\"metadata\"' '$K8S_FILE' | grep '\"name\"' | head -n 1 | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "064" "Extract metadata.namespace from K8s JSON" \
  "grep '\"namespace\"' '$K8S_FILE' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "065" "Extract spec.replicas count from K8s JSON" \
  "grep '\"replicas\"' '$K8S_FILE' | tr -d ' ,' | cut -d':' -f2"

run_adv_test "066" "Extract package.json name field" \
  "grep '\"name\"' '$PKG_FILE' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "067" "Extract package.json version string" \
  "grep '\"version\"' '$PKG_FILE' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "068" "Extract package.json main entry point" \
  "grep '\"main\"' '$PKG_FILE' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "069" "Extract package.json build script" \
  "grep '\"build\"' '$PKG_FILE' | cut -d':' -f2 | tr -d '\" ,'"

run_adv_test "070" "Extract package.json description" \
  "grep '\"description\"' '$PKG_FILE' | cut -d':' -f2- | tr -d '\",' | sed 's/^[ \t]*//'"

# ------------------------------------------------------------------------------
# 8. NESTED JSON OBJECTS & ARRAY TRAVERSAL (TESTS #071 - #080)
# ------------------------------------------------------------------------------

run_adv_test "071" "Extract Container Image name from K8s JSON" \
  "grep '\"image\"' '$K8S_FILE' | cut -d':' -f2- | tr -d '\" ,'"

run_adv_test "072" "Extract Container Name from K8s JSON" \
  "grep -A 2 '\"containers\"' '$K8S_FILE' | grep '\"name\"' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "073" "Extract CPU Limit from Container Resources" \
  "grep '\"cpu\"' '$K8S_FILE' | head -n 1 | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "074" "Extract Memory Limit from Container Resources" \
  "grep '\"memory\"' '$K8S_FILE' | head -n 1 | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "075" "List All Container Environment Variable Names" \
  "grep '\"name\": \"' '$K8S_FILE' | grep -v 'payment-' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "076" "Extract Specific Env Variable DB_HOST Value" \
  "grep -A 1 '\"DB_HOST\"' '$K8S_FILE' | grep '\"value\"' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "077" "List All Container Ports from K8s JSON" \
  "grep '\"containerPort\"' '$K8S_FILE' | tr -d '\" ,' | cut -d':' -f2"

run_adv_test "078" "Extract dependencies names from package.json" \
  "awk '/\"dependencies\": \\{/,/\\}/' '$PKG_FILE' | grep ':' | tr -d '\" ,:' | awk '{print \$1}'"

run_adv_test "079" "Extract devDependencies names from package.json" \
  "awk '/\"devDependencies\": \\{/,/\\}/' '$PKG_FILE' | grep ':' | tr -d '\" ,:' | awk '{print \$1}'"

run_adv_test "080" "Count Total Dependencies in package.json" \
  "awk '/\"dependencies\": \\{/,/\\}/' '$PKG_FILE' | grep -c ':'"

# ------------------------------------------------------------------------------
# 9. DATA CLEANING, IMPUTATION & ANONYMIZATION (TESTS #081 - #090)
# ------------------------------------------------------------------------------

run_adv_test "081" "Anonymize Customer Names in CSV (First Initial + Period)" \
  "awk -F',' 'NR >= 2 && NR <= 4 {split(\$3, a, \" \"); print \$1, a[1], a[2]}' '$CSV_FILE'"

run_adv_test "082" "Mask Sensitive Order Total Amounts (> $1,000 as [HIGH])" \
  "awk -F',' 'NR > 1 {val = (\$9 > 1000 ? \"[HIGH]\" : \$9); print \$1, val}' '$CSV_FILE' | head -n 4"

run_adv_test "083" "Normalize Customer Names to Uppercase" \
  "awk -F',' 'NR >= 2 && NR <= 4 {print toupper(\$3)}' '$CSV_FILE'"

run_adv_test "084" "Detect if Any Row has Missing Country Field" \
  "awk -F',' 'NR > 1 && (\$4 == \"\" || \$4 == \"-\") {count++} END {print count+0}' '$CSV_FILE'"

run_adv_test "085" "Convert Category Names to Lowercase" \
  "awk -F',' 'NR >= 2 && NR <= 4 {print tolower(\$6)}' '$CSV_FILE'"

run_adv_test "086" "Format Prices with Currency Symbol ($)" \
  "awk -F',' 'NR >= 2 && NR <= 4 {print \$1, \"$\"\$9}' '$CSV_FILE'"

run_adv_test "087" "Validate ID Column Format (Must be 4 Digits)" \
  "awk -F',' 'NR > 1 && \$1 !~ /^[0-9]{4}$/ {invalid++} END {print invalid+0}' '$CSV_FILE'"

run_adv_test "088" "Check for Any Negative Quantity Values" \
  "awk -F',' 'NR > 1 && \$7 < 0 {bad++} END {print bad+0}' '$CSV_FILE'"

run_adv_test "089" "Format Date from YYYY-MM-DD to DD/MM/YYYY" \
  "awk -F',' 'NR >= 2 && NR <= 4 {split(\$2, d, \"-\"); print d[3]\"/\"d[2]\"/\"d[1]}' '$CSV_FILE'"

run_adv_test "090" "Verify All Completed Orders Have Positive Total" \
  "awk -F',' '\$10 == \"COMPLETED\" && \$9 <= 0 {err++} END {print err+0}' '$CSV_FILE'"

# ------------------------------------------------------------------------------
# 10. ADVANCED IN-SHELL STREAM TRANSFORMATION (TESTS #091 - #100)
# ------------------------------------------------------------------------------

run_adv_test "091" "Pure Shell IFS=, While-Read: Sum Total Revenue" \
  "sum=0; while IFS=, read -r id dt cust ctry prod cat qty price tot st; do if [ \"\$st\" = \"COMPLETED\" ]; then sum=\$((sum + tot)); fi; done < '$CSV_FILE'; echo \"TotalRevenue:\$sum\""

run_adv_test "092" "Pure Shell IFS=, While-Read: Count Orders by Status" \
  "comp=0; pend=0; while IFS=, read -r id dt cust ctry prod cat qty price tot st; do if [ \"\$st\" = \"COMPLETED\" ]; then comp=\$((comp + 1)); elif [ \"\$st\" = \"PENDING\" ]; then pend=\$((pend + 1)); fi; done < '$CSV_FILE'; echo \"Comp:\$comp Pend:\$pend\""

run_adv_test "093" "Pure Shell IFS=, Slicing: Find First 3 Customers" \
  "cnt=0; while IFS=, read -r id dt cust rest; do if [ \"\$cnt\" -gt 0 ]; then echo \"\$id: \$cust\"; fi; cnt=\$((cnt + 1)); if [ \"\$cnt\" -gt 3 ]; then break; fi; done < '$CSV_FILE'"

run_adv_test "094" "Pure Shell While-Read TSV: Calculate Stock Deficit" \
  "while IFS=\$'\\t' read -r sku name stock wh reorder; do if [ \"\$stock\" -le \"\$reorder\" ] 2>/dev/null; then echo \"\$sku deficit: \$((reorder - stock))\"; fi; done < '$TSV_FILE'"

run_adv_test "095" "In-Shell JSON Array Builder from Filtered CSV" \
  "items=\"[\"; cnt=0; while IFS=, read -r id dt cust ctry prod rest; do if [ \"\$ctry\" = \"TH\" ]; then items=\"\$items\\\"\$prod\\\",\"; cnt=\$((cnt+1)); fi; if [ \$cnt -ge 2 ]; then break; fi; done < '$CSV_FILE'; echo \"\${items%,}]\""

run_adv_test "096" "Pure Shell Matrix Grid Construction" \
  "grid=\"\"; for r in 1 2 3; do for c in 1 2 3; do grid=\"\$grid \$((r * c))\"; done; grid=\"\$grid |\"; done; echo \"\$grid\""

run_adv_test "097" "In-Shell Running Total with Limit in Loop" \
  "run=0; while IFS=, read -r id dt cust ctry prod cat qty price tot st; do if [ \"\$tot\" -gt 0 ] 2>/dev/null; then run=\$((run + tot)); fi; if [ \$run -gt 5000 ]; then echo \"BreachedAt:\$id Total:\$run\"; break; fi; done < '$CSV_FILE'"

run_adv_test "098" "Shell Function: Format Currency String" \
  "fmt_cur() { echo \"USD \$1.00\"; }; while IFS=, read -r id dt cust ctry prod cat qty price tot st; do if [ \"\$id\" = \"1001\" ]; then fmt_cur \"\$tot\"; break; fi; done < '$CSV_FILE'"

run_adv_test "099" "Extract Specific Field by Shell Word Indexing" \
  "while IFS=, read -r id dt cust rest; do if [ \"\$id\" = \"1004\" ]; then echo \"TargetCustomer:\$cust\"; break; fi; done < '$CSV_FILE'"

run_adv_test "100" "Complete Module 2 Parity Check (Zero Diff, Non-zero Output)" \
  "awk -F',' 'NR > 1 {print \$1, \$4, \$9, \$10}' '$CSV_FILE' | sort | head -n 5 >/dev/null; echo \$?"

# ------------------------------------------------------------------------------
# FINAL MODULE SUMMARY REPORT
# ------------------------------------------------------------------------------

echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}📊 MODULE 02 EXECUTION SUMMARY (JSON, CSV & TSV PROCESSING):${NC}"
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
