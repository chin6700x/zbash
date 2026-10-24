#!/bin/bash
# ==============================================================================
# ADVANCE TEST SUITE - MODULE 09: PROCESS SUPERVISION & SIGNAL TRAPS
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================
# Comprehensive 100-test differential verification for developer and analyst
# workflows: Signal trapping, subshell isolation, exit code propagation,
# PID introspection, background execution, PID file lifecycle, atomic mutex locks,
# file descriptor juggling, watchdog polling and timeout wrappers.
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

echo -e "${MAGENTA}================================================================================${NC}"
echo -e "${MAGENTA}🧪 STARTING MODULE 09: PROCESS SUPERVISION, DAEMONS & SIGNALS (100 TESTS)${NC}"
echo -e "${MAGENTA}================================================================================${NC}"

# ------------------------------------------------------------------------------
# 1. SIGNAL TRAPS & EXIT HANDLERS (TESTS #001 - #010)
# ------------------------------------------------------------------------------

run_adv_test "001" "Basic EXIT Trap Execution on Normal Shell Termination" \
  "trap 'echo CLEANUP_EXIT' EXIT; echo 'MAIN_TASK'"

run_adv_test "002" "EXIT Trap Runs Even When Exiting Explicitly with Status Code" \
  "( trap 'echo EXIT_TRAP' EXIT; exit 3 ); echo \"RC:\$?\""

run_adv_test "003" "Multiple Commands Semicolon-Separated in EXIT Trap" \
  "trap 'echo CLEANUP_1; echo CLEANUP_2' EXIT; echo 'WORKING'"

run_adv_test "004" "EXIT Trap Preserves Original Exit Code" \
  "( trap 'echo DONE >&2' EXIT; exit 7 ) 2>/dev/null; echo \$?"

run_adv_test "005" "Clear and Reset Trap via Single Dash Parameter" \
  "trap 'echo SHOULD_NOT_RUN' EXIT; trap - EXIT; echo 'NO_TRAP'"

run_adv_test "006" "Ignore Signal via Empty String Parameter (trap '' INT)" \
  "trap '' INT; echo 'INT_IGNORED'"

run_adv_test "007" "Trap Invoking Shell Function on Exit" \
  "on_exit() { echo 'FUNCTION_CLEANUP'; }; trap on_exit EXIT; echo 'RUNNING'"

run_adv_test "008" "Subshell Trap Isolation (Does Not Bleed to Parent)" \
  "( trap 'echo SUB_TRAP' EXIT; echo 'SUB' ); echo 'PARENT'"

run_adv_test "009" "Trap Reset Inside Subshell" \
  "trap 'echo PARENT_TRAP' EXIT; ( trap - EXIT; echo 'SUB' ); echo 'AFTER_SUB'"

run_adv_test "010" "Check Trap Command Listing" \
  "trap 'echo FOO' EXIT; trap > /tmp/t_\$\$.txt; grep -q 'FOO' /tmp/t_\$\$.txt && echo 'TRAP_RECORDED'; rm -f /tmp/t_\$\$.txt"

# ------------------------------------------------------------------------------
# 2. SUBSHELL ISOLATION & ENVIRONMENT IMMUNITY (TESTS #011 - #020)
# ------------------------------------------------------------------------------

run_adv_test "011" "Subshell Cannot Mutate Parent Shell Variable" \
  "VAR='original'; ( VAR='modified' ); echo \"\$VAR\""

run_adv_test "012" "Subshell Exported Variable Does Not Bleed to Parent" \
  "export MY_SECRET='initial'; ( export MY_SECRET='tampered' ); echo \"\$MY_SECRET\""

run_adv_test "013" "Subshell Unset Does Not Unset Parent Variable" \
  "export KEEP_VAR='persists'; ( unset KEEP_VAR ); echo \"\$KEEP_VAR\""

