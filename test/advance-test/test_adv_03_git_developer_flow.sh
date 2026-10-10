#!/bin/bash
# ==============================================================================
# ADVANCE-TEST MODULE 03: GIT DEVELOPER FLOW & AUTOMATION (100 TESTS)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

# Setup fixture
generate_git_fixture
REPO_DIR="$FIXTURES_DIR/mock_git_repo"

echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}🚀 STARTING ADVANCE-TEST MODULE 03: GIT DEVELOPER FLOW & AUTOMATION${NC}"
echo -e "   Target: zbash vs macOS /bin/bash (100 Differential Tests)"
echo -e "${BOLD}${CYAN}================================================================================${NC}"

# ------------------------------------------------------------------------------
# 1. GIT STATUS & WORKING TREE INSPECTIONS (TESTS #001 - #010)
# ------------------------------------------------------------------------------

run_adv_test "001" "Check Current Active Branch" \
  "cd '$REPO_DIR' && git branch --show-current"

run_adv_test "002" "Check Working Tree Clean Porcelain Status" \
  "cd '$REPO_DIR' && git status --porcelain"

run_adv_test "003" "Count Total Tracked Files in Repository" \
  "cd '$REPO_DIR' && git ls-files | wc -l | tr -d ' '"

run_adv_test "004" "List Tracked Files in packages/ Subdirectory" \
  "cd '$REPO_DIR' && git ls-files packages/"

run_adv_test "005" "Verify HEAD Points to Valid 40-Char SHA-1" \
  "cd '$REPO_DIR' && git rev-parse HEAD | grep -E -o '^[0-9a-f]{40}$' | wc -l | tr -d ' '"

run_adv_test "006" "Check Short 7-Char Commit Hash" \
  "cd '$REPO_DIR' && git rev-parse --short HEAD | wc -c | tr -d ' '"

run_adv_test "007" "Total Commit Count on Main Branch" \
  "cd '$REPO_DIR' && git rev-list --count HEAD | tr -d ' '"

run_adv_test "008" "Check If Repository is Bare" \
  "cd '$REPO_DIR' && git rev-parse --is-bare-repository"

run_adv_test "009" "Check Repository Top-level Root Directory" \
  "cd '$REPO_DIR' && basename \"\$(git rev-parse --show-toplevel)\""

run_adv_test "010" "List Tracked Files Sorted Alphabetically" \
  "cd '$REPO_DIR' && git ls-files | sort"

# ------------------------------------------------------------------------------
# 2. GIT LOG ANALYSIS & AUTHOR METRICS (TESTS #011 - #020)
# ------------------------------------------------------------------------------

run_adv_test "011" "List Unique Commit Author Names" \
  "cd '$REPO_DIR' && git log --format='%an' | sort -u"

run_adv_test "012" "List Unique Commit Author Emails" \
  "cd '$REPO_DIR' && git log --format='%ae' | sort -u"

run_adv_test "013" "Commit Count per Author via Shortlog" \
  "cd '$REPO_DIR' && git shortlog -s -n | tr -s ' ' | sed 's/^[ ]*//'"

run_adv_test "014" "Identify Top Committer Name" \
  "cd '$REPO_DIR' && git shortlog -s -n | head -n 1 | awk '{print \$2, \$3}'"

run_adv_test "015" "Extract Latest Commit Subject Line" \
  "cd '$REPO_DIR' && git log -1 --format='%s'"

run_adv_test "016" "Extract Initial Scaffold Commit Subject Line" \
  "cd '$REPO_DIR' && git log --reverse --format='%s' | head -n 1"

run_adv_test "017" "Count Commits Made in February 2026" \
  "cd '$REPO_DIR' && git log --since='2026-02-01' --until='2026-02-28' --oneline | wc -l | tr -d ' '"

run_adv_test "018" "Count Commits Touching 'core'" \
  "cd '$REPO_DIR' && git log --grep='core' --oneline | wc -l | tr -d ' '"

run_adv_test "019" "Extract Oneline Commit History List" \
  "cd '$REPO_DIR' && git log --format='%s' | head -n 4"

run_adv_test "020" "Check Author and Short Date Formatting" \
  "cd '$REPO_DIR' && git log -1 --format='%an [%ad]' --date=short"

# ------------------------------------------------------------------------------
# 3. CONVENTIONAL COMMITS PARSING (TESTS #021 - #030)
# ------------------------------------------------------------------------------

run_adv_test "021" "Count 'feat' Feature Commits" \
  "cd '$REPO_DIR' && git log --format='%s' | grep -c '^feat'"

