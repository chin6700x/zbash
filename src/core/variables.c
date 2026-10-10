/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "core_internal.h"

/* ========================================================================= */
/* Array Management                                                          */
/* ========================================================================= */

void zbash_set_array(zbash_state_t *state, const char *name, int count, char **elements) {
    if (!name) return;
    for (int i = 0; i < state->array_count; i++) {
        if (strcmp(state->arrays[i].name, name) == 0) {
            for (int j = 0; j < state->arrays[i].count; j++) {
                free(state->arrays[i].elements[j]);
            }
            free(state->arrays[i].elements);
            state->arrays[i].elements = malloc(sizeof(char *) * (count > 0 ? count : 1));
            for (int j = 0; j < count; j++) {
                state->arrays[i].elements[j] = strdup(elements[j]);
            }
            state->arrays[i].count = count;
            return;
        }
    }
    if (state->array_count < ZBASH_MAX_ARRAYS) {
        zbash_array_t *arr = &state->arrays[state->array_count++];
        arr->name = strdup(name);
        arr->elements = malloc(sizeof(char *) * (count > 0 ? count : 1));
        for (int j = 0; j < count; j++) {
            arr->elements[j] = strdup(elements[j]);
        }
        arr->count = count;
        arr->cap = count;
    }
}

zbash_array_t *zbash_find_array(zbash_state_t *state, const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < state->array_count; i++) {
        if (strcmp(state->arrays[i].name, name) == 0) {
            return &state->arrays[i];
        }
    }
    return NULL;
}

const char *zbash_get_array_elem(zbash_state_t *state, const char *name, int index) {
    if (!name || index < 0) return "";
    zbash_array_t *arr = zbash_find_array(state, name);
    if (!arr) {
        if (index == 0) {
            const char *env_val = getenv(name);
            return env_val ? env_val : "";
        }
        return "";
    }
    if (index >= arr->count) return "";
    return arr->elements[index] ? arr->elements[index] : "";
}

void zbash_set_array_elem(zbash_state_t *state, const char *name, int index, const char *val) {
    if (!name || index < 0) return;
    zbash_array_t *arr = zbash_find_array(state, name);
    if (!arr) {
        const char *existing = getenv(name);
        int init_count = (index + 1 > 1) ? (index + 1) : 1;
        char **elems = calloc(init_count, sizeof(char *));
        if (existing) {
            elems[0] = strdup(existing);
        }
        elems[index] = strdup(val ? val : "");
        if (state->array_count < ZBASH_MAX_ARRAYS) {
            zbash_array_t *narr = &state->arrays[state->array_count++];
            narr->name = strdup(name);
            narr->elements = elems;
            narr->count = init_count;
            narr->cap = init_count;
        } else {
            for (int j = 0; j < init_count; j++) if (elems[j]) free(elems[j]);
            free(elems);
        }
        return;
    }

    if (index < arr->count) {
        if (arr->elements[index]) free(arr->elements[index]);
        arr->elements[index] = strdup(val ? val : "");
    } else {
        int new_count = index + 1;
        char **new_elems = realloc(arr->elements, sizeof(char *) * new_count);
        if (new_elems) {
            arr->elements = new_elems;
            for (int j = arr->count; j < index; j++) {
                arr->elements[j] = NULL;
            }
            arr->elements[index] = strdup(val ? val : "");
            arr->count = new_count;
            arr->cap = new_count;
        }
    }
}

void zbash_unset_array(zbash_state_t *state, const char *name) {
    if (!state || !name) return;
    for (int i = 0; i < state->array_count; i++) {
        if (strcmp(state->arrays[i].name, name) == 0) {
            free(state->arrays[i].name);
            for (int j = 0; j < state->arrays[i].count; j++) {
                if (state->arrays[i].elements[j]) free(state->arrays[i].elements[j]);
            }
            free(state->arrays[i].elements);
            for (int k = i; k < state->array_count - 1; k++) {
                state->arrays[k] = state->arrays[k + 1];
            }
            state->array_count--;
            return;
        }
    }
}

void zbash_unset_array_elem(zbash_state_t *state, const char *name, int index) {
    if (!state || !name || index < 0) return;
    zbash_array_t *arr = zbash_find_array(state, name);
    if (!arr || index >= arr->count) return;
    if (arr->elements[index]) {
        free(arr->elements[index]);
        arr->elements[index] = NULL;
    }
}

