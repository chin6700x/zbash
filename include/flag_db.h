/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZBASH_FLAG_DB_H
#define ZBASH_FLAG_DB_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char short_opt;          /* e.g. 'l' for -l, 0 if long only */
    const char *long_opt;    /* e.g. "--all" or NULL */
    const char *arg_desc;    /* e.g. "<prompt>" or NULL */
    const char *description; /* e.g. "do not allow backslashes to escape characters" */
} zbash_flag_entry_t;

typedef struct {
    const char *cmd_name;
    const char *synopsis;    /* e.g. "tar [-c|-x|-t] [options] [archive] [files...]" */
    const char *summary;     /* e.g. "manipulate tape archives" */
    const zbash_flag_entry_t *flags;
} zbash_cmd_desc_t;

/* Lookup command metadata */
const zbash_cmd_desc_t *zbash_find_cmd_desc(const char *cmd_name);

/* Lookup specific flag for a command */
const zbash_flag_entry_t *zbash_find_flag(const zbash_cmd_desc_t *desc, char short_opt, const char *long_opt);

/* Inspect current line and cursor position to generate a 1-line Ribbon string (returns allocated ANSI string or NULL) */
char *zbash_inspect_ribbon(const char *line, size_t len, size_t cursor_pos);

#ifdef __cplusplus
}
#endif

#endif /* ZBASH_FLAG_DB_H */
