/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

int builtin_exit(zbash_state_t *state, int argc, char **argv) {
    if (argc > 1) {
        const char *arg = argv[1];
        int start = (arg[0] == '+' || arg[0] == '-') ? 1 : 0;
        bool is_num = (arg[start] != '\0');
        for (int k = start; arg[k]; k++) {
            if (!isdigit((unsigned char)arg[k])) { is_num = false; break; }
        }
        if (!is_num) {
            const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
            int lineno = state->lineno > 0 ? state->lineno : 0;
            fprintf(stderr, "%s: line %d: exit: %s: numeric argument required\n", src, lineno, arg);
            state->exit_code = 255;
            state->should_exit = true;
            return 255;
        }
        state->exit_code = atoi(arg);
    } else {
        state->exit_code = state->last_status;
    }

    if (state->exit_trap) {
        char *t = state->exit_trap;
        state->exit_trap = NULL;
        state->should_exit = false;
        zbash_run_line(state, t);
        free(t);
    }
    state->should_exit = true;
    return state->exit_code;
}

int builtin_wait(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state && state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state ? (state->lineno > 0 ? state->lineno : 1) : 1;

    if (argc <= 1) {
        int status;
        while (waitpid(-1, &status, 0) > 0) {
            if (state && state->sig_traps[SIGCHLD] && state->sig_traps[SIGCHLD][0] != '\0') {
                zbash_run_line(state, state->sig_traps[SIGCHLD]);
            }
        }
        return 0;
    }

    int last_rc = 0;
    for (int i = 1; i < argc; i++) {
        pid_t p = 0;
        int slot = -1;
        int r = zbash_parse_jobspec(state, argv[i], &p, &slot);
        if (r == -1) {
            fprintf(stderr, "%s: line %d: wait: %s: no such job\n", src, lineno, argv[i]);
            last_rc = 127;
            continue;
        } else if (r == -2) {
            fprintf(stderr, "%s: line %d: wait: `%s': not a pid or valid job spec\n", src, lineno, argv[i]);
            last_rc = 127;
            continue;
        }

        bool is_child = (slot >= 0) || (p == state->last_bg_pid);
        if (!is_child) {
            for (int k = 0; k < 64; k++) {
                if (state->jobs[k].pid == p) { is_child = true; break; }
            }
        }

        if (p > 0) {
            int status = 0;
            pid_t w = waitpid(p, &status, 0);
            if (w > 0) {
                if (state && state->sig_traps[SIGCHLD] && state->sig_traps[SIGCHLD][0] != '\0') {
                    zbash_run_line(state, state->sig_traps[SIGCHLD]);
                }
                last_rc = WIFEXITED(status) ? WEXITSTATUS(status) : (128 + WTERMSIG(status));
                if (slot >= 0) {
                    state->jobs[slot].pid = 0;
                    state->jobs[slot].job_id = 0;
                    state->job_count--;
                }
            } else {
                if (!is_child) {
                    fprintf(stderr, "%s: line %d: wait: pid %d is not a child of this shell\n", src, lineno, p);
                    last_rc = 127;
                } else {
                    last_rc = 0;
                }
            }
        }
    }
    return last_rc;
}

typedef struct {
    const char *name;
    bool val;
} shopt_entry_t;

static shopt_entry_t shopt_table[] = {
    {"cdable_vars", false},
    {"cdspell", false},
    {"checkhash", false},
    {"checkwinsize", false},
    {"cmdhist", true},
    {"compat31", false},
    {"dotglob", false},
    {"execfail", false},
    {"expand_aliases", true},
    {"extdebug", false},
    {"extglob", false},
    {"extquote", true},
    {"failglob", false},
    {"force_fignore", true},
    {"gnu_errfmt", false},
    {"histappend", false},
    {"histreedit", false},
    {"histverify", false},
    {"hostcomplete", true},
    {"huponexit", false},
    {"interactive_comments", true},
    {"lithist", false},
    {"login_shell", false},
    {"mailwarn", false},
    {"no_empty_cmd_completion", false},
    {"nocaseglob", false},
    {"nocasematch", false},
    {"nullglob", false},
    {"progcomp", true},
    {"promptvars", true},
    {"restricted_shell", false},
    {"shift_verbose", false},
    {"sourcepath", true},
    {"xpg_echo", false},
};
#define NUM_SHOPT_OPTIONS (sizeof(shopt_table)/sizeof(shopt_table[0]))

static shopt_entry_t set_o_table[] = {
    {"allexport", false},
    {"braceexpand", true},
    {"emacs", true},
    {"errexit", false},
    {"errtrace", false},
    {"functrace", false},
    {"hashall", true},
    {"histexpand", true},
    {"history", true},
    {"ignoreeof", false},
    {"interactive-comments", true},
    {"keyword", false},
    {"monitor", true},
    {"noclobber", false},
    {"noexec", false},
    {"noglob", false},
    {"nolog", false},
    {"notify", false},
    {"nounset", false},
    {"onecmd", false},
    {"physical", false},
    {"pipefail", false},
    {"posix", false},
    {"privileged", false},
    {"verbose", false},
    {"vi", false},
    {"xtrace", false},
};
#define NUM_SET_O_OPTIONS (sizeof(set_o_table)/sizeof(set_o_table[0]))

static bool get_shopt_val(zbash_state_t *state, int idx) {
    if (strcmp(shopt_table[idx].name, "expand_aliases") == 0) return state ? state->expand_aliases : shopt_table[idx].val;
    if (strcmp(shopt_table[idx].name, "xpg_echo") == 0) return state ? state->xpg_echo : shopt_table[idx].val;
    if (strcmp(shopt_table[idx].name, "sourcepath") == 0) return state ? state->sourcepath : shopt_table[idx].val;
    if (strcmp(shopt_table[idx].name, "extdebug") == 0) return state ? state->extdebug : shopt_table[idx].val;
    if (strcmp(shopt_table[idx].name, "extglob") == 0) return state ? state->extglob : shopt_table[idx].val;
    return shopt_table[idx].val;
}