run_adv_test "022" "Count 'fix' Bugfix Commits" \
  "cd '$REPO_DIR' && git log --format='%s' | grep -c '^fix'"

run_adv_test "023" "Count 'chore' Maintenance Commits" \
  "cd '$REPO_DIR' && git log --format='%s' | grep -c '^chore'"

run_adv_test "024" "Count 'docs' Documentation Commits" \
  "cd '$REPO_DIR' && git log --format='%s' | grep -c '^docs'"

run_adv_test "025" "Extract Distinct Scopes in Conventional Commits" \
  "cd '$REPO_DIR' && git log --format='%s' | grep -o '([a-z0-9_-]*)' | tr -d '()' | sort -u"

run_adv_test "026" "Extract Commit Subject Without Type/Scope Prefix" \
  "cd '$REPO_DIR' && git log -1 --format='%s' | sed 's/^[a-z]*([^)]*): //'"

run_adv_test "027" "Group Conventional Commit Types Count" \
  "cd '$REPO_DIR' && git log --format='%s' | cut -d'(' -f1 | cut -d':' -f1 | sort | uniq -c | tr -s ' ' | sed 's/^[ ]*//'"

run_adv_test "028" "Check If Any Breaking Changes Exist (!:)" \
  "cd '$REPO_DIR' && git log --format='%s' | grep -c '!:'"

run_adv_test "029" "Generate Changelog Markdown List from feat/fix" \
  "cd '$REPO_DIR' && git log --format='%s' | grep -E '^feat|^fix' | sed 's/^/- /' | head -n 3"

run_adv_test "030" "Extract First Word of Each Commit Subject" \
  "cd '$REPO_DIR' && git log --format='%s' | awk '{print \$1}' | head -n 4"

# ------------------------------------------------------------------------------
# 4. SEMVER (SEMANTIC VERSIONING) EXTRACTION (TESTS #031 - #040)
# ------------------------------------------------------------------------------

run_adv_test "031" "List All Git Tags Sorted" \
  "cd '$REPO_DIR' && git tag -l | sort -V"

run_adv_test "032" "Get Latest Tag on Current Branch" \
  "cd '$REPO_DIR' && git describe --tags --abbrev=0"

run_adv_test "033" "Extract Major Version from Latest Tag" \
  "cd '$REPO_DIR' && tag=\$(git describe --tags --abbrev=0); echo \"\${tag#v}\" | cut -d'.' -f1"

run_adv_test "034" "Extract Minor Version from Latest Tag" \
  "cd '$REPO_DIR' && tag=\$(git describe --tags --abbrev=0); echo \"\${tag#v}\" | cut -d'.' -f2"

run_adv_test "035" "Extract Patch Version from Latest Tag" \
  "cd '$REPO_DIR' && tag=\$(git describe --tags --abbrev=0); echo \"\${tag#v}\" | cut -d'.' -f3 | cut -d'-' -f1"

run_adv_test "036" "Check If Pre-release Tag Contains Hyphen" \
  "cd '$REPO_DIR' && git tag -l '*beta*' | head -n 1"

run_adv_test "037" "Calculate Next Patch Version in Shell" \
  "cd '$REPO_DIR' && tag=\$(git describe --tags --abbrev=0); v=\"\${tag#v}\"; maj=\$(echo \$v | cut -d. -f1); min=\$(echo \$v | cut -d. -f2); pat=\$(echo \$v | cut -d. -f3); echo \"v\${maj}.\${min}.\$((pat + 1))\""

run_adv_test "038" "Calculate Next Minor Version in Shell" \
  "cd '$REPO_DIR' && tag=\$(git describe --tags --abbrev=0); v=\"\${tag#v}\"; maj=\$(echo \$v | cut -d. -f1); min=\$(echo \$v | cut -d. -f2); echo \"v\${maj}.\$((min + 1)).0\""

run_adv_test "039" "Calculate Next Major Version in Shell" \
  "cd '$REPO_DIR' && tag=\$(git describe --tags --abbrev=0); v=\"\${tag#v}\"; maj=\$(echo \$v | cut -d. -f1); echo \"v\$((maj + 1)).0.0\""

run_adv_test "040" "Count Total Tagged Release Milestones" \
  "cd '$REPO_DIR' && git tag -l 'v*.*.*' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 5. BRANCHING & DIVERGENCE ANALYSIS (TESTS #041 - #050)
# ------------------------------------------------------------------------------

run_adv_test "041" "List All Local Branches" \
  "cd '$REPO_DIR' && git branch --list | tr -d '* ' | sort"

