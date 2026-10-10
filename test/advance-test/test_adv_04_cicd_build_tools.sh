#!/bin/bash
# ==============================================================================
# ADVANCE-TEST MODULE 04: CI/CD BUILD TOOLS & PACKAGING (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

# Setup fixture
generate_cicd_fixture
CICD_DIR="$FIXTURES_DIR/cicd_pipeline"

echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}🚀 STARTING ADVANCE-TEST MODULE 04: CI/CD BUILD TOOLS & PACKAGING${NC}"
echo -e "   Target: zbash vs macOS /bin/bash (100 Differential Tests)"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

# ------------------------------------------------------------------------------
# 1. MATRIX BUILD RUNNER SIMULATION (TESTS #001 - #010)
# ------------------------------------------------------------------------------

run_adv_test "001" "Count Total Matrix Combinations" \
  "wc -l < '$CICD_DIR/matrix.txt' | tr -d ' '"

run_adv_test "002" "Filter Matrix Targets for Darwin OS" \
  "grep '^darwin:' '$CICD_DIR/matrix.txt'"

run_adv_test "003" "Filter Matrix Targets for Linux OS" \
  "grep '^linux:' '$CICD_DIR/matrix.txt'"

run_adv_test "004" "Extract All Unique Compilers in Matrix" \
  "cut -d':' -f3 '$CICD_DIR/matrix.txt' | sort -u"

run_adv_test "005" "Extract All Target Architectures in Matrix" \
  "cut -d':' -f2 '$CICD_DIR/matrix.txt' | sort -u"

run_adv_test "006" "Count ARM64 Targets in Matrix" \
  "grep -c ':arm64:' '$CICD_DIR/matrix.txt'"

run_adv_test "007" "Construct Matrix Target Tuple List" \
  "awk -F':' '{print \$1\"-\"\$2\"-\"\$3}' '$CICD_DIR/matrix.txt'"

run_adv_test "008" "Check If darwin:arm64:clang Exists in Matrix" \
  "grep -c '^darwin:arm64:clang\$' '$CICD_DIR/matrix.txt'"

run_adv_test "009" "Generate JSON Matrix Array for GitHub Actions" \
  "arr=\"[\"; while IFS=: read -r os arch cc; do arr=\"\$arr{\\\"os\\\":\\\"\$os\\\",\\\"arch\\\":\\\"\$arch\\\"},\"; done < '$CICD_DIR/matrix.txt'; echo \"\${arr%,}]\""

run_adv_test "010" "Formatted Matrix Table Output" \
  "awk -F':' '{printf \"%-8s %-8s %-8s\\\\n\", \$1, \$2, \$3}' '$CICD_DIR/matrix.txt'"

# ------------------------------------------------------------------------------
# 2. COMPILER FLAG INHERITANCE & CASCADING OPTIONS (TESTS #011 - #020)
# ------------------------------------------------------------------------------

run_adv_test "011" "Base CFLAGS Definition in Shell" \
  "cflags='-Wall -Wextra'; echo \"\$cflags\""

run_adv_test "012" "Append Architecture Optimization Flag" \
  "cflags='-Wall -Wextra'; cflags=\"\$cflags -march=armv8-a\"; echo \"\$cflags\""

run_adv_test "013" "Append Release Mode Optimization (-O3 -flto)" \
  "cflags='-Wall -Wextra'; cflags=\"\$cflags -O3 -flto\"; echo \"\$cflags\""

run_adv_test "014" "Check If -Wall is Present in CFLAGS" \
  "cflags='-Wall -Wextra -O3'; echo \"\$cflags\" | grep -q -- '-Wall'; echo \$?"

run_adv_test "015" "Strip Debug Flag (-g) via Parameter Expansion" \
  "cflags='-Wall -g -O3'; echo \"\${cflags//-g /}\""

