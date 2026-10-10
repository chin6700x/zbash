/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZBASH_CORE_INTERNAL_H
#define ZBASH_CORE_INTERNAL_H

#include "zbash.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <glob.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

/* Lexer / Parser shared parser interface */
extern int zbash_parse_line(zbash_state_t *state, const char *line, zbash_pipeline_t *pipelines, int max_pipelines);

/* Fast identifier verification */
static inline bool is_valid_ident(const char *s, size_t len) {
    if (len == 0 || len >= 128) return false;
    if (!isalpha((unsigned char)s[0]) && s[0] != '_') return false;
    for (size_t i = 1; i < len; i++) {
        if (!isalnum((unsigned char)s[i]) && s[i] != '_') return false;
    }
    return true;
}

static inline bool is_valid_func_name(const char *s, size_t len) {
    if (len == 0 || len >= 128) return false;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (isspace(c) || c == '/' || c == ';' || c == '&' || c == '|' ||
            c == '(' || c == ')' || c == '{' || c == '}' || c == '<' ||
            c == '>' || c == '$' || c == '`' || c == '"' || c == '\'') {
            return false;
        }
    }
    return true;
}

/* String and Token helpers */
bool is_word_match(const char *p, const char *word, size_t wlen);
bool is_statement_incomplete(const char *buf);
bool is_newline_separator(const char *start, size_t i);
bool is_assignment(const char *arg);
void free_cmd_strings(zbash_cmd_t *cmd);

/* Execution & Redirection (exec.c) */
int apply_redirections(zbash_state_t *state, zbash_cmd_t *cmd);
int zbash_apply_redirections_saved(zbash_state_t *state, zbash_cmd_t *cmd, int saved_fds[][2], int *saved_count);
void zbash_restore_redirections(zbash_state_t *state, int saved_fds[][2], int saved_count);
int execute_single_cmd(zbash_state_t *state, zbash_cmd_t *cmd);
int execute_pipeline(zbash_state_t *state, zbash_pipeline_t *pipeline);

/* Variables & Arrays (variables.c) */
bool check_array_def(zbash_state_t *state, const char *p);

/* Control structures & Functions (control.c) */
bool check_func_def(zbash_state_t *state, const char *p);
int exec_if(zbash_state_t *state, const char *cmd);
int exec_for_c(zbash_state_t *state, const char *cmd);
int exec_for_in(zbash_state_t *state, const char *cmd);
int exec_while(zbash_state_t *state, const char *cmd);
int exec_until(zbash_state_t *state, const char *cmd);
int exec_case(zbash_state_t *state, const char *cmd);

#endif /* ZBASH_CORE_INTERNAL_H */