bool check_array_def(zbash_state_t *state, const char *p) {
    return check_array_def_ext(state, p, true);
}

bool check_array_def_ext(zbash_state_t *state, const char *p, bool expand) {
    while (*p && isspace((unsigned char)*p)) p++;
    const char *eq = strchr(p, '=');
    if (!eq || eq == p || *(eq + 1) != '(') return false;
    bool is_append = false;
    size_t an_len = eq - p;
    if (eq > p && *(eq - 1) == '+') {
        is_append = true;
        an_len--;
    }
    if (!is_valid_ident(p, an_len)) return false;

    char arr_name[128];
    strncpy(arr_name, p, an_len);
    arr_name[an_len] = '\0';

    if (zbash_is_readonly(state, arr_name)) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
        if (state->lineno > 0) {
            fprintf(stderr, "%s: line %d: %s: readonly variable\n", src, state->lineno, arr_name);
        } else {
            fprintf(stderr, "%s: %s: readonly variable\n", src, arr_name);
        }
        state->last_status = 1;
        return true;
    }

    const char *b_start = eq + 2;
    const char *cp = b_start;
    int pdepth = 1;
    bool in_s = false, in_d = false;
    while (*cp && pdepth > 0) {
        if (*cp == '\\' && *(cp + 1)) {
            cp += 2;
            continue;
        }
        if (*cp == '\'' && !in_d) in_s = !in_s;
        else if (*cp == '"' && !in_s) in_d = !in_d;
        else if (!in_s && !in_d) {
            if (*cp == '(') pdepth++;
            else if (*cp == ')') {
                pdepth--;
                if (pdepth == 0) break;
            }
        }
        cp++;
    }
    if (pdepth != 0) return false;
    const char *close_paren = cp;

    /* If character immediately after ')' is not space, semicolon, '&', or '\0', this is not an array definition (e.g. a=(4*3)/2) */
    if (*(close_paren + 1) != '\0' && !isspace((unsigned char)*(close_paren + 1)) && *(close_paren + 1) != ';' && *(close_paren + 1) != '&') {
        return false;
    }

    size_t abuf_len = close_paren - b_start;
    char *abuf = malloc(abuf_len + 1);
    strncpy(abuf, b_start, abuf_len);
    abuf[abuf_len] = '\0';

    bool is_int = zbash_is_integer(state, arr_name);

    if (!is_append) {
        zbash_unset_array(state, arr_name);
        unsetenv(arr_name);
    }

    zbash_array_t *arr = zbash_find_array(state, arr_name);
    int current_index = (is_append && arr) ? arr->count : 0;

    char *t_ptr = abuf;
    while (*t_ptr) {
        while (*t_ptr && isspace((unsigned char)*t_ptr)) t_ptr++;
        if (*t_ptr == '\0') break;
        if (*t_ptr == '#') {
            while (*t_ptr && *t_ptr != '\n') t_ptr++;
            continue;
        }

        if (*t_ptr == '&' || *t_ptr == ';' || *t_ptr == '|') {
            const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "./array.tests";
            int lineno = state->lineno > 0 ? state->lineno : 1;
            fprintf(stderr, "%s: line %d: syntax error near unexpected token `%c'\n", src, lineno, *t_ptr);
            fprintf(stderr, "%s: line %d: `%s'\n", src, lineno, p);
            state->last_status = 1;
            free(abuf);
            return true;
        }

        /* Check for [subscript]=val or [subscript]+=val */
        bool is_sub_assign = false;
        bool elem_append = false;
        char *sub_start = NULL;
        char *sub_end = NULL;

        if (*t_ptr == '[') {
            char *scan_b = t_ptr + 1;
            int bdepth = 1;
            bool s_s = false, s_d = false;
            while (*scan_b && bdepth > 0) {
                if (*scan_b == '\'' && !s_d) s_s = !s_s;
                else if (*scan_b == '"' && !s_s) s_d = !s_d;
                else if (!s_s && !s_d) {
                    if (*scan_b == '[') bdepth++;
                    else if (*scan_b == ']') {
                        bdepth--;
                        if (bdepth == 0) break;
                    }
                }
                scan_b++;
            }
            if (bdepth == 0) {
                if (scan_b[1] == '=') {
                    is_sub_assign = true;
                    elem_append = false;
                    sub_start = t_ptr + 1;
                    sub_end = scan_b;
                } else if (scan_b[1] == '+' && scan_b[2] == '=') {
                    is_sub_assign = true;
                    elem_append = true;
                    sub_start = t_ptr + 1;
                    sub_end = scan_b;
                }
            }
        }

        if (is_sub_assign) {
            size_t sub_len = sub_end - sub_start;
            char sub_buf[512];
            if (sub_len >= sizeof(sub_buf)) sub_len = sizeof(sub_buf) - 1;
            strncpy(sub_buf, sub_start, sub_len);
            sub_buf[sub_len] = '\0';

            char *val_start = sub_end + (elem_append ? 3 : 2);
            char *val_end = val_start;
            bool v_s = false, v_d = false;
            int vp_depth = 0;
            while (*val_end) {
                if (*val_end == '\\' && *(val_end + 1)) { val_end += 2; continue; }
                if (*val_end == '\'' && !v_d) v_s = !v_s;
                else if (*val_end == '"' && !v_s) v_d = !v_d;
                else if (!v_s && !v_d) {
                    if (*val_end == '(') vp_depth++;
                    else if (*val_end == ')') { if (vp_depth > 0) vp_depth--; }
                    else if (vp_depth == 0 && isspace((unsigned char)*val_end)) break;
                }
                val_end++;
            }
            size_t v_len = val_end - val_start;
            char val_buf[4096];
            if (v_len >= sizeof(val_buf)) v_len = sizeof(val_buf) - 1;
            strncpy(val_buf, val_start, v_len);
            val_buf[v_len] = '\0';
            t_ptr = val_end;

            const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "./array.tests";
            int lineno = state->lineno > 0 ? state->lineno : 1;

            if (sub_buf[0] == '\0') {
                fprintf(stderr, "%s: line %d: []=%s: bad array subscript\n", src, lineno, val_buf);
                state->last_status = 1;
                continue;
            }
            if (strcmp(sub_buf, "*") == 0 || strcmp(sub_buf, "@") == 0) {
                fprintf(stderr, "%s: line %d: [%s]=%s: cannot assign to non-numeric index\n", src, lineno, sub_buf, val_buf);
                state->last_status = 1;
                continue;
            }

            long long s_val = zbash_eval_arithmetic(state, sub_buf);
            if (s_val < 0) {
                fprintf(stderr, "%s: line %d: [%s]=%s: bad array subscript\n", src, lineno, sub_buf, val_buf);
                state->last_status = 1;
                continue;
            }
            int sub_idx = (int)s_val;

            char *exp_val = zbash_expand_vars(state, val_buf);
            const char *final_val = exp_val ? exp_val : val_buf;
            char clean_val[4096];
            size_t flen = strlen(final_val);
            if (flen >= 2 && ((final_val[0] == '"' && final_val[flen - 1] == '"') || (final_val[0] == '\'' && final_val[flen - 1] == '\''))) {
                strncpy(clean_val, final_val + 1, flen - 2);
                clean_val[flen - 2] = '\0';
                final_val = clean_val;
            }

            if (elem_append) {
                const char *old = zbash_get_array_elem(state, arr_name, sub_idx);
                if (is_int) {
                    long long old_n = (old && *old) ? zbash_eval_arithmetic(state, old) : 0;
                    long long add_n = (final_val && *final_val) ? zbash_eval_arithmetic(state, final_val) : 0;
                    char nbuf[32];
                    snprintf(nbuf, sizeof(nbuf), "%lld", old_n + add_n);
                    zbash_set_array_elem(state, arr_name, sub_idx, nbuf);
                } else {
                    char new_val[8192];
                    snprintf(new_val, sizeof(new_val), "%s%s", old ? old : "", final_val ? final_val : "");
                    zbash_set_array_elem(state, arr_name, sub_idx, new_val);
                }
            } else {
                if (is_int) {
                    long long n = (final_val && *final_val) ? zbash_eval_arithmetic(state, final_val) : 0;
                    char nbuf[32];
                    snprintf(nbuf, sizeof(nbuf), "%lld", n);
                    zbash_set_array_elem(state, arr_name, sub_idx, nbuf);
                } else {
                    zbash_set_array_elem(state, arr_name, sub_idx, final_val);
                }
            }
            if (exp_val) free(exp_val);
            current_index = sub_idx + 1;
            continue;
        }

        /* Normal sequential element word */
        char *w_start = t_ptr;
        char *w_end = t_ptr;
        bool in_sq = false, in_dq = false;
        int p_depth = 0, br_depth = 0;
        while (*w_end) {
            if (*w_end == '\\' && *(w_end + 1)) {
                w_end += 2;
                continue;
            }
            if (*w_end == '\'' && !in_dq) {
                in_sq = !in_sq;
            } else if (*w_end == '"' && !in_sq) {
                in_dq = !in_dq;
            } else if (!in_sq && !in_dq) {
                if (*w_end == '$' && *(w_end + 1) == '(') {
                    p_depth++;
                    w_end += 2;
                    continue;
                } else if (*w_end == '(') {
                    p_depth++;
                } else if (*w_end == ')') {
                    if (p_depth > 0) p_depth--;
                } else if (*w_end == '{') {
                    br_depth++;
                } else if (*w_end == '}') {
                    if (br_depth > 0) br_depth--;
                } else if (p_depth == 0 && br_depth == 0 && isspace((unsigned char)*w_end)) {
                    break;
                }
            }
            w_end++;
        }
        size_t w_len = w_end - w_start;
        char tok[4096];
        if (w_len >= sizeof(tok)) w_len = sizeof(tok) - 1;
        strncpy(tok, w_start, w_len);
        tok[w_len] = '\0';
        t_ptr = w_end;

        /* Positional parameters "$@" */
        if (strcmp(tok, "\"$@\"") == 0 || strcmp(tok, "\"${@}\"") == 0 || strcmp(tok, "$@") == 0 || strcmp(tok, "${@}") == 0) {
            for (int p_arg = 1; p_arg < state->argc; p_arg++) {
                zbash_set_array_elem(state, arr_name, current_index++, state->argv[p_arg]);
            }
            continue;
        }

        /* Check for array expansion "${src[@]}" or ${src[@]} */
        bool is_arr_exp = false;
        char src_name[128] = {0};
        bool is_ind = false;
        size_t t_len = strlen(tok);
        bool q_arr = (t_len >= 8 && tok[0] == '"' && tok[1] == '$' && tok[2] == '{' && tok[t_len - 2] == '}' && tok[t_len - 1] == '"');
        bool uq_arr = (t_len >= 6 && tok[0] == '$' && tok[1] == '{' && tok[t_len - 1] == '}');
        if (q_arr || uq_arr) {
            char *at = strstr(tok, "[@]");
            if (at) {
                size_t off = q_arr ? 3 : 2;
                if (tok[off] == '!') {
                    is_ind = true;
                    off++;
                }
                size_t n_len = at - (tok + off);
                if (n_len > 0 && n_len < sizeof(src_name)) {
                    strncpy(src_name, tok + off, n_len);
                    src_name[n_len] = '\0';
                    is_arr_exp = true;
                }
            } else if (q_arr && tok[3] == '!') {
                char ivar[128] = {0};
                size_t iv_len = (t_len - 2) - 4;
                if (iv_len > 0 && iv_len < sizeof(ivar)) {
                    strncpy(ivar, tok + 4, iv_len);
                    ivar[iv_len] = '\0';
                    char num_buf[32];
                    const char *rval = NULL;
                    char *rdyn = NULL;
                    if (zbash_get_param_value(state, ivar, num_buf, sizeof(num_buf), &rval, &rdyn)) {
                        if (rval) {
                            char *iat = strstr(rval, "[@]");
                            if (iat) {
                                size_t alen = iat - rval;
                                if (alen > 0 && alen < sizeof(src_name)) {
                                    strncpy(src_name, rval, alen);
                                    src_name[alen] = '\0';
                                    is_arr_exp = true;
                                }
                            }
                        }
                    }
                    if (rdyn) free(rdyn);
                }
            }
        }
        if (is_arr_exp) {
            zbash_array_t *s_arr = zbash_find_array(state, src_name);
            if (s_arr) {
                if (is_ind) {
                    for (int a = 0; a < s_arr->count; a++) {
                        if (!s_arr->elements[a]) continue;
                        char idx_str[16];
                        snprintf(idx_str, sizeof(idx_str), "%d", a);
                        zbash_set_array_elem(state, arr_name, current_index++, idx_str);
                    }
                } else {
                    for (int a = 0; a < s_arr->count; a++) {
                        if (!s_arr->elements[a]) continue;
                        if (uq_arr && s_arr->elements[a][0] == '\0') continue;
                        zbash_set_array_elem(state, arr_name, current_index++, s_arr->elements[a]);
                    }
                }
                continue;
            } else {
                const char *s_val = getenv(src_name);
                if (s_val) {
                    if (is_ind) {
                        zbash_set_array_elem(state, arr_name, current_index++, "0");
                    } else {
                        zbash_set_array_elem(state, arr_name, current_index++, s_val);
                    }
                }
                continue;
            }
        }

        /* Globbing for * or ? if unquoted */
        if (tok[0] != '\'' && tok[0] != '"' && (strchr(tok, '*') || strchr(tok, '?'))) {
            glob_t g;
            if (glob(tok, 0, NULL, &g) == 0) {
                for (size_t m = 0; m < g.gl_pathc; m++) {
                    zbash_set_array_elem(state, arr_name, current_index++, g.gl_pathv[m]);
                }
                globfree(&g);
                continue;
            }
        }

        /* Unquoted command substitution or unquoted expansion that splits on IFS */
        bool is_unquoted_cmd_sub = (expand && tok[0] == '$' && tok[1] == '(');
        bool is_unquoted_var = (expand && tok[0] == '$' && tok[1] != '\0' && tok[0] != '"');

        if (is_unquoted_cmd_sub || is_unquoted_var) {
            char *exp = zbash_expand_vars(state, tok);
            if (exp) {
                const char *ifs = getenv("IFS");
                if (!ifs) ifs = " \t\n";
                char *ep = exp;
                while (*ep) {
                    while (*ep && strchr(ifs, *ep)) ep++;
                    if (!*ep) break;
                    char *w = ep;
                    while (*ep && !strchr(ifs, *ep)) ep++;
                    char saved = *ep;
                    *ep = '\0';
                    zbash_set_array_elem(state, arr_name, current_index++, w);
                    *ep = saved;
                }
                free(exp);
                continue;
            }
        }

        /* Quoted or literal word */
        char *exp_tok = expand ? zbash_expand_vars(state, tok) : NULL;
        const char *final_tok = exp_tok ? exp_tok : tok;
        char clean_tok[4096];
        size_t tlen = strlen(final_tok);
        if (tlen >= 2 && ((final_tok[0] == '"' && final_tok[tlen - 1] == '"') || (final_tok[0] == '\'' && final_tok[tlen - 1] == '\''))) {
            strncpy(clean_tok, final_tok + 1, tlen - 2);
            clean_tok[tlen - 2] = '\0';
            final_tok = clean_tok;
        } else if (strchr(final_tok, '\\')) {
            size_t c_idx = 0;
            for (size_t k = 0; final_tok[k]; k++) {
                if (final_tok[k] == '\\' && final_tok[k + 1]) {
                    k++;
                }
                clean_tok[c_idx++] = final_tok[k];
            }
            clean_tok[c_idx] = '\0';
            final_tok = clean_tok;
        }

        if (is_int) {
            long long n = (final_tok && *final_tok) ? zbash_eval_arithmetic(state, final_tok) : 0;
            char nbuf[32];
            snprintf(nbuf, sizeof(nbuf), "%lld", n);
            zbash_set_array_elem(state, arr_name, current_index++, nbuf);
        } else {
            zbash_set_array_elem(state, arr_name, current_index++, final_tok);
        }
        if (exp_tok) free(exp_tok);
    }

    if (!zbash_find_array(state, arr_name)) {
        zbash_set_array(state, arr_name, 0, NULL);
    }

    free(abuf);

    const char *rest = close_paren + 1;
    while (*rest && isspace((unsigned char)*rest)) rest++;
    if (*rest == ';') rest++;
    while (*rest && isspace((unsigned char)*rest)) rest++;
    if (*rest != '\0' && *rest != '#') {
        zbash_run_line(state, rest);
    }

    return true;
}