run_adv_test "016" "Cascading Include Directories (-Iinclude -Isrc)" \
  "inc=''; for d in include src; do inc=\"\$inc -I\$d\"; done; echo \"\${inc# }\""

run_adv_test "017" "Construct LDFLAGS with Strip Options" \
  "ldflags='-pthread'; os='darwin'; if [ \"\$os\" = \"darwin\" ]; then ldflags=\"\$ldflags -Wl,-dead_strip\"; fi; echo \"\$ldflags\""

run_adv_test "018" "Override Compiler CC from Default to clang" \
  "CC=\${CC:-gcc}; CC='clang'; echo \"Using:\$CC\""

run_adv_test "019" "Extract Target Arch from Triple" \
  "triple='aarch64-apple-darwin23'; arch=\"\${triple%%-*}\"; echo \"\$arch\""

run_adv_test "020" "Build Configuration Summary Banner" \
  "mode='RELEASE'; arch='arm64'; echo \"=== BUILD MODE:\$mode ARCH:\$arch ===\""

# ------------------------------------------------------------------------------
# 3. ARTIFACT PACKAGING & COMPRESSION (TESTS #021 - #030)
# ------------------------------------------------------------------------------

run_adv_test "021" "Create Tarball Archive of Build Artifacts" \
  "cd '$CICD_DIR' && tar -czf release.tar.gz bin/gateway dist/bundle.js && [ -f release.tar.gz ]; echo \$?"

run_adv_test "022" "List Files Inside Tarball Table of Contents" \
  "cd '$CICD_DIR' && tar -tzf release.tar.gz | sort"

run_adv_test "023" "Verify Tarball Contains bin/gateway" \
  "cd '$CICD_DIR' && tar -tzf release.tar.gz | grep -c 'bin/gateway'"

run_adv_test "024" "Verify Tarball Contains dist/bundle.js" \
  "cd '$CICD_DIR' && tar -tzf release.tar.gz | grep -c 'dist/bundle.js'"

run_adv_test "025" "Calculate SHA256 Hash of gateway Binary" \
  "cd '$CICD_DIR' && (sha256sum bin/gateway 2>/dev/null || shasum -a 256 bin/gateway) | awk '{print \$1}'"

run_adv_test "026" "Extract Size of Compressed Tarball > 0" \
  "cd '$CICD_DIR' && sz=\$(wc -c < release.tar.gz | tr -d ' '); [ \"\$sz\" -gt 50 ]; echo \$?"

run_adv_test "027" "Test Gzip Compression Integrity (-t)" \
  "cd '$CICD_DIR' && gzip -t release.tar.gz; echo \$?"

run_adv_test "028" "Check Total Files Packaged in Archive" \
  "cd '$CICD_DIR' && tar -tzf release.tar.gz | wc -l | tr -d ' '"

run_adv_test "029" "Extract Version String from C Source" \
  "grep 'version' '$CICD_DIR/src/version.c' | cut -d'\"' -f2"

run_adv_test "030" "Clean Up Created Tarball" \
  "cd '$CICD_DIR' && rm -f release.tar.gz && [ ! -f release.tar.gz ]; echo \$?"

# ------------------------------------------------------------------------------
# 4. DOTENV CONFIGURATION CASCADING (TESTS #031 - #040)
# ------------------------------------------------------------------------------

run_adv_test "031" "Read APP_NAME from .env" \
  "grep '^APP_NAME=' '$CICD_DIR/.env' | cut -d'=' -f2"

run_adv_test "032" "Read Base APP_PORT from .env" \
  "grep '^APP_PORT=' '$CICD_DIR/.env' | cut -d'=' -f2"

run_adv_test "033" "Read Overridden APP_PORT from .env.local" \
  "grep '^APP_PORT=' '$CICD_DIR/.env.local' | cut -d'=' -f2"

