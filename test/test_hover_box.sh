#!/bin/bash
# ==============================================================================
# ⚡ ZBASH: Monaco-Grade Hover Box & Dynamic JIT Tests
# ==============================================================================
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR/.."

# Build test runner
clang -O3 -Wall -Wextra -Iinclude \
    src/interactive/flag_db.c \
    src/interactive/hover_box.c \
    test/test_hover_box.c \
    -o test/test_hover_box_bin

./test/test_hover_box_bin
rm -f test/test_hover_box_bin
