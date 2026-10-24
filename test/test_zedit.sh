#!/bin/bash
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$DIR/bin/zbash"

# 1. Compile test runner
clang -O3 -I"$DIR/include" \
    "$DIR/test/test_zedit.c" \
    "$DIR/src/editor/zedit_buffer.c" \
    "$DIR/src/editor/zedit_viewport.c" \
    "$DIR/src/editor/zedit_syntax.c" \
    "$DIR/src/editor/zedit_actions.c" \
    "$DIR/src/editor/zedit_core.c" \
    -o /tmp/test_zedit_runner

# 2. Run C unit tests
/tmp/test_zedit_runner
rm -f /tmp/test_zedit_runner

# 3. Test zedit builtin inside zbash executable
echo "Testing zedit command inside zbash binary..."
TYPE_OUTPUT=$("$BIN" -c "type zedit")
if [[ "$TYPE_OUTPUT" == *"zedit is a shell builtin"* ]]; then
    echo "  [\033[1;32mPASS\033[0m] 'type zedit' recognized as shell builtin"
else
    echo "  [\033[1;31mFAIL\033[0m] 'type zedit' not recognized as builtin: $TYPE_OUTPUT"
    exit 1
fi

USAGE_OUTPUT=$("$BIN" -c "zedit" 2>&1 || true)
if [[ "$USAGE_OUTPUT" == *"usage: zedit"* ]]; then
    echo "  [\033[1;32mPASS\033[0m] 'zedit' prints usage when called without argument"
else
    echo "  [\033[1;31mFAIL\033[0m] 'zedit' usage string unexpected: $USAGE_OUTPUT"
    exit 1
fi

echo -e "\n\033[1;32m⚡ ALL ZEDIT INTEGRATION TESTS PASSED 100%!\033[0m"
