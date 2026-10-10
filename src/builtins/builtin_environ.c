/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

static bool is_valid_ident_name(const char *name, size_t len) {
    if (len == 0) return false;
    if (!isalpha((unsigned char)name[0]) && name[0] != '_') return false;
    for (size_t i = 1; i < len; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') return false;
    }
    return true;
}

static char *format_func_for_export(const zbash_func_t *fn) {
    if (!fn || !fn->body) return strdup("() {  :\n}");
    char *copy = strdup(fn->body);
    char *lines[256];
    int line_count = 0;
    char *l = strtok(copy, "\n");
    while (l && line_count < 256) {
        while (*l && isspace((unsigned char)*l)) l++;
        size_t len = strlen(l);
        while (len > 0 && isspace((unsigned char)l[len - 1])) l[--len] = '\0';
        if (len > 0 && *l != '#') {
            lines[line_count++] = l;
        }
        l = strtok(NULL, "\n");
    }

    size_t cap = strlen(fn->body) * 2 + 256;
    char *out = malloc(cap);
    strcpy(out, "() {  ");

    char cur_delim[128] = {0};
    bool in_hd = false;

    for (int i = 0; i < line_count; i++) {
        char *line_s = lines[i];
        if (in_hd) {
            strcat(out, line_s);
            if (strcmp(line_s, cur_delim) == 0) {
                strcat(out, "\n\n");
                in_hd = false;
                cur_delim[0] = '\0';
            } else {
                strcat(out, "\n");
            }
            continue;
        }

        char *hd_pos = strstr(line_s, "<<");
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
                strncpy(cur_delim, delim, sizeof(cur_delim) - 1);
                in_hd = true;
            }
            while (*dend && isspace((unsigned char)*dend)) dend++;
            if (*dend) {
                size_t prefix_len = hd_pos - line_s;
                char prefix[256] = {0};
                if (prefix_len < sizeof(prefix)) {
                    strncpy(prefix, line_s, prefix_len);
                    prefix[prefix_len] = '\0';
                }
                char formatted[512];
                snprintf(formatted, sizeof(formatted), "%s > /dev/null <<%s", prefix, delim);
                if (i > 0) strcat(out, " ");
                strcat(out, formatted);
                strcat(out, "\n");
                continue;
            }
        }

        if (i > 0) strcat(out, " ");
        strcat(out, line_s);
        size_t llen = strlen(line_s);
        bool has_semi = (llen > 0 && line_s[llen - 1] == ';');
        bool is_control = (strncmp(line_s, "if ", 3) == 0 || strncmp(line_s, "then", 4) == 0 ||
                           strcmp(line_s, "do") == 0 || strncmp(line_s, "else", 4) == 0 ||
                           strncmp(line_s, "elif ", 5) == 0 || strncmp(line_s, "case ", 5) == 0);
        bool is_last = (i == line_count - 1);
        if (!is_control && !has_semi && !is_last) {
            strcat(out, ";");
        }
        strcat(out, "\n");
    }
    strcat(out, "}");
    free(copy);
    return out;
}

int builtin_export(zbash_state_t *state, int argc, char **argv) {
    if (argc == 1) {
        extern char **environ;
        for (char **env = environ; *env != NULL; env++) {
            printf("export %s\n", *env);
        }
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "-f") == 0) {
        int ret = 0;
        for (int i = 2; i < argc; i++) {
            const char *fn_name = argv[i];
            zbash_func_t *fn = zbash_find_func(state, fn_name);
            if (!fn) {
                const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
                int lineno = state->lineno > 0 ? state->lineno : 1;
                fprintf(stderr, "%s: line %d: export: %s: not a function\n", src, lineno, fn_name);
                ret = 1;
                continue;
            }
            char env_var[128];
            snprintf(env_var, sizeof(env_var), "BASH_FUNC_%s%%%%", fn->name);
            char *formatted = format_func_for_export(fn);
            if (formatted) {
                setenv(env_var, formatted, 1);
                free(formatted);
            }
        }
        return ret;
    }

    int ret = 0;
    for (int i = 1; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        bool is_append = (eq > argv[i] && *(eq - 1) == '+');
        size_t name_len = is_append ? (size_t)(eq - 1 - argv[i]) : (eq ? (size_t)(eq - argv[i]) : strlen(argv[i]));
        if (!is_valid_ident_name(argv[i], name_len)) {
            fprintf(stderr, "zbash: export: `%s': not a valid identifier\n", argv[i]);
            ret = 1;
            continue;
        }

        char name[128];
        if (name_len >= sizeof(name)) name_len = sizeof(name) - 1;
        strncpy(name, argv[i], name_len);
        name[name_len] = '\0';

        zbash_set_var_attr(state, name, ZBASH_ATTR_EXPORT);

        if (eq) {
            if (zbash_assign_var(state, name, eq + 1, is_append) != 0) {
                ret = 1;
            }
        } else {
            /* If only name given, ensure it exists or export current value */
            char *val = getenv(name);
            if (val) {
                setenv(name, val, 1);
            }
        }
    }
    return ret;
}

