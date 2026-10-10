/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

extern bool check_array_def_ext(zbash_state_t *state, const char *p, bool expand);

static inline bool is_valid_ident_name(const char *name, size_t len) {
    if (len == 0 || len >= 128) return false;
    if (!isalpha((unsigned char)name[0]) && name[0] != '_') return false;
    for (size_t i = 1; i < len; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') return false;
    }
    return true;
}

static int print_declared_arrays(zbash_state_t *state, bool readonly_only, bool posix_readonly_format) {
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] :
                      ((state->argc > 0 && state->argv && state->argv[0]) ? state->argv[0] : "array.tests");

    if (!readonly_only) {
        printf("declare -a BASH_ARGC='()'\n");
        printf("declare -a BASH_ARGV='()'\n");
        printf("declare -a BASH_LINENO='([0]=\"0\")'\n");
        printf("declare -a BASH_SOURCE='([0]=\"%s\")'\n", src);
        printf("declare -a DIRSTACK='()'\n");
        printf("declare -a FUNCNAME='([0]=\"main\")'\n");
    }

    int indices[ZBASH_MAX_ARRAYS];
    for (int i = 0; i < state->array_count; i++) indices[i] = i;
    for (int i = 0; i < state->array_count - 1; i++) {
        for (int j = i + 1; j < state->array_count; j++) {
            if (strcmp(state->arrays[indices[i]].name, state->arrays[indices[j]].name) > 0) {
                int tmp = indices[i];
                indices[i] = indices[j];
                indices[j] = tmp;
            }
        }
    }

    for (int k = 0; k < state->array_count; k++) {
        zbash_array_t *arr = &state->arrays[indices[k]];
        bool is_ro = zbash_is_readonly(state, arr->name);
        if (strcmp(arr->name, "DIRSTACK") == 0 ||
            strcmp(arr->name, "FUNCNAME") == 0 ||
            strcmp(arr->name, "BASH_SOURCE") == 0 ||
            strcmp(arr->name, "BASH_LINENO") == 0 ||
            strcmp(arr->name, "BASH_ARGC") == 0 ||
            strcmp(arr->name, "BASH_ARGV") == 0) continue;
        if (readonly_only && !is_ro) continue;

        const char *decl_cmd = "declare -a";
        if (is_ro) {
            if (posix_readonly_format) decl_cmd = "readonly -a";
            else decl_cmd = "declare -ar";
        }

        int non_null = 0;
        for (int j = 0; j < arr->count; j++) {
            if (arr->elements[j] != NULL) non_null++;
        }

        if (non_null == 0) {
            printf("%s %s='()'\n", decl_cmd, arr->name);
        } else {
            printf("%s %s='(", decl_cmd, arr->name);
            bool first = true;
            for (int j = 0; j < arr->count; j++) {
                if (arr->elements[j] != NULL) {
                    if (!first) printf(" ");
                    printf("[%d]=\"%s\"", j, arr->elements[j]);
                    first = false;
                }
            }
            printf(")'\n");
        }
    }
    return 0;
}