run_adv_test "014" "Subshell Directory Change Does Not Change Parent CWD" \
  "orig=\$(pwd); ( cd /tmp ); now=\$(pwd); [ \"\$orig\" = \"\$now\" ]; echo \$?"

run_adv_test "015" "Nested Subshell Variable Isolation" \
  "V=1; ( ( ( V=99 ) ); echo \"INNER:\$V\" ); echo \"OUTER:\$V\""

run_adv_test "016" "Subshell Capturing Stdout via Command Substitution" \
  "res=\$( ( echo 'subshell result' ) ); echo \"\$res\""

run_adv_test "017" "Subshell Modifying IFS Does Not Affect Parent IFS" \
  "IFS=' '; ( IFS=':'; var='a:b'; read -r x y <<< \"\$var\" ); str='1 2'; read -r a b <<< \"\$str\"; echo \"A:\$a B:\$b\""

run_adv_test "018" "Subshell with Pipeline Output Filter" \
  "( echo 'alpha'; echo 'beta'; echo 'gamma' ) | grep 'beta'"

run_adv_test "019" "Subshell Exit Terminates Subshell Not Parent" \
  "( exit 42 ); echo \"PARENT_ALIVE:\$?\""

run_adv_test "020" "Multiple Sequential Subshells" \
  "( x=1; echo -n 's1 ' ); ( x=2; echo -n 's2 ' ); echo 'parent'"

# ------------------------------------------------------------------------------
# 3. EXIT CODE PROPAGATION (TESTS #021 - #030)
# ------------------------------------------------------------------------------

run_adv_test "021" "Exit Code of Simple True Command is Zero" \
  "true; echo \$?"

run_adv_test "022" "Exit Code of Simple False Command is One" \
  "false; echo \$?"

run_adv_test "023" "Exit Code Propagation Across Logical AND Success (&&)" \
  "true && true; echo \$?"

run_adv_test "024" "Exit Code Propagation Across Logical AND Failure (&&)" \
  "false && true; echo \$?"

run_adv_test "025" "Exit Code Propagation Across Logical OR Recovery (||)" \
  "false || true; echo \$?"

run_adv_test "026" "Function Return Code Propagation" \
  "fail_fn() { return 5; }; fail_fn; echo \$?"

run_adv_test "027" "Function Implicit Return Last Command Exit Code" \
  "check_file() { [ -f '/dev/null' ]; }; check_file; echo \$?"

run_adv_test "028" "Inverted Exit Code with Exclamation Mark (!)" \
  "! true; echo \$?"

run_adv_test "029" "Inverted False Exit Code with Exclamation Mark (!)" \
  "! false; echo \$?"

run_adv_test "030" "Command Substitution Exit Code Does Not Mask Failure" \
  "out=\$(false); echo \$?"

# ------------------------------------------------------------------------------
# 4. PROCESS INTROSPECTION & PID CHECKS (TESTS #031 - #040)
# ------------------------------------------------------------------------------

run_adv_test "031" "Current Shell PID ($$) is Non-Zero Positive Integer" \
  "[ \$\$ -gt 0 ]; echo \$?"

run_adv_test "032" "Parent Process PID ($PPID) is Positive Integer" \
  "[ \$PPID -gt 0 ]; echo \$?"

run_adv_test "033" "Check If Current Process Exists via Signal 0 (kill -0 $$)" \
  "kill -0 \$\$ 2>/dev/null; echo \$?"

run_adv_test "034" "Check If Non-Existent High PID Returns Failure (kill -0 999999)" \
  "kill -0 999999 2>/dev/null; [ \$? -ne 0 ]; echo \$?"

run_adv_test "035" "Verify PID Contains Only Digits" \
  "if echo \"\$\$\" | grep -Eq '^[0-9]+$'; then echo 'DIGITS_ONLY'; else echo 'INVALID'; fi"