int builtin_unset(zbash_state_t *state, int argc, char **argv) {
    int ret = 0;
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
    int lineno = state->lineno > 0 ? state->lineno : 1;

    bool unset_funcs = false;
    bool unset_vars = false;
    int idx = 1;

    while (idx < argc && argv[idx][0] == '-' && argv[idx][1] != '\0') {
        if (strcmp(argv[idx], "--") == 0) { idx++; break; }
        const char *opt = argv[idx] + 1;
        while (*opt) {
            if (*opt == 'f') unset_funcs = true;
            else if (*opt == 'v') unset_vars = true;
            else {
                if (lineno > 0) {
                    fprintf(stderr, "%s: line %d: unset: -%c: invalid option\nunset: usage: unset [-f] [-v] [name ...]\n", src, lineno, *opt);
                } else {
                    fprintf(stderr, "%s: unset: -%c: invalid option\nunset: usage: unset [-f] [-v] [name ...]\n", src, *opt);
                }
                return 2;
            }
            opt++;
        }
        idx++;
    }

    if (unset_funcs && unset_vars) {
        if (lineno > 0) {
            fprintf(stderr, "%s: line %d: unset: cannot simultaneously unset a function and a variable\n", src, lineno);
        } else {
            fprintf(stderr, "%s: unset: cannot simultaneously unset a function and a variable\n", src);
        }
        return 1;
    }

    for (int i = idx; i < argc; i++) {
        const char *arg = argv[i];

        if (!unset_funcs && !strchr(arg, '[') && !is_valid_ident_name(arg, strlen(arg))) {
            if (lineno > 0) {
                fprintf(stderr, "%s: line %d: unset: `%s': not a valid identifier\n", src, lineno, arg);
            } else {
                fprintf(stderr, "%s: unset: `%s': not a valid identifier\n", src, arg);
            }
            ret = 1;
            continue;
        }

        if (unset_funcs) {
            zbash_func_t *fn = zbash_find_func(state, arg);
            if (fn && fn->is_readonly) {
                if (lineno > 0) {
                    fprintf(stderr, "%s: line %d: unset: %s: cannot unset: readonly function\n", src, lineno, arg);
                } else {
                    fprintf(stderr, "%s: unset: %s: cannot unset: readonly function\n", src, arg);
                }
                ret = 1;
                continue;
            }
            zbash_remove_func(state, arg);
            continue;
        }

        const char *bracket = strchr(arg, '[');
        if (bracket) {
            char aname[128];
            size_t nlen = bracket - arg;
            if (nlen >= sizeof(aname)) nlen = sizeof(aname) - 1;
            strncpy(aname, arg, nlen);
            aname[nlen] = '\0';

            const char *close_b = strrchr(bracket + 1, ']');
            if (close_b) {
                char sub_str[128];
                size_t slen = close_b - (bracket + 1);
                if (slen >= sizeof(sub_str)) slen = sizeof(sub_str) - 1;
                strncpy(sub_str, bracket + 1, slen);
                sub_str[slen] = '\0';

                if (zbash_is_readonly(state, aname)) {
                    fprintf(stderr, "%s: line %d: unset: %s: cannot unset: readonly variable\n", src, lineno, arg);
                    ret = 1;
                    continue;
                }
                zbash_array_t *arr = zbash_find_array(state, aname);
                if (!arr) {
                    if (getenv(aname)) {
                        fprintf(stderr, "%s: line %d: unset: %s: not an array variable\n", src, lineno, aname);
                        ret = 1;
                    }
                    continue;
                }
                if (strcmp(sub_str, "*") == 0 || strcmp(sub_str, "@") == 0) {
                    zbash_unset_array(state, aname);
                } else {
                    int arr_idx = (int)zbash_eval_arithmetic(state, sub_str);
                    zbash_unset_array_elem(state, aname, arr_idx);
                }
                continue;
            }
        }

        if (zbash_is_readonly(state, arg)) {
            fprintf(stderr, "%s: line %d: unset: %s: cannot unset: readonly variable\n", src, lineno, arg);
            ret = 1;
            continue;
        }

        /* If !unset_vars and the variable is not set, try unsetting function */
        bool var_exists = (getenv(arg) != NULL || zbash_find_array(state, arg) != NULL);
        if (!unset_vars && !var_exists && zbash_find_func(state, arg) != NULL) {
            zbash_remove_func(state, arg);
            continue;
        }

        unsetenv(arg);
        zbash_remove_var_attr(state, arg);
        zbash_unset_array(state, arg);
    }
    return ret;
}

