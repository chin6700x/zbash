/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZBASH_HIGHLIGHT_H
#define ZBASH_HIGHLIGHT_H

#include "zbash.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Token categories for syntax highlighting */
typedef enum {
    ZBASH_HL_DEFAULT = 0,
    ZBASH_HL_CMD_VALID,     /* Command exists in PATH (Bright Green) */
    ZBASH_HL_CMD_INVALID,   /* Command not found / error (Bright Red + Underline) */
    ZBASH_HL_BUILTIN,       /* zbash builtin command (Electric Cyan) */
    ZBASH_HL_FUNC,          /* User-defined shell function (Light Cyan) */
    ZBASH_HL_FLAG,          /* -a, --option (Warm Amber / Yellow) */
    ZBASH_HL_STRING,        /* 'single' or "double" quotes closed (Warm Orange) */
    ZBASH_HL_UNCLOSED_STR,  /* Unclosed quote alert (Dim Gray + Alert) */
    ZBASH_HL_OPERATOR,      /* |, &&, ||, ;, >, < (Neon Magenta) */
    ZBASH_HL_VARIABLE,      /* $VAR, ${VAR} (Sky Blue) */
    ZBASH_HL_COMMENT        /* # comment (Subdued Gray) */
} zbash_hl_token_type_t;

/* PATH Cache Interface: O(1) hash lookup for executables */
void zbash_path_cache_init(void);
void zbash_path_cache_free(void);
void zbash_path_cache_refresh(void);
bool zbash_path_cache_lookup(const char *cmd_name);

/* Highlight Engine: Transforms a raw input line into an ANSI colored string */
char *zbash_highlight_line(zbash_state_t *state, const char *line, size_t len, size_t cursor_pos);

/* Interactive Silicon Line Editor (SLE) with Live Highlighting */
char *zbash_read_line_highlighted(zbash_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* ZBASH_HIGHLIGHT_H */