run_adv_test "042" "Count Commits Ahead on feat/arm64-neon vs main" \
  "cd '$REPO_DIR' && git rev-list --count main..feat/arm64-neon | tr -d ' '"

run_adv_test "043" "Find Merge-Base Common Ancestor Hash" \
  "cd '$REPO_DIR' && git merge-base main feat/arm64-neon | cut -c1-8"

run_adv_test "044" "Show Files Changed Between main and feat/arm64-neon" \
  "cd '$REPO_DIR' && git diff --name-only main feat/arm64-neon"

run_adv_test "045" "Check If Branch feat/arm64-neon Exists" \
  "cd '$REPO_DIR' && git show-ref --verify --quiet refs/heads/feat/arm64-neon; echo \$?"

run_adv_test "046" "Check If Non-existent Branch Returns Error" \
  "cd '$REPO_DIR' && git show-ref --verify --quiet refs/heads/non-existent-branch; echo \$?"

run_adv_test "047" "Inspect Feature Branch Tip Subject" \
  "cd '$REPO_DIR' && git log -1 --format='%s' feat/arm64-neon"

run_adv_test "048" "Extract Author of Feature Branch Commit" \
  "cd '$REPO_DIR' && git log -1 --format='%an' feat/arm64-neon"

run_adv_test "049" "Count Insertions on Feature Branch" \
  "cd '$REPO_DIR' && git diff --shortstat main feat/arm64-neon | awk '{print \$1, \$2}'"

run_adv_test "050" "List Branches Containing Tag v1.0.0" \
  "cd '$REPO_DIR' && git branch --contains v1.0.0 | tr -d '* '"

# ------------------------------------------------------------------------------
# 6. MONOREPO CHANGESET & PACKAGE FILTERING (TESTS #051 - #060)
# ------------------------------------------------------------------------------

run_adv_test "051" "Count Commits Touching packages/core" \
  "cd '$REPO_DIR' && git log --oneline packages/core | wc -l | tr -d ' '"

run_adv_test "052" "Count Commits Touching packages/cli" \
  "cd '$REPO_DIR' && git log --oneline packages/cli | wc -l | tr -d ' '"

run_adv_test "053" "List All Sub-packages in Monorepo" \
  "cd '$REPO_DIR' && ls packages | sort"

run_adv_test "054" "Count Commits Touching src/main.c" \
  "cd '$REPO_DIR' && git log --oneline src/main.c | wc -l | tr -d ' '"

run_adv_test "055" "Count Commits Touching docs/ Directory" \
  "cd '$REPO_DIR' && git log --oneline docs/ | wc -l | tr -d ' '"

run_adv_test "056" "Monorepo Affected Package Check: packages/core" \
  "cd '$REPO_DIR' && git log -1 --name-only | grep -q 'packages/core'; echo \$?"

run_adv_test "057" "List File Extensions Present in Repository" \
  "cd '$REPO_DIR' && git ls-files | sed 's/.*\\.//' | sort -u"

run_adv_test "058" "Count Tracked C Source Files (*.c)" \
  "cd '$REPO_DIR' && git ls-files '*.c' | wc -l | tr -d ' '"

run_adv_test "059" "Count Tracked Markdown Files (*.md)" \
  "cd '$REPO_DIR' && git ls-files '*.md' | wc -l | tr -d ' '"

run_adv_test "060" "Count Tracked Python Files (*.py)" \
  "cd '$REPO_DIR' && git ls-files '*.py' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 7. GIT DIFF & CHANGE STATISTICS (TESTS #061 - #070)
# ------------------------------------------------------------------------------

run_adv_test "061" "Diff Files Modified Between v0.1.0 and v1.0.0" \
  "cd '$REPO_DIR' && git diff --name-only v0.1.0 v1.0.0 | sort"

run_adv_test "062" "Count Files Changed Between v0.1.0 and v1.0.0" \
  "cd '$REPO_DIR' && git diff --name-only v0.1.0 v1.0.0 | wc -l | tr -d ' '"

run_adv_test "063" "Check If README.md Changed Between v0.1.0 and v1.0.0" \
  "cd '$REPO_DIR' && git diff --name-only v0.1.0 v1.0.0 | grep -c 'README.md'"

run_adv_test "064" "Extract Commit Subject Range v0.1.0..v0.2.0" \
  "cd '$REPO_DIR' && git log --format='%s' v0.1.0..v0.2.0"

