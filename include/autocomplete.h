/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZBASH_AUTOCOMPLETE_H
#define ZBASH_AUTOCOMPLETE_H

#include "zbash.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMPLETE_CMD_BUILTIN = 0,
    COMPLETE_CMD_PATH,
    COMPLETE_CMD_FUNC,
    COMPLETE_CMD_ALIAS,
    COMPLETE_HISTORY,
    COMPLETE_FLAG,
    COMPLETE_FILE,
    COMPLETE_DIR
} zbash_completion_type_t;

typedef struct {
    char full_text[256];       /* Full completed string: e.g. "whoami" */
    char display_suffix[256];  /* Ghost suffix to render: e.g. "ami" */
    char summary[128];         /* Short description: e.g. "print effective userid" */
    zbash_completion_type_t type;
} zbash_candidate_t;

typedef struct {
    zbash_candidate_t *items;
    size_t count;
    size_t capacity;
    size_t selected_index;
    size_t replace_start;      /* Offset in line_buf where token starts */
    size_t replace_len;        /* Length of token being replaced */
} zbash_candidate_list_t;

/* Initialize and refresh the sorted prefix command index */
void zbash_prefix_index_init(void);
void zbash_prefix_index_free(void);

/* Find the single best candidate for Inline Ghost Text (sub-50ns lookup) */
bool zbash_find_inline_ghost(zbash_state_t *state, const char *line, size_t len, 
                            size_t cursor_pos, zbash_candidate_t *out_ghost);

/* Populate all candidates matching prefix at cursor */
size_t zbash_get_completions(zbash_state_t *state, const char *line, size_t len, 
                             size_t cursor_pos, zbash_candidate_list_t *out_list);

/* Free memory allocated in candidate list */
void zbash_candidate_list_free(zbash_candidate_list_t *list);

/* Circular Inline Tab Cycling API */
bool zbash_tab_cycle_next(zbash_state_t *state, char *line_buf, size_t *line_len, 
                          size_t *cursor_pos, char *out_ribbon, size_t max_ribbon);
void zbash_tab_cycle_reset(void);
bool zbash_tab_cycle_is_active(void);
bool zbash_tab_cycle_cancel(char *line_buf, size_t *line_len, size_t *cursor_pos);

/* Display Monaco Visual Mini-Palette when multiple candidates exist on Tab */
bool zbash_show_completion_menu(zbash_state_t *state, char *line_buf, size_t *line_len, 
                               size_t *cursor_pos, int prompt_visual_width);

#ifdef __cplusplus
}
#endif

#endif /* ZBASH_AUTOCOMPLETE_H */