/* ========================================================================= */
/* Variable Attributes & Assignments                                         */
/* ========================================================================= */

uint32_t zbash_get_var_attr(zbash_state_t *state, const char *name) {
    if (!state || !name) return 0;
    for (int i = 0; i < state->var_attr_count; i++) {
        if (strcmp(state->var_attrs[i].name, name) == 0) {
            return state->var_attrs[i].flags;
        }
    }
    return 0;
}

void zbash_set_var_attr(zbash_state_t *state, const char *name, uint32_t flag) {
    if (!state || !name) return;
    for (int i = 0; i < state->var_attr_count; i++) {
        if (strcmp(state->var_attrs[i].name, name) == 0) {
            state->var_attrs[i].flags |= flag;
            return;
        }
    }
    if (state->var_attr_count < ZBASH_MAX_VAR_ATTRS) {
        state->var_attrs[state->var_attr_count].name = strdup(name);
        state->var_attrs[state->var_attr_count].flags = flag;
        state->var_attr_count++;
    }
}

void zbash_clear_var_attr(zbash_state_t *state, const char *name, uint32_t flag) {
    if (!state || !name) return;
    for (int i = 0; i < state->var_attr_count; i++) {
        if (strcmp(state->var_attrs[i].name, name) == 0) {
            state->var_attrs[i].flags &= ~flag;
            return;
        }
    }
}

