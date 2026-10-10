/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

static const char *const darwin_sig_names[] = {
    "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "EMT", "FPE",
    "KILL", "BUS", "SEGV", "SYS", "PIPE", "ALRM", "TERM", "URG",
    "STOP", "TSTP", "CONT", "CHLD", "TTIN", "TTOU", "IO", "XCPU",
    "XFSZ", "VTALRM", "PROF", "WINCH", "INFO", "USR1", "USR2"
};

static void print_signal_table(void) {
    for (int i = 1; i <= 31; i++) {
        printf("%2d) SIG%-6s\t", i, darwin_sig_names[i - 1]);
        if (i % 4 == 0 || i == 31) {
            printf("\n");
        }
    }
}

static int print_single_sig(zbash_state_t *state, const char *arg) {
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state->lineno > 0 ? state->lineno : 0;

    bool is_num = true;
    for (int k = 0; arg[k]; k++) {
        if (!isdigit((unsigned char)arg[k])) { is_num = false; break; }
    }

    if (is_num) {
        int sig = atoi(arg);
        if (sig > 128) sig -= 128;
        if (sig >= 1 && sig <= 31) {
            printf("%s\n", darwin_sig_names[sig - 1]);
            return 0;
        } else {
            fprintf(stderr, "%s: line %d: kill: %s: invalid signal specification\n", src, lineno, arg);
            return 1;
        }
    } else {
        const char *name = arg;
        if (strncasecmp(name, "SIG", 3) == 0) name += 3;
        for (int i = 0; i < 31; i++) {
            if (strcasecmp(name, darwin_sig_names[i]) == 0) {
                printf("%d\n", i + 1);
                return 0;
            }
        }
        fprintf(stderr, "%s: line %d: kill: %s: invalid signal specification\n", src, lineno, arg);
        return 1;
    }
}

static int parse_sigspec(const char *spec) {
    if (!spec || !*spec) return -1;
    if (strcasecmp(spec, "DEBUG") == 0) return 1000;
    if (strcasecmp(spec, "RETURN") == 0) return 1001;
    if (strcasecmp(spec, "ERR") == 0) return 1002;
    if (strcasecmp(spec, "EXIT") == 0 || strcmp(spec, "0") == 0) return 0;

    bool is_num = true;
    for (int k = 0; spec[k]; k++) {
        if (!isdigit((unsigned char)spec[k])) { is_num = false; break; }
    }
    if (is_num) {
        int n = atoi(spec);
        if (n >= 0 && n <= 31) return n;
        return -1;
    }

    const char *name = spec;
    if (strncasecmp(name, "SIG", 3) == 0) name += 3;
    for (int i = 0; i < 31; i++) {
        if (strcasecmp(name, darwin_sig_names[i]) == 0) {
            return i + 1;
        }
    }
    return -1;
}

static void print_single_trap(zbash_state_t *state, int code) {
    if (code == 0) {
        if (state->exit_trap) printf("trap -- '%s' EXIT\n", state->exit_trap);
    } else if (code == 1000) {
        if (state->debug_trap) printf("trap -- '%s' DEBUG\n", state->debug_trap);
    } else if (code == 1001) {
        if (state->return_trap) printf("trap -- '%s' RETURN\n", state->return_trap);
    } else if (code >= 1 && code <= 31) {
        if (state->sig_traps[code]) printf("trap -- '%s' SIG%s\n", state->sig_traps[code], darwin_sig_names[code - 1]);
    }
}

static void print_all_traps(zbash_state_t *state) {
    if (state->exit_trap) {
        printf("trap -- '%s' EXIT\n", state->exit_trap);
    }
    for (int s = 1; s <= 31; s++) {
        if (state->sig_traps[s]) {
            printf("trap -- '%s' SIG%s\n", state->sig_traps[s], darwin_sig_names[s - 1]);
        }
    }
    if (state->debug_trap) {
        printf("trap -- '%s' DEBUG\n", state->debug_trap);
    }
    if (state->return_trap) {
        printf("trap -- '%s' RETURN\n", state->return_trap);
    }
}