int builtin_set(zbash_state_t *state, int argc, char **argv) {
    if (argc == 1) {
        extern char **environ;
        for (char **env = environ; *env != NULL; env++) {
            char *eq = strchr(*env, '=');
            if (!eq) {
                printf("%s\n", *env);
                continue;
            }
            size_t klen = eq - *env;
            fwrite(*env, 1, klen + 1, stdout);
            const char *val = eq + 1;
            bool needs_quote = false;
            if (val[0] == '~' || val[0] == '#') {
                needs_quote = true;
            } else {
                for (const char *p = val; *p; p++) {
                    unsigned char c = (unsigned char)*p;
                    if (isspace(c) || c == '\'' || c == '"' || c == '\\' || c == '$' || c == '`' ||
                        c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' ||
                        c == '<' || c == '>' || c == ';' || c == '&' || c == '|' || c == '*' ||
                        c == '?' || c == '!' || c == '^') {
                        needs_quote = true;
                        break;
                    }
                }
            }
            if (needs_quote) {
                putchar('\'');
                for (const char *p = val; *p; p++) {
                    if (*p == '\'') {
                        fputs("'\\''", stdout);
                    } else {
                        putchar(*p);
                    }
                }
                putchar('\'');
            } else {
                fputs(val, stdout);
            }
            putchar('\n');
        }
        return 0;
    }

    int idx = 1;
    bool saw_dash_dash = false;
    while (idx < argc) {
        if (strcmp(argv[idx], "--") == 0) {
            saw_dash_dash = true;
            idx++;
            break;
        } else if (strcmp(argv[idx], "-") == 0) {
            idx++;
            break;
        } else if ((argv[idx][0] == '-' || argv[idx][0] == '+') && argv[idx][1] != '\0') {
            bool turn_on = (argv[idx][0] == '-');
            if (strcmp(argv[idx] + 1, "o") == 0) {
                if (idx + 1 < argc) {
                    if (!set_set_o_option_by_name(state, argv[idx + 1], turn_on)) {
                        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
                        int lineno = state->lineno > 0 ? state->lineno : 0;
                        if (lineno > 0) {
                            fprintf(stderr, "%s: line %d: set: %s: invalid option name\n", src, lineno, argv[idx + 1]);
                        } else {
                            fprintf(stderr, "%s: set: %s: invalid option name\n", src, argv[idx + 1]);
                        }
                        return 1;
                    }
                    idx += 2;
                } else {
                    print_set_o_table(state, !turn_on);
                    idx++;
                    return 0;
                }
            } else {
                for (int c = 1; argv[idx][c]; c++) {
                    char ch = argv[idx][c];
                    if (ch == 'e') state->errexit = turn_on;
                    else if (ch == 'x') state->xtrace = turn_on;
                    else if (ch == 'u') state->nounset = turn_on;
                    else if (ch == 'T') state->functrace = turn_on;
                    else if (ch == 'C') state->noclobber = turn_on;
                    else if (ch == 'm') state->monitor_mode = turn_on;
                    else if (ch == 'H') state->histexp_enabled = turn_on;
                    else if (ch == 'r') {
                        if (turn_on) {
                            state->restricted = true;
                            zbash_set_var_attr(state, "PATH", ZBASH_ATTR_READONLY);
                            zbash_set_var_attr(state, "SHELL", ZBASH_ATTR_READONLY);
                            zbash_set_var_attr(state, "ENV", ZBASH_ATTR_READONLY);
                            zbash_set_var_attr(state, "BASH_ENV", ZBASH_ATTR_READONLY);
                        } else {
                            if (state->restricted) {
                                const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
                                int lineno = state->lineno > 0 ? state->lineno : 0;
                                if (lineno > 0) {
                                    fprintf(stderr, "%s: line %d: set: +r: invalid option\nset: usage: set [--abefhkmnptuvxBCHP] [-o option] [arg ...]\n", src, lineno);
                                } else {
                                    fprintf(stderr, "%s: set: +r: invalid option\nset: usage: set [--abefhkmnptuvxBCHP] [-o option] [arg ...]\n", src);
                                }
                                return 1;
                            }
                        }
                    } else if (strchr("abfhkmvnptBHP", ch) != NULL) {
                        /* other standard flags */
                    } else {
                        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
                        int lineno = state->lineno > 0 ? state->lineno : 0;
                        if (lineno > 0) {
                            fprintf(stderr, "%s: line %d: set: %c%c: invalid option\nset: usage: set [--abefhkmnptuvxBCHP] [-o option] [arg ...]\n", src, lineno, turn_on ? '-' : '+', ch);
                        } else {
                            fprintf(stderr, "%s: set: %c%c: invalid option\nset: usage: set [--abefhkmnptuvxBCHP] [-o option] [arg ...]\n", src, turn_on ? '-' : '+', ch);
                        }
                        return 2;
                    }
                }
                idx++;
            }
        } else {
            /* Non-option argument: stop option parsing, positional parameters begin */
            break;
        }
    }

    if (saw_dash_dash || idx < argc) {
        int new_count = argc - idx;
        char **new_argv = malloc((new_count + 2) * sizeof(char *));
        if (new_argv) {
            new_argv[0] = (state->argc > 0 && state->argv && state->argv[0]) ? state->argv[0] : "zbash";
            for (int k = 0; k < new_count; k++) {
                new_argv[k + 1] = strdup(argv[idx + k]);
            }
            new_argv[new_count + 1] = NULL;
            state->argv = new_argv;
            state->argc = new_count + 1;
            state->positional_params_modified = true;
        }
    }
    return 0;
}