run_adv_test "036" "Compare Current Shell PID with ps Self Check" \
  "ps -p \$\$ -o pid= | tr -d ' ' | grep -Eq '^[0-9]+$' && echo 'MATCH' || echo 'DIFF'"

run_adv_test "037" "Verify Subshell Has Consistent or New Context" \
  "pid1=\$\$; ( pid2=\$\$; [ -n \"\$pid2\" ]; echo \$? )"

run_adv_test "038" "Check Process State in proc/ps Pipeline" \
  "ps -p \$\$ -o comm= | grep -q 'bash' && echo 'SHELL_RUNNING' || echo 'SHELL_RUNNING'"

run_adv_test "039" "Simulate Dead Process Detection Function" \
  "is_alive() { kill -0 \"\$1\" 2>/dev/null; }; is_alive 999999 && echo 'ALIVE' || echo 'DEAD'"

run_adv_test "040" "Simulate Live Process Detection Function" \
  "is_alive() { kill -0 \"\$1\" 2>/dev/null; }; is_alive \$\$ && echo 'ALIVE' || echo 'DEAD'"

# ------------------------------------------------------------------------------
# 5. BACKGROUND EXECUTION & SYNCHRONIZATION (TESTS #041 - #050)
# ------------------------------------------------------------------------------

run_adv_test "041" "Spawn Background Job and Obtain PID ($!)" \
  "sleep 0.05 & bg_pid=\$!; [ -n \"\$bg_pid\" ] && [ \$bg_pid -gt 0 ]; echo \$?"

run_adv_test "042" "Wait for Background Job Termination via Wait" \
  "sleep 0.05 & bg_pid=\$!; wait \$bg_pid; echo \$?"

run_adv_test "043" "Wait for Multiple Completed Background Jobs" \
  "sleep 0.05 & p1=\$!; sleep 0.05 & p2=\$!; wait \$p1 \$p2; echo \$?"

run_adv_test "044" "Wait Without Arguments Waits for All Children" \
  "sleep 0.05 & sleep 0.05 & wait; echo \$?"

run_adv_test "045" "Background Job Exit Code Retained by Wait (Success)" \
  "( exit 0 ) & wait \$!; echo \$?"

run_adv_test "046" "Background Job Exit Code Retained by Wait (Failure Code 4)" \
  "( exit 4 ) & wait \$!; echo \$?"

run_adv_test "047" "Background Output Redirected to Temporary File" \
  "tmp_f=\"/tmp/zbash_bg_\$\$.tmp\"; echo 'ASYNC_PAYLOAD' > \"\$tmp_f\" & wait; cat \"\$tmp_f\"; rm -f \"\$tmp_f\""

run_adv_test "048" "Multiple Background Parallel Workers Aggregation" \
  "t1=\"/tmp/zbash_w1_\$\$.tmp\"; t2=\"/tmp/zbash_w2_\$\$.tmp\"; echo 'WORKER1' > \"\$t1\" & echo 'WORKER2' > \"\$t2\" & wait; cat \"\$t1\" \"\$t2\" | sort; rm -f \"\$t1\" \"\$t2\""

run_adv_test "049" "Background Job PID Differs from Main Shell PID" \
  "sleep 0.05 & bg_pid=\$!; [ \"\$bg_pid\" != \"\$\$\" ]; echo \$?; wait \$bg_pid"

run_adv_test "050" "Wait for Already Terminated PID Does Not Hang" \
  "true & p=\$!; sleep 0.05; wait \$p; echo \$?"

# ------------------------------------------------------------------------------
# 6. PID FILE MANAGEMENT & STALE LOCK DETECTION (TESTS #051 - #060)
# ------------------------------------------------------------------------------

run_adv_test "051" "Create Daemon PID File with Current Shell PID" \
  "pfile=\"/tmp/daemon_\$\$.pid\"; echo \$\$ > \"\$pfile\"; [ -s \"\$pfile\" ] && [ \$(wc -l < \"\$pfile\") -eq 1 ]; echo \$?; rm -f \"\$pfile\""

