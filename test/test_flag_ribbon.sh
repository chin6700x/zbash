#!/bin/bash
# ==============================================================================
# ⚡ ZBASH: Monaco-Grade Flag Introspection & Ribbon Tests
# ==============================================================================
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================

set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR/.."

# Build test runner
clang -O3 -Wall -Wextra -Iinclude src/interactive/flag_db.c test/test_flag_db.c -o test/test_flag_db_bin

./test/test_flag_db_bin
rm -f test/test_flag_db_bin
