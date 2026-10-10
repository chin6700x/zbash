/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZBASH_PARSER_INTERNAL_H
#define ZBASH_PARSER_INTERNAL_H

#include "zbash.h"
#include <ctype.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* UTF-8 and pattern operations (src/parser/pattern.c) */
bool is_utf8_mode(void);
size_t utf8_char_count(const char *s);
const char *utf8_char_at(const char *s, int char_idx);
char *apply_pattern_removal(const char *env_v, const char *pat, bool is_prefix, bool longest);
char *apply_pattern_subst(const char *env_v, const char *raw_pat, const char *replacement, bool replace_all);
char *zbash_expand_pattern(zbash_state_t *state, const char *pat);

/* Command parsing (src/parser/cmd_parser.c) */
void zbash_parse_simple_cmd(zbash_state_t *state, const char *cmd_str, zbash_cmd_t *cmd);

/* Pipeline & Line parsing (src/parser/parser.c) */
int zbash_parse_line(zbash_state_t *state, const char *line, zbash_pipeline_t *pipelines, int max_pipelines);

#endif /* ZBASH_PARSER_INTERNAL_H */
