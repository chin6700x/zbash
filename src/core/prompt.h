/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZBASH_PROMPT_H
#define ZBASH_PROMPT_H

#include "zbash.h"
#include <stdbool.h>
#include <stddef.h>

bool zbash_get_git_info(char *branch_out, size_t maxlen, bool *is_dirty);
void zbash_format_short_path(const char *pwd, char *out, size_t maxlen);
char *zbash_build_prompt(zbash_state_t *state, bool is_libedit);
void zbash_load_rc(zbash_state_t *state);

#endif /* ZBASH_PROMPT_H */
