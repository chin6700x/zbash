#
# ⚡ ZBASH: Bare-Metal Apple Silicon POSIX/Bash Modernization Suite
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

CC ?= clang
CFLAGS = -O3 -Wall -Wextra -pthread -fomit-frame-pointer -flto -Iinclude
ASFLAGS = -O3 -Wall

ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -Wl,-dead_strip -Wl,-fixup_chains
else
    CFLAGS += -D_GNU_SOURCE
    LDFLAGS = -Wl,--gc-sections
endif

BIN_DIR = bin
SRC_DIR = src

SRCS = src/main.c src/core/engine.c src/core/variables.c src/core/arith.c src/core/exec.c src/core/control.c \
       src/parser/parser.c \
       src/parser/cmd_parser.c \
       src/parser/expand.c \
       src/parser/pattern.c \
       src/parser/braces.c \
       src/builtins/builtins.c \
       src/builtins/builtin_cd.c \
       src/builtins/builtin_echo.c \
       src/builtins/builtin_read.c \
       src/builtins/builtin_test.c \
       src/builtins/builtin_declare.c \
       src/builtins/builtin_signal.c \
       src/builtins/builtin_environ.c \
       src/builtins/builtin_eval.c \
       src/builtins/builtin_misc.c \
       src/builtins/builtin_printf.c \
       src/builtins/builtin_dstack.c \
       src/builtins/builtin_caller.c \
       src/builtins/builtin_history.c \
       src/builtins/builtin_jobs.c
OBJS = $(SRCS:.c=.o)

ifeq ($(UNAME_M),arm64)
    ASM_SRCS = src/asm/simd_lexer.s
    ASM_OBJS = $(ASM_SRCS:.s=.o)
    OBJS += $(ASM_OBJS)
    CFLAGS += -DZBASH_USE_NEON
endif

all: $(BIN_DIR)/zbash

$(BIN_DIR)/zbash: $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)
	@ln -sf zbash $(BIN_DIR)/xbash
	@codesign -s - -f $@ 2>/dev/null || true

DEPS = $(SRCS:.c=.d)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.o: %.s
	$(CC) $(ASFLAGS) -c $< -o $@

-include $(DEPS)

test: $(BIN_DIR)/zbash
	@bash test/test_zbash.sh

arena: $(BIN_DIR)/zbash
	@bash arena/arena_zbash.sh

clean:
	rm -f $(OBJS) $(DEPS) $(BIN_DIR)/zbash $(BIN_DIR)/xbash

.PHONY: all test arena clean
