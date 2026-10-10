/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"
#include <ctype.h>
#include <limits.h>

static void print_error_prefix(zbash_state_t *state, const char *cmd) {
    if (state && state->source_depth > 0) {
        fprintf(stderr, "%s: line %d: %s: ", state->source_stack[state->source_depth - 1], state->lineno, cmd);
    } else {
        fprintf(stderr, "zbash: %s: ", cmd);
    }
}

static zbash_array_t *ensure_dstack(zbash_state_t *state) {
    if (!state) return NULL;
    zbash_array_t *arr = zbash_find_array(state, "DIRSTACK");
    if (!arr || arr->count == 0) {
        const char *pwd = getenv("PWD");
        char cwd[PATH_MAX];
        if (!pwd || pwd[0] != '/') {
            if (getcwd(cwd, sizeof(cwd))) pwd = cwd;
            else pwd = "/";
        }
        char *init_elem = strdup(pwd);
        zbash_set_array(state, "DIRSTACK", 1, &init_elem);
        free(init_elem);
        arr = zbash_find_array(state, "DIRSTACK");
    } else {
        /* Keep DIRSTACK[0] in sync with current PWD */
        const char *pwd = getenv("PWD");
        if (pwd && pwd[0] == '/' && (!arr->elements[0] || strcmp(arr->elements[0], pwd) != 0)) {
            if (arr->elements[0]) free(arr->elements[0]);
            arr->elements[0] = strdup(pwd);
        }
    }
    return arr;
}

static void format_entry(const char *dir, bool long_flag, char *out, size_t out_sz) {
    if (!dir) {
        if (out_sz > 0) out[0] = '\0';
        return;
    }
    if (!long_flag) {
        const char *home = getenv("HOME");
        if (home && home[0] != '\0') {
            size_t hlen = strlen(home);
            if (strcmp(dir, home) == 0) {
                snprintf(out, out_sz, "~");
                return;
            } else if (strncmp(dir, home, hlen) == 0 && dir[hlen] == '/') {
                snprintf(out, out_sz, "~%s", dir + hlen);
                return;
            }
        }
    }
    snprintf(out, out_sz, "%s", dir);
}

static void print_stack(zbash_state_t *state, bool long_flag, bool per_line, bool verbose, int single_idx) {
    zbash_array_t *arr = ensure_dstack(state);
    if (!arr || arr->count == 0) return;

    char fbuf[PATH_MAX];
    if (single_idx >= 0 && single_idx < arr->count) {
        format_entry(arr->elements[single_idx], long_flag, fbuf, sizeof(fbuf));
        if (verbose) {
            printf("%2d  %s\n", single_idx, fbuf);
        } else {
            printf("%s\n", fbuf);
        }
        fflush(stdout);
        return;
    }

    if (verbose) {
        for (int i = 0; i < arr->count; i++) {
            format_entry(arr->elements[i], long_flag, fbuf, sizeof(fbuf));
            printf("%2d  %s\n", i, fbuf);
        }
    } else if (per_line) {
        for (int i = 0; i < arr->count; i++) {
            format_entry(arr->elements[i], long_flag, fbuf, sizeof(fbuf));
            printf("%s\n", fbuf);
        }
    } else {
        for (int i = 0; i < arr->count; i++) {
            format_entry(arr->elements[i], long_flag, fbuf, sizeof(fbuf));
            printf("%s%s", (i > 0 ? " " : ""), fbuf);
        }
        printf("\n");
    }
    fflush(stdout);
}

int builtin_dirs(zbash_state_t *state, int argc, char **argv) {
    zbash_array_t *arr = ensure_dstack(state);
    bool clear_stack = false;
    bool long_flag = false;
    bool per_line = false;
    bool verbose = false;
    int target_idx = -1;
    bool has_target = false;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (arg[0] == '+') {
            /* +N */
            char *endp = NULL;
            long val = strtol(arg + 1, &endp, 10);
            if (!endp || *endp != '\0') {
                print_error_prefix(state, "dirs");
                fprintf(stderr, "%s: invalid number\n", arg);
                fprintf(stderr, "dirs: usage: dirs [-clpv] [+N] [-N]\n");
                return 1;
            }
            if (val < 0 || val >= arr->count) {
                print_error_prefix(state, "dirs");
                fprintf(stderr, "%s: directory stack index out of range\n", arg + 1);
                return 1;
            }
            target_idx = (int)val;
            has_target = true;
        } else if (arg[0] == '-' && isdigit((unsigned char)arg[1])) {
            /* -N */
            char *endp = NULL;
            long val = strtol(arg + 1, &endp, 10);
            if (!endp || *endp != '\0') {
                print_error_prefix(state, "dirs");
                fprintf(stderr, "%s: invalid number\n", arg);
                fprintf(stderr, "dirs: usage: dirs [-clpv] [+N] [-N]\n");
                return 1;
            }
            if (val < 0 || val >= arr->count) {
                print_error_prefix(state, "dirs");
                fprintf(stderr, "%s: directory stack index out of range\n", arg + 1);
                return 1;
            }
            target_idx = (arr->count - 1) - (int)val;
            has_target = true;
        } else if (arg[0] == '-') {
            /* options -c, -l, -p, -v */
            for (const char *p = arg + 1; *p; p++) {
                if (*p == 'c') clear_stack = true;
                else if (*p == 'l') long_flag = true;
                else if (*p == 'p') per_line = true;
                else if (*p == 'v') verbose = true;
                else {
                    print_error_prefix(state, "dirs");
                    fprintf(stderr, "%s: invalid number\n", arg);
                    fprintf(stderr, "dirs: usage: dirs [-clpv] [+N] [-N]\n");
                    return 1;
                }
            }
        } else {
            print_error_prefix(state, "dirs");
            fprintf(stderr, "%s: invalid option\n", arg);
            fprintf(stderr, "dirs: usage: dirs [-clpv] [+N] [-N]\n");
            return 1;
        }
    }

    if (clear_stack) {
        /* Clear all elements except element 0 */
        if (arr && arr->count > 1) {
            for (int i = 1; i < arr->count; i++) {
                free(arr->elements[i]);
                arr->elements[i] = NULL;
            }
            arr->count = 1;
        }
        return 0;
    }

    print_stack(state, long_flag, per_line, verbose, has_target ? target_idx : -1);
    return 0;
}