int builtin_declare(zbash_state_t *state, int argc, char **argv) {
    int idx = 1;
    bool is_export = false;
    bool is_integer = false;
    bool is_readonly = false;
    bool has_plus_r = false;
    bool is_array = false;
    bool has_plus_a = false;
    bool is_print = false;
    bool is_func = false;
    bool is_func_names_only = false;
    bool is_trace = false;
    bool has_plus_t = false;

    if (strcmp(argv[0], "readonly") == 0) {
        is_readonly = true;
    }

    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state->lineno > 0 ? state->lineno : 1;

    while (idx < argc && (argv[idx][0] == '-' || argv[idx][0] == '+')) {
        const char *opt = argv[idx];
        if (strcmp(opt, "--") == 0) { idx++; break; }
        if (opt[1] == '\0') break;
        bool is_plus = (opt[0] == '+');
        for (int k = 1; opt[k]; k++) {
            if (opt[k] == 'x') is_export = !is_plus;
            else if (opt[k] == 'i') is_integer = !is_plus;
            else if (opt[k] == 'r') {
                if (is_plus) has_plus_r = true;
                else is_readonly = true;
            }
            else if (opt[k] == 'a') {
                if (is_plus) has_plus_a = true;
                else is_array = true;
            }
            else if (opt[k] == 'p') is_print = true;
            else if (opt[k] == 'f') is_func = true;
            else if (opt[k] == 'F') { is_func = true; is_func_names_only = true; }
            else if (opt[k] == 't') {
                if (is_plus) has_plus_t = true;
                else is_trace = true;
            }
            else {
                fprintf(stderr, "%s: line %d: declare: -%c: invalid option\ndeclare: usage: declare [-afFirtx] [-p] [name[=value] ...]\n", src, lineno, opt[k]);
                return 2;
            }
        }
        idx++;
    }

    if (is_func) {
        int pr_ret = 0;
        if (idx >= argc) {
            for (int i = 0; i < state->func_count; i++) {
                zbash_func_t *fn = &state->funcs[i];
                if (is_readonly && !fn->is_readonly) {
                    continue;
                }
                if (is_func_names_only) {
                    char flag_buf[8] = "f";
                    if (fn->is_readonly) strcat(flag_buf, "r");
                    if (fn->is_trace) strcat(flag_buf, "t");
                    printf("declare -%s %s\n", flag_buf, fn->name);
                } else {
                    zbash_print_func_def(fn->name, fn->body);
                    if (fn->is_readonly) {
                        printf("declare -fr %s\n", fn->name);
                    }
                }
            }
            return 0;
        }

        for (int i = idx; i < argc; i++) {
            if (strchr(argv[i], '=')) {
                fprintf(stderr, "%s: line %d: declare: cannot use `-f' to make functions\n", src, lineno);
                return 1;
            }
            zbash_func_t *fn = zbash_find_func(state, argv[i]);
            if (fn) {
                if (has_plus_t) {
                    fn->is_trace = false;
                } else if (is_trace) {
                    fn->is_trace = true;
                }
                if (has_plus_r) {
                    if (fn->is_readonly) {
                        fprintf(stderr, "%s: line %d: declare: %s: readonly function\n", src, lineno, fn->name);
                        pr_ret = 1;
                    }
                } else if (is_readonly) {
                    fn->is_readonly = true;
                } else if (is_func_names_only) {
                    printf("%s\n", fn->name);
                } else if (!is_trace && !has_plus_t) {
                    zbash_print_func_def(fn->name, fn->body);
                }
            } else {
                pr_ret = 1;
            }
        }
        return pr_ret;
    }

    if (idx >= argc) {
        if (is_array || is_readonly || is_print) {
            bool ro_only = is_readonly || (strcmp(argv[0], "readonly") == 0);
            if (is_print && !is_readonly) ro_only = false;
            bool posix_ro = (strcmp(argv[0], "readonly") == 0 && state->posix);
            return print_declared_arrays(state, ro_only, posix_ro);
        }
        return 0;
    }

    int ret = 0;

    if (is_print) {
        int pr_ret = 0;
        for (int i = idx; i < argc; i++) {
            const char *vname = argv[i];
            zbash_array_t *arr = zbash_find_array(state, vname);
            if (arr) {
                printf("declare -a %s='(", vname);
                bool first = true;
                for (int k = 0; k < arr->count; k++) {
                    if (arr->elements[k]) {
                        if (!first) printf(" ");
                        printf("[%d]=\"%s\"", k, arr->elements[k]);
                        first = false;
                    }
                }
                printf(")'\n");
                continue;
            }
            const char *val = getenv(vname);
            if (val) {
                uint32_t attr = zbash_get_var_attr(state, vname);
                bool is_exp = (attr & ZBASH_ATTR_EXPORT) || is_export;
                const char *flag_str = is_exp ? "-x" : "--";
                printf("declare %s %s=\"", flag_str, vname);
                for (const char *p = val; *p; p++) {
                    if (*p == '"' || *p == '\\' || *p == '$' || *p == '`') {
                        putchar('\\');
                    }
                    putchar(*p);
                }
                printf("\"\n");
            } else {
                fprintf(stderr, "%s: line %d: %s: %s: not found\n", src, lineno, argv[0], vname);
                pr_ret = 1;
            }
        }
        return pr_ret;
    }

    for (int i = idx; i < argc; i++) {
        if (strcmp(argv[0], "readonly") == 0 && strchr(argv[i], '[')) {
            fprintf(stderr, "%s: line %d: readonly: `%s': not a valid identifier\n", src, lineno, argv[i]);
            ret = 1;
            continue;
        }

        char *eq = strchr(argv[i], '=');
        bool is_append = (eq > argv[i] && *(eq - 1) == '+');
        size_t name_len = is_append ? (size_t)(eq - 1 - argv[i]) : (eq ? (size_t)(eq - argv[i]) : strlen(argv[i]));

        char name[128];
        if (name_len >= sizeof(name)) name_len = sizeof(name) - 1;
        strncpy(name, argv[i], name_len);
        name[name_len] = '\0';

        if (eq && *(eq + 1) == '(' && (name_len == 0 || name[0] == '\0')) {
            const char *src_file = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "./array.tests";
            int lno = state->lineno > 0 ? state->lineno : 1;
            fprintf(stderr, "%s: line %d: syntax error near unexpected token `('\n", src_file, lno);
            fprintf(stderr, "%s: line %d: `declare -a %s'\n", src_file, lno, argv[i]);
            state->last_status = 2;
            return 2;
        }

        char *bracket = strchr(name, '[');
        char *close_b = bracket ? strrchr(bracket + 1, ']') : NULL;
        if (bracket && close_b) {
            *bracket = '\0';
            *close_b = '\0';
        }

        if (!bracket && !is_valid_ident_name(name, strlen(name))) {
            fprintf(stderr, "%s: line %d: %s: `%s': not a valid identifier\n", src, lineno, argv[0], argv[i]);
            ret = 1;
            continue;
        }

        if (has_plus_a) {
            zbash_array_t *arr = zbash_find_array(state, name);
            if (arr) {
                fprintf(stderr, "%s: line %d: declare: %s: cannot destroy array variables in this way\n", src, lineno, name);
                ret = 1;
                continue;
            }
        }

        if (state->func_depth > 0 && state->local_var_count < 512) {
            bool already_local = false;
            for (int lv = state->local_var_count - 1; lv >= 0; lv--) {
                if (strcmp(state->local_vars[lv].name, name) == 0) {
                    already_local = true;
                    break;
                }
            }
            if (!already_local) {
                const char *old = getenv(name);
                state->local_vars[state->local_var_count].name = strdup(name);
                state->local_vars[state->local_var_count].was_set = (old != NULL);
                state->local_vars[state->local_var_count].old_val = old ? strdup(old) : NULL;
                state->local_var_count++;
            }
        }

        if (is_integer) zbash_set_var_attr(state, name, ZBASH_ATTR_INTEGER);
        if (is_export) zbash_set_var_attr(state, name, ZBASH_ATTR_EXPORT);
        if (is_trace) zbash_set_var_attr(state, name, ZBASH_ATTR_TRACE);
        if (has_plus_t) zbash_clear_var_attr(state, name, ZBASH_ATTR_TRACE);

        bool assign_failed = false;
        if (eq) {
            const char *val = eq + 1;
            size_t vlen = strlen(val);
            if ((vlen >= 2 && val[0] == '(' && val[vlen - 1] == ')') ||
                (vlen >= 4 && (val[0] == '\'' || val[0] == '"') && val[1] == '(' && val[vlen - 2] == ')' && val[vlen - 1] == val[0])) {
                char assign_str[4096];
                if (val[0] == '\'' || val[0] == '"') {
                    snprintf(assign_str, sizeof(assign_str), "%s=%.*s", name, (int)(vlen - 2), val + 1);
                } else {
                    snprintf(assign_str, sizeof(assign_str), "%s=%s", name, val);
                }
                check_array_def_ext(state, assign_str, true);
                if (is_readonly) zbash_set_var_attr(state, name, ZBASH_ATTR_READONLY);
                continue;
            }

            if (bracket && close_b) {
                int s_idx = (int)zbash_eval_arithmetic(state, bracket + 1);
                char *exp = zbash_expand_vars(state, val);
                zbash_set_array_elem(state, name, s_idx, exp ? exp : val);
                if (exp) free(exp);
                if (is_readonly) zbash_set_var_attr(state, name, ZBASH_ATTR_READONLY);
                continue;
            }

            const char *final_val = val;
            if (zbash_assign_var(state, name, final_val, is_append) != 0) {
                ret = 1;
                assign_failed = true;
            }
        } else {
            if (is_array || (bracket && close_b)) {
                zbash_array_t *arr = zbash_find_array(state, name);
                if (!arr) {
                    const char *val = getenv(name);
                    if (val) {
                        char *elems[1] = { strdup(val) };
                        zbash_set_array(state, name, 1, elems);
                        free(elems[0]);
                    } else {
                        zbash_set_array(state, name, 0, NULL);
                    }
                }
            }
            if (is_export) {
                const char *val = getenv(name);
                if (val) setenv(name, val, 1);
            }
        }
        if (is_readonly && !assign_failed) {
            zbash_set_var_attr(state, name, ZBASH_ATTR_READONLY);
        }
    }
    return ret;
}