run_adv_test "034" "Cascading Resolution of APP_PORT (.env.local over .env)" \
  "port=\$(grep '^APP_PORT=' '$CICD_DIR/.env' | cut -d'=' -f2); local_p=\$(grep '^APP_PORT=' '$CICD_DIR/.env.local' 2>/dev/null | cut -d'=' -f2); port=\${local_p:-\$port}; echo \"Port:\$port\""

run_adv_test "035" "Read DEBUG Flag (Only Present in .env.local)" \
  "grep '^DEBUG=' '$CICD_DIR/.env.local' | cut -d'=' -f2"

run_adv_test "036" "Read DB_POOL (Only Present in .env)" \
  "grep '^DB_POOL=' '$CICD_DIR/.env' | cut -d'=' -f2"

run_adv_test "037" "List All Combined Unique Variable Keys" \
  "cat '$CICD_DIR/.env' '$CICD_DIR/.env.local' | cut -d'=' -f1 | sort -u"

run_adv_test "038" "Check If NODE_ENV is Set to development" \
  "grep '^NODE_ENV=' '$CICD_DIR/.env' | cut -d'=' -f2"

run_adv_test "039" "Cascading LOG_LEVEL Resolution" \
  "lvl=\$(grep '^LOG_LEVEL=' '$CICD_DIR/.env' | cut -d'=' -f2); loc=\$(grep '^LOG_LEVEL=' '$CICD_DIR/.env.local' 2>/dev/null | cut -d'=' -f2); echo \"\${loc:-\$lvl}\""

run_adv_test "040" "Count Overridden Variables in .env.local" \
  "cat '$CICD_DIR/.env.local' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 5. BUILD TIMINGS & BENCHMARK SIMULATION (TESTS #041 - #050)
# ------------------------------------------------------------------------------

run_adv_test "041" "Calculate Elapsed Duration in Shell Arithmetic" \
  "t_start=1000; t_end=1450; echo \"Elapsed:\$((t_end - t_start))ms\""

run_adv_test "042" "Calculate Multi-step Total Duration" \
  "t1=120; t2=340; t3=45; echo \"Total:\$((t1 + t2 + t3))ms\""

run_adv_test "043" "Verify Build Finished Within 60,000ms Threshold" \
  "dur=1850; [ \"\$dur\" -le 60000 ]; echo \$?"

run_adv_test "044" "Format Milliseconds into mm:ss Display" \
  "ms=125000; sec=\$((ms / 1000)); m=\$((sec / 60)); s=\$((sec % 60)); printf \"%02d:%02d\\\\n\" \$m \$s"

run_adv_test "045" "Compare Speedup Factor in Integer Arithmetic" \
  "t_old=5000; t_new=100; echo \"\$((t_old / t_new))x Faster\""

run_adv_test "046" "Find Longest Running Step" \
  "steps='120 450 210 80'; max=0; for s in \$steps; do if [ \"\$s\" -gt \"\$max\" ]; then max=\$s; fi; done; echo \"Max:\$max\""

run_adv_test "047" "Compute Average Duration across 3 Jobs" \
  "j1=100; j2=200; j3=300; echo \"Avg:\$(( (j1 + j2 + j3) / 3 ))\""

run_adv_test "048" "Check If Any Step Exceeded 10,000ms Timeout" \
  "steps='200 850 1200'; bad=0; for s in \$steps; do if [ \"\$s\" -gt 10000 ]; then bad=1; fi; done; echo \$bad"

run_adv_test "049" "Generate Timing Breakdown Summary" \
  "echo 'Step:Compile Time:240ms | Step:Test Time:85ms'"

run_adv_test "050" "Simulate Build Run ID Number" \
  "run_id=42; echo \"RUN_ID:#\${run_id}\""

# ------------------------------------------------------------------------------
# 6. ARTIFACT VERIFICATION & SIZE AUDITING (TESTS #051 - #060)
# ------------------------------------------------------------------------------

run_adv_test "051" "Check Exact Byte Size of gateway Binary" \
  "wc -c < '$CICD_DIR/bin/gateway' | tr -d ' '"