static int do_change_dir(const char *target) {
    char cur_pwd[PATH_MAX];
    const char *epwd = getenv("PWD");
    if (epwd && epwd[0] == '/') {
        strncpy(cur_pwd, epwd, sizeof(cur_pwd) - 1);
        cur_pwd[sizeof(cur_pwd) - 1] = '\0';
    } else {
        if (!getcwd(cur_pwd, sizeof(cur_pwd))) cur_pwd[0] = '\0';
    }

    if (chdir(target) != 0) {
        return -1;
    }

    if (cur_pwd[0] != '\0') {
        setenv("OLDPWD", cur_pwd, 1);
    }

    char new_logical[PATH_MAX];
    if (target[0] == '/') {
        strncpy(new_logical, target, sizeof(new_logical) - 1);
        new_logical[sizeof(new_logical) - 1] = '\0';
    } else {
        if (!getcwd(new_logical, sizeof(new_logical))) {
            strncpy(new_logical, target, sizeof(new_logical) - 1);
            new_logical[sizeof(new_logical) - 1] = '\0';
        }
    }
    setenv("PWD", new_logical, 1);
    return 0;
}

int builtin_pushd(zbash_state_t *state, int argc, char **argv) {
    zbash_array_t *arr = ensure_dstack(state);
    bool no_cd = false;
    const char *dir_arg = NULL;
    int rotate_idx = -1;
    bool has_rotate = false;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "-n") == 0) {
            no_cd = true;
        } else if (arg[0] == '+') {
            char *endp = NULL;
            long val = strtol(arg + 1, &endp, 10);
            if (!endp || *endp != '\0') {
                print_error_prefix(state, "pushd");
                fprintf(stderr, "%s: invalid number\n", arg);
                fprintf(stderr, "pushd: usage: pushd [dir | +N | -N] [-n]\n");
                return 1;
            }
            if (val < 0 || val >= arr->count) {
                print_error_prefix(state, "pushd");
                fprintf(stderr, "%s: directory stack index out of range\n", arg);
                return 1;
            }
            rotate_idx = (int)val;
            has_rotate = true;
        } else if (arg[0] == '-' && isdigit((unsigned char)arg[1])) {
            char *endp = NULL;
            long val = strtol(arg + 1, &endp, 10);
            if (!endp || *endp != '\0') {
                print_error_prefix(state, "pushd");
                fprintf(stderr, "%s: invalid number\n", arg);
                fprintf(stderr, "pushd: usage: pushd [dir | +N | -N] [-n]\n");
                return 1;
            }
            if (val < 0 || val >= arr->count) {
                print_error_prefix(state, "pushd");
                fprintf(stderr, "%s: directory stack index out of range\n", arg);
                return 1;
            }
            rotate_idx = (arr->count - 1) - (int)val;
            has_rotate = true;
        } else if (arg[0] == '-') {
            print_error_prefix(state, "pushd");
            fprintf(stderr, "%s: invalid number\n", arg);
            fprintf(stderr, "pushd: usage: pushd [dir | +N | -N] [-n]\n");
            return 1;
        } else {
            dir_arg = arg;
        }
    }

    if (!dir_arg && !has_rotate) {
        /* No arguments: swap top two elements */
        if (arr->count < 2) {
            print_error_prefix(state, "pushd");
            fprintf(stderr, "no other directory\n");
            return 1;
        }
        if (!no_cd) {
            if (do_change_dir(arr->elements[1]) != 0) {
                print_error_prefix(state, "pushd");
                fprintf(stderr, "%s: %s\n", arr->elements[1], strerror(errno));
                return 1;
            }
        }
        char *tmp = arr->elements[0];
        arr->elements[0] = arr->elements[1];
        arr->elements[1] = tmp;
        print_stack(state, false, false, false, -1);
        return 0;
    }

    if (has_rotate) {
        /* Rotate stack so that rotate_idx is at index 0 */
        if (!no_cd) {
            if (do_change_dir(arr->elements[rotate_idx]) != 0) {
                print_error_prefix(state, "pushd");
                fprintf(stderr, "%s: %s\n", arr->elements[rotate_idx], strerror(errno));
                return 1;
            }
        }
        if (rotate_idx > 0) {
            char **new_elems = malloc(sizeof(char *) * arr->count);
            int pos = 0;
            for (int i = rotate_idx; i < arr->count; i++) {
                new_elems[pos++] = arr->elements[i];
            }
            for (int i = 0; i < rotate_idx; i++) {
                new_elems[pos++] = arr->elements[i];
            }
            free(arr->elements);
            arr->elements = new_elems;
        }
        if (!no_cd) {
            print_stack(state, false, false, false, -1);
        }
        return 0;
    }

    /* Push new directory */
    if (!no_cd) {
        if (do_change_dir(dir_arg) != 0) {
            print_error_prefix(state, "pushd");
            fprintf(stderr, "%s: %s\n", dir_arg, strerror(errno));
            return 1;
        }
        const char *new_pwd = getenv("PWD");
        char **new_elems = malloc(sizeof(char *) * (arr->count + 1));
        new_elems[0] = strdup(new_pwd ? new_pwd : dir_arg);
        for (int i = 0; i < arr->count; i++) {
            new_elems[i + 1] = arr->elements[i];
        }
        free(arr->elements);
        arr->elements = new_elems;
        arr->count++;
    } else {
        /* -n: insert at index 1 without chdir */
        char **new_elems = malloc(sizeof(char *) * (arr->count + 1));
        new_elems[0] = arr->elements[0];
        new_elems[1] = strdup(dir_arg);
        for (int i = 1; i < arr->count; i++) {
            new_elems[i + 1] = arr->elements[i];
        }
        free(arr->elements);
        arr->elements = new_elems;
        arr->count++;
    }

    print_stack(state, false, false, false, -1);
    return 0;
}