run_adv_test "065" "Count Commits in Range v0.1.0..v1.0.0" \
  "cd '$REPO_DIR' && git rev-list --count v0.1.0..v1.0.0 | tr -d ' '"

run_adv_test "066" "Extract Added Lines with '+' Prefix Between Tags" \
  "cd '$REPO_DIR' && git diff v0.1.0 v0.2.0 packages/cli/ | grep '^+' | grep -v '^+++' | head -n 1"

run_adv_test "067" "Verify No Deletions Between v0.1.0 and v0.2.0" \
  "cd '$REPO_DIR' && git diff v0.1.0 v0.2.0 | grep '^-' | grep -v '^---' | wc -l | tr -d ' '"

run_adv_test "068" "Inspect Patch Header Lines" \
  "cd '$REPO_DIR' && git show --format='' v0.1.0 | head -n 2"

run_adv_test "069" "Check Total Lines in src/main.c" \
  "cd '$REPO_DIR' && wc -l < src/main.c | tr -d ' '"

run_adv_test "070" "Show Commit Hashes in Ascending Chronological Order" \
  "cd '$REPO_DIR' && git log --reverse --format='%h' | head -n 3"

# ------------------------------------------------------------------------------
# 8. GIT HOOKS & LINTER PRE-COMMIT CHECKS (TESTS #071 - #080)
# ------------------------------------------------------------------------------

run_adv_test "071" "Pre-commit: Check for Trailing Whitespace in Tracked Files" \
  "cd '$REPO_DIR' && git ls-files | xargs grep -n '[ \t]\$' | wc -l | tr -d ' '"

run_adv_test "072" "Pre-commit: Verify All Commit Messages <= 72 Chars" \
  "cd '$REPO_DIR' && git log --format='%s' | awk 'length > 72' | wc -l | tr -d ' '"

run_adv_test "073" "Pre-commit: Ensure All Commits Start with Conventional Prefix" \
  "cd '$REPO_DIR' && git log --format='%s' | grep -v -E '^(feat|fix|chore|docs|refactor|test|ci)' | wc -l | tr -d ' '"

run_adv_test "074" "Pre-commit: Check for Forbidden Hardcoded Tokens (FIXME/TODO)" \
  "cd '$REPO_DIR' && git grep -E 'FIXME|TODO' | wc -l | tr -d ' '"

run_adv_test "075" "Pre-commit: Verify Files End with Trailing Newline" \
  "cd '$REPO_DIR' && bad=0; for f in \$(git ls-files); do [ -s \"\$f\" ] && [ \"\$(tail -c 1 \"\$f\")\" != \"\" ] && bad=\$((bad+1)); done; echo \"\$bad\""

run_adv_test "076" "Verify Active Branch Name Matches Conventional Pattern" \
  "cd '$REPO_DIR' && b=\$(git branch --show-current); echo \"\$b\" | grep -E -q '^(main|master|feat/|fix/|release/)'; echo \$?"

run_adv_test "077" "Verify All Author Emails Have Valid @ Domain" \
  "cd '$REPO_DIR' && git log --format='%ae' | grep -v '@' | wc -l | tr -d ' '"

run_adv_test "078" "Check for Any Oversized Files (> 1MB)" \
  "cd '$REPO_DIR' && big=0; for f in \$(git ls-files); do sz=\$(wc -c < \"\$f\" | tr -d ' '); if [ \"\$sz\" -gt 1048576 ]; then big=\$((big+1)); fi; done; echo \"\$big\""

run_adv_test "079" "Verify README.md Has Heading Header" \
  "cd '$REPO_DIR' && head -n 1 README.md"

run_adv_test "080" "Simulate commit-msg Hook Exit Code on Valid Message" \
  "msg='feat(core): new capability'; if echo \"\$msg\" | grep -q -E '^(feat|fix|chore):'; then echo 0; else echo 1; fi"

# ------------------------------------------------------------------------------
# 9. RELEASE PACKAGING & ARTIFACT METADATA (TESTS #081 - #090)
# ------------------------------------------------------------------------------

run_adv_test "081" "Construct Release Tarball Name from Git Describe" \
  "cd '$REPO_DIR' && tag=\$(git describe --tags --abbrev=0); echo \"zephyr-\${tag}.tar.gz\""

run_adv_test "082" "Check Git Describe Full Output on Current Commit" \
  "cd '$REPO_DIR' && git describe --tags"

run_adv_test "083" "Generate Release Notes Header with Tag and Date" \
  "cd '$REPO_DIR' && tag=\$(git describe --tags --abbrev=0); dt=\$(date +%Y-%m); echo \"## Release \${tag} (\${dt})\""