run_adv_test "052" "Check Exact Byte Size of bundle.js" \
  "wc -c < '$CICD_DIR/dist/bundle.js' | tr -d ' '"

run_adv_test "053" "Verify Binary Size <= 10,000 Bytes Budget" \
  "sz=\$(wc -c < '$CICD_DIR/bin/gateway' | tr -d ' '); [ \"\$sz\" -le 10000 ]; echo \$?"

run_adv_test "054" "Calculate Total Size of All Artifacts Combined" \
  "s1=\$(wc -c < '$CICD_DIR/bin/gateway' | tr -d ' '); s2=\$(wc -c < '$CICD_DIR/dist/bundle.js' | tr -d ' '); echo \"\$((s1 + s2))\""

run_adv_test "055" "Verify Artifact Directory is Non-Empty" \
  "[ -s '$CICD_DIR/bin/gateway' ]; echo \$?"

run_adv_test "056" "Find Largest Artifact in Output" \
  "s1=\$(wc -c < '$CICD_DIR/bin/gateway' | tr -d ' '); s2=\$(wc -c < '$CICD_DIR/dist/bundle.js' | tr -d ' '); if [ \"\$s1\" -ge \"\$s2\" ]; then echo 'gateway'; else echo 'bundle.js'; fi"

run_adv_test "057" "Generate Size Audit Status (PASS/FAIL)" \
  "sz=\$(wc -c < '$CICD_DIR/bin/gateway' | tr -d ' '); if [ \"\$sz\" -le 5000 ]; then echo 'PASS'; else echo 'FAIL'; fi"

run_adv_test "058" "Detect Any 0-Byte Empty Artifacts" \
  "empty=0; for f in '$CICD_DIR/bin/gateway' '$CICD_DIR/dist/bundle.js'; do if [ ! -s \"\$f\" ]; then empty=1; fi; done; echo \$empty"

run_adv_test "059" "Calculate Size in Kilobytes (KB)" \
  "sz=\$(wc -c < '$CICD_DIR/dist/bundle.js' | tr -d ' '); echo \"\$((sz / 1024))KB\""

run_adv_test "060" "Verify SHA256 Hash Length (64 Hex Characters)" \
  "hash=\$( (sha256sum '$CICD_DIR/bin/gateway' 2>/dev/null || shasum -a 256 '$CICD_DIR/bin/gateway') | awk '{print \$1}' ); echo \${#hash}"

# ------------------------------------------------------------------------------
# 7. DEPENDENCY GRAPH & STEP ORDERING (TESTS #061 - #070)
# ------------------------------------------------------------------------------

run_adv_test "061" "Simulate Step 1: Lint (Exit 0)" \
  "echo 'linting...' >/dev/null; echo \$?"

run_adv_test "062" "Simulate Step 2: Compile (Exit 0)" \
  "echo 'compiling...' >/dev/null; echo \$?"

run_adv_test "063" "Simulate Step 3: Test (Exit 0)" \
  "echo 'testing...' >/dev/null; echo \$?"

run_adv_test "064" "Simulate Step 4: Package (Exit 0)" \
  "echo 'packaging...' >/dev/null; echo \$?"

run_adv_test "065" "Conditional Packaging Only If Tests Pass" \
  "test_rc=0; if [ \"\$test_rc\" -eq 0 ]; then echo 'package_ok'; else echo 'skip'; fi"

run_adv_test "066" "Early Exit on Build Step Failure Simulation" \
  "step1_rc=1; if [ \"\$step1_rc\" -ne 0 ]; then echo 'aborted'; exit 0; fi"

run_adv_test "067" "Track Completed Pipeline Steps in String" \
  "log=''; for s in lint build test package; do log=\"\$log \$s\"; done; echo \"\${log# }\""

run_adv_test "068" "Verify All 4 Pipeline Steps Completed" \
  "steps='lint build test package'; echo \$steps | wc -w | tr -d ' '"

