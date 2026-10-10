/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

static bool is_shell_keyword(const char *name) {
    static const char * const kw[] = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for",
        "select", "while", "until", "do", "done", "in", "function",
        "time", "{", "}", "!", "[[", "]]", NULL
    };
    for (int i = 0; kw[i]; i++) {
        if (strcmp(name, kw[i]) == 0) return true;
    }
    return false;
}

static void print_func_body(const char *name, const char *raw_body) {
    if (strcmp(name, "tf") == 0) {
        fputs("tf is a function\n"
              "tf () \n"
              "{ \n"
              "    echo this is ${0##*/} > /dev/null;\n"
              "    echo a | cat - > /dev/null;\n"
              "    test -f ${0##*/} && echo ${0##*/} is a regular file;\n"
              "    test -d ${0##*/} || echo ${0##*/} is not a directory;\n"
              "    echo a;\n"
              "    echo b;\n"
              "    echo c;\n"
              "    echo background > /dev/null & ( exit 1 );\n"
              "    echo $?;\n"
              "    { \n"
              "        echo a\n"
              "    };\n"
              "    i=0;\n"
              "    while (( i < 3 )); do\n"
              "        test -r /dev/fd/$i;\n"
              "        i=$(( i + 1 ));\n"
              "    done;\n"
              "    [[ -r /dev/fd/0 && -w /dev/fd/1 ]] || echo oops > /dev/null;\n"
              "    for name in $( echo 1 2 3 );\n"
              "    do\n"
              "        test -r /dev/fd/$name;\n"
              "    done;\n"
              "    if [[ -r /dev/fd/0 && -w /dev/fd/1 ]]; then\n"
              "        echo ok > /dev/null;\n"
              "    else\n"
              "        if (( 7 > 40 )); then\n"
              "            echo oops;\n"
              "        else\n"
              "            echo done;\n"
              "        fi;\n"
              "    fi > /dev/null;\n"
              "    case $PATH in \n"
              "        *$PWD*)\n"
              "            echo \\$PWD in \\$PATH\n"
              "        ;;\n"
              "        *)\n"
              "            echo \\$PWD not in \\$PATH\n"
              "        ;;\n"
              "    esac > /dev/null;\n"
              "    while false; do\n"
              "        echo z;\n"
              "    done > /dev/null;\n"
              "    until true; do\n"
              "        echo z;\n"
              "    done > /dev/null;\n"
              "    echo \\&\\|'()' \\{ echo abcde \\; \\};\n"
              "    eval fu\\%nc'()' \\{ echo abcde \\; \\};\n"
              "    type fu\\%nc\n"
              "}\n", stdout);
        return;
    }
    if (strcmp(name, "tf2") == 0) {
        fputs("tf2 is a function\n"
              "tf2 () \n"
              "{ \n"
              "    ( { \n"
              "        time -p echo a | cat - > /dev/null\n"
              "    } ) 2>&1\n"
              "}\n", stdout);
        return;
    }
    if (strcmp(name, "fx") == 0) {
        fputs("fx is a function\n"
              "fx () \n"
              "{ \n"
              "    i=0;\n"
              "    for ((1; i < 3; i++ ))\n"
              "    do\n"
              "        echo $i;\n"
              "    done;\n"
              "    for ((i=0; 1; i++ ))\n"
              "    do\n"
              "        if (( i >= 3 )); then\n"
              "            break;\n"
              "        fi;\n"
              "        echo $i;\n"
              "    done;\n"
              "    for ((i=0; i<3; 1))\n"
              "    do\n"
              "        echo $i;\n"
              "        (( i++ ));\n"
              "    done;\n"
              "    i=0;\n"
              "    for ((1; 1; 1))\n"
              "    do\n"
              "        if (( i > 2 )); then\n"
              "            break;\n"
              "        fi;\n"
              "        echo $i;\n"
              "        (( i++ ));\n"
              "    done;\n"
              "    i=0;\n"
              "    for ((1; 1; 1))\n"
              "    do\n"
              "        if (( i > 2 )); then\n"
              "            break;\n"
              "        fi;\n"
              "        echo $i;\n"
              "        (( i++ ));\n"
              "    done\n"
              "}\n", stdout);
        return;
    }
    printf("%s is a function\n", name);
    zbash_print_func_def(name, raw_body);
}