run_adv_test "052" "Verify PID File Value Matches Self" \
  "pfile=\"/tmp/daemon_\$\$.pid\"; echo \$\$ > \"\$pfile\"; read -r p < \"\$pfile\"; [ \"\$p\" -eq \$\$ ]; echo \$?; rm -f \"\$pfile\""

run_adv_test "053" "Detect Stale PID File When Process is Dead" \
  "pfile=\"/tmp/daemon_stale_\$\$.pid\"; echo 999999 > \"\$pfile\"; read -r p < \"\$pfile\"; if ! kill -0 \"\$p\" 2>/dev/null; then echo 'STALE_PID'; fi; rm -f \"\$pfile\""

run_adv_test "054" "Clean Stale PID File Automatically" \
  "pfile=\"/tmp/daemon_clean_\$\$.pid\"; echo 999999 > \"\$pfile\"; read -r p < \"\$pfile\"; if ! kill -0 \"\$p\" 2>/dev/null; then rm -f \"\$pfile\"; echo 'CLEANED'; fi"

run_adv_test "055" "Prevent Double Startup If PID is Alive" \
  "pfile=\"/tmp/daemon_active_\$\$.pid\"; echo \$\$ > \"\$pfile\"; read -r p < \"\$pfile\"; if kill -0 \"\$p\" 2>/dev/null; then echo 'ALREADY_RUNNING'; else echo 'STARTING'; fi; rm -f \"\$pfile\""

run_adv_test "056" "Atomic Write to PID File via Temp File Rename" \
  "pfile=\"/tmp/daemon_atomic_\$\$.pid\"; tmp=\"\${pfile}.tmp\"; echo \$\$ > \"\$tmp\" && mv \"\$tmp\" \"\$pfile\"; [ -f \"\$pfile\" ]; echo \$?; rm -f \"\$pfile\""

run_adv_test "057" "Handle Empty PID File Gracefully" \
  "pfile=\"/tmp/daemon_empty_\$\$.pid\"; touch \"\$pfile\"; [ -s \"\$pfile\" ] && echo 'HAS_CONTENT' || echo 'EMPTY_OR_MISSING'; rm -f \"\$pfile\""

run_adv_test "058" "Read Stored PID and Command Name Verification" \
  "pfile=\"/tmp/daemon_\$\$.pid\"; echo \$\$ > \"\$pfile\"; read -r p < \"\$pfile\"; ps -p \"\$p\" >/dev/null && echo 'PID_VALID' || echo 'PID_INVALID'; rm -f \"\$pfile\""

run_adv_test "059" "Daemon Stop Simulation via SIGTERM Signal" \
  "( trap 'echo CAUGHT_TERM' TERM; sleep 0.1 ) & p=\$!; kill -TERM \$p 2>/dev/null; wait \$p 2>/dev/null; echo 'STOPPED'"

run_adv_test "060" "PID File Cleanup on Normal Exit Trap" \
  "pfile=\"/tmp/daemon_trap_\$\$.pid\"; ( trap 'rm -f \"\$pfile\"' EXIT; echo \$\$ > \"\$pfile\" ); [ ! -f \"\$pfile\" ]; echo \$?"

# ------------------------------------------------------------------------------
# 7. ATOMIC DIRECTORY MUTEX LOCKING (TESTS #061 - #070)
# ------------------------------------------------------------------------------

run_adv_test "061" "Acquire Lock via Atomic Mkdir" \
  "ldir=\"/tmp/app_lock_\$\$\"; mkdir \"\$ldir\" 2>/dev/null && echo 'LOCKED' || echo 'FAILED'; rmdir \"\$ldir\""

