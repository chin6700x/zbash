#!/bin/bash
# ==============================================================================
# ⚡ ZBASH: Phase 3 Developer UX & Two-Line Starship Prompt Tests
# ==============================================================================
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

set -e
ZBASH_BIN="$(cd "$(dirname "$0")/../bin" && pwd)/zbash"

PASS=0
FAIL=0

test_case() {
    local name="$1"
    local result="$2"
    if [ "$result" -eq 0 ]; then
        echo "  [PASS] $name"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $name"
        FAIL=$((FAIL + 1))
    fi
}

echo "=== Running Zbash Phase 3 UX Test Suite ==="

# Test 1: Binary executable
"$ZBASH_BIN" -c "exit 0"
test_case "Binary executable and exit code 0" $?

# Test 2: --norc flag bypasses ~/.zbashrc
TMP_HOME=$(mktemp -d /tmp/zbash_test_home_XXXXXX)
echo 'export ZBASH_LOADED_RC="yes"' > "$TMP_HOME/.zbashrc"

OUT1=$(HOME="$TMP_HOME" python3 -c "
import pty, os, select
master, slave = pty.openpty()
if os.fork() == 0:
    os.close(master)
    os.dup2(slave, 0); os.dup2(slave, 1); os.dup2(slave, 2)
    os.close(slave)
    os.execv('$ZBASH_BIN', ['zbash'])
os.close(slave)
out = b''
os.write(master, b'echo RC:[\$ZBASH_LOADED_RC]\nexit\n')
while True:
    r, _, _ = select.select([master], [], [], 0.5)
    if not r: break
    d = os.read(master, 1024)
    if not d: break
    out += d
os.close(master)
print(out.decode('utf-8', errors='ignore'))
")

echo "$OUT1" | grep -q "RC:\[yes\]"
test_case "Auto-load ~/.zbashrc in interactive mode" $?

OUT2=$(HOME="$TMP_HOME" python3 -c "
import pty, os, select
master, slave = pty.openpty()
if os.fork() == 0:
    os.close(master)
    os.dup2(slave, 0); os.dup2(slave, 1); os.dup2(slave, 2)
    os.close(slave)
    os.execv('$ZBASH_BIN', ['zbash', '--norc'])
os.close(slave)
out = b''
os.write(master, b'echo RC:[\$ZBASH_LOADED_RC]\nexit\n')
while True:
    r, _, _ = select.select([master], [], [], 0.5)
    if not r: break
    d = os.read(master, 1024)
    if not d: break
    out += d
os.close(master)
print(out.decode('utf-8', errors='ignore'))
")

echo "$OUT2" | grep -q "RC:\[\]"
test_case "--norc flag ignores ~/.zbashrc" $?

# Test 3: Two-Line Starship Prompt contains ╭─, ⚡, zbash, and ╰─❯
OUT3=$(python3 -c "
import pty, os, select
master, slave = pty.openpty()
if os.fork() == 0:
    os.close(master)
    os.dup2(slave, 0); os.dup2(slave, 1); os.dup2(slave, 2)
    os.close(slave)
    os.environ['TERM'] = 'xterm-256color'
    os.execv('$ZBASH_BIN', ['zbash', '--norc'])
os.close(slave)
out = b''
os.write(master, b'exit\n')
while True:
    r, _, _ = select.select([master], [], [], 0.5)
    if not r: break
    d = os.read(master, 1024)
    if not d: break
    out += d
os.close(master)
print(out.decode('utf-8', errors='ignore'))
")

echo "$OUT3" | grep -q "╭─"
test_case "Two-Line prompt header frame ╭─ rendered" $?

echo "$OUT3" | grep -q "zbash"
test_case "Prompt contains zbash badge" $?

echo "$OUT3" | grep -q "git:(main)"
test_case "Prompt contains zero-fork git:(main) branch" $?

echo "$OUT3" | grep -q "╰─"
test_case "Two-Line prompt bottom frame ╰─ rendered" $?

# Test 4: ZBASH_CMD_DURATION tracking
OUT4=$(python3 -c "
import pty, os, select
master, slave = pty.openpty()
if os.fork() == 0:
    os.close(master)
    os.dup2(slave, 0); os.dup2(slave, 1); os.dup2(slave, 2)
    os.close(slave)
    os.environ['TERM'] = 'xterm-256color'
    os.execv('$ZBASH_BIN', ['zbash', '--norc'])
os.close(slave)
out = b''
os.write(master, b'sleep 0.02\necho DUR:\${ZBASH_CMD_DURATION}ms\nexit\n')
while True:
    r, _, _ = select.select([master], [], [], 0.5)
    if not r: break
    d = os.read(master, 1024)
    if not d: break
    out += d
os.close(master)
print(out.decode('utf-8', errors='ignore'))
")

echo "$OUT4" | grep -E -q "DUR:[0-9]+(\.[0-9]+)?ms"
test_case "High-Resolution ZBASH_CMD_DURATION tracked" $?

# Test 5: Dynamic Exit Code badge on error
OUT5=$(python3 -c "
import pty, os, select
master, slave = pty.openpty()
if os.fork() == 0:
    os.close(master)
    os.dup2(slave, 0); os.dup2(slave, 1); os.dup2(slave, 2)
    os.close(slave)
    os.environ['TERM'] = 'xterm-256color'
    os.execv('$ZBASH_BIN', ['zbash', '--norc'])
os.close(slave)
out = b''
os.write(master, b'false\nexit\n')
while True:
    r, _, _ = select.select([master], [], [], 0.5)
    if not r: break
    d = os.read(master, 1024)
    if not d: break
    out += d
os.close(master)
print(out.decode('utf-8', errors='ignore'))
")

echo "$OUT5" | grep -q "\[1\]"
test_case "Prompt displays [1] exit error badge on command failure" $?

rm -rf "$TMP_HOME"

echo ""
echo "Summary: $PASS passed, $FAIL failed."
if [ "$FAIL" -eq 0 ]; then
    echo "⚡ ALL PHASE 3 UX TESTS PASSED!"
    exit 0
else
    echo "❌ SOME TESTS FAILED!"
    exit 1
fi