run_adv_test "084" "Verify Release Notes Contains Recent Commit Subjects" \
  "cd '$REPO_DIR' && git log -2 --format='* %s'"

run_adv_test "085" "Simulate Checksum Manifest Generation" \
  "echo 'zephyr-v1.1.0.tar.gz' | sha256sum 2>/dev/null || shasum -a 256 <<< 'zephyr-v1.1.0.tar.gz' | awk '{print \$1}'"

run_adv_test "086" "Check Git Tree SHA on HEAD" \
  "cd '$REPO_DIR' && git cat-file -p HEAD | grep '^tree ' | awk '{print \$2}' | cut -c1-8"

run_adv_test "087" "Verify All Tags Start with Prefix 'v'" \
  "cd '$REPO_DIR' && git tag -l | grep -v '^v' | wc -l | tr -d ' '"

run_adv_test "088" "Check Total Release Tags Count" \
  "cd '$REPO_DIR' && git tag -l 'v*' | wc -l | tr -d ' '"

run_adv_test "089" "Inspect Initial Commit Hash" \
  "cd '$REPO_DIR' && git rev-list --max-parents=0 HEAD | cut -c1-8"

run_adv_test "090" "Clean Working Copy Verification Script (Exit 0 if clean)" \
  "cd '$REPO_DIR' && [ -z \"\$(git status --porcelain)\" ]; echo \$?"

# ------------------------------------------------------------------------------
# 10. IN-SHELL GIT AUTOMATION SCRIPTS (TESTS #091 - #100)
# ------------------------------------------------------------------------------

run_adv_test "091" "Pure Shell While-Read Loop over Git Log" \
  "cd '$REPO_DIR' && count=0; while read -r line; do count=\$((count + 1)); done <<< \"\$(git log --oneline)\"; echo \"LogEntries:\$count\""

run_adv_test "092" "Pure Shell SemVer Bump Script in Function" \
  "bump_patch() { local v=\"\${1#v}\"; local maj=\$(cut -d. -f1 <<< \"\$v\"); local min=\$(cut -d. -f2 <<< \"\$v\"); local pat=\$(cut -d. -f3 <<< \"\$v\"); echo \"v\${maj}.\${min}.\$((pat + 1))\"; }; bump_patch 'v1.1.0'"

run_adv_test "093" "Monorepo Change-Detector Function in Shell" \
  "cd '$REPO_DIR' && check_pkg() { if git log -1 --name-only | grep -q \"packages/\$1\"; then echo \"\$1:changed\"; else echo \"\$1:clean\"; fi; }; check_pkg 'core'"

run_adv_test "094" "Changelog Generator Loop in Pure Shell" \
  "cd '$REPO_DIR' && out=\"\"; while read -r sub; do out=\"\$out | \$sub\"; done <<< \"\$(git log -2 --format='%s')\"; echo \"\${out# | }\""

run_adv_test "095" "Git Contributor Ranking in Shell" \
  "cd '$REPO_DIR' && git log --format='%an' | sort | uniq -c | sort -nr | head -n 2 | tr -s ' ' | sed 's/^[ ]*//'"

run_adv_test "096" "Check If Working Directory Has Stashed Changes" \
  "cd '$REPO_DIR' && git stash list | wc -l | tr -d ' '"

run_adv_test "097" "Extract Author and Committer Matching Check" \
  "cd '$REPO_DIR' && git log -1 --format='%an == %cn'"

run_adv_test "098" "Construct Structured Git Release JSON Object" \
  "cd '$REPO_DIR' && tag=\$(git describe --tags --abbrev=0); sha=\$(git rev-parse --short HEAD); echo \"{\\\"tag\\\":\\\"\$tag\\\",\\\"sha\\\":\\\"\$sha\\\"}\""

run_adv_test "099" "Git Diffstat Summary Parser in Shell" \
  "cd '$REPO_DIR' && git diff --shortstat v0.1.0 v1.0.0 | awk '{print \$1, \$4}'"

run_adv_test "100" "Complete Module 3 Parity & Integrity Proof" \
  "cd '$REPO_DIR' && git rev-list --count HEAD >/dev/null; echo \$?"

# ------------------------------------------------------------------------------
# FINAL MODULE SUMMARY REPORT
# ------------------------------------------------------------------------------

echo ""
echo -e "${BOLD}${CYAN}================================================================================${NC}"
echo -e "${BOLD}📊 MODULE 03 EXECUTION SUMMARY (GIT DEVELOPER FLOW & AUTOMATION):${NC}"
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