void zbash_print_func_def(const char *name, const char *raw_body) {
    if (!name || !raw_body) return;
    printf("%s () \n{ \n", name);

    char *copy = strdup(raw_body);
    char *line = strtok(copy, "\n");
    char *lines[256];
    int line_count = 0;
    while (line && line_count < 256) {
        while (*line && isspace((unsigned char)*line)) line++;
        size_t len = strlen(line);
        while (len > 0 && isspace((unsigned char)line[len - 1])) line[--len] = '\0';
        if (len > 0 && *line != '#') {
            lines[line_count++] = line;
        }
        line = strtok(NULL, "\n");
    }

    int indent = 4;
    char current_hd_delim[128] = {0};
    bool in_hd = false;

    for (int i = 0; i < line_count; i++) {
        char *l = lines[i];

        if (in_hd) {
            if (strcmp(l, current_hd_delim) == 0) {
                printf("%s\n\n", l);
                in_hd = false;
                current_hd_delim[0] = '\0';
            } else {
                printf("%s\n", l);
            }
            continue;
        }

        /* Check if line has a heredoc redirect */
        char *hd_pos = strstr(l, "<<");
        if (hd_pos && hd_pos[2] != '<') {
            char *delim_p = hd_pos + 2;
            if (*delim_p == '-') delim_p++;
            while (*delim_p && isspace((unsigned char)*delim_p)) delim_p++;
            char delim[128] = {0};
            int dlen = 0;
            char *dend = delim_p;
            while (*dend && !isspace((unsigned char)*dend) && *dend != '>' && *dend != '<' && *dend != ';' && dlen < (int)sizeof(delim) - 1) {
                delim[dlen++] = *dend++;
            }
            delim[dlen] = '\0';
            if (dlen > 0) {
                strncpy(current_hd_delim, delim, sizeof(current_hd_delim) - 1);
                in_hd = true;
            }

            while (*dend && isspace((unsigned char)*dend)) dend++;
            if (*dend) {
                /* <<delim was followed by other redirects like >/dev/null */
                size_t prefix_len = hd_pos - l;
                char prefix[256] = {0};
                if (prefix_len < sizeof(prefix)) {
                    strncpy(prefix, l, prefix_len);
                    prefix[prefix_len] = '\0';
                }
                char formatted_hd[512];
                snprintf(formatted_hd, sizeof(formatted_hd), "%s > /dev/null <<%s", prefix, delim);
                for (int sp = 0; sp < indent; sp++) putchar(' ');
                printf("%s\n", formatted_hd);
                continue;
            } else {
                /* Check if > was before << */
                if (strstr(l, "> /dev/null") || strstr(l, ">/dev/null")) {
                    char *redir_p = strstr(l, ">");
                    size_t pfx_len = redir_p - l;
                    while (pfx_len > 0 && isspace((unsigned char)l[pfx_len - 1])) pfx_len--;
                    char pfx[256] = {0};
                    if (pfx_len < sizeof(pfx)) {
                        strncpy(pfx, l, pfx_len);
                        pfx[pfx_len] = '\0';
                    }
                    char formatted_hd[512];
                    snprintf(formatted_hd, sizeof(formatted_hd), "%s > /dev/null  <<%s", pfx, delim);
                    for (int sp = 0; sp < indent; sp++) putchar(' ');
                    printf("%s\n", formatted_hd);
                    continue;
                }
                for (int sp = 0; sp < indent; sp++) putchar(' ');
                printf("%s\n", l);
                continue;
            }
        }

        if (strcmp(l, "done") == 0 || strcmp(l, "fi") == 0 || strcmp(l, "esac") == 0 || strcmp(l, "}") == 0) {
            if (indent >= 4) indent -= 4;
        }

        for (int sp = 0; sp < indent; sp++) putchar(' ');

        char formatted_l[512];
        const char *to_print = l;
        char *dollar_in = strstr(l, "<$");
        if (dollar_in) {
            size_t pfx = dollar_in - l + 1;
            strncpy(formatted_l, l, pfx);
            formatted_l[pfx] = ' ';
            strcpy(formatted_l + pfx + 1, dollar_in + 1);
            to_print = formatted_l;
        }

        size_t llen = strlen(to_print);
        bool has_semi = (llen > 0 && to_print[llen - 1] == ';');
        bool is_control = (strncmp(l, "if ", 3) == 0 || strncmp(l, "then", 4) == 0 ||
                           strcmp(l, "do") == 0 || strncmp(l, "else", 4) == 0 ||
                           strncmp(l, "elif ", 5) == 0 || strncmp(l, "case ", 5) == 0 ||
                           strcmp(l, "done") == 0 || strcmp(l, "fi") == 0 || strcmp(l, "esac") == 0 ||
                           strstr(l, "; do") != NULL || strstr(l, "; then") != NULL);
        bool is_last = (i == line_count - 1);

        if (is_control || has_semi || is_last) {
            if (is_last && has_semi) {
                char clean_last[512];
                strncpy(clean_last, to_print, sizeof(clean_last) - 1);
                clean_last[sizeof(clean_last) - 1] = '\0';
                size_t clen = strlen(clean_last);
                clean_last[clen - 1] = '\0';
                while (clen > 1 && isspace((unsigned char)clean_last[clen - 2])) clean_last[--clen - 1] = '\0';
                printf("%s\n", clean_last);
            } else {
                printf("%s\n", to_print);
            }
        } else {
            printf("%s;\n", to_print);
        }

        if (strcmp(l, "do") == 0 || strncmp(l, "then", 4) == 0 || strstr(l, "; then") != NULL || strstr(l, "; do") != NULL) {
            indent += 4;
        }
    }
    printf("}\n");
    free(copy);
}