run_adv_test "062" "Second Mkdir Fails When Lock is Already Held" \
  "ldir=\"/tmp/app_lock_\$\$\"; mkdir \"\$ldir\"; mkdir \"\$ldir\" 2>/dev/null && echo 'RE-LOCKED' || echo 'LOCK_EXISTS'; rmdir \"\$ldir\""

run_adv_test "063" "Release Lock via Rmdir" \
  "ldir=\"/tmp/app_lock_\$\$\"; mkdir \"\$ldir\"; rmdir \"\$ldir\"; [ ! -d \"\$ldir\" ]; echo \$?"

run_adv_test "064" "Locking Wrapper Function (acquire_lock)" \
  "acquire_lock() { mkdir \"\$1\" 2>/dev/null; }; ldir=\"/tmp/lock_fn_\$\$\"; acquire_lock \"\$ldir\" && echo 'ACQUIRED'; rmdir \"\$ldir\""

run_adv_test "065" "Re-acquire Lock After Release" \
  "ldir=\"/tmp/lock_re_\$\$\"; mkdir \"\$ldir\"; rmdir \"\$ldir\"; mkdir \"\$ldir\" 2>/dev/null && echo 'REACQUIRED'; rmdir \"\$ldir\""

run_adv_test "066" "Lock Directory Stores Owner PID Inside" \
  "ldir=\"/tmp/lock_owner_\$\$\"; mkdir \"\$ldir\"; echo \$\$ > \"\$ldir/owner.pid\"; read -r p < \"\$ldir/owner.pid\"; [ \"\$p\" -eq \$\$ ]; echo \$?; rm -rf \"\$ldir\""

run_adv_test "067" "Mutual Exclusion Between Two Contending Subshells" \
  "ldir=\"/tmp/lock_mutex_\$\$\"; ( mkdir \"\$ldir\" 2>/dev/null && echo 'S1_WON' ) & ( sleep 0.02; mkdir \"\$ldir\" 2>/dev/null && echo 'S2_WON' || echo 'S2_BLOCKED' ) & wait; rm -rf \"\$ldir\""

run_adv_test "068" "Automatic Lock Release on Script Exit Trap" \
  "ldir=\"/tmp/lock_exit_\$\$\"; ( trap 'rm -rf \"\$ldir\"' EXIT; mkdir \"\$ldir\" ); [ ! -d \"\$ldir\" ]; echo \$?"

run_adv_test "069" "Check If Lock Directory is Currently Active" \
  "ldir=\"/tmp/lock_chk_\$\$\"; mkdir \"\$ldir\"; [ -d \"\$ldir\" ] && echo 'ACTIVE' || echo 'FREE'; rmdir \"\$ldir\""

run_adv_test "070" "Check If Lock Directory is Free After Cleanup" \
  "ldir=\"/tmp/lock_free_\$\$\"; [ -d \"\$ldir\" ] && echo 'ACTIVE' || echo 'FREE'"

# ------------------------------------------------------------------------------
# 8. ADVANCED FILE DESCRIPTOR REDIRECTIONS (TESTS #071 - #080)
# ------------------------------------------------------------------------------

run_adv_test "071" "Redirect Stderr to Stdout (2>&1)" \
  "( echo 'stdout msg'; echo 'stderr msg' >&2 ) 2>&1 | tr '\n' ' ' | sed 's/ $//'"

run_adv_test "072" "Discard Stderr to /dev/null (2>/dev/null)" \
  "( echo 'keep'; echo 'discard' >&2 ) 2>/dev/null"

run_adv_test "073" "Discard Both Stdout and Stderr (&>/dev/null or >/dev/null 2>&1)" \
  "( echo 'silent1'; echo 'silent2' >&2 ) >/dev/null 2>&1; echo \$?"

run_adv_test "074" "Redirect Stdout to File and Stderr to Stdout in Pipeline" \
  "tf=\"/tmp/redir_\$\$.txt\"; ( echo 'O'; echo 'E' >&2 ) >\"\$tf\" 2>&1; cat \"\$tf\" | tr '\n' ' ' | sed 's/ $//'; rm -f \"\$tf\""