int builtin_shift(zbash_state_t *state, int argc, char **argv) {
    int count = (argc > 1) ? atoi(argv[1]) : 1;
    int num_pos = (state->argc > 1) ? (state->argc - 1) : 0;
    if (count < 0 || count > num_pos) {
        return 1;
    }
    for (int i = 1; i <= num_pos - count; i++) {
        state->argv[i] = state->argv[i + count];
    }
    state->argc -= count;
    state->argv[state->argc] = NULL;
    return 0;
}

int builtin_local(zbash_state_t *state, int argc, char **argv) {
    if (state->func_depth <= 0) {
        fprintf(stderr, "zbash: local: can only be used in a function\n");
        return 1;
    }
    bool is_integer = false;
    bool is_readonly = false;
    bool is_export = false;
    int idx = 1;

    while (idx < argc && argv[idx][0] == '-' && argv[idx][1] != '\0') {
        if (strcmp(argv[idx], "--") == 0) { idx++; break; }
        for (int c = 1; argv[idx][c]; c++) {
            if (argv[idx][c] == 'i') is_integer = true;
            else if (argv[idx][c] == 'r') is_readonly = true;
            else if (argv[idx][c] == 'x') is_export = true;
        }
        idx++;
    }

    for (int i = idx; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        char name[128];
        const char *new_val = "";
        if (eq) {
            size_t nlen = eq - argv[i];
            if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
            strncpy(name, argv[i], nlen);
            name[nlen] = '\0';
            new_val = eq + 1;
        } else {
            strncpy(name, argv[i], sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
            const char *cur = getenv(name);
            new_val = cur ? cur : "";
        }

        if (state->local_var_count < 512) {
            const char *old = getenv(name);
            state->local_vars[state->local_var_count].name = strdup(name);
            state->local_vars[state->local_var_count].was_set = (old != NULL);
            state->local_vars[state->local_var_count].old_val = old ? strdup(old) : NULL;
            state->local_var_count++;
        }

        if (is_integer) zbash_set_var_attr(state, name, ZBASH_ATTR_INTEGER);
        if (is_export) zbash_set_var_attr(state, name, ZBASH_ATTR_EXPORT);
        if (is_readonly) zbash_set_var_attr(state, name, ZBASH_ATTR_READONLY);

        if (eq) {
            zbash_assign_var(state, name, new_val, false);
        } else {
            unsetenv(name);
        }
    }
    return 0;
}

int builtin_return(zbash_state_t *state, int argc, char **argv) {
    state->return_called = true;
    state->return_code = (argc > 1) ? atoi(argv[1]) : state->last_status;
    return state->return_code;
}