static void set_shopt_val(zbash_state_t *state, int idx, bool val) {
    shopt_table[idx].val = val;
    if (strcmp(shopt_table[idx].name, "expand_aliases") == 0 && state) state->expand_aliases = val;
    if (strcmp(shopt_table[idx].name, "xpg_echo") == 0 && state) state->xpg_echo = val;
    if (strcmp(shopt_table[idx].name, "sourcepath") == 0 && state) state->sourcepath = val;
    if (strcmp(shopt_table[idx].name, "extdebug") == 0 && state) state->extdebug = val;
    if (strcmp(shopt_table[idx].name, "extglob") == 0 && state) state->extglob = val;
}

static bool get_set_o_val(zbash_state_t *state, int idx) {
    if (strcmp(set_o_table[idx].name, "errexit") == 0) return state ? state->errexit : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "xtrace") == 0) return state ? state->xtrace : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "nounset") == 0) return state ? state->nounset : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "posix") == 0) return state ? state->posix : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "noclobber") == 0) return state ? state->noclobber : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "functrace") == 0) return state ? state->functrace : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "verbose") == 0) return state ? state->verbose : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "noexec") == 0) return state ? state->noexec : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "history") == 0) return state ? state->history_enabled : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "histexpand") == 0) return state ? state->histexp_enabled : set_o_table[idx].val;
    if (strcmp(set_o_table[idx].name, "monitor") == 0) return state ? state->monitor_mode : set_o_table[idx].val;
    return set_o_table[idx].val;
}

static void set_set_o_val(zbash_state_t *state, int idx, bool val) {
    set_o_table[idx].val = val;
    if (strcmp(set_o_table[idx].name, "errexit") == 0 && state) state->errexit = val;
    if (strcmp(set_o_table[idx].name, "xtrace") == 0 && state) state->xtrace = val;
    if (strcmp(set_o_table[idx].name, "nounset") == 0 && state) state->nounset = val;
    if (strcmp(set_o_table[idx].name, "posix") == 0 && state) state->posix = val;
    if (strcmp(set_o_table[idx].name, "noclobber") == 0 && state) state->noclobber = val;
    if (strcmp(set_o_table[idx].name, "functrace") == 0 && state) state->functrace = val;
    if (strcmp(set_o_table[idx].name, "verbose") == 0 && state) state->verbose = val;
    if (strcmp(set_o_table[idx].name, "noexec") == 0 && state) state->noexec = val;
    if (strcmp(set_o_table[idx].name, "history") == 0 && state) {
        state->history_enabled = val;
        if (val && state->history_count == 0) {
            zbash_history_load(state, NULL);
        }
    }
    if (strcmp(set_o_table[idx].name, "histexpand") == 0 && state) state->histexp_enabled = val;
    if (strcmp(set_o_table[idx].name, "monitor") == 0 && state) state->monitor_mode = val;
}

bool set_set_o_option_by_name(zbash_state_t *state, const char *name, bool turn_on) {
    for (size_t i = 0; i < NUM_SET_O_OPTIONS; i++) {
        if (strcmp(set_o_table[i].name, name) == 0) {
            set_set_o_val(state, i, turn_on);
            return true;
        }
    }
    return false;
}

void print_set_o_table(zbash_state_t *state, bool reusable) {
    for (size_t i = 0; i < NUM_SET_O_OPTIONS; i++) {
        bool val = get_set_o_val(state, i);
        if (reusable) {
            printf("set %co %s\n", val ? '-' : '+', set_o_table[i].name);
        } else {
            printf("%-15s\t%s\n", set_o_table[i].name, val ? "on" : "off");
        }
    }
}

int builtin_shopt(zbash_state_t *state, int argc, char **argv) {
    bool mode_set = false;
    bool mode_unset = false;
    bool mode_quiet = false;
    bool mode_print = false;
    bool mode_o = false;
    int idx = 1;

    while (idx < argc && argv[idx][0] == '-' && argv[idx][1] != '\0') {
        if (strcmp(argv[idx], "--") == 0) { idx++; break; }
        const char *opt = argv[idx] + 1;
        while (*opt) {
            if (*opt == 's') mode_set = true;
            else if (*opt == 'u') mode_unset = true;
            else if (*opt == 'q') mode_quiet = true;
            else if (*opt == 'p') mode_print = true;
            else if (*opt == 'o') mode_o = true;
            else {
                const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state && state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
                int lineno = state ? state->lineno : 0;
                if (lineno > 0) {
                    fprintf(stderr, "%s: line %d: shopt: -%c: invalid option\nshopt: usage: shopt [-pqsu] [-o long-option] optname [optname...]\n", src, lineno, *opt);
                } else {
                    fprintf(stderr, "%s: shopt: -%c: invalid option\nshopt: usage: shopt [-pqsu] [-o long-option] optname [optname...]\n", src, *opt);
                }
                return 2;
            }
            opt++;
        }
        idx++;
    }

    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state && state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state ? state->lineno : 0;

    /* If option names are provided */
    if (idx < argc) {
        int ret = 0;
        for (int i = idx; i < argc; i++) {
            const char *optname = argv[i];
            int found = -1;
            if (mode_o) {
                for (size_t k = 0; k < NUM_SET_O_OPTIONS; k++) {
                    if (strcmp(set_o_table[k].name, optname) == 0) { found = (int)k; break; }
                }
            } else {
                for (size_t k = 0; k < NUM_SHOPT_OPTIONS; k++) {
                    if (strcmp(shopt_table[k].name, optname) == 0) { found = (int)k; break; }
                }
            }

            if (found < 0) {
                if (lineno > 0) {
                    fprintf(stderr, "%s: line %d: shopt: %s: invalid %soption name\n", src, lineno, optname, mode_o ? "" : "shell ");
                } else {
                    fprintf(stderr, "%s: shopt: %s: invalid %soption name\n", src, optname, mode_o ? "" : "shell ");
                }
                ret = 1;
                continue;
            }

            if (mode_set) {
                if (mode_o) set_set_o_val(state, found, true);
                else set_shopt_val(state, found, true);
            } else if (mode_unset) {
                if (mode_o) set_set_o_val(state, found, false);
                else set_shopt_val(state, found, false);
            }

            bool val = mode_o ? get_set_o_val(state, found) : get_shopt_val(state, found);

            if (mode_print) {
                if (mode_set && !val) continue;
                if (mode_unset && val) continue;
                if (mode_o) {
                    printf("set %co %s\n", val ? '-' : '+', set_o_table[found].name);
                } else {
                    printf("shopt -%c %s\n", val ? 's' : 'u', shopt_table[found].name);
                }
            } else if (!mode_set && !mode_unset && !mode_quiet) {
                printf("%-15s\t%s\n", mode_o ? set_o_table[found].name : shopt_table[found].name, val ? "on" : "off");
            }
            if (!val) {
                if (!mode_set && !mode_unset) ret = 1;
            }
        }
        return ret;
    }

    /* No option names provided: print all */
    size_t count = mode_o ? NUM_SET_O_OPTIONS : NUM_SHOPT_OPTIONS;
    for (size_t k = 0; k < count; k++) {
        bool val = mode_o ? get_set_o_val(state, k) : get_shopt_val(state, k);
        const char *name = mode_o ? set_o_table[k].name : shopt_table[k].name;

        if (mode_set && !val) continue;
        if (mode_unset && val) continue;

        if (mode_print) {
            if (mode_o) {
                printf("set %co %s\n", val ? '-' : '+', name);
            } else {
                printf("shopt -%c %s\n", val ? 's' : 'u', name);
            }
        } else {
            printf("%-15s\t%s\n", name, val ? "on" : "off");
        }
    }
    return 0;
}