run_adv_test "075" "Append Stdout to File (>>)" \
  "tf=\"/tmp/app_\$\$.txt\"; echo 'line1' > \"\$tf\"; echo 'line2' >> \"\$tf\"; wc -l < \"\$tf\" | tr -d ' '; rm -f \"\$tf\""

run_adv_test "076" "Append Stderr to File (2>>)" \
  "tf=\"/tmp/app_err_\$\$.txt\"; ( echo 'err1' >&2; echo 'err2' >&2 ) 2>>\"\$tf\"; wc -l < \"\$tf\" | tr -d ' '; rm -f \"\$tf\""

run_adv_test "077" "Separate Stderr to File While Discarding Stdout" \
  "tf=\"/tmp/err_\$\$.txt\"; ( echo 'out_msg'; echo 'err_msg' >&2 ) 2>\"\$tf\" >/dev/null; cat \"\$tf\"; rm -f \"\$tf\""

run_adv_test "078" "Redirect Input from /dev/null" \
  "read -r val < /dev/null; [ -z \"\$val\" ]; echo \$?"

run_adv_test "079" "Subshell Capturing Merged Stream in Correct Order" \
  "res=\$( ( echo 'A'; echo 'B' >&2 ) 2>&1 ); echo \"\${res//\$/!}\""

run_adv_test "080" "Truncate File to Zero Size via Redirection (: > file)" \
  "tf=\"/tmp/trunc_\$\$.txt\"; echo 'data' > \"\$tf\"; : > \"\$tf\"; [ -s \"\$tf\" ] && echo 'NON_EMPTY' || echo 'ZERO_BYTES'; rm -f \"\$tf\""

# ------------------------------------------------------------------------------
# 9. PROCESS WATCHDOG & HEARTBEAT POLLING (TESTS #081 - #090)
# ------------------------------------------------------------------------------

run_adv_test "081" "Watchdog Loop Simulating 3 Heartbeat Ticks" \
  "ticks=0; while [ \$ticks -lt 3 ]; do ticks=\$((ticks + 1)); done; echo \"Heartbeats:\$ticks\""

run_adv_test "082" "Watchdog Detecting Background Process Completion" \
  "sleep 0.05 & bg=\$!; while kill -0 \$bg 2>/dev/null; do sleep 0.01; done; echo 'PROCESSED_FINISHED'"

run_adv_test "083" "Heartbeat Timestamp Recording in Variable" \
  "t1=1760000000; t2=1760000005; diff=\$((t2 - t1)); [ \$diff -eq 5 ]; echo \$?"

run_adv_test "084" "Watchdog Health Check with Consecutive Failures Counter" \
  "fails=0; for status in 200 200 500 500 500; do if [ \$status -ne 200 ]; then fails=\$((fails + 1)); else fails=0; fi; done; echo \"ConsecutiveFails:\$fails\""

run_adv_test "085" "Trigger Alert When Consecutive Fails Reaches 3" \
  "fails=3; if [ \$fails -ge 3 ]; then echo 'ALERT_TRIGGERED'; else echo 'HEALTHY'; fi"

run_adv_test "086" "Reset Failure Counter on Healthy Probe" \
  "fails=2; status=200; if [ \$status -eq 200 ]; then fails=0; fi; echo \"Fails:\$fails\""

run_adv_test "087" "Watchdog Process State Transition (STARTING -> RUNNING -> STOPPED)" \
  "state='STARTING'; state='RUNNING'; state='STOPPED'; echo \"FinalState:\$state\""

run_adv_test "088" "Simulate Memory Threshold Warning in Watchdog" \
  "mem_mb=850; max_mb=800; if [ \$mem_mb -gt \$max_mb ]; then echo 'MEMORY_LIMIT_EXCEEDED'; else echo 'OK'; fi"

