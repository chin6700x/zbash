/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "zedit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int builtin_zedit(zbash_state_t *state, int argc, char **argv) {
    bool force_terminal = false;
    bool force_gui = false;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--terminal") == 0) {
            force_terminal = true;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--gui") == 0) {
            force_gui = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("usage: zedit [-t|-g] <filename>\n");
            printf("  -t, --terminal   Force in-terminal split micro-IDE\n");
            printf("  -g, --gui        Force desktop companion window (zedit-ui)\n");
            return 0;
        } else if (!filepath) {
            filepath = argv[i];
        }
    }

    if (!filepath) {
        if (g_zedit_session.is_active) {
            /* Toggle focus and resume interactive session */
            zedit_toggle_focus(&g_zedit_session);
            return zedit_run(&g_zedit_session, state);
        }
        fprintf(stderr, "usage: zedit [-t|-g] <filename>\n");
        return 1;
    }

    zedit_env_t env = zedit_detect_environment();

    /* 1. Try launching companion GUI window if requested or in GUI desktop */
    if (force_gui || (!force_terminal && env != ZEDIT_ENV_HEADLESS)) {
        if (zedit_spawn_gui(filepath)) {
            return 0;
        }
        /* If companion GUI was not found or failed, smoothly fallback to in-terminal */
    }

    /* 2. In-Terminal Split Micro-IDE */
    if (!zedit_open(&g_zedit_session, filepath)) {
        fprintf(stderr, "zedit: cannot open '%s'\n", filepath);
        return 1;
    }

    return zedit_run(&g_zedit_session, state);
}