int builtin_trap(zbash_state_t *state, int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "-l") == 0) {
        print_signal_table();
        return 0;
    }
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "-p") == 0)) {
        print_all_traps(state);
        return 0;
    }

    if (strcmp(argv[1], "-p") == 0) {
        for (int i = 2; i < argc; i++) {
            int code = parse_sigspec(argv[i]);
            if (code >= 0) print_single_trap(state, code);
        }
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "-") == 0) {
        if (state->exit_trap) { free(state->exit_trap); state->exit_trap = NULL; }
        if (state->debug_trap) { free(state->debug_trap); state->debug_trap = NULL; }
        if (state->return_trap) { free(state->return_trap); state->return_trap = NULL; }
        for (int s = 1; s <= 31; s++) {
            if (state->sig_traps[s]) { free(state->sig_traps[s]); state->sig_traps[s] = NULL; }
            if (!state->sig_inherited_ign[s]) signal(s, SIG_DFL);
        }
        return 0;
    }

    const char *action = NULL;
    int sig_start = 2;

    bool all_signals = true;
    for (int i = 1; i < argc; i++) {
        if (parse_sigspec(argv[i]) < 0) { all_signals = false; break; }
    }

    if (all_signals) {
        action = "-";
        sig_start = 1;
    } else {
        action = argv[1];
        sig_start = 2;
    }

    for (int i = sig_start; i < argc; i++) {
        int code = parse_sigspec(argv[i]);
        if (code < 0) {
            const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
            int lineno = state->lineno > 0 ? state->lineno : 1;
            fprintf(stderr, "%s: line %d: trap: %s: invalid signal specification\n", src, lineno, argv[i]);
            return 1;
        }

        if (code == 0) {
            if (state->exit_trap) { free(state->exit_trap); state->exit_trap = NULL; }
            if (strcmp(action, "-") != 0) {
                state->exit_trap = strdup(action);
            }
        } else if (code == 1000) {
            if (state->debug_trap) { free(state->debug_trap); state->debug_trap = NULL; }
            if (strcmp(action, "-") != 0) {
                state->debug_trap = strdup(action);
            }
        } else if (code == 1001) {
            if (state->return_trap) { free(state->return_trap); state->return_trap = NULL; }
            if (strcmp(action, "-") != 0) {
                state->return_trap = strdup(action);
            }
        } else if (code >= 1 && code <= 31) {
            if (!state->interactive && state->sig_inherited_ign[code]) {
                continue;
            }
            if (state->sig_traps[code]) { free(state->sig_traps[code]); state->sig_traps[code] = NULL; }
            if (strcmp(action, "-") == 0) {
                signal(code, SIG_DFL);
            } else if (action[0] == '\0') {
                state->sig_traps[code] = strdup("");
                signal(code, SIG_IGN);
            } else {
                state->sig_traps[code] = strdup(action);
                signal(code, SIG_DFL);
            }
        }
    }
    return 0;
}

int builtin_kill(zbash_state_t *state, int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "kill: usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]\n");
        return 1;
    }

    if (strcmp(argv[1], "-l") == 0) {
        if (argc == 2) {
            print_signal_table();
            return 0;
        }
        int ret = 0;
        for (int i = 2; i < argc; i++) {
            if (print_single_sig(state, argv[i]) != 0) ret = 1;
        }
        return ret;
    }

    int sig = SIGTERM;
    int idx = 1;
    if (argv[1][0] == '-') {
        if (argv[1][1] == 's' && argc > 2) {
            const char *sname = argv[2];
            if (strncasecmp(sname, "SIG", 3) == 0) sname += 3;
            for (int i = 0; i < 31; i++) {
                if (strcasecmp(sname, darwin_sig_names[i]) == 0) { sig = i + 1; break; }
            }
            idx = 3;
        } else if (argv[1][1] == 'n' && argc > 2) {
            sig = atoi(argv[2]);
            idx = 3;
        } else if (isdigit((unsigned char)argv[1][1])) {
            sig = atoi(argv[1] + 1);
            idx = 2;
        } else {
            const char *sname = argv[1] + 1;
            if (strncasecmp(sname, "SIG", 3) == 0) sname += 3;
            for (int i = 0; i < 31; i++) {
                if (strcasecmp(sname, darwin_sig_names[i]) == 0) { sig = i + 1; break; }
            }
            idx = 2;
        }
    }

    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state && state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state ? (state->lineno > 0 ? state->lineno : 1) : 1;

    int ret = 0;
    for (int i = idx; i < argc; i++) {
        pid_t p = 0;
        int slot = -1;
        int r = zbash_parse_jobspec(state, argv[i], &p, &slot);
        if (r == -1) {
            fprintf(stderr, "%s: line %d: kill: %s: no such job\n", src, lineno, argv[i]);
            ret = 1;
            continue;
        } else if (r == -2) {
            fprintf(stderr, "%s: line %d: kill: %s: arguments must be process or job IDs\n", src, lineno, argv[i]);
            ret = 1;
            continue;
        }
        if (p > 0) {
            if (kill(p, sig) != 0) {
                ret = 1;
            }
        }
    }
    return ret;
}
