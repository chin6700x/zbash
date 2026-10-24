/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

bool zbash_is_builtin(const char *name) {
    if (!name) return false;
    return (strcmp(name, "cd") == 0 ||
            strcmp(name, "pwd") == 0 ||
            strcmp(name, "echo") == 0 ||
            strcmp(name, "exit") == 0 ||
            strcmp(name, "export") == 0 ||
            strcmp(name, "unset") == 0 ||
            strcmp(name, "set") == 0 ||
            strcmp(name, "true") == 0 ||
            strcmp(name, ":") == 0 ||
            strcmp(name, "false") == 0 ||
            strcmp(name, "type") == 0 ||
            strcmp(name, "shift") == 0 ||
            strcmp(name, "exec") == 0 ||
            strcmp(name, "read") == 0 ||
            strcmp(name, "test") == 0 ||
            strcmp(name, "[") == 0 ||
            strcmp(name, "[[") == 0 ||
            strcmp(name, "let") == 0 ||
            strcmp(name, "declare") == 0 ||
            strcmp(name, "typeset") == 0 ||
            strcmp(name, "readonly") == 0 ||
            strcmp(name, "shopt") == 0 ||
            strcmp(name, "alias") == 0 ||
            strcmp(name, "unalias") == 0 ||
            strcmp(name, "source") == 0 ||
            strcmp(name, ".") == 0 ||
            strcmp(name, "eval") == 0 ||
            strcmp(name, "local") == 0 ||
            strcmp(name, "trap") == 0 ||
            strcmp(name, "wait") == 0 ||
            strcmp(name, "break") == 0 ||
            strcmp(name, "continue") == 0 ||
            strcmp(name, "return") == 0 ||
            strcmp(name, "builtin") == 0 ||
            strcmp(name, "command") == 0 ||
            strcmp(name, "umask") == 0 ||
            strcmp(name, "enable") == 0 ||
            strcmp(name, "times") == 0 ||
            strcmp(name, "hash") == 0 ||
            strcmp(name, "kill") == 0 ||
            strcmp(name, "printf") == 0 ||
            strcmp(name, "dirs") == 0 ||
            strcmp(name, "pushd") == 0 ||
            strcmp(name, "popd") == 0 ||
            strcmp(name, "caller") == 0 ||
            strcmp(name, "getopts") == 0 ||
            strcmp(name, "help") == 0 ||
            strcmp(name, "history") == 0 ||
            strcmp(name, "fc") == 0 ||
            strcmp(name, "jobs") == 0 ||
            strcmp(name, "fg") == 0 ||
            strcmp(name, "bg") == 0 ||
            strcmp(name, "disown") == 0 ||
            strcmp(name, "zedit") == 0);
}

bool zbash_is_builtin_enabled(zbash_state_t *state, const char *name) {
    if (!zbash_is_builtin(name)) return false;
    if (!state) return true;
    for (int i = 0; i < state->disabled_builtin_count; i++) {
        if (strcmp(state->disabled_builtins[i], name) == 0) return false;
    }
    return true;
}