int builtin_alias(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state && state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state ? (state->lineno > 0 ? state->lineno : 0) : 0;

    if (argc == 1 || (argc == 2 && strcmp(argv[1], "-p") == 0)) {
        for (int i = 0; i < state->alias_count; i++) {
            printf("alias %s='%s'\n", state->aliases[i].name, state->aliases[i].val);
        }
        return 0;
    }

    if (argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0' && strcmp(argv[1], "--") != 0 && strcmp(argv[1], "-p") != 0) {
        if (lineno > 0) {
            fprintf(stderr, "%s: line %d: alias: %s: invalid option\nalias: usage: alias [-p] [name[=value] ... ]\n", src, lineno, argv[1]);
        } else {
            fprintf(stderr, "%s: alias: %s: invalid option\nalias: usage: alias [-p] [name[=value] ... ]\n", src, argv[1]);
        }
        return 2;
    }

    int rc = 0;
    int start_i = (argc > 1 && strcmp(argv[1], "--") == 0) ? 2 : 1;
    for (int i = start_i; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (eq) {
            *eq = '\0';
            const char *name = argv[i];
            const char *val = eq + 1;
            bool found = false;
            for (int a = 0; a < state->alias_count; a++) {
                if (strcmp(state->aliases[a].name, name) == 0) {
                    free(state->aliases[a].val);
                    state->aliases[a].val = strdup(val);
                    found = true;
                    break;
                }
            }
            if (!found && state->alias_count < ZBASH_MAX_ALIASES) {
                state->aliases[state->alias_count].name = strdup(name);
                state->aliases[state->alias_count].val = strdup(val);
                state->alias_count++;
            }
            *eq = '=';
        } else {
            bool found = false;
            for (int a = 0; a < state->alias_count; a++) {
                if (strcmp(state->aliases[a].name, argv[i]) == 0) {
                    printf("alias %s='%s'\n", state->aliases[a].name, state->aliases[a].val);
                    found = true;
                    break;
                }
            }
            if (!found) {
                if (lineno > 0) {
                    fprintf(stderr, "%s: line %d: alias: %s: not found\n", src, lineno, argv[i]);
                } else {
                    fprintf(stderr, "%s: alias: %s: not found\n", src, argv[i]);
                }
                rc = 1;
            }
        }
    }
    return rc;
}

int builtin_unalias(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state && state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state ? (state->lineno > 0 ? state->lineno : 0) : 0;

    if (argc < 2) {
        if (lineno > 0) {
            fprintf(stderr, "%s: line %d: unalias: usage: unalias [-a] name [name ...]\n", src, lineno);
        } else {
            fprintf(stderr, "unalias: usage: unalias [-a] name [name ...]\n");
        }
        return 1;
    }

    if (argv[1][0] == '-' && argv[1][1] != '\0' && strcmp(argv[1], "-a") != 0 && strcmp(argv[1], "--") != 0) {
        if (lineno > 0) {
            fprintf(stderr, "%s: line %d: unalias: %s: invalid option\nunalias: usage: unalias [-a] name [name ...]\n", src, lineno, argv[1]);
        } else {
            fprintf(stderr, "%s: unalias: %s: invalid option\nunalias: usage: unalias [-a] name [name ...]\n", src, argv[1]);
        }
        return 2;
    }

    if (strcmp(argv[1], "-a") == 0) {
        for (int i = 0; i < state->alias_count; i++) {
            free(state->aliases[i].name);
            free(state->aliases[i].val);
        }
        state->alias_count = 0;
        return 0;
    }

    int rc = 0;
    int start_i = (strcmp(argv[1], "--") == 0) ? 2 : 1;
    for (int i = start_i; i < argc; i++) {
        bool found = false;
        for (int a = 0; a < state->alias_count; a++) {
            if (strcmp(state->aliases[a].name, argv[i]) == 0) {
                free(state->aliases[a].name);
                free(state->aliases[a].val);
                for (int j = a; j < state->alias_count - 1; j++) {
                    state->aliases[j] = state->aliases[j + 1];
                }
                state->alias_count--;
                found = true;
                break;
            }
        }
        if (!found) {
            if (lineno > 0) {
                fprintf(stderr, "%s: line %d: unalias: %s: not found\n", src, lineno, argv[i]);
            } else {
                fprintf(stderr, "%s: unalias: %s: not found\n", src, argv[i]);
            }
            rc = 1;
        }
    }
    return rc;
}

