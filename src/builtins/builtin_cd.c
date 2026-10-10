/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

static void normalize_logical_path(const char *base, const char *target, char *out, size_t out_sz) {
    char combined[2048];
    if (target[0] == '/') {
        strncpy(combined, target, sizeof(combined) - 1);
        combined[sizeof(combined) - 1] = '\0';
    } else {
        snprintf(combined, sizeof(combined), "%s/%s", (base && *base) ? base : "/", target);
    }

    char *parts[128];
    int nparts = 0;
    char *saveptr = NULL;
    char *tok = strtok_r(combined, "/", &saveptr);
    while (tok) {
        if (strcmp(tok, ".") == 0) {
            /* ignore current dir */
        } else if (strcmp(tok, "..") == 0) {
            if (nparts > 0) nparts--;
        } else {
            if (nparts < 128) parts[nparts++] = tok;
        }
        tok = strtok_r(NULL, "/", &saveptr);
    }

    if (nparts == 0) {
        snprintf(out, out_sz, "/");
    } else {
        out[0] = '\0';
        for (int i = 0; i < nparts; i++) {
            size_t cur = strlen(out);
            snprintf(out + cur, out_sz - cur, "/%s", parts[i]);
        }
    }
}

int builtin_cd(zbash_state_t *state, int argc, char **argv) {
    if (state && state->restricted) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
        int lineno = state->lineno > 0 ? state->lineno : 1;
        if (lineno > 0) {
            fprintf(stderr, "%s: line %d: cd: restricted\n", src, lineno);
        } else {
            fprintf(stderr, "%s: cd: restricted\n", src);
        }
        return 1;
    }

    const char *target = NULL;
    bool print_target = false;

    const char *env_pwd = getenv("PWD");
    char cur_pwd[1024];
    if (env_pwd && env_pwd[0] == '/') {
        strncpy(cur_pwd, env_pwd, sizeof(cur_pwd) - 1);
        cur_pwd[sizeof(cur_pwd) - 1] = '\0';
    } else {
        if (getcwd(cur_pwd, sizeof(cur_pwd)) == NULL) cur_pwd[0] = '\0';
    }

    bool physical = false;
    int arg_idx = 1;
    while (arg_idx < argc && (strcmp(argv[arg_idx], "-P") == 0 || strcmp(argv[arg_idx], "-L") == 0)) {
        if (strcmp(argv[arg_idx], "-P") == 0) physical = true;
        else if (strcmp(argv[arg_idx], "-L") == 0) physical = false;
        arg_idx++;
    }

    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state && state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = (state && state->lineno > 0) ? state->lineno : 0;

    if (arg_idx >= argc || strcmp(argv[arg_idx], "~") == 0) {
        target = getenv("HOME");
        if (!target) {
            if (lineno > 0) {
                fprintf(stderr, "%s: line %d: cd: HOME not set\n", src, lineno);
            } else {
                fprintf(stderr, "%s: cd: HOME not set\n", src);
            }
            return 1;
        }
    } else if (strcmp(argv[arg_idx], "-") == 0) {
        target = getenv("OLDPWD");
        if (!target) {
            if (lineno > 0) {
                fprintf(stderr, "%s: line %d: cd: OLDPWD not set\n", src, lineno);
            } else {
                fprintf(stderr, "%s: cd: OLDPWD not set\n", src);
            }
            return 1;
        }
        print_target = true;
    } else if (argv[arg_idx][0] == '-' && isdigit((unsigned char)argv[arg_idx][1])) {
        int idx = atoi(argv[arg_idx] + 1);
        zbash_array_t *arr = zbash_find_array(state, "DIRSTACK");
        if (arr && idx >= 0 && idx < arr->count && arr->elements[idx]) {
            target = arr->elements[idx];
            print_target = true;
        } else {
            if (lineno > 0) {
                fprintf(stderr, "%s: line %d: cd: bad directory stack index: %s\n", src, lineno, argv[arg_idx]);
            } else {
                fprintf(stderr, "%s: cd: bad directory stack index: %s\n", src, argv[arg_idx]);
            }
            return 1;
        }
    } else {
        target = argv[arg_idx];
    }

    char cdpath_target[2048];
    if (target[0] != '/' && target[0] != '.' && getenv("CDPATH")) {
        const char *cdpath = getenv("CDPATH");
        char cp_buf[2048];
        strncpy(cp_buf, cdpath, sizeof(cp_buf) - 1);
        cp_buf[sizeof(cp_buf) - 1] = '\0';

        char *saveptr = NULL;
        char *dir = strtok_r(cp_buf, ":", &saveptr);
        while (dir) {
            char cand[2048];
            if (*dir == '\0' || strcmp(dir, ".") == 0) {
                snprintf(cand, sizeof(cand), "%s", target);
            } else {
                snprintf(cand, sizeof(cand), "%s/%s", dir, target);
            }
            struct stat st;
            if (stat(cand, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (*dir != '\0' && strcmp(dir, ".") != 0) {
                    print_target = true;
                }
                strncpy(cdpath_target, cand, sizeof(cdpath_target) - 1);
                cdpath_target[sizeof(cdpath_target) - 1] = '\0';
                target = cdpath_target;
                break;
            }
            dir = strtok_r(NULL, ":", &saveptr);
        }
    }

    char new_logical[1024];
    normalize_logical_path(cur_pwd, target, new_logical, sizeof(new_logical));

    if (chdir(target) != 0) {
        const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
        int lineno = (state && state->lineno > 0) ? state->lineno : 0;
        fprintf(stderr, "%s: line %d: cd: %s: %s\n", src, lineno, target, strerror(errno));
        return 1;
    }

    if (print_target) {
        printf("%s\n", target[0] == '/' ? target : new_logical);
    }

    if (cur_pwd[0] != '\0') {
        setenv("OLDPWD", cur_pwd, 1);
    }

    if (physical) {
        char real_cwd[1024];
        if (getcwd(real_cwd, sizeof(real_cwd))) {
            setenv("PWD", real_cwd, 1);
            if (state) zbash_set_array_elem(state, "DIRSTACK", 0, real_cwd);
        }
    } else {
        setenv("PWD", new_logical, 1);
        if (state) zbash_set_array_elem(state, "DIRSTACK", 0, new_logical);
    }

    if (state) {
        zbash_trigger_hook(state, "chpwd");
    }

    return 0;
}

int builtin_pwd(int argc, char **argv) {
    bool physical = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0) physical = true;
        else if (strcmp(argv[i], "-L") == 0) physical = false;
    }

    if (!physical) {
        const char *p = getenv("PWD");
        if (p && p[0] == '/') {
            printf("%s\n", p);
            fflush(stdout);
            return 0;
        }
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        fflush(stdout);
        return 0;
    }
    perror("zbash: pwd");
    return 1;
}
