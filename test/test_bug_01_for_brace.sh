#!/usr/bin/env bash
#
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# 🧪 Regression Test: BUG-FIX-01 (For-Loop Brace Expansion Collision)
#

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ZBASH="$DIR/bin/zbash"

echo "🧪 Running Regression Test Suite: BUG-FIX-01 (For-Loop Brace Expansion)..."

# Case 1: Simple loop with brace expansion
out1=$("$ZBASH" -c 'for i in {1..3}; do echo -n "$i "; done')
if [ "$out1" != "1 2 3 " ]; then
    echo "❌ Case 1 failed: expected '1 2 3 ', got '$out1'"
    exit 1
fi
echo "  ✓ Case 1: Simple loop with brace expansion PASS"

# Case 2: Nested loops with brace expansion
out2=$("$ZBASH" -c 'for i in {1..2}; do for j in {a..b}; do echo -n "$i$j "; done; done')
if [ "$out2" != "1a 1b 2a 2b " ]; then
    echo "❌ Case 2 failed: expected '1a 1b 2a 2b ', got '$out2'"
    exit 1
fi
echo "  ✓ Case 2: Nested loops with brace expansion PASS"

# Case 3: Multiple consecutive loops with brace expansion in multi-line script
TMP_SCRIPT="/tmp/zbash_test_bug_01_$$.sh"
cat << 'EOF' > "$TMP_SCRIPT"
for i in {1..3}; do
    echo "outer $i"
    for j in a b; do
        echo "inner $j"
    done
done

for x in 1 2; do
    echo "second loop $x"
done
EOF

out3=$("$ZBASH" "$TMP_SCRIPT")
rm -f "$TMP_SCRIPT"

if ! echo "$out3" | grep -q "second loop 1"; then
    echo "❌ Case 3 failed: 'second loop 1' missing from output"
    exit 1
fi
if ! echo "$out3" | grep -q "second loop 2"; then
    echo "❌ Case 3 failed: 'second loop 2' missing from output"
    exit 1
fi
echo "  ✓ Case 3: Sequential loops with brace expansion in script stream PASS"

# Case 4: C-style arithmetic for with braces still works
out4=$("$ZBASH" -c 'for ((i=1; i<=3; i++)) { echo -n "$i "; }')
if [ "$out4" != "1 2 3 " ]; then
    echo "❌ Case 4 failed: expected '1 2 3 ', got '$out4'"
    exit 1
fi
echo "  ✓ Case 4: C-style arithmetic for with braces PASS"

echo "🏆 ALL REGRESSION CASES PASSED! BUG-FIX-01 IS FULLY RESOLVED."
