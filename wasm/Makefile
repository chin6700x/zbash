#
# ⚡ ZBASH: WebAssembly Build System
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#

EMCC = /opt/homebrew/bin/emcc
WEB_DIR = web
SRC_DIR = ../src
INCLUDE_DIR = ../include

SRCS = $(SRC_DIR)/core/engine.c \
       $(SRC_DIR)/core/prompt.c \
       $(SRC_DIR)/core/variables.c \
       $(SRC_DIR)/core/arith.c \
       $(SRC_DIR)/core/exec.c \
       $(SRC_DIR)/core/control.c \
       $(SRC_DIR)/parser/parser.c \
       $(SRC_DIR)/parser/cmd_parser.c \
       $(SRC_DIR)/parser/expand.c \
       $(SRC_DIR)/parser/pattern.c \
       $(SRC_DIR)/parser/braces.c \
       $(SRC_DIR)/builtins/builtins.c \
       $(SRC_DIR)/builtins/builtin_cd.c \
       $(SRC_DIR)/builtins/builtin_echo.c \
       $(SRC_DIR)/builtins/builtin_read.c \
       $(SRC_DIR)/builtins/builtin_test.c \
       $(SRC_DIR)/builtins/builtin_declare.c \
       $(SRC_DIR)/builtins/builtin_signal.c \
       $(SRC_DIR)/builtins/builtin_environ.c \
       $(SRC_DIR)/builtins/builtin_eval.c \
       $(SRC_DIR)/builtins/builtin_misc.c \
       $(SRC_DIR)/builtins/builtin_printf.c \
       $(SRC_DIR)/builtins/builtin_dstack.c \
       $(SRC_DIR)/builtins/builtin_caller.c \
       $(SRC_DIR)/builtins/builtin_history.c \
       $(SRC_DIR)/builtins/builtin_jobs.c \
       src/wasm_bridge.c

CFLAGS = -O3 -flto -Wall -Wextra -I$(INCLUDE_DIR) -D__EMSCRIPTEN__

LDFLAGS = -O3 -flto \
          -s WASM=1 \
          -s ALLOW_MEMORY_GROWTH=1 \
          -s NO_EXIT_RUNTIME=1 \
          -s FORCE_FILESYSTEM=1 \
          -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall","UTF8ToString","stringToUTF8","FS"]' \
          -s EXPORTED_FUNCTIONS='["_zbash_wasm_init","_zbash_wasm_eval","_zbash_wasm_get_stdout","_zbash_wasm_get_stderr","_zbash_wasm_clear_output","_main"]'

TARGET_JS = $(WEB_DIR)/zbash.js
TARGET_WASM = $(WEB_DIR)/zbash.wasm

all: $(TARGET_JS)

$(TARGET_JS): $(SRCS)
	@mkdir -p $(WEB_DIR)
	$(EMCC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS)
	@echo "  ⚡ zbash.wasm generated successfully in $(WEB_DIR)!"
	@ls -lh $(TARGET_JS) $(TARGET_WASM)

clean:
	rm -f $(TARGET_JS) $(TARGET_WASM) $(WEB_DIR)/*.map

.PHONY: all clean