run_adv_test "069" "Simulate continue-on-error Step" \
  "err_step() { return 1; }; err_step || true; echo \$?"

run_adv_test "070" "Calculate Pipeline Completion Percentage (3 of 4)" \
  "done=3; total=4; echo \"\$(( done * 100 / total ))%\""

# ------------------------------------------------------------------------------
# 8. SOFTWARE BILL OF MATERIALS (SBOM) GENERATION (TESTS #071 - #080)
# ------------------------------------------------------------------------------

run_adv_test "071" "Count Total Components in components.txt" \
  "wc -l < '$CICD_DIR/components.txt' | tr -d ' '"

run_adv_test "072" "Extract All Component Names Sorted" \
  "cut -d':' -f1 '$CICD_DIR/components.txt' | sort"

run_adv_test "073" "Extract All Component Licenses Sorted Unique" \
  "cut -d':' -f4 '$CICD_DIR/components.txt' | sort -u"

run_adv_test "074" "List Components Using MIT License" \
  "awk -F':' '\$4 == \"MIT\" {print \$1}' '$CICD_DIR/components.txt' | sort"

run_adv_test "075" "List Components Using Apache-2.0 License" \
  "awk -F':' '\$4 == \"Apache-2.0\" {print \$1}' '$CICD_DIR/components.txt'"

run_adv_test "076" "Check If Any Component Uses GPL-3.0" \
  "awk -F':' '\$4 ~ /GPL/ {print \$1, \$4}' '$CICD_DIR/components.txt'"

run_adv_test "077" "Generate Markdown Table of Components" \
  "awk -F':' 'BEGIN {print \"| Component | Version | License |\"; print \"|---|---|---|\"} {printf \"| %s | %s | %s |\\\\n\", \$1, \$3, \$4}' '$CICD_DIR/components.txt' | head -n 3"

run_adv_test "078" "Construct JSON SBOM Component Array" \
  "arr=\"[\"; while IFS=: read -r name mod ver lic; do arr=\"\$arr{\\\"name\\\":\\\"\$name\\\",\\\"ver\\\":\\\"\$ver\\\"},\"; done < '$CICD_DIR/components.txt'; echo \"\${arr%,}]\""

run_adv_test "079" "Find Silicon Driver Component Details" \
  "grep '^driver:' '$CICD_DIR/components.txt'"

run_adv_test "080" "Count Unique Licenses in Project" \
  "cut -d':' -f4 '$CICD_DIR/components.txt' | sort -u | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 9. MULTI-TARGET BUILD FAILURE HANDLING & RETRIES (TESTS #081 - #090)
# ------------------------------------------------------------------------------

run_adv_test "081" "Simulate Retry Loop: Succeed on Attempt 2" \
  "att=1; while [ \"\$att\" -le 3 ]; do if [ \"\$att\" -eq 2 ]; then echo \"SuccessOn:\$att\"; break; fi; att=\$((att + 1)); done"

run_adv_test "082" "Simulate Retry Loop: Fail After Max 3 Attempts" \
  "att=1; ok=0; while [ \"\$att\" -le 3 ]; do att=\$((att + 1)); done; echo \"MaxAttemptsReached:\$att\""

run_adv_test "083" "Exponential Backoff Interval Calculation (2^attempt)" \
  "att=3; echo \"Backoff:\$(( 1 << att ))s\""

run_adv_test "084" "Record Warning Log on Failed Attempt" \
  "att=1; echo \"WARN: Attempt \$att failed, retrying...\" | cut -d' ' -f1-2"

run_adv_test "085" "Non-zero Exit Code Preservation" \
  "(exit 42); echo \$?"

run_adv_test "086" "Fast-Fail Simulation on First Error" \
  "failed=0; for j in 0 1 0; do if [ \"\$j\" -ne 0 ]; then failed=1; break; fi; done; echo \"Halted:\$failed\""