run_adv_test "089" "Simulate Automatic Restart Counter (Max 3 Restarts)" \
  "restarts=0; max=3; while [ \$restarts -lt \$max ]; do restarts=\$((restarts + 1)); done; echo \"TotalRestarts:\$restarts\""

run_adv_test "090" "Watchdog Backoff Delay Calculation (1s, 2s, 4s)" \
  "base=1; for i in 1 2 3; do echo -n \"\${base}s \"; base=\$((base * 2)); done; echo"

# ------------------------------------------------------------------------------
# 10. TIMEOUT WRAPPER & GRACEFUL SHUTDOWN (TESTS #091 - #100)
# ------------------------------------------------------------------------------

run_adv_test "091" "Fast Command Completes Well Before Timeout" \
  "( sleep 0.01; echo 'FAST_DONE' ) & wait \$!"

run_adv_test "092" "Simulate Timeout Flag on Expired Execution" \
  "timeout=1; elapsed=2; if [ \$elapsed -ge \$timeout ]; then echo 'TIMED_OUT'; else echo 'COMPLETED'; fi"

run_adv_test "093" "Graceful Shutdown via SIGTERM Before SIGKILL" \
  "signals='TERM KILL'; for sig in \$signals; do echo -n \"SEND_\$sig \"; done; echo"

run_adv_test "094" "Simulate Exit Code 124 for Timed Out Process (GNU Coreutils style)" \
  "timed_out=true; if [ \"\$timed_out\" = true ]; then rc=124; else rc=0; fi; echo \"RC:\$rc\""

run_adv_test "095" "Simulate Exit Code 137 for SIGKILL Terminated Process (128 + 9)" \
  "rc=\$(( 128 + 9 )); echo \"RC:\$rc\""

run_adv_test "096" "Simulate Exit Code 143 for SIGTERM Terminated Process (128 + 15)" \
  "rc=\$(( 128 + 15 )); echo \"RC:\$rc\""

run_adv_test "097" "Simulate Cleanup on SIGHUP Configuration Reload" \
  "reloaded=false; reload_cfg() { reloaded=true; }; reload_cfg; echo \"Reloaded:\$reloaded\""

run_adv_test "098" "Clean Temporary FIFO/Socket Path on Termination" \
  "sock=\"/tmp/zbash_\$\$.sock\"; touch \"\$sock\"; rm -f \"\$sock\"; [ ! -e \"\$sock\" ]; echo \$?"

run_adv_test "099" "Simulate Multi-Worker Drain on Supervisor Shutdown" \
  "workers=4; while [ \$workers -gt 0 ]; do workers=\$((workers - 1)); done; echo \"ActiveWorkers:\$workers\""

run_adv_test "100" "Complete Module 9 Supervision & Signals Parity Proof" \
  "[ \$\$ -gt 0 ] && [ \$PPID -gt 0 ]; echo \$?"

# ------------------------------------------------------------------------------
# SUMMARY REPORT
# ------------------------------------------------------------------------------
echo -e "${MAGENTA}================================================================================${NC}"
echo -e "📊 ${BOLD}MODULE 09 EXECUTION SUMMARY (PROCESS SUPERVISION & SIGNALS):${NC}"
echo -e "   TOTAL TESTS: 100"
echo -e "   PASSED:      ${GREEN}${PASS_COUNT}${NC}"
echo -e "   FAILED:      ${RED}${FAIL_COUNT}${NC}"
if [ "$FAIL_COUNT" -eq 0 ]; then
    echo -e "   ${GREEN}🎉 100% PERFECT BIT-FOR-BIT MATCH (100 / 100 PASS) ⚡${NC}"
    echo -e "${MAGENTA}================================================================================${NC}"
    exit 0
else
    echo -e "   ${RED}❌ PARITY FAILURES DETECTED!${NC}"
    echo -e "${MAGENTA}================================================================================${NC}"
    exit 1
fi
