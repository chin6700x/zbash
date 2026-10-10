/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

int builtin_caller(zbash_state_t *state, int argc, char **argv) {
    if (!state) return 1;

    /* If not inside a function or frame_count < 2, caller returns 1 */
    if (state->frame_count < 2) {
        return 1;
    }

    if (argc == 1) {
        /* caller with no args: prints "<line> <file>" of caller of current subroutine */
        int top = state->frame_count - 1;
        int line = state->frames[top].call_line;
        const char *file = state->frames[top - 1].file_name;
        printf("%d %s\n", line, file);
        return 0;
    }

    /* argc > 1: caller [EXPR] */
    const char *expr = argv[1];
    char *endptr = NULL;
    long frame_idx = strtol(expr, &endptr, 10);
    if (*expr == '\0' || *endptr != '\0' || frame_idx < 0) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] :
                          ((state->argc > 0 && state->argv && state->argv[0]) ? state->argv[0] : "zbash");
        int lineno = state->lineno > 0 ? state->lineno : 0;
        fprintf(stderr, "%s: line %d: caller: %s: invalid number\n", src, lineno, expr);
        fprintf(stderr, "caller: usage: caller [EXPR]\n");
        return 1;
    }

    /* Total available caller frames is (state->frame_count - 1) */
    if (frame_idx >= (state->frame_count - 1)) {
        return 1;
    }

    int target = (state->frame_count - 1) - (int)frame_idx;
    int line = state->frames[target].call_line;
    const char *sub = state->frames[target - 1].func_name;
    const char *file = state->frames[target - 1].file_name;

    printf("%d %s %s\n", line, sub, file);
    return 0;
}
