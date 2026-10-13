#!/bin/bash
# ==============================================================================
# ⚡ ZBASH: Autocomplete & Ghost Text Unit Test Runner
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
    src/interactive/autocomplete.c \
    src/interactive/flag_db.c \
    src/interactive/hover_box.c \
    src/interactive/highlight.o \
    src/core/prompt.o \
    src/builtins/builtins.o \
    src/core/exec.o \
    src/core/control.o \
    src/core/variables.o \
    src/parser/pattern.o \
    src/parser/expand.o \
    src/parser/braces.o \
    src/parser/cmd_parser.o \
    src/builtins/builtin_declare.o \
    src/builtins/builtin_history.o \
    test/test_autocomplete.c \
    -o test/test_autocomplete_bin

./test/test_autocomplete_bin
rm -f test/test_autocomplete_bin