run_adv_test "087" "Aggregate Exit Codes from 3 Jobs" \
  "j1=0; j2=0; j3=1; echo \"\$((j1 + j2 + j3))\""

run_adv_test "088" "Check If Build Matrix Had Any Failure" \
  "tot_err=1; if [ \"\$tot_err\" -gt 0 ]; then echo 'FAILED'; else echo 'SUCCESS'; fi"

run_adv_test "089" "Clean Workspace on Failure Simulation" \
  "cleaned=0; if [ 1 -ne 0 ]; then cleaned=1; fi; echo \"Cleaned:\$cleaned\""

run_adv_test "090" "Success Flag Check When All Exit Codes Zero" \
  "e1=0; e2=0; e3=0; if [ \$((e1 + e2 + e3)) -eq 0 ]; then echo 'PASS'; else echo 'FAIL'; fi"

# ------------------------------------------------------------------------------
# 10. IN-SHELL CI/CD PIPELINE ORCHESTRATION (TESTS #091 - #100)
# ------------------------------------------------------------------------------

run_adv_test "091" "Pure Shell While-Read Loop over Matrix Targets" \
  "count=0; while IFS=: read -r os arch cc; do count=\$((count + 1)); done < '$CICD_DIR/matrix.txt'; echo \"TotalJobs:\$count\""

run_adv_test "092" "Dotenv Cascading Function in Shell" \
  "get_env() { local k=\"\$1\"; local v=\$(grep \"^\$k=\" '$CICD_DIR/.env.local' 2>/dev/null | cut -d= -f2); if [ -z \"\$v\" ]; then v=\$(grep \"^\$k=\" '$CICD_DIR/.env' 2>/dev/null | cut -d= -f2); fi; echo \"\$v\"; }; get_env 'APP_PORT'"

run_adv_test "093" "Artifact Checksum Manifest Generator in Shell" \
  "for f in '$CICD_DIR/bin/gateway'; do sha=\$( (sha256sum \"\$f\" 2>/dev/null || shasum -a 256 \"\$f\") | awk '{print \$1}' ); echo \"\$(basename \"\$f\"): \${sha:0:8}\"; done"

run_adv_test "094" "SBOM License Auditor in Pure Shell" \
  "gpl_found=0; while IFS=: read -r name mod ver lic; do if [ \"\$lic\" = \"GPL-3.0\" ]; then gpl_found=1; fi; done < '$CICD_DIR/components.txt'; echo \"GPLDetected:\$gpl_found\""

run_adv_test "095" "CI Pipeline Status Badge Generator" \
  "status='success'; echo \"https://img.shields.io/badge/build-\${status}-green\""

run_adv_test "096" "Job Summary Markdown Formatter in Shell" \
  "jobs='darwin-arm64 linux-x86_64'; out='# Build Summary'; for j in \$jobs; do out=\"\$out | \$j: OK\"; done; echo \"\$out\""

run_adv_test "097" "Compiler Flag Builder in Shell Function" \
  "build_flags() { local opt=\"\$1\"; echo \"-O\${opt} -Wall -Werror\"; }; build_flags 3"

run_adv_test "098" "Simulate Monorepo Dependency Matrix Build Order" \
  "echo '1:kernel 2:lexer 3:arena 4:term'"

run_adv_test "099" "Clean Workspace Verification Post-Build" \
  "[ -f '$CICD_DIR/.env' ] && [ ! -f '$CICD_DIR/release.tar.gz' ]; echo \$?"

run_adv_test "100" "Complete Module 4 Parity & Integrity Proof" \
  "awk -F':' '{print \$1, \$2}' '$CICD_DIR/matrix.txt' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# FINAL MODULE SUMMARY REPORT
# ------------------------------------------------------------------------------

echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}📊 MODULE 04 EXECUTION SUMMARY (CI/CD BUILD TOOLS & PACKAGING):${NC}"
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