int builtin_umask(int argc, char **argv) {
    bool print_posix = false;
    bool symbolic = false;
    int idx = 1;

    while (idx < argc && argv[idx][0] == '-' && argv[idx][1] != '\0') {
        const char *opt = argv[idx] + 1;
        if (strcmp(argv[idx], "--") == 0) { idx++; break; }
        if (strcmp(argv[idx], "-S") == 0) { symbolic = true; idx++; continue; }
        if (strcmp(argv[idx], "-p") == 0) { print_posix = true; idx++; continue; }
        bool recognized = false;
        while (*opt) {
            if (*opt == 'p') { print_posix = true; recognized = true; }
            else if (*opt == 'S') { symbolic = true; recognized = true; }
            opt++;
        }
        if (!recognized) break;
        idx++;
    }

    mode_t cur = umask(0);
    umask(cur);

    if (idx >= argc) {
        if (symbolic) {
            char u_str[4] = {0}, g_str[4] = {0}, o_str[4] = {0};
            int u_len = 0, g_len = 0, o_len = 0;
            if (!(cur & 0400)) u_str[u_len++] = 'r';
            if (!(cur & 0200)) u_str[u_len++] = 'w';
            if (!(cur & 0100)) u_str[u_len++] = 'x';
            if (!(cur & 0040)) g_str[g_len++] = 'r';
            if (!(cur & 0020)) g_str[g_len++] = 'w';
            if (!(cur & 0010)) g_str[g_len++] = 'x';
            if (!(cur & 0004)) o_str[o_len++] = 'r';
            if (!(cur & 0002)) o_str[o_len++] = 'w';
            if (!(cur & 0001)) o_str[o_len++] = 'x';

            if (print_posix) {
                printf("umask -S u=%s,g=%s,o=%s\n", u_str, g_str, o_str);
            } else {
                printf("u=%s,g=%s,o=%s\n", u_str, g_str, o_str);
            }
        } else {
            if (print_posix) {
                printf("umask %04o\n", (unsigned int)cur);
            } else {
                printf("%04o\n", (unsigned int)cur);
            }
        }
        fflush(stdout);
        return 0;
    }

    const char *arg = argv[idx];
    if (isdigit((unsigned char)arg[0])) {
        mode_t new_mask = (mode_t)strtol(arg, NULL, 8);
        umask(new_mask);
        return 0;
    }

    mode_t m = cur;
    char buf[256];
    strncpy(buf, arg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *saveptr = NULL;
    char *clause = strtok_r(buf, ",", &saveptr);
    while (clause) {
        char *eq = strchr(clause, '=');
        if (!eq) {
            fprintf(stderr, "umask: `%c': invalid symbolic mode character\n", clause[0]);
            return 1;
        }
        *eq = '\0';
        const char *who = clause;
        const char *perms = eq + 1;
        for (const char *pc = perms; *pc; pc++) {
            if (*pc != 'r' && *pc != 'w' && *pc != 'x') {
                fprintf(stderr, "umask: `%c': invalid symbolic mode character\n", *pc);
                return 1;
            }
        }
        bool apply_u = (strchr(who, 'u') != NULL || strchr(who, 'a') != NULL || *who == '\0');
        bool apply_g = (strchr(who, 'g') != NULL || strchr(who, 'a') != NULL || *who == '\0');
        bool apply_o = (strchr(who, 'o') != NULL || strchr(who, 'a') != NULL || *who == '\0');

        if (apply_u) {
            m |= 0700;
            if (strchr(perms, 'r')) m &= ~0400;
            if (strchr(perms, 'w')) m &= ~0200;
            if (strchr(perms, 'x')) m &= ~0100;
        }
        if (apply_g) {
            m |= 0070;
            if (strchr(perms, 'r')) m &= ~0040;
            if (strchr(perms, 'w')) m &= ~0020;
            if (strchr(perms, 'x')) m &= ~0010;
        }
        if (apply_o) {
            m |= 0007;
            if (strchr(perms, 'r')) m &= ~0004;
            if (strchr(perms, 'w')) m &= ~0002;
            if (strchr(perms, 'x')) m &= ~0001;
        }
        clause = strtok_r(NULL, ",", &saveptr);
    }
    umask(m);
    return 0;
}

int builtin_enable(zbash_state_t *state, int argc, char **argv) {
    bool disable = false;
    bool special_only = false;
    bool all = false;
    bool print = false;

    int idx = 1;
    while (idx < argc && argv[idx][0] == '-') {
        for (int k = 1; argv[idx][k]; k++) {
            if (argv[idx][k] == 'n') disable = true;
            else if (argv[idx][k] == 's') special_only = true;
            else if (argv[idx][k] == 'a') all = true;
            else if (argv[idx][k] == 'p') print = true;
        }
        idx++;
    }

    (void)all;
    (void)print;
    if (idx >= argc) {
        if (special_only) {
            static const char *const bash_special_builtins[] = {
                ".", ":", "break", "continue", "eval", "exec", "exit",
                "export", "readonly", "return", "set", "shift", "source",
                "times", "trap", "unset"
            };
            if (disable) {
                for (int i = 0; i < 16; i++) {
                    if (!zbash_is_builtin_enabled(state, bash_special_builtins[i])) {
                        printf("enable -n %s\n", bash_special_builtins[i]);
                    }
                }
            } else {
                for (int i = 0; i < 16; i++) {
                    if (zbash_is_builtin_enabled(state, bash_special_builtins[i])) {
                        printf("enable %s\n", bash_special_builtins[i]);
                    }
                }
            }
        }
        return 0;
    }

    for (int i = idx; i < argc; i++) {
        const char *name = argv[i];
        if (disable) {
            if (state->disabled_builtin_count < 32) {
                strncpy(state->disabled_builtins[state->disabled_builtin_count++], name, 31);
            }
        } else {
            for (int k = 0; k < state->disabled_builtin_count; k++) {
                if (strcmp(state->disabled_builtins[k], name) == 0) {
                    for (int j = k; j < state->disabled_builtin_count - 1; j++) {
                        strncpy(state->disabled_builtins[j], state->disabled_builtin_count > 0 ? state->disabled_builtins[j + 1] : "", 31);
                    }
                    state->disabled_builtin_count--;
                    break;
                }
            }
        }
    }
    return 0;
}

int builtin_times(void) {
    struct rusage self, ch;
    getrusage(RUSAGE_SELF, &self);
    getrusage(RUSAGE_CHILDREN, &ch);

    long s_u_sec = self.ru_utime.tv_sec / 60;
    double s_u_s = (self.ru_utime.tv_sec % 60) + self.ru_utime.tv_usec / 1000000.0;
    long s_s_sec = self.ru_stime.tv_sec / 60;
    double s_s_s = (self.ru_stime.tv_sec % 60) + self.ru_stime.tv_usec / 1000000.0;

    long c_u_sec = ch.ru_utime.tv_sec / 60;
    double c_u_s = (ch.ru_utime.tv_sec % 60) + ch.ru_utime.tv_usec / 1000000.0;
    long c_s_sec = ch.ru_stime.tv_sec / 60;
    double c_s_s = (ch.ru_stime.tv_sec % 60) + ch.ru_stime.tv_usec / 1000000.0;

    printf("%ldm%.3fs %ldm%.3fs\n", s_u_sec, s_u_s, s_s_sec, s_s_s);
    printf("%ldm%.3fs %ldm%.3fs\n", c_u_sec, c_u_s, c_s_sec, c_s_s);
    return 0;
}

typedef struct {
    char name[64];
    char path[256];
    int hits;
} hash_sort_item_t;

static int hash_cmp(const void *a, const void *b) {
    const hash_sort_item_t *ha = (const hash_sort_item_t *)a;
    const hash_sort_item_t *hb = (const hash_sort_item_t *)b;
    return strcmp(ha->name, hb->name);
}

int builtin_hash(zbash_state_t *state, int argc, char **argv) {
    if (!state) return 0;

    int idx = 1;
    bool reset = false;
    const char *add_path = NULL;

    while (idx < argc && argv[idx][0] == '-') {
        if (strcmp(argv[idx], "--") == 0) { idx++; break; }
        if (strcmp(argv[idx], "-r") == 0) {
            reset = true;
            idx++;
        } else if (strcmp(argv[idx], "-p") == 0) {
            if (idx + 1 < argc) {
                add_path = argv[idx + 1];
                idx += 2;
            } else {
                return 2;
            }
        } else {
            break;
        }
    }

    if (reset) {
        state->hash_count = 0;
        if (idx >= argc) return 0;
    }

    if (add_path) {
        if (idx >= argc) return 2;
        const char *name = argv[idx];
        int found = -1;
        for (int i = 0; i < state->hash_count; i++) {
            if (strcmp(state->hash_table[i].name, name) == 0) {
                found = i;
                break;
            }
        }
        if (found < 0) {
            if (state->hash_count < 128) {
                found = state->hash_count++;
            } else {
                found = 0;
            }
        }
        strncpy(state->hash_table[found].name, name, sizeof(state->hash_table[found].name) - 1);
        state->hash_table[found].name[sizeof(state->hash_table[found].name) - 1] = '\0';
        strncpy(state->hash_table[found].path, add_path, sizeof(state->hash_table[found].path) - 1);
        state->hash_table[found].path[sizeof(state->hash_table[found].path) - 1] = '\0';
        state->hash_table[found].hits = 0;
        return 0;
    }

    if (idx >= argc) {
        if (state->hash_count == 0) {
            printf("hash: hash table empty\n");
            return 0;
        }
        hash_sort_item_t items[128];
        for (int i = 0; i < state->hash_count; i++) {
            strncpy(items[i].name, state->hash_table[i].name, sizeof(items[i].name) - 1);
            items[i].name[sizeof(items[i].name) - 1] = '\0';
            strncpy(items[i].path, state->hash_table[i].path, sizeof(items[i].path) - 1);
            items[i].path[sizeof(items[i].path) - 1] = '\0';
            items[i].hits = state->hash_table[i].hits;
        }
        qsort(items, state->hash_count, sizeof(hash_sort_item_t), hash_cmp);
        printf("hits\tcommand\n");
        for (int i = 0; i < state->hash_count; i++) {
            printf("%4d\t%s\n", items[i].hits, items[i].path);
        }
        return 0;
    }

    /* Hash given command names from PATH */
    int rc = 0;
    for (int i = idx; i < argc; i++) {
        const char *name = argv[i];
        char *path_env = getenv("PATH");
        if (!path_env) path_env = "/usr/bin:/bin";
        char *path_copy = strdup(path_env);
        char *dir = strtok(path_copy, ":");
        bool found = false;
        while (dir) {
            char full[1024];
            snprintf(full, sizeof(full), "%s/%s", dir, name);
            if (zbash_check_exec(full)) {
                int pos = -1;
                for (int h = 0; h < state->hash_count; h++) {
                    if (strcmp(state->hash_table[h].name, name) == 0) {
                        pos = h; break;
                    }
                }
                if (pos < 0 && state->hash_count < 128) pos = state->hash_count++;
                if (pos >= 0) {
                    strncpy(state->hash_table[pos].name, name, sizeof(state->hash_table[pos].name) - 1);
                    state->hash_table[pos].name[sizeof(state->hash_table[pos].name) - 1] = '\0';
                    strncpy(state->hash_table[pos].path, full, sizeof(state->hash_table[pos].path) - 1);
                    state->hash_table[pos].path[sizeof(state->hash_table[pos].path) - 1] = '\0';
                    state->hash_table[pos].hits = 0;
                }
                found = true;
                break;
            }
            dir = strtok(NULL, ":");
        }
        free(path_copy);
        if (!found) {
            fprintf(stderr, "zbash: hash: %s: not found\n", name);
            rc = 1;
        }
    }
    return rc;
}

int builtin_exec(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state && state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state ? (state->lineno > 0 ? state->lineno : 1) : 1;

    if (state && state->restricted && argc > 1) {
        if (lineno > 0) {
            fprintf(stderr, "%s: line %d: exec: restricted\n", src, lineno);
        } else {
            fprintf(stderr, "%s: exec: restricted\n", src);
        }
        return 1;
    }

    const char *arg0_override = NULL;
    bool dash_arg0 = false;
    bool clean_env = false;

    int idx = 1;
    while (idx < argc && argv[idx][0] == '-') {
        if (strcmp(argv[idx], "--") == 0) { idx++; break; }
        if (strcmp(argv[idx], "-c") == 0) { clean_env = true; idx++; continue; }
        if (strcmp(argv[idx], "-l") == 0) { dash_arg0 = true; idx++; continue; }
        if (strcmp(argv[idx], "-a") == 0 && idx + 1 < argc) {
            arg0_override = argv[idx + 1];
            idx += 2;
            continue;
        }
        if (lineno > 0) {
            fprintf(stderr, "%s: line %d: exec: %s: invalid option\nexec: usage: exec [-cl] [-a name] file [redirection ...]\n", src, lineno, argv[idx]);
        } else {
            fprintf(stderr, "%s: exec: %s: invalid option\nexec: usage: exec [-cl] [-a name] file [redirection ...]\n", src, argv[idx]);
        }
        return 2;
    }

    if (idx >= argc) return 0;

    const char *prog = argv[idx];
    int new_argc = argc - idx;
    char **new_argv = malloc((new_argc + 1) * sizeof(char *));
    if (!new_argv) return 1;

    for (int i = 0; i < new_argc; i++) {
        new_argv[i] = argv[idx + i];
    }
    new_argv[new_argc] = NULL;

    char custom_arg0[256];
    if (arg0_override) {
        if (dash_arg0) {
            snprintf(custom_arg0, sizeof(custom_arg0), "-%s", arg0_override);
            new_argv[0] = custom_arg0;
        } else {
            new_argv[0] = (char *)arg0_override;
        }
    } else if (dash_arg0) {
        snprintf(custom_arg0, sizeof(custom_arg0), "-%s", argv[idx]);
        new_argv[0] = custom_arg0;
    }

    if (clean_env) {
        extern char **environ;
        environ = (char *[]){ NULL };
    }

    execvp(prog, new_argv);
    fprintf(stderr, "zbash: exec: %s: not found\n", prog);
    free(new_argv);
    exit(127);
}

#if defined(__aarch64__) || defined(__arm64__)
#define ZBASH_ARCH_STR "ARM64"
#elif defined(__x86_64__)
#define ZBASH_ARCH_STR "x86_64"
#elif defined(__EMSCRIPTEN__)
#define ZBASH_ARCH_STR "WASM"
#else
#define ZBASH_ARCH_STR "POSIX"
#endif

typedef struct {
    const char *name;
    const char *synopsis;
    const char *desc;
} zbash_builtin_doc_t;

static const zbash_builtin_doc_t builtin_docs[] = {
    { "cd", "cd [-L|-P] [dir]", "Change the current working directory to dir (default $HOME)." },
    { "pwd", "pwd [-L|-P]", "Print the absolute pathname of the current working directory." },
    { "echo", "echo [-neE] [arg ...]", "Write arguments to the standard output." },
    { "printf", "printf format [arguments ...]", "Format and print arguments according to format string." },
    { "pushd", "pushd [dir | +N | -N]", "Push a directory onto the directory stack or rotate the stack." },
    { "popd", "popd [+N | -N]", "Pop a directory from the directory stack." },
    { "dirs", "dirs [-clpv] [+N] [-N]", "Display the list of currently remembered directories in stack." },
    { "export", "export [-n] [name[=value] ...]", "Set export attribute for shell variables to environment." },
    { "unset", "unset [-f] [-v] [name ...]", "Unset values and attributes of variables and functions." },
    { "set", "set [--abefhkmnptuvxBCHP] [-o option] [arg ...]", "Set or unset shell options and positional parameters." },
    { "declare", "declare [-afFirtx] [-p] [name[=value] ...]", "Declare variables and give them attributes." },
    { "typeset", "typeset [-afFirtx] [-p] [name[=value] ...]", "Equivalent to declare." },
    { "readonly", "readonly [-af] [name[=value] ...]", "Mark variables or functions as unmodifiable." },
    { "local", "local [name[=value] ...]", "Create local variables scoped to the current function." },
    { "read", "read [-ers] [-u fd] [-t timeout] [name ...]", "Read a line from standard input and assign fields to variables." },
    { "type", "type [-afptP] name [name ...]", "Display information about command type (builtin, alias, function, file)." },
    { "test", "test [expr]", "Evaluate conditional expression (equivalent to [ expr ])." },
    { "[", "[ expr ]", "Evaluate conditional expression." },
    { "[[", "[[ expr ]]", "Evaluate conditional expression with extended operators." },
    { "let", "let arg [arg ...]", "Evaluate arithmetic expressions." },
    { "eval", "eval [arg ...]", "Construct command by concatenating arguments and execute it." },
    { "exec", "exec [-cl] [-a name] file [redirections]", "Replace shell process with the specified command." },
    { "exit", "exit [n]", "Exit the shell with a status of n (default status of last command)." },
    { "source", "source filename [arguments ...]", "Execute commands from filename in current shell environment." },
    { ".", ". filename [arguments ...]", "Execute commands from filename in current shell environment." },
    { "alias", "alias [-p] [name[=value] ...]", "Define or display aliases." },
    { "unalias", "unalias [-a] name [name ...]", "Remove aliases." },
    { "shopt", "shopt [-pqsu] [-o] [optname ...]", "Set and unset shell optional behaviors." },
    { "history", "history [-c] [-d offset] [n]", "Display or manipulate the command history list." },
    { "fc", "fc [-e ename] [-nlr] [first] [last]", "Fix command / re-execute or list commands from history." },
    { "jobs", "jobs [-lnprs] [jobspec ...]", "Display status of background and suspended jobs." },
    { "fg", "fg [jobspec]", "Place job in the foreground." },
    { "bg", "bg [jobspec ...]", "Place jobs in the background." },
    { "disown", "disown [-h] [-ar] [jobspec ...]", "Remove jobs from active job table." },
    { "wait", "wait [n ...]", "Wait for process or job completion and return its exit status." },
    { "kill", "kill [-s sigspec | -n signum | -sigspec] pid | jobspec ...", "Send a signal to processes or jobs." },
    { "trap", "trap [-lp] [arg signal_spec ...]", "Define shell handlers on receipt of signals or conditions." },
    { "break", "break [n]", "Exit from within a for, while, or until loop." },
    { "continue", "continue [n]", "Resume next iteration of an enclosing loop." },
    { "return", "return [n]", "Return from a shell function or sourced script." },
    { "shift", "shift [n]", "Shift positional parameters left by n." },
    { "getopts", "getopts optstring name [arg ...]", "Parse positional parameters as command options." },
    { "umask", "umask [-p] [-S] [mode]", "Display or set the file mode creation mask." },
    { "builtin", "builtin [shell-builtin [arg ...]]", "Execute shell builtin bypassing functions and aliases." },
    { "command", "command [-pVv] command [arg ...]", "Execute simple command bypassing functions." },
    { "enable", "enable [-pnds] [-a] [-f filename] [name ...]", "Enable and disable builtin shell commands." },
    { "hash", "hash [-lr] [-p pathname] [-dt] [name ...]", "Remember or display program locations." },
    { "times", "times", "Print accumulated user and system CPU times." },
    { "caller", "caller [expr]", "Return context of current subroutine call." },
    { "true", "true", "Return a successful (0) exit status." },
    { ":", ":", "Null command; return a successful (0) exit status." },
    { "false", "false", "Return an unsuccessful (1) exit status." },
    { NULL, NULL, NULL }
};

int builtin_help(int argc, char **argv) {
    if (argc > 1) {
        const char *query = argv[1];
        while (*query == '-') query++;
        bool found = false;
        for (int i = 0; builtin_docs[i].name; i++) {
            if (strcmp(builtin_docs[i].name, query) == 0) {
                printf("%s: %s\n", builtin_docs[i].name, builtin_docs[i].synopsis);
                printf("    %s\n", builtin_docs[i].desc);
                found = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "zbash: help: no help topics match `%s'. Try `help' to see the list of builtins.\n", argv[1]);
            return 1;
        }
        return 0;
    }

    /* Full builtins listing with native Zephyr identity */
    printf("╭────────────────────────────────────────────────────────────────────────────╮\n");
    printf("│  ZEPHYR zbash Shell Engine — V.0.0.1 (%s)                               │\n", ZBASH_ARCH_STR);
    printf("│  Bare-Metal Silicon Velocity · POSIX Standard · Native ISO C11             │\n");
    printf("│  © 2026 Wirot Chookeaw (chin6700x)                                         │\n");
    printf("╰────────────────────────────────────────────────────────────────────────────╯\n");
    printf("Type `help <command>' for detailed syntax and options.\n\n");
    printf("  📁 Navigation & Dirstack : cd, pwd, pushd, popd, dirs\n");
    printf("  🔢 Variables & Scope     : export, unset, set, declare, readonly, local, typeset\n");
    printf("  🔀 Control Flow & Logic  : if, for, while, until, case, break, continue, return\n");
    printf("  ⚙️ Execution & State     : eval, exec, source, ., builtin, command, alias, unalias, shopt\n");
    printf("  🧵 Jobs & Process Flow   : jobs, fg, bg, disown, wait, kill, trap\n");
    printf("  📝 I/O & Text Utilities  : echo, printf, read, test, [, [[, let, history, fc, caller\n\n");
    printf("POSIX-compliant shell engine with sub-millisecond cold start and NEON SIMD acceleration.\n");
    return 0;
}

static inline bool is_valid_ident_name(const char *name, size_t len) {
    if (len == 0) return false;
    if (!isalpha((unsigned char)name[0]) && name[0] != '_') return false;
    for (size_t i = 1; i < len; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') return false;
    }
    return true;
}

static int getopts_finish(zbash_state_t *state, const char *varname, const char *val, bool valid_ident) {
    if (!valid_ident) {
        const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] :
                          (state && state->argv && state->argv[0] ? state->argv[0] : "zbash");
        int lineno = state ? (state->stmt_start_lineno > 0 ? state->stmt_start_lineno : (state->lineno > 0 ? state->lineno : 1)) : 1;
        fprintf(stderr, "%s: line %d: getopts: `%s': not a valid identifier\n", src, lineno, varname);
        return 1;
    }
    zbash_assign_var(state, varname, val ? val : "", false);
    return 0;
}