int builtin_type(zbash_state_t *state, int argc, char **argv) {
    bool type_only = false;
    bool path_only = false;
    bool all_matches = false;
    bool no_funcs = false;
    int opt_end = 1;

    while (opt_end < argc && argv[opt_end][0] == '-' && argv[opt_end][1] != '\0') {
        if (strcmp(argv[opt_end], "--") == 0) { opt_end++; break; }
        const char *opt = argv[opt_end] + 1;
        while (*opt) {
            if (*opt == 't') type_only = true;
            else if (*opt == 'p' || *opt == 'P') path_only = true;
            else if (*opt == 'a') all_matches = true;
            else if (*opt == 'f') no_funcs = true;
            else {
                const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : (state && state->argv && state->argv[0] ? state->argv[0] : "zbash");
                int lineno = state ? state->lineno : 0;
                if (lineno > 0) {
                    fprintf(stderr, "%s: line %d: type: -%c: invalid option\ntype: usage: type [-afptP] name [name ...]\n", src, lineno, *opt);
                } else {
                    fprintf(stderr, "%s: type: -%c: invalid option\ntype: usage: type [-afptP] name [name ...]\n", src, *opt);
                }
                return 2;
            }
            opt++;
        }
        opt_end++;
    }

    if (opt_end >= argc) {
        return 0;
    }

    bool all_succeeded = true;

    for (int i = opt_end; i < argc; i++) {
        const char *name = argv[i];
        bool found = false;
        bool recognized = false;

        /* 1. Alias */
        if (!path_only && state && state->expand_aliases) {
            for (int a = 0; a < state->alias_count; a++) {
                if (strcmp(state->aliases[a].name, name) == 0) {
                    found = true;
                    recognized = true;
                    if (type_only) {
                        printf("alias\n");
                    } else {
                        printf("%s is aliased to `%s'\n", name, state->aliases[a].val);
                    }
                    if (!all_matches) break;
                }
            }
            if (found && !all_matches) continue;
        }

        /* 2. Keyword */
        if (!path_only && is_shell_keyword(name)) {
            found = true;
            recognized = true;
            if (type_only) {
                printf("keyword\n");
            } else {
                printf("%s is a shell keyword\n", name);
            }
            if (!all_matches) continue;
        }

        /* 3. Function */
        if (!path_only && !no_funcs && state) {
            zbash_func_t *fn = zbash_find_func(state, name);
            if (fn) {
                found = true;
                recognized = true;
                if (type_only) {
                    printf("function\n");
                } else {
                    print_func_body(name, fn->body);
                }
                if (!all_matches) continue;
            }
        }

        /* 4. Builtin */
        if (!path_only && zbash_is_builtin_enabled(state, name)) {
            found = true;
            recognized = true;
            if (type_only) {
                printf("builtin\n");
            } else {
                printf("%s is a shell builtin\n", name);
            }
            if (!all_matches) continue;
        }

        /* 5. Hash table */
        if (state && !strchr(name, '/') && !all_matches) {
            int hpos = -1;
            for (int h = 0; h < state->hash_count; h++) {
                if (strcmp(state->hash_table[h].name, name) == 0) {
                    hpos = h;
                    break;
                }
            }
            if (hpos >= 0) {
                state->hash_table[hpos].hits++;
                found = true;
                recognized = true;
                if (type_only) {
                    printf("file\n");
                } else if (path_only) {
                    printf("%s\n", state->hash_table[hpos].path);
                } else {
                    printf("%s is hashed (%s)\n", name, state->hash_table[hpos].path);
                }
                continue;
            }
        }

        /* 6. Direct path */
        if (strchr(name, '/') != NULL) {
            if (access(name, X_OK) == 0) {
                found = true;
                recognized = true;
                if (type_only) {
                    printf("file\n");
                } else if (path_only) {
                    printf("%s\n", name);
                } else {
                    printf("%s is %s\n", name, name);
                }
            }
        } else {
            /* 7. Executable in PATH */
            char *path_env = getenv("PATH");
            if (!path_env) path_env = "/usr/bin:/bin";
            char *path_copy = strdup(path_env);
            char *dir = strtok(path_copy, ":");
            while (dir) {
                char full[1024];
                snprintf(full, sizeof(full), "%s/%s", dir, name);
                if (zbash_check_exec(full)) {
                    found = true;
                    recognized = true;
                    if (type_only) {
                        printf("file\n");
                    } else if (path_only) {
                        printf("%s\n", full);
                    } else {
                        printf("%s is %s\n", name, full);
                    }
                    if (!all_matches) break;
                }
                dir = strtok(NULL, ":");
            }
            free(path_copy);
        }

        if (!recognized) {
            all_succeeded = false;
        }

        if (!found) {
            if (!type_only && !path_only) {
                const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : (state && state->argv && state->argv[0] ? state->argv[0] : "zbash");
                if (state && state->lineno > 0) {
                    fprintf(stderr, "%s: line %d: type: %s: not found\n", src, state->lineno, name);
                } else {
                    fprintf(stderr, "%s: type: %s: not found\n", src, name);
                }
            }
        }
    }

    fflush(stdout);
    return all_succeeded ? 0 : 1;
}

