/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZBASH_HOVER_BOX_H
#define ZBASH_HOVER_BOX_H

#include "flag_db.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Show interactive Monaco-style floating help box overlaying terminal.
 * Invoked on F1, Alt-H, or Esc-h.
 * Returns true if help box was shown, false if no help available.
 */
bool zbash_show_hover_help(const char *line, size_t len, size_t cursor_pos);

/*
 * Dynamic JIT Man-Page Extractor (Phase 4):
 * Parses command synopsis and options from local manpages on the fly
 * and caches results in memory.
 */
const zbash_cmd_desc_t *zbash_jit_man_lookup(const char *cmd_name);

#ifdef __cplusplus
}
#endif

#endif /* ZBASH_HOVER_BOX_H */