int zbash_exec_builtin(zbash_state_t *state, int argc, char **argv) {
    if (argc < 1 || !argv[0]) return 0;
    const char *cmd = argv[0];

    if (strcmp(cmd, "echo") == 0) return builtin_echo(state, argc, argv);
    if (strcmp(cmd, "printf") == 0) return builtin_printf(state, argc, argv);
    if (strcmp(cmd, "dirs") == 0) return builtin_dirs(state, argc, argv);
    if (strcmp(cmd, "pushd") == 0) return builtin_pushd(state, argc, argv);
    if (strcmp(cmd, "popd") == 0) return builtin_popd(state, argc, argv);
    if (strcmp(cmd, "caller") == 0) return builtin_caller(state, argc, argv);
    if (strcmp(cmd, "cd") == 0) return builtin_cd(state, argc, argv);
    if (strcmp(cmd, "pwd") == 0) return builtin_pwd(argc, argv);
    if (strcmp(cmd, "exit") == 0) return builtin_exit(state, argc, argv);
    if (strcmp(cmd, "export") == 0) return builtin_export(state, argc, argv);
    if (strcmp(cmd, "unset") == 0) return builtin_unset(state, argc, argv);
    if (strcmp(cmd, "set") == 0) return builtin_set(state, argc, argv);
    if (strcmp(cmd, "true") == 0 || strcmp(cmd, ":") == 0) return 0;
    if (strcmp(cmd, "false") == 0) return 1;
    if (strcmp(cmd, "type") == 0) return builtin_type(state, argc, argv);
    if (strcmp(cmd, "test") == 0 || strcmp(cmd, "[") == 0 || strcmp(cmd, "[[") == 0) return builtin_test(state, argc, argv);
    if (strcmp(cmd, "let") == 0) return builtin_let(state, argc, argv);
    if (strcmp(cmd, "declare") == 0 || strcmp(cmd, "typeset") == 0 || strcmp(cmd, "readonly") == 0) return builtin_declare(state, argc, argv);
    if (strcmp(cmd, "shopt") == 0) return builtin_shopt(state, argc, argv);
    if (strcmp(cmd, "alias") == 0) return builtin_alias(state, argc, argv);
    if (strcmp(cmd, "unalias") == 0) return builtin_unalias(state, argc, argv);
    if (strcmp(cmd, "shift") == 0) return builtin_shift(state, argc, argv);
    if (strcmp(cmd, "read") == 0) return builtin_read(state, argc, argv);
    if (strcmp(cmd, "eval") == 0) return builtin_eval(state, argc, argv);
    if (strcmp(cmd, "local") == 0) return builtin_local(state, argc, argv);
    if (strcmp(cmd, "return") == 0) return builtin_return(state, argc, argv);
    if (strcmp(cmd, "builtin") == 0) return builtin_builtin(state, argc, argv);
    if (strcmp(cmd, "command") == 0) return builtin_command(state, argc, argv);
    if (strcmp(cmd, "getopts") == 0) return builtin_getopts(state, argc, argv);
    if (strcmp(cmd, "umask") == 0) return builtin_umask(argc, argv);
    if (strcmp(cmd, "enable") == 0) return builtin_enable(state, argc, argv);
    if (strcmp(cmd, "hash") == 0) return builtin_hash(state, argc, argv);
    if (strcmp(cmd, "times") == 0) return builtin_times();
    if (strcmp(cmd, "kill") == 0) return builtin_kill(state, argc, argv);
    if (strcmp(cmd, "break") == 0) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
        int lineno = state->lineno > 0 ? state->lineno : 1;
        if (state->loop_depth <= 0) {
            fprintf(stderr, "%s: line %d: break: only meaningful in a `for', `while', or `until' loop\n", src, lineno);
            return 0;
        }
        if (argc > 1 && atoi(argv[1]) <= 0) {
            fprintf(stderr, "%s: line %d: break: %s: loop count out of range\n", src, lineno, argv[1]);
            return 1;
        }
        state->loop_break = true;
        state->loop_break_count = (argc > 1) ? atoi(argv[1]) : 1;
        if (state->loop_break_count < 1) state->loop_break_count = 1;
        return 0;
    }
    if (strcmp(cmd, "continue") == 0) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
        int lineno = state->lineno > 0 ? state->lineno : 1;
        if (state->loop_depth <= 0) {
            fprintf(stderr, "%s: line %d: continue: only meaningful in a `for', `while', or `until' loop\n", src, lineno);
            return 0;
        }
        if (argc > 1 && atoi(argv[1]) <= 0) {
            fprintf(stderr, "%s: line %d: continue: %s: loop count out of range\n", src, lineno, argv[1]);
            return 1;
        }
        state->loop_continue = true;
        state->loop_continue_count = (argc > 1) ? atoi(argv[1]) : 1;
        if (state->loop_continue_count < 1) state->loop_continue_count = 1;
        return 0;
    }
    if (strcmp(cmd, "trap") == 0) return builtin_trap(state, argc, argv);
    if (strcmp(cmd, "wait") == 0) return builtin_wait(state, argc, argv);
    if (strcmp(cmd, "source") == 0 || strcmp(cmd, ".") == 0) {
        if (argc < 2) {
            fprintf(stderr, "zbash: %s: filename argument required\n", cmd);
            return 2;
        }
        if (state && state->restricted && strchr(argv[1], '/')) {
            const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
            int lineno = state->lineno > 0 ? state->lineno : 1;
            fprintf(stderr, "%s: line %d: %s: %s: restricted\n", src, lineno, cmd, argv[1]);
            return 1;
        }
        return zbash_source_file_args(state, argv[1], argc - 1, argv + 1);
    }
    if (strcmp(cmd, "exec") == 0) return builtin_exec(state, argc, argv);
    if (strcmp(cmd, "help") == 0) return builtin_help(argc, argv);
    if (strcmp(cmd, "history") == 0) return builtin_history(state, argc, argv);
    if (strcmp(cmd, "fc") == 0) return builtin_fc(state, argc, argv);
    if (strcmp(cmd, "jobs") == 0) return builtin_jobs(state, argc, argv);
    if (strcmp(cmd, "fg") == 0) return builtin_fg(state, argc, argv);
    if (strcmp(cmd, "bg") == 0) return builtin_bg(state, argc, argv);
    if (strcmp(cmd, "disown") == 0) return builtin_disown(state, argc, argv);
    if (strcmp(cmd, "zedit") == 0) return builtin_zedit(state, argc, argv);

    return 0;
}