int builtin_popd(zbash_state_t *state, int argc, char **argv) {
    zbash_array_t *arr = ensure_dstack(state);
    bool no_cd = false;
    int remove_idx = 0;
    bool has_remove_idx = false;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "-n") == 0) {
            no_cd = true;
        } else if (arg[0] == '+') {
            char *endp = NULL;
            long val = strtol(arg + 1, &endp, 10);
            if (!endp || *endp != '\0') {
                print_error_prefix(state, "popd");
                fprintf(stderr, "%s: invalid number\n", arg);
                fprintf(stderr, "popd: usage: popd [+N | -N] [-n]\n");
                return 1;
            }
            if (val < 0 || val >= arr->count) {
                print_error_prefix(state, "popd");
                fprintf(stderr, "%s: directory stack index out of range\n", arg);
                return 1;
            }
            remove_idx = (int)val;
            has_remove_idx = true;
        } else if (arg[0] == '-' && isdigit((unsigned char)arg[1])) {
            char *endp = NULL;
            long val = strtol(arg + 1, &endp, 10);
            if (!endp || *endp != '\0') {
                print_error_prefix(state, "popd");
                fprintf(stderr, "%s: invalid number\n", arg);
                fprintf(stderr, "popd: usage: popd [+N | -N] [-n]\n");
                return 1;
            }
            if (val < 0 || val >= arr->count) {
                print_error_prefix(state, "popd");
                fprintf(stderr, "%s: directory stack index out of range\n", arg);
                return 1;
            }
            remove_idx = (arr->count - 1) - (int)val;
            has_remove_idx = true;
        } else {
            print_error_prefix(state, "popd");
            fprintf(stderr, "%s: invalid number\n", arg);
            fprintf(stderr, "popd: usage: popd [+N | -N] [-n]\n");
            return 1;
        }
    }

    if (arr->count <= 1) {
        print_error_prefix(state, "popd");
        fprintf(stderr, "directory stack empty\n");
        return 1;
    }

    if (!has_remove_idx) {
        remove_idx = 0;
    }

    if (remove_idx == 0 && !no_cd) {
        if (do_change_dir(arr->elements[1]) != 0) {
            print_error_prefix(state, "popd");
            fprintf(stderr, "%s: %s\n", arr->elements[1], strerror(errno));
            return 1;
        }
    }

    free(arr->elements[remove_idx]);
    for (int i = remove_idx; i < arr->count - 1; i++) {
        arr->elements[i] = arr->elements[i + 1];
    }
    arr->count--;

    print_stack(state, false, false, false, -1);
    return 0;
}