void zbash_remove_var_attr(zbash_state_t *state, const char *name) {
    if (!state || !name) return;
    for (int i = 0; i < state->var_attr_count; i++) {
        if (strcmp(state->var_attrs[i].name, name) == 0) {
            free(state->var_attrs[i].name);
            for (int k = i; k < state->var_attr_count - 1; k++) {
                state->var_attrs[k] = state->var_attrs[k + 1];
            }
            state->var_attr_count--;
            return;
        }
    }
}

bool zbash_is_readonly(zbash_state_t *state, const char *name) {
    return (zbash_get_var_attr(state, name) & ZBASH_ATTR_READONLY) != 0;
}

bool zbash_is_integer(zbash_state_t *state, const char *name) {
    return (zbash_get_var_attr(state, name) & ZBASH_ATTR_INTEGER) != 0;
}

int zbash_assign_var(zbash_state_t *state, const char *name, const char *val, bool is_append) {
    if (!state || !name) return 0;
    if (zbash_is_readonly(state, name)) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
        if (state->lineno > 0) {
            fprintf(stderr, "%s: line %d: %s: readonly variable\n", src, state->lineno, name);
        } else {
            fprintf(stderr, "%s: %s: readonly variable\n", src, name);
        }
        state->last_status = 1;
        return 1;
    }

    if (strcmp(name, "LINENO") == 0) {
        state->lineno = atoi(val ? val : "0");
        return 0;
    }

    zbash_array_t *arr = zbash_find_array(state, name);
    if (arr) {
        if (zbash_is_integer(state, name)) {
            long long num = 0;
            if (is_append) {
                const char *old = (arr->count > 0 && arr->elements[0]) ? arr->elements[0] : "";
                long long old_num = (old && *old) ? zbash_eval_arithmetic(state, old) : 0;
                long long add_num = (val && *val) ? zbash_eval_arithmetic(state, val) : 0;
                num = old_num + add_num;
            } else {
                num = (val && *val) ? zbash_eval_arithmetic(state, val) : 0;
            }
            char nbuf[32];
            snprintf(nbuf, sizeof(nbuf), "%lld", num);
            zbash_set_array_elem(state, name, 0, nbuf);
        } else {
            if (is_append) {
                const char *old = (arr->count > 0 && arr->elements[0]) ? arr->elements[0] : "";
                char new_val[8192];
                snprintf(new_val, sizeof(new_val), "%s%s", old, val ? val : "");
                zbash_set_array_elem(state, name, 0, new_val);
            } else {
                zbash_set_array_elem(state, name, 0, val ? val : "");
            }
        }
        return 0;
    }
    if (zbash_is_integer(state, name)) {
        long long num = 0;
        if (is_append) {
            const char *old = getenv(name);
            long long old_num = (old && *old) ? zbash_eval_arithmetic(state, old) : 0;
            long long add_num = (val && *val) ? zbash_eval_arithmetic(state, val) : 0;
            num = old_num + add_num;
        } else {
            num = (val && *val) ? zbash_eval_arithmetic(state, val) : 0;
        }
        char nbuf[32];
        snprintf(nbuf, sizeof(nbuf), "%lld", num);
        setenv(name, nbuf, 1);
    } else {
        if (is_append) {
            const char *old = getenv(name);
            char new_val[8192];
            snprintf(new_val, sizeof(new_val), "%s%s", old ? old : "", val ? val : "");
            setenv(name, new_val, 1);
        } else {
            setenv(name, val ? val : "", 1);
        }
    }
    return 0;
}