int builtin_getopts(zbash_state_t *state, int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "getopts: usage: getopts optstring name [arg]\n");
        return 2;
    }
    const char *optstring = argv[1];
    const char *varname = argv[2];

    if (optstring[0] == '-' && strcmp(optstring, "--") != 0) {
        const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] :
                          (state && state->argv && state->argv[0] ? state->argv[0] : "zbash");
        int lineno = state ? (state->stmt_start_lineno > 0 ? state->stmt_start_lineno : (state->lineno > 0 ? state->lineno : 1)) : 1;
        fprintf(stderr, "%s: line %d: getopts: %s: invalid option\n", src, lineno, optstring);
        fprintf(stderr, "getopts: usage: getopts optstring name [arg]\n");
        return 2;
    }

    if (strcmp(optstring, "--") == 0) {
        if (argc < 4) {
            fprintf(stderr, "getopts: usage: getopts optstring name [arg]\n");
            return 2;
        }
        optstring = argv[2];
        varname = argv[3];
        argv += 2;
        argc -= 2;
    }

    bool valid_ident = is_valid_ident_name(varname, strlen(varname));

    const char *optind_str = getenv("OPTIND");
    int optind = (optind_str && *optind_str) ? atoi(optind_str) : 1;
    if (optind < 1) optind = 1;

    const char *opterr_str = getenv("OPTERR");
    int opterr = (opterr_str && *opterr_str) ? atoi(opterr_str) : 1;

    if (state->getopts_last_optind != optind) {
        state->getopts_subindex = 1;
        state->getopts_last_optind = optind;
    }
    if (state->getopts_subindex < 1) {
        state->getopts_subindex = 1;
    }

    int num_args = (argc > 3) ? (argc - 3) : (state->argc > 1 ? state->argc - 1 : 0);
    char **arg_array = (argc > 3) ? (argv + 3) : (state->argv + 1);

    if (optind > num_args) {
        if (valid_ident) zbash_assign_var(state, varname, "?", false);
        state->getopts_subindex = 1;
        state->getopts_last_optind = optind;
        return 1;
    }

    char *cur_arg = arg_array[optind - 1];
    if (!cur_arg || cur_arg[0] != '-' || cur_arg[1] == '\0' || strcmp(cur_arg, "--") == 0) {
        if (cur_arg && strcmp(cur_arg, "--") == 0) {
            optind++;
            char num_buf[16];
            snprintf(num_buf, sizeof(num_buf), "%d", optind);
            zbash_assign_var(state, "OPTIND", num_buf, false);
        }
        if (valid_ident) zbash_assign_var(state, varname, "?", false);
        state->getopts_subindex = 1;
        state->getopts_last_optind = optind;
        return 1;
    }

    int subindex = state->getopts_subindex;
    if (subindex < 1 || subindex >= (int)strlen(cur_arg)) {
        subindex = 1;
    }

    char opt_char = cur_arg[subindex];
    bool is_silent = (optstring[0] == ':');
    const char *spec_str = is_silent ? (optstring + 1) : optstring;
    const char *opt_spec = strchr(spec_str, opt_char);
    const char *progname = (state && state->argv && state->argv[0]) ? state->argv[0] : "zbash";

    if (!opt_spec || opt_char == ':') {
        char unk[2] = {opt_char, '\0'};
        if (is_silent) {
            zbash_assign_var(state, "OPTARG", unk, false);
        } else {
            unsetenv("OPTARG");
            if (opterr != 0) {
                fprintf(stderr, "%s: illegal option -- %c\n", progname, opt_char);
            }
        }
        if (cur_arg[subindex + 1] != '\0') {
            state->getopts_subindex = subindex + 1;
            state->getopts_last_optind = optind;
        } else {
            state->getopts_subindex = 1;
            optind++;
            state->getopts_last_optind = optind;
            char num_buf[16];
            snprintf(num_buf, sizeof(num_buf), "%d", optind);
            zbash_assign_var(state, "OPTIND", num_buf, false);
        }
        return getopts_finish(state, varname, "?", valid_ident);
    }

    if (opt_spec[1] != ':') {
        char val_str[2] = {opt_char, '\0'};
        unsetenv("OPTARG");

        if (cur_arg[subindex + 1] != '\0') {
            state->getopts_subindex = subindex + 1;
            state->getopts_last_optind = optind;
        } else {
            state->getopts_subindex = 1;
            optind++;
            state->getopts_last_optind = optind;
            char num_buf[16];
            snprintf(num_buf, sizeof(num_buf), "%d", optind);
            zbash_assign_var(state, "OPTIND", num_buf, false);
        }
        return getopts_finish(state, varname, val_str, valid_ident);
    }

    char val_str[2] = {opt_char, '\0'};

    if (cur_arg[subindex + 1] != '\0') {
        zbash_assign_var(state, "OPTARG", cur_arg + subindex + 1, false);
        state->getopts_subindex = 1;
        optind++;
        state->getopts_last_optind = optind;
        char num_buf[16];
        snprintf(num_buf, sizeof(num_buf), "%d", optind);
        zbash_assign_var(state, "OPTIND", num_buf, false);
        return getopts_finish(state, varname, val_str, valid_ident);
    } else if (optind < num_args) {
        zbash_assign_var(state, "OPTARG", arg_array[optind], false);
        state->getopts_subindex = 1;
        optind += 2;
        state->getopts_last_optind = optind;
        char num_buf[16];
        snprintf(num_buf, sizeof(num_buf), "%d", optind);
        zbash_assign_var(state, "OPTIND", num_buf, false);
        return getopts_finish(state, varname, val_str, valid_ident);
    } else {
        state->getopts_subindex = 1;
        optind++;
        state->getopts_last_optind = optind;
        char num_buf[16];
        snprintf(num_buf, sizeof(num_buf), "%d", optind);
        zbash_assign_var(state, "OPTIND", num_buf, false);

        if (is_silent) {
            char unk[2] = {opt_char, '\0'};
            zbash_assign_var(state, "OPTARG", unk, false);
            return getopts_finish(state, varname, ":", valid_ident);
        } else {
            unsetenv("OPTARG");
            if (opterr != 0) {
                fprintf(stderr, "%s: option requires an argument -- %c\n", progname, opt_char);
            }
            return getopts_finish(state, varname, "?", valid_ident);
        }
    }
}