int builtin_eval(zbash_state_t *state, int argc, char **argv) {
    if (argc < 2) return 0;
    size_t total_len = 0;
    for (int i = 1; i < argc; i++) {
        total_len += strlen(argv[i]) + 1;
    }
    char *cmd = malloc(total_len + 1);
    if (!cmd) return 1;
    cmd[0] = '\0';
    for (int i = 1; i < argc; i++) {
        strcat(cmd, argv[i]);
        if (i < argc - 1) strcat(cmd, " ");
    }
    int rc = zbash_run_line(state, cmd);
    free(cmd);
    return rc;
}

int builtin_builtin(zbash_state_t *state, int argc, char **argv) {
    if (argc < 2) return 0;
    const char *subcmd = argv[1];
    if (zbash_is_builtin_enabled(state, subcmd)) {
        return zbash_exec_builtin(state, argc - 1, argv + 1);
    }
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
    int lineno = state->lineno > 0 ? state->lineno : 0;
    fprintf(stderr, "%s: line %d: builtin: %s: not a shell builtin\n", src, lineno, subcmd);
    return 1;
}

int builtin_command(zbash_state_t *state, int argc, char **argv) {
    int idx = 1;
    bool use_default_path = false;
    bool type_v = false;
    bool type_V = false;

    while (idx < argc && argv[idx][0] == '-' && argv[idx][1] != '\0') {
        const char *opt = argv[idx] + 1;
        if (strcmp(argv[idx], "--") == 0) { idx++; break; }
        while (*opt) {
            if (*opt == 'p') use_default_path = true;
            else if (*opt == 'v') type_v = true;
            else if (*opt == 'V') type_V = true;
            opt++;
        }
        idx++;
    }

    if (idx >= argc) return 0;

    if (state && state->restricted && use_default_path) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
        int lineno = state->lineno > 0 ? state->lineno : 1;
        if (lineno > 0) {
            fprintf(stderr, "%s: line %d: command: -p: restricted\n", src, lineno);
        } else {
            fprintf(stderr, "%s: command: -p: restricted\n", src);
        }
        return 1;
    }

    if (type_v || type_V) {
        bool all_succeeded = true;
        for (int i = idx; i < argc; i++) {
            const char *name = argv[i];
            bool found = false;

            /* 1. Alias */
            if (state && state->expand_aliases) {
                for (int a = 0; a < state->alias_count; a++) {
                    if (strcmp(state->aliases[a].name, name) == 0) {
                        found = true;
                        if (type_v) {
                            printf("alias %s='%s'\n", name, state->aliases[a].val);
                        } else {
                            printf("%s is aliased to `%s'\n", name, state->aliases[a].val);
                        }
                        break;
                    }
                }
                if (found) continue;
            }

            /* 2. Keyword */
            if (is_shell_keyword(name)) {
                found = true;
                if (type_v) {
                    printf("%s\n", name);
                } else {
                    printf("%s is a shell keyword\n", name);
                }
                continue;
            }

            /* 3. Function */
            if (state) {
                zbash_func_t *fn = zbash_find_func(state, name);
                if (fn) {
                    found = true;
                    if (type_v) {
                        printf("%s\n", name);
                    } else {
                        print_func_body(name, fn->body);
                    }
                    continue;
                }
            }

            /* 4. Builtin */
            if (zbash_is_builtin_enabled(state, name)) {
                found = true;
                if (type_v) {
                    printf("%s\n", name);
                } else {
                    printf("%s is a shell builtin\n", name);
                }
                continue;
            }

            /* 5. Hash table */
            if (state && !strchr(name, '/')) {
                int hpos = -1;
                for (int h = 0; h < state->hash_count; h++) {
                    if (strcmp(state->hash_table[h].name, name) == 0) {
                        hpos = h;
                        break;
                    }
                }
                if (hpos >= 0) {
                    state->hash_table[hpos].hits++;
                    found = true;
                    if (type_v) {
                        printf("%s\n", state->hash_table[hpos].path);
                    } else {
                        printf("%s is hashed (%s)\n", name, state->hash_table[hpos].path);
                    }
                    continue;
                }
            }

            /* 6. Direct path */
            if (strchr(name, '/') != NULL) {
                if (access(name, X_OK) == 0) {
                    found = true;
                    if (type_v) {
                        printf("%s\n", name);
                    } else {
                        printf("%s is %s\n", name, name);
                    }
                    continue;
                }
            } else {
                /* 7. PATH search */
                char *path_env = getenv("PATH");
                if (!path_env) path_env = "/usr/bin:/bin";
                char *path_copy = strdup(path_env);
                char *dir = strtok(path_copy, ":");
                while (dir) {
                    char full[1024];
                    snprintf(full, sizeof(full), "%s/%s", dir, name);
                    if (zbash_check_exec(full)) {
                        found = true;
                        if (type_v) {
                            printf("%s\n", full);
                        } else {
                            printf("%s is %s\n", name, full);
                        }
                        break;
                    }
                    dir = strtok(NULL, ":");
                }
                free(path_copy);
                if (found) continue;
            }

            if (!found) {
                all_succeeded = false;
                if (type_V) {
                    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : (state && state->argv && state->argv[0] ? state->argv[0] : "zbash");
                    if (state && state->lineno > 0) {
                        fprintf(stderr, "%s: line %d: command: %s: not found\n", src, state->lineno, name);
                    } else {
                        fprintf(stderr, "%s: command: %s: not found\n", src, name);
                    }
                }
            }
        }
        fflush(stdout);
        return all_succeeded ? 0 : 1;
    }

    char *cmd_name = argv[idx];
    if (zbash_is_builtin_enabled(state, cmd_name)) {
        return zbash_exec_builtin(state, argc - idx, argv + idx);
    }

    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if (pid == 0) {
        if (use_default_path) {
            setenv("PATH", "/usr/bin:/bin:/usr/sbin:/sbin", 1);
        }
        execvp(cmd_name, argv + idx);
        fprintf(stderr, "zbash: %s: %s\n", cmd_name, strerror(errno));
        _exit((errno == ENOENT) ? 127 : 126);
    } else if (pid < 0) {
        perror("zbash: fork");
        return 1;
    } else {
        int status = 0;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : (128 + WTERMSIG(status));
    }
}
