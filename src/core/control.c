/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "core_internal.h"

/* ========================================================================= */
/* Function Management                                                       */
/* ========================================================================= */

int zbash_register_func(zbash_state_t *state, const char *name, const char *body, int start_lineno) {
    if (!name || !body) return -1;
    for (int i = 0; i < state->func_count; i++) {
        if (strcmp(state->funcs[i].name, name) == 0) {
            if (state->funcs[i].is_readonly) {
                const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
                int lineno = state->lineno > 0 ? state->lineno : 1;
                fprintf(stderr, "%s: line %d: %s: readonly function\n", src, lineno, name);
                state->last_status = 1;
                return -1;
            }
            free(state->funcs[i].body);
            state->funcs[i].body = strdup(body);
            state->funcs[i].start_lineno = start_lineno;
            return 0;
        }
    }
    if (state->func_count < ZBASH_MAX_FUNCS) {
        state->funcs[state->func_count].name = strdup(name);
        state->funcs[state->func_count].body = strdup(body);
        state->funcs[state->func_count].start_lineno = start_lineno;
        state->funcs[state->func_count].is_readonly = false;
        state->func_count++;
        return 0;
    }
    return -1;
}

zbash_func_t *zbash_find_func(zbash_state_t *state, const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < state->func_count; i++) {
        if (strcmp(state->funcs[i].name, name) == 0) {
            return &state->funcs[i];
        }
    }
    return NULL;
}

int zbash_remove_func(zbash_state_t *state, const char *name) {
    if (!state || !name) return -1;
    for (int i = 0; i < state->func_count; i++) {
        if (strcmp(state->funcs[i].name, name) == 0) {
            if (state->funcs[i].is_readonly) {
                const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
                int lineno = state->lineno > 0 ? state->lineno : 1;
                fprintf(stderr, "%s: line %d: unset: %s: cannot unset: readonly function\n", src, lineno, name);
                state->last_status = 1;
                return -1;
            }
            free(state->funcs[i].name);
            free(state->funcs[i].body);
            for (int k = i; k < state->func_count - 1; k++) {
                state->funcs[k] = state->funcs[k + 1];
            }
            state->func_count--;
            return 0;
        }
    }
    return 0;
}

int zbash_exec_func(zbash_state_t *state, zbash_func_t *fn, int argc, char **argv) {
    if (!fn || !fn->body) return 0;
    int saved_argc = state->argc;
    char **saved_argv = state->argv;
    int saved_local_mark = state->local_var_count;

    /* Make private copy of argv so shift does not corrupt calling pipeline's argv */
    char **orig_copies = malloc(sizeof(char *) * (argc > 0 ? argc : 1));
    char **func_argv = malloc(sizeof(char *) * (argc + 1));
    orig_copies[0] = (saved_argv && saved_argc > 0 && saved_argv[0]) ? strdup(saved_argv[0]) : strdup(argv[0]);
    func_argv[0] = orig_copies[0];
    for (int a = 1; a < argc; a++) {
        orig_copies[a] = strdup(argv[a]);
        func_argv[a] = orig_copies[a];
    }
    func_argv[argc] = NULL;

    state->argc = argc;
    state->argv = func_argv;
    state->return_called = false;
    state->func_depth++;
    int saved_lineno = state->lineno;
    if (fn->start_lineno > 0) state->lineno = fn->start_lineno;

    int frame_idx = state->frame_count;
    if (frame_idx < ZBASH_MAX_FRAMES) {
        state->frames[frame_idx].func_name = fn->name;
        const char *cur_file = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] :
                               (state->frame_count > 0 ? state->frames[state->frame_count - 1].file_name : "main");
        state->frames[frame_idx].file_name = cur_file;
        state->frames[frame_idx].call_line = saved_lineno;
        state->frames[frame_idx].def_line = fn->start_lineno;
        state->frame_count++;
        zbash_sync_call_stack_arrays(state);
    }

    char *saved_debug_trap = state->debug_trap ? strdup(state->debug_trap) : NULL;
    char *saved_return_trap = state->return_trap ? strdup(state->return_trap) : NULL;

    bool inherit_trap = (fn->is_trace || state->functrace);
    if (!inherit_trap) {
        if (state->debug_trap) { free(state->debug_trap); state->debug_trap = NULL; }
        if (state->return_trap) { free(state->return_trap); state->return_trap = NULL; }
    } else {
        if (state->debug_trap) {
            free(state->debug_trap);
            state->debug_trap = saved_debug_trap ? strdup(saved_debug_trap) : NULL;
        }
        if (state->return_trap) {
            free(state->return_trap);
            state->return_trap = saved_return_trap ? strdup(saved_return_trap) : NULL;
        }
    }

    if (inherit_trap && state->debug_trap && !state->in_debug_trap) {
        zbash_trigger_debug_trap(state, fn->start_lineno);
    }

    FILE *mem_fp = fmemopen(fn->body, strlen(fn->body), "r");
    int rc = 0;
    if (mem_fp) {
        rc = zbash_run_stream(state, mem_fp);
        fclose(mem_fp);
    }

    if (state->return_trap && !state->in_return_trap) {
        if (state->functrace) zbash_trigger_debug_trap(state, fn->start_lineno);
        state->in_return_trap = true;
        int prev_l = state->lineno;
        state->lineno = fn->start_lineno;
        bool saved_suppress = state->suppress_debug_trap;
        state->suppress_debug_trap = true;
        zbash_run_line(state, state->return_trap);
        state->suppress_debug_trap = saved_suppress;
        state->lineno = prev_l;
        state->in_return_trap = false;
    }

    if (state->debug_trap) free(state->debug_trap);
    state->debug_trap = saved_debug_trap;
    if (state->return_trap) free(state->return_trap);
    state->return_trap = saved_return_trap;

    if (state->frame_count > 1) {
        state->frame_count--;
        zbash_sync_call_stack_arrays(state);
    }

    state->lineno = saved_lineno;
    state->func_depth--;
    for (int a = 0; a < argc; a++) free(orig_copies[a]);
    free(orig_copies);
    free(func_argv);

    state->argc = saved_argc;
    state->argv = saved_argv;
    int ret = state->return_called ? state->return_code : rc;
    state->return_called = false;

    /* Restore local variables declared during this function invocation */
    while (state->local_var_count > saved_local_mark) {
        state->local_var_count--;
        if (state->local_vars[state->local_var_count].was_set) {
            setenv(state->local_vars[state->local_var_count].name,
                   state->local_vars[state->local_var_count].old_val ? state->local_vars[state->local_var_count].old_val : "",
                   1);
        } else {
            unsetenv(state->local_vars[state->local_var_count].name);
        }
        free(state->local_vars[state->local_var_count].name);
        if (state->local_vars[state->local_var_count].old_val) {
            free(state->local_vars[state->local_var_count].old_val);
        }
    }

    return ret;
}

bool check_func_def(zbash_state_t *state, const char *p) {
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) return false;

    char func_name[128] = {0};
    const char *brace = strchr(p, '{');

    if (strncmp(p, "function", 8) == 0 && (isspace((unsigned char)p[8]) || p[8] == '(')) {
        const char *fn_start = p + 8;
        while (*fn_start && isspace((unsigned char)*fn_start)) fn_start++;
        const char *fn_end = fn_start;
        while (*fn_end && !isspace((unsigned char)*fn_end) && *fn_end != '(' && *fn_end != '{') fn_end++;
        size_t nlen = fn_end - fn_start;
        if (state && state->posix && !is_valid_ident(fn_start, nlen)) {
            char bad_name[128] = {0};
            if (nlen < sizeof(bad_name)) {
                strncpy(bad_name, fn_start, nlen);
                bad_name[nlen] = '\0';
            }
            const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
            int lineno = state->lineno > 0 ? state->lineno : 1;
            fprintf(stderr, "%s: line %d: `%s': not a valid identifier\n", src, lineno, bad_name);
            state->should_exit = true;
            state->exit_code = 2;
            state->last_status = 2;
            return true;
        }
        if (is_valid_func_name(fn_start, nlen)) {
            strncpy(func_name, fn_start, nlen);
            func_name[nlen] = '\0';
        }
    } else {
        const char *open_p = strchr(p, '(');
        const char *close_p = open_p ? strchr(open_p + 1, ')') : NULL;
        if (open_p && close_p && (!brace || close_p < brace)) {
            bool paren_empty = true;
            for (const char *t = open_p + 1; t < close_p; t++) {
                if (!isspace((unsigned char)*t)) { paren_empty = false; break; }
            }
            if (paren_empty) {
                size_t nlen = open_p - p;
                while (nlen > 0 && isspace((unsigned char)p[nlen - 1])) nlen--;
                if (is_valid_func_name(p, nlen)) {
                    strncpy(func_name, p, nlen);
                    func_name[nlen] = '\0';
                }
            }
        }
    }

    if (!func_name[0] || !brace) return false;

    const char *b_start = brace + 1;
    if (*b_start == '\r') b_start++;
    if (*b_start == '\n') b_start++;
    int b_depth = 1;
    bool in_s = false, in_d = false;
    const char *b_ptr = b_start;
    const char *b_end = NULL;

    while (*b_ptr && b_depth > 0) {
        char c = *b_ptr;
        if (c == '\\' && *(b_ptr + 1)) {
            b_ptr += 2;
            continue;
        }
        if (!in_s && !in_d && c == '#' && (b_ptr == b_start || isspace((unsigned char)*(b_ptr - 1)) || *(b_ptr - 1) == ';')) {
            while (*b_ptr && *b_ptr != '\n') b_ptr++;
            if (!*b_ptr) break;
            b_ptr++;
            continue;
        }
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;
        else if (!in_s && !in_d) {
            if (c == '{') b_depth++;
            else if (c == '}') {
                b_depth--;
                if (b_depth == 0) { b_end = b_ptr; break; }
            }
        }
        b_ptr++;
    }

    if (!b_end) return false;

    const char *after = b_end + 1;
    while (*after && isspace((unsigned char)*after)) after++;
    if (*after == ';') {
        after++;
        while (*after && isspace((unsigned char)*after)) after++;
    }
    if (*after && *after != '#') {
        /* Line contains further commands or tokens; do not consume entire line here */
        return false;
    }

    size_t blen = b_end - b_start;
    char *body = malloc(blen + 1);
    strncpy(body, b_start, blen);
    body[blen] = '\0';

    int nl_before_brace = 0;
    for (const char *tc = p; tc < brace; tc++) {
        if (*tc == '\n') nl_before_brace++;
    }
    int base_line = (state && state->stmt_start_lineno > 0) ? state->stmt_start_lineno : (state ? state->lineno : 1);
    int start_lineno = base_line + nl_before_brace;

    zbash_register_func(state, func_name, body, start_lineno);
    free(body);
    return true;
}

/* ========================================================================= */
/* Compound Control Structures                                               */
/* ========================================================================= */

static void extract_for_items(zbash_state_t *state, const char *items_str, char ***out_words, int *out_count) {
    char **words = malloc(256 * sizeof(char *));
    int count = 0;
    const char *p = items_str;

    while (*p && count < 256) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p || *p == '#') break;

        char tok[2048] = {0};
        size_t tlen = 0;
        bool had_quotes = false;
        bool in_single = false, in_double = false;

        while (*p && (in_single || in_double || !isspace((unsigned char)*p)) && tlen < sizeof(tok) - 1) {
            char c = *p;
            if (c == '\'' && !in_double) {
                had_quotes = true;
                in_single = !in_single;
                tok[tlen++] = *p++;
            } else if (c == '"' && !in_single) {
                had_quotes = true;
                in_double = !in_double;
                tok[tlen++] = *p++;
            } else if (c == '\\' && in_double && p[1] != '\0') {
                tok[tlen++] = *p++;
                tok[tlen++] = *p++;
            } else if (c == '\\' && !in_single && !in_double && p[1] != '\0') {
                had_quotes = true;
                tok[tlen++] = *p++;
                tok[tlen++] = *p++;
            } else if (c == '$' && p[1] == '(' && !in_single) {
                tok[tlen++] = *p++;
                tok[tlen++] = *p++;
                int pd = 1;
                while (*p && pd > 0 && tlen < sizeof(tok) - 1) {
                    if (*p == '(') pd++;
                    else if (*p == ')') pd--;
                    tok[tlen++] = *p++;
                }
            } else {
                tok[tlen++] = *p++;
            }
        }
        tok[tlen] = '\0';

        if (strcmp(tok, "\"$@\"") == 0 || strcmp(tok, "\"${@}\"") == 0 ||
            strncmp(tok, "\"${@-", 5) == 0 || strncmp(tok, "\"${@:-", 6) == 0) {
            if (state->argc > 1) {
                for (int a = 1; a < state->argc && count < 256; a++) {
                    words[count++] = strdup(state->argv[a]);
                }
            } else if (strncmp(tok, "\"${@-", 5) == 0 || strncmp(tok, "\"${@:-", 6) == 0) {
                words[count++] = strdup("");
            }
            continue;
        }

        char *exp = zbash_expand_vars(state, tok);
        if (had_quotes) {
            words[count++] = exp ? exp : strdup(tok);
        } else {
            char *src = exp ? exp : tok;
            char **bwords = zbash_expand_braces(src);
            if (bwords) {
                for (size_t b_idx = 0; bwords[b_idx] && count < 256; b_idx++) {
                    char *w_save = NULL;
                    char *w = strtok_r(bwords[b_idx], " \t\r\n", &w_save);
                    while (w && count < 256) {
                        words[count++] = strdup(w);
                        w = strtok_r(NULL, " \t\r\n", &w_save);
                    }
                    free(bwords[b_idx]);
                }
                free(bwords);
            } else {
                char *w_save = NULL;
                char *w = strtok_r(src, " \t\r\n", &w_save);
                while (w && count < 256) {
                    words[count++] = strdup(w);
                    w = strtok_r(NULL, " \t\r\n", &w_save);
                }
            }
            if (exp) free(exp);
        }
    }

    *out_words = words;
    *out_count = count;
}

static void run_loop_body(zbash_state_t *state, const char *body_buf, int first_stmt_line) {
    if (!body_buf) return;
    while (*body_buf == '\r' || *body_buf == '\n') {
        body_buf++;
        first_stmt_line++;
    }
    if (!*body_buf) return;
    int prev_l = state->lineno;
    state->lineno = first_stmt_line - 1;
    FILE *bfp = fmemopen((char *)body_buf, strlen(body_buf), "r");
    if (bfp) {
        zbash_run_stream(state, bfp);
        fclose(bfp);
    } else {
        state->lineno = first_stmt_line;
        zbash_run_line(state, body_buf);
    }
    state->lineno = prev_l;
}

int exec_if(zbash_state_t *state, const char *cmd) {
    if (!cmd) return 0;
    while (*cmd && isspace((unsigned char)*cmd)) cmd++;
    const char *orig_cmd = cmd;
    int if_start_line = state->lineno;
    if (strncmp(cmd, "if ", 3) == 0) cmd += 3;
    else if (strncmp(cmd, "if[", 3) == 0) cmd += 2;
    else return 0;

    typedef struct {
        char *cond;
        char *body;
        int body_start_line;
    } if_branch_t;

    if_branch_t branches[32];
    int branch_count = 0;
    memset(branches, 0, sizeof(branches));

    char cond_buf[8192] = {0};
    size_t cond_len = 0;
    char body_buf[16384] = {0};
    size_t body_len = 0;
    bool in_cond = true;

    bool in_s = false, in_d = false;
    int inner_if_d = 0;
    const char *p = cmd;

    while (*p) {
        char c = *p;
        if (c == '\\' && p[1]) {
            if (in_cond) {
                if (cond_len + 2 < sizeof(cond_buf)) { cond_buf[cond_len++] = *p++; cond_buf[cond_len++] = *p++; }
            } else {
                if (body_len + 2 < sizeof(body_buf)) { body_buf[body_len++] = *p++; body_buf[body_len++] = *p++; }
            }
            continue;
        }

        if (!in_s && !in_d && c == '#' && (p == cmd || isspace((unsigned char)p[-1]) || p[-1] == ';')) {
            while (*p && *p != '\n') {
                if (in_cond) {
                    if (cond_len + 1 < sizeof(cond_buf)) cond_buf[cond_len++] = *p;
                } else {
                    if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
                }
                p++;
            }
            if (*p == '\n') {
                if (in_cond) {
                    if (cond_len + 1 < sizeof(cond_buf)) cond_buf[cond_len++] = *p;
                } else {
                    if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
                }
                p++;
            }
            continue;
        }

        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;

        if (!in_s && !in_d) {
            bool at_word_start = (p == cmd || isspace((unsigned char)p[-1]) || p[-1] == ';');
            if (at_word_start) {
                if (is_word_match(p, "if", 2)) {
                    inner_if_d++;
                } else if (is_word_match(p, "fi", 2)) {
                    if (inner_if_d > 0) {
                        inner_if_d--;
                    } else {
                        if (!in_cond && branch_count < 32) {
                            while (body_len > 0 && (isspace((unsigned char)body_buf[body_len - 1]) || body_buf[body_len - 1] == ';')) {
                                body_buf[--body_len] = '\0';
                            }
                            branches[branch_count].body = strdup(body_buf);
                            branch_count++;
                        }
                        p += 2;
                        break;
                    }
                } else if (inner_if_d == 0) {
                    if (in_cond && is_word_match(p, "then", 4)) {
                        while (cond_len > 0 && (isspace((unsigned char)cond_buf[cond_len - 1]) || cond_buf[cond_len - 1] == ';')) {
                            cond_buf[--cond_len] = '\0';
                        }
                        if (branch_count < 32) {
                            branches[branch_count].cond = strdup(cond_buf);
                        }
                        cond_buf[0] = '\0';
                        cond_len = 0;
                        body_buf[0] = '\0';
                        body_len = 0;
                        in_cond = false;
                        p += 4;
                        while (*p && (*p == ' ' || *p == '\t')) p++;
                        if (*p == '\r') p++;
                        if (*p == '\n') p++;
                        int nl_before_branch = 0;
                        for (const char *tc = orig_cmd; tc < p; tc++) { if (*tc == '\n') nl_before_branch++; }
                        if (branch_count < 32) branches[branch_count].body_start_line = if_start_line + nl_before_branch;
                        continue;
                    } else if (!in_cond && is_word_match(p, "elif", 4)) {
                        while (body_len > 0 && (isspace((unsigned char)body_buf[body_len - 1]) || body_buf[body_len - 1] == ';')) {
                            body_buf[--body_len] = '\0';
                        }
                        if (branch_count < 32) {
                            branches[branch_count].body = strdup(body_buf);
                            branch_count++;
                        }
                        body_buf[0] = '\0';
                        body_len = 0;
                        cond_buf[0] = '\0';
                        cond_len = 0;
                        in_cond = true;
                        p += 4;
                        continue;
                    } else if (!in_cond && is_word_match(p, "else", 4)) {
                        while (body_len > 0 && (isspace((unsigned char)body_buf[body_len - 1]) || body_buf[body_len - 1] == ';')) {
                            body_buf[--body_len] = '\0';
                        }
                        if (branch_count < 32) {
                            branches[branch_count].body = strdup(body_buf);
                            branch_count++;
                        }
                        body_buf[0] = '\0';
                        body_len = 0;
                        if (branch_count < 32) {
                            branches[branch_count].cond = NULL;
                        }
                        in_cond = false;
                        p += 4;
                        while (*p && (*p == ' ' || *p == '\t')) p++;
                        if (*p == '\r') p++;
                        if (*p == '\n') p++;
                        int nl_before_branch = 0;
                        for (const char *tc = orig_cmd; tc < p; tc++) { if (*tc == '\n') nl_before_branch++; }
                        if (branch_count < 32) branches[branch_count].body_start_line = if_start_line + nl_before_branch;
                        continue;
                    }
                }
            }
        }

        if (in_cond) {
            if (cond_len + 1 < sizeof(cond_buf)) cond_buf[cond_len++] = *p;
            cond_buf[cond_len] = '\0';
        } else {
            if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
            body_buf[body_len] = '\0';
        }
        p++;
    }

    zbash_cmd_t redir_cmd;
    memset(&redir_cmd, 0, sizeof(redir_cmd));
    const char *rp = p;
    while (*rp && isspace((unsigned char)*rp)) rp++;
    bool has_redir = false;
    int saved_fds[ZBASH_MAX_REDIRS][2];
    int saved_count = 0;
    if (*rp) {
        zbash_parse_simple_cmd(state, rp, &redir_cmd);
        if (redir_cmd.redir_count > 0 || redir_cmd.in_file || redir_cmd.out_file || redir_cmd.err_file || redir_cmd.err_to_out || redir_cmd.out_to_err) {
            has_redir = true;
            if (zbash_apply_redirections_saved(state, &redir_cmd, saved_fds, &saved_count) != 0) {
                zbash_restore_redirections(state, saved_fds, saved_count);
                free_cmd_strings(&redir_cmd);
                for (int b = 0; b < branch_count; b++) {
                    if (branches[b].cond) free(branches[b].cond);
                    if (branches[b].body) free(branches[b].body);
                }
                return 1;
            }
        }
    }

    bool executed = false;
    for (int b = 0; b < branch_count; b++) {
        if (branches[b].cond == NULL) {
            executed = true;
            if (branches[b].body && branches[b].body[0]) {
                run_loop_body(state, branches[b].body, branches[b].body_start_line);
            } else {
                state->last_status = 0;
            }
            break;
        } else {
            bool saved_errexit = state->errexit;
            state->errexit = false;
            int cond_rc = zbash_run_line(state, branches[b].cond);
            state->errexit = saved_errexit;
            state->should_exit = false;
            if (cond_rc == 0) {
                executed = true;
                if (branches[b].body && branches[b].body[0]) {
                    run_loop_body(state, branches[b].body, branches[b].body_start_line);
                } else {
                    state->last_status = 0;
                }
                break;
            }
        }
    }

    if (!executed) {
        state->last_status = 0;
    }

    if (has_redir) {
        fflush(stdout);
        fflush(stderr);
        zbash_restore_redirections(state, saved_fds, saved_count);
        free_cmd_strings(&redir_cmd);
    }

    for (int b = 0; b < branch_count; b++) {
        if (branches[b].cond) free(branches[b].cond);
        if (branches[b].body) free(branches[b].body);
    }

    return state->last_status;
}

int exec_while(zbash_state_t *state, const char *cmd) {
    if (!cmd) return 0;
    while (*cmd && isspace((unsigned char)*cmd)) cmd++;
    const char *orig_cmd = cmd;
    int while_start_line = state->lineno;
    bool is_until = (strncmp(cmd, "until", 5) == 0);
    cmd += 5;

    char cond_buf[4096] = {0};
    size_t cond_len = 0;
    char body_buf[16384] = {0};
    size_t body_len = 0;
    bool in_cond = true;

    bool in_s = false, in_d = false;
    int inner_loop_d = 0;
    bool cmd_start = true;
    const char *p = cmd;
    const char *body_code_start = NULL;

    while (*p) {
        char c = *p;
        if (c == '\\' && p[1]) {
            if (in_cond) {
                if (cond_len + 2 < sizeof(cond_buf)) { cond_buf[cond_len++] = *p++; cond_buf[cond_len++] = *p++; }
            } else {
                if (body_len + 2 < sizeof(body_buf)) { body_buf[body_len++] = *p++; body_buf[body_len++] = *p++; }
            }
            continue;
        }

        if (!in_s && !in_d && c == '#' && (p == cmd || isspace((unsigned char)p[-1]) || p[-1] == ';')) {
            while (*p && *p != '\n') {
                if (in_cond) {
                    if (cond_len + 1 < sizeof(cond_buf)) cond_buf[cond_len++] = *p;
                } else {
                    if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
                }
                p++;
            }
            if (*p == '\n') {
                if (in_cond) {
                    if (cond_len + 1 < sizeof(cond_buf)) cond_buf[cond_len++] = *p;
                } else {
                    if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
                }
                p++;
            }
            continue;
        }

        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;

        if (!in_s && !in_d) {
            bool at_word_start = (p == cmd || isspace((unsigned char)p[-1]) || p[-1] == ';');
            if (at_word_start) {
                if (cmd_start && (is_word_match(p, "for", 3) || is_word_match(p, "while", 5) || is_word_match(p, "until", 5))) {
                    inner_loop_d++;
                } else if (is_word_match(p, "done", 4)) {
                    if (inner_loop_d > 0) {
                        inner_loop_d--;
                    } else {
                        while (body_len > 0 && (isspace((unsigned char)body_buf[body_len - 1]) || body_buf[body_len - 1] == ';')) {
                            body_buf[--body_len] = '\0';
                        }
                        p += 4;
                        break;
                    }
                } else if (inner_loop_d == 0 && in_cond && is_word_match(p, "do", 2)) {
                    while (cond_len > 0 && (isspace((unsigned char)cond_buf[cond_len - 1]) || cond_buf[cond_len - 1] == ';')) {
                        cond_buf[--cond_len] = '\0';
                    }
                    in_cond = false;
                    cmd_start = true;
                    p += 2;
                    while (*p && (*p == ' ' || *p == '\t')) p++;
                    if (*p == '\r') p++;
                    if (*p == '\n') p++;
                    body_code_start = p;
                    continue;
                } else if (is_word_match(p, "then", 4) || is_word_match(p, "else", 4) || is_word_match(p, "elif", 4)) {
                    cmd_start = true;
                }
            }
            if (*p == ';' || *p == '\n' || *p == '|' || *p == '&' || *p == '(' || *p == ')') {
                cmd_start = true;
            } else if (!isspace((unsigned char)*p)) {
                cmd_start = false;
            }
        }

        if (in_cond) {
            if (cond_len + 1 < sizeof(cond_buf)) cond_buf[cond_len++] = *p;
            cond_buf[cond_len] = '\0';
        } else {
            if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
            body_buf[body_len] = '\0';
        }
        p++;
    }

    zbash_cmd_t redir_cmd;
    memset(&redir_cmd, 0, sizeof(redir_cmd));
    const char *rp = p;
    while (*rp && isspace((unsigned char)*rp)) rp++;
    bool has_redir = false;
    int saved_fds[ZBASH_MAX_REDIRS][2];
    int saved_count = 0;
    if (*rp) {
        zbash_parse_simple_cmd(state, rp, &redir_cmd);
        if (redir_cmd.redir_count > 0 || redir_cmd.in_file || redir_cmd.out_file || redir_cmd.err_file || redir_cmd.err_to_out || redir_cmd.out_to_err) {
            has_redir = true;
            if (zbash_apply_redirections_saved(state, &redir_cmd, saved_fds, &saved_count) != 0) {
                zbash_restore_redirections(state, saved_fds, saved_count);
                free_cmd_strings(&redir_cmd);
                return 1;
            }
        }
    }

    int nl_before_body = 0;
    if (body_code_start) {
        for (const char *tc = orig_cmd; tc < body_code_start; tc++) { if (*tc == '\n') nl_before_body++; }
    }
    int body_start_line = while_start_line + nl_before_body;

    int iters = 0;
    int last_body_status = 0;
    state->loop_depth++;
    while (1) {
        bool saved_errexit = state->errexit;
        state->errexit = false;
        int cond_rc = zbash_run_line(state, cond_buf);
        state->errexit = saved_errexit;
        state->should_exit = false;
        if (is_until ? (cond_rc == 0) : (cond_rc != 0)) {
            state->last_status = last_body_status;
            break;
        }
        iters++;
        if (body_buf[0]) {
            run_loop_body(state, body_buf, body_start_line);
            last_body_status = state->last_status;
        }
        if (state->loop_continue) {
            if (state->loop_continue_count > 1) {
                state->loop_continue_count--;
                break;
            } else {
                state->loop_continue = false;
                state->loop_continue_count = 0;
                continue;
            }
        }
        if (state->loop_break) {
            state->loop_break_count--;
            if (state->loop_break_count <= 0) {
                state->loop_break = false;
                state->loop_break_count = 0;
            }
            break;
        }
        if (state->should_exit || state->return_called) {
            break;
        }
    }
    state->loop_depth--;

    state->last_status = (iters == 0) ? 0 : last_body_status;
    if (has_redir) {
        fflush(stdout);
        fflush(stderr);
        zbash_restore_redirections(state, saved_fds, saved_count);
        free_cmd_strings(&redir_cmd);
    }
    return state->last_status;
}

int exec_until(zbash_state_t *state, const char *cmd) {
    return exec_while(state, cmd);
}

static bool is_str_empty_or_ws(const char *s) {
    if (!s) return true;
    while (*s) {
        if (!isspace((unsigned char)*s)) return false;
        s++;
    }
    return true;
}

int exec_for_c(zbash_state_t *state, const char *cmd) {
    if (!cmd) return 0;
    while (*cmd && isspace((unsigned char)*cmd)) cmd++;
    const char *p = strstr(cmd, "((");
    if (!p) return 0;
    p += 2;

    int p_depth = 2;
    bool in_sq = false, in_dq = false;
    const char *scan = p;
    const char *end_paren = NULL;

    while (*scan && p_depth > 0) {
        char c = *scan;
        if (c == '\\' && *(scan + 1)) {
            scan += 2;
            continue;
        }
        if (c == '\'' && !in_dq) in_sq = !in_sq;
        else if (c == '"' && !in_sq) in_dq = !in_dq;
        else if (!in_sq && !in_dq) {
            if (c == '(') p_depth++;
            else if (c == ')') {
                p_depth--;
                if (p_depth == 0) {
                    end_paren = scan - 1;
                    break;
                }
            }
        }
        scan++;
    }

    if (!end_paren) return 0;

    size_t elen = end_paren - p;
    char expr_buf[2048] = {0};
    if (elen >= sizeof(expr_buf)) elen = sizeof(expr_buf) - 1;
    strncpy(expr_buf, p, elen);
    expr_buf[elen] = '\0';
    p = scan + 1;

    /* Semicolon validation outside quotes and nested parentheses */
    int semi_count = 0;
    int inner_pd = 0;
    in_sq = false; in_dq = false;
    char *s1 = NULL;
    char *s2 = NULL;
    for (size_t i = 0; expr_buf[i]; i++) {
        char c = expr_buf[i];
        if (c == '\\' && expr_buf[i + 1]) { i++; continue; }
        if (c == '\'' && !in_dq) in_sq = !in_sq;
        else if (c == '"' && !in_sq) in_dq = !in_dq;
        else if (!in_sq && !in_dq) {
            if (c == '(') inner_pd++;
            else if (c == ')') { if (inner_pd > 0) inner_pd--; }
            else if (inner_pd == 0 && c == ';') {
                semi_count++;
                if (semi_count == 1) s1 = &expr_buf[i];
                else if (semi_count == 2) s2 = &expr_buf[i];
            }
        }
    }

    if (semi_count < 2) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : (state->argv[0] ? state->argv[0] : "zbash");
        if (state->lineno > 0) {
            fprintf(stderr, "%s: line %d: syntax error: arithmetic expression required\n", src, state->lineno);
            fprintf(stderr, "%s: line %d: syntax error: `(( %s ))'\n", src, state->lineno, expr_buf);
        } else {
            fprintf(stderr, "%s: syntax error: arithmetic expression required\n", src);
            fprintf(stderr, "%s: syntax error: `(( %s ))'\n", src, expr_buf);
        }
        state->last_status = 2;
        return 2;
    }
    if (semi_count > 2) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : (state->argv[0] ? state->argv[0] : "zbash");
        if (state->lineno > 0) {
            fprintf(stderr, "%s: line %d: syntax error: `;' unexpected\n", src, state->lineno);
            fprintf(stderr, "%s: line %d: syntax error: `(( %s ))'\n", src, state->lineno, expr_buf);
        } else {
            fprintf(stderr, "%s: syntax error: `;' unexpected\n", src);
            fprintf(stderr, "%s: syntax error: `(( %s ))'\n", src, expr_buf);
        }
        state->last_status = 2;
        return 2;
    }

    *s1 = '\0';
    *s2 = '\0';
    char *init_str = expr_buf;
    char *cond_str = s1 + 1;
    char *step_str = s2 + 1;

    while (*p && (*p == ';' || isspace((unsigned char)*p))) p++;
    bool use_braces = false;
    if (*p == '{') {
        use_braces = true;
        p++;
    } else if (is_word_match(p, "do", 2)) {
        p += 2;
    }
    while (*p && (*p == ' ' || *p == '\t')) p++;
    if (*p == '\r') p++;
    if (*p == '\n') p++;
    const char *body_code_start = p;

    char body_buf[16384] = {0};
    size_t body_len = 0;
    int inner_loop_d = 0;
    int brace_d = use_braces ? 1 : 0;
    bool in_s = false, in_d = false;
    bool cmd_start = true;

    while (*p) {
        char c = *p;
        if (!in_s && !in_d && c == '#' && (p == cmd || isspace((unsigned char)p[-1]) || p[-1] == ';')) {
            while (*p && *p != '\n') {
                if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
                p++;
            }
            if (*p == '\n') {
                if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
                p++;
            }
            continue;
        }

        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;

        if (!in_s && !in_d) {
            if (use_braces) {
                if (c == '{') brace_d++;
                else if (c == '}') {
                    brace_d--;
                    if (brace_d == 0) {
                        while (body_len > 0 && (isspace((unsigned char)body_buf[body_len - 1]) || body_buf[body_len - 1] == ';')) {
                            body_buf[--body_len] = '\0';
                        }
                        p++;
                        break;
                    }
                }
            } else {
                bool at_word_start = (isspace((unsigned char)p[-1]) || p[-1] == ';');
                if (at_word_start) {
                    if (cmd_start && (is_word_match(p, "for", 3) || is_word_match(p, "while", 5) || is_word_match(p, "until", 5))) {
                        inner_loop_d++;
                    } else if (is_word_match(p, "done", 4)) {
                        if (inner_loop_d > 0) {
                            inner_loop_d--;
                        } else {
                            while (body_len > 0 && (isspace((unsigned char)body_buf[body_len - 1]) || body_buf[body_len - 1] == ';')) {
                                body_buf[--body_len] = '\0';
                            }
                            p += 4;
                            break;
                        }
                    } else if (is_word_match(p, "do", 2) || is_word_match(p, "then", 4) || is_word_match(p, "else", 4) || is_word_match(p, "elif", 4)) {
                        cmd_start = true;
                    }
                }
            }
            if (*p == ';' || *p == '\n' || *p == '|' || *p == '&' || *p == '(' || *p == ')') {
                cmd_start = true;
            } else if (!isspace((unsigned char)*p)) {
                cmd_start = false;
            }
        }
        if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
        p++;
    }
    body_buf[body_len] = '\0';

    zbash_cmd_t redir_cmd;
    memset(&redir_cmd, 0, sizeof(redir_cmd));
    const char *rp = p;
    while (*rp && isspace((unsigned char)*rp)) rp++;
    bool has_redir = false;
    int saved_fds[ZBASH_MAX_REDIRS][2];
    int saved_count = 0;
    if (*rp) {
        zbash_parse_simple_cmd(state, rp, &redir_cmd);
        if (redir_cmd.redir_count > 0 || redir_cmd.in_file || redir_cmd.out_file || redir_cmd.err_file || redir_cmd.err_to_out || redir_cmd.out_to_err) {
            has_redir = true;
            if (zbash_apply_redirections_saved(state, &redir_cmd, saved_fds, &saved_count) != 0) {
                zbash_restore_redirections(state, saved_fds, saved_count);
                free_cmd_strings(&redir_cmd);
                return 1;
            }
        }
    }

    int for_start_line = state->lineno;
    int nl_before_body = 0;
    if (body_code_start) {
        for (const char *tc = cmd; tc < body_code_start; tc++) { if (*tc == '\n') nl_before_body++; }
    }
    int body_start_line = for_start_line + nl_before_body;

    if (init_str && !is_str_empty_or_ws(init_str)) {
        zbash_trigger_debug_trap(state, for_start_line);
        zbash_eval_arithmetic(state, init_str);
    }

    int iters = 0;
    state->loop_depth++;
    while (1) {
        if (cond_str && !is_str_empty_or_ws(cond_str)) {
            zbash_trigger_debug_trap(state, for_start_line);
            long long cval = zbash_eval_arithmetic(state, cond_str);
            if (cval == 0) break;
        }
        iters++;
        if (body_buf[0]) {
            run_loop_body(state, body_buf, body_start_line);
        }
        if (state->loop_continue) {
            if (state->loop_continue_count > 1) {
                state->loop_continue_count--;
                break;
            } else {
                state->loop_continue = false;
                state->loop_continue_count = 0;
            }
        } else if (state->loop_break) {
            state->loop_break_count--;
            if (state->loop_break_count <= 0) {
                state->loop_break = false;
                state->loop_break_count = 0;
            }
            break;
        }
        if (state->should_exit || state->return_called) break;

        if (step_str && !is_str_empty_or_ws(step_str)) {
            zbash_trigger_debug_trap(state, for_start_line);
            zbash_eval_arithmetic(state, step_str);
        }
    }
    state->loop_depth--;

    if (iters == 0) state->last_status = 0;
    if (has_redir) {
        fflush(stdout);
        fflush(stderr);
        zbash_restore_redirections(state, saved_fds, saved_count);
        free_cmd_strings(&redir_cmd);
    }
    return state->last_status;
}

int exec_for_in(zbash_state_t *state, const char *cmd) {
    if (!cmd) return 0;
    while (*cmd && isspace((unsigned char)*cmd)) cmd++;
    if (strncmp(cmd, "for ", 4) != 0) return 0;
    const char *p = cmd + 4;
    while (*p && isspace((unsigned char)*p)) p++;

    const char *tok_start = p;
    while (*p && !isspace((unsigned char)*p) && *p != ';' && *p != '\n') p++;
    size_t tok_len = p - tok_start;

    if (!is_valid_ident(tok_start, tok_len)) {
        char bad_id[128] = {0};
        if (tok_len < sizeof(bad_id)) {
            strncpy(bad_id, tok_start, tok_len);
            bad_id[tok_len] = '\0';
        }
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
        int lineno = state->lineno > 0 ? state->lineno : 1;
        fprintf(stderr, "%s: line %d: `%s': not a valid identifier\n", src, lineno, bad_id);
        state->last_status = 1;
        return 1;
    }

    char var_name[128] = {0};
    if (tok_len < sizeof(var_name)) {
        strncpy(var_name, tok_start, tok_len);
        var_name[tok_len] = '\0';
    }

    if (zbash_is_readonly(state, var_name)) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
        int lineno = state->lineno > 0 ? state->lineno : 1;
        fprintf(stderr, "%s: line %d: %s: readonly variable\n", src, lineno, var_name);
        state->last_status = 1;
        return 1;
    }

    while (*p && isspace((unsigned char)*p)) p++;

    bool has_in = false;
    char items_str[4096] = {0};
    size_t items_len = 0;

    if (is_word_match(p, "in", 2)) {
        has_in = true;
        p += 2;
        while (*p && isspace((unsigned char)*p)) p++;
    }

    bool in_s = false, in_d = false;
    const char *body_code_start = NULL;
    while (*p) {
        char c = *p;
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;

        if (!in_s && !in_d) {
            bool at_word_start = (isspace((unsigned char)p[-1]) || p[-1] == ';');
            if (at_word_start && is_word_match(p, "do", 2)) {
                p += 2;
                while (*p && (*p == ' ' || *p == '\t')) p++;
                if (*p == '\r') p++;
                if (*p == '\n') p++;
                body_code_start = p;
                break;
            }
        }
        if (has_in) {
            if (items_len + 1 < sizeof(items_str)) items_str[items_len++] = *p;
        }
        p++;
    }
    while (items_len > 0 && (isspace((unsigned char)items_str[items_len - 1]) || items_str[items_len - 1] == ';')) {
        items_str[--items_len] = '\0';
    }

    char body_buf[16384] = {0};
    size_t body_len = 0;
    int inner_loop_d = 0;
    in_s = false; in_d = false;
    bool cmd_start = true;

    while (*p) {
        char c = *p;
        if (!in_s && !in_d && c == '#' && (p == cmd || isspace((unsigned char)p[-1]) || p[-1] == ';')) {
            while (*p && *p != '\n') {
                if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
                p++;
            }
            if (*p == '\n') {
                if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
                p++;
            }
            continue;
        }

        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;

        if (!in_s && !in_d) {
            bool at_word_start = (isspace((unsigned char)p[-1]) || p[-1] == ';');
            if (at_word_start) {
                if (cmd_start && (is_word_match(p, "for", 3) || is_word_match(p, "while", 5) || is_word_match(p, "until", 5))) {
                    inner_loop_d++;
                } else if (is_word_match(p, "done", 4)) {
                    if (inner_loop_d > 0) {
                        inner_loop_d--;
                    } else {
                        while (body_len > 0 && (isspace((unsigned char)body_buf[body_len - 1]) || body_buf[body_len - 1] == ';')) {
                            body_buf[--body_len] = '\0';
                        }
                        p += 4;
                        break;
                    }
                } else if (is_word_match(p, "do", 2)) {
                    cmd_start = true;
                    if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p++;
                    if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p++;
                    continue;
                } else if (is_word_match(p, "then", 4) || is_word_match(p, "else", 4) || is_word_match(p, "elif", 4)) {
                    cmd_start = true;
                    for (int k = 0; k < 4 && *p; k++) {
                        if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p++;
                    }
                    continue;
                }
            }
            if (*p == ';' || *p == '\n' || *p == '|' || *p == '&' || *p == '(' || *p == ')') {
                cmd_start = true;
            } else if (!isspace((unsigned char)*p)) {
                cmd_start = false;
            }
        }
        if (body_len + 1 < sizeof(body_buf)) body_buf[body_len++] = *p;
        p++;
    }
    body_buf[body_len] = '\0';

    zbash_cmd_t redir_cmd;
    memset(&redir_cmd, 0, sizeof(redir_cmd));
    const char *rp = p;
    while (*rp && isspace((unsigned char)*rp)) rp++;
    bool has_redir = false;
    int saved_fds[ZBASH_MAX_REDIRS][2];
    int saved_count = 0;
    if (*rp) {
        zbash_parse_simple_cmd(state, rp, &redir_cmd);
        if (redir_cmd.redir_count > 0 || redir_cmd.in_file || redir_cmd.out_file || redir_cmd.err_file || redir_cmd.err_to_out || redir_cmd.out_to_err) {
            has_redir = true;
            if (zbash_apply_redirections_saved(state, &redir_cmd, saved_fds, &saved_count) != 0) {
                zbash_restore_redirections(state, saved_fds, saved_count);
                free_cmd_strings(&redir_cmd);
                return 1;
            }
        }
    }

    char **words = NULL;
    int word_count = 0;

    int nl_before_body = 0;
    if (body_code_start) {
        for (const char *tc = cmd; tc < body_code_start; tc++) { if (*tc == '\n') nl_before_body++; }
    }
    int for_start_line = state->lineno;
    int body_start_line = for_start_line + nl_before_body;
    state->loop_depth++;

    if (!has_in) {
        int iters = 0;
        for (int a = 1; a < state->argc; a++) {
            iters++;
            setenv(var_name, state->argv[a], 1);
            if (body_buf[0]) run_loop_body(state, body_buf, body_start_line);
            if (state->loop_continue) {
                if (state->loop_continue_count > 1) { state->loop_continue_count--; break; }
                else { state->loop_continue = false; state->loop_continue_count = 0; continue; }
            }
            if (state->loop_break) {
                state->loop_break_count--;
                if (state->loop_break_count <= 0) { state->loop_break = false; state->loop_break_count = 0; }
                break;
            }
            if (state->should_exit || state->return_called) break;
        }
        if (iters == 0) state->last_status = 0;
        goto done_for;
    }

    const char *it_p = items_str;
    while (*it_p && isspace((unsigned char)*it_p)) it_p++;
    if (!*it_p) {
        state->last_status = 0;
        goto done_for;
    }

    if (strcmp(it_p, "\"$@\"") == 0 || strcmp(it_p, "$@") == 0 ||
        strcmp(it_p, "\"$*\"") == 0 || strcmp(it_p, "$*") == 0) {
        int iters = 0;
        for (int a = 1; a < state->argc; a++) {
            iters++;
            setenv(var_name, state->argv[a], 1);
            if (body_buf[0]) run_loop_body(state, body_buf, body_start_line);
            if (state->loop_continue) {
                if (state->loop_continue_count > 1) { state->loop_continue_count--; break; }
                else { state->loop_continue = false; state->loop_continue_count = 0; continue; }
            }
            if (state->loop_break) {
                state->loop_break_count--;
                if (state->loop_break_count <= 0) { state->loop_break = false; state->loop_break_count = 0; }
                break;
            }
            if (state->should_exit || state->return_called) break;
        }
        if (iters == 0) state->last_status = 0;
        goto done_for;
    }

    int start_val = 0, end_val = 0, step_val = 0;
    if (sscanf(items_str, "{%d..%d..%d}", &start_val, &end_val, &step_val) == 3 ||
        sscanf(items_str, "{%d..%d}", &start_val, &end_val) == 2) {
        if (step_val == 0) step_val = (start_val <= end_val) ? 1 : -1;
        else if (start_val > end_val && step_val > 0) step_val = -step_val;

        int iters = 0;
        for (int v = start_val; (step_val > 0 ? v <= end_val : v >= end_val); v += step_val) {
            iters++;
            char num_str[32];
            snprintf(num_str, sizeof(num_str), "%d", v);
            setenv(var_name, num_str, 1);
            if (body_buf[0]) run_loop_body(state, body_buf, body_start_line);
            if (state->loop_continue) {
                if (state->loop_continue_count > 1) { state->loop_continue_count--; break; }
                else { state->loop_continue = false; state->loop_continue_count = 0; continue; }
            }
            if (state->loop_break) {
                state->loop_break_count--;
                if (state->loop_break_count <= 0) { state->loop_break = false; state->loop_break_count = 0; }
                break;
            }
            if (state->should_exit || state->return_called) break;
        }
        if (iters == 0) state->last_status = 0;
        goto done_for;
    }

    if (strstr(items_str, "[@") != NULL || strstr(items_str, "[*") != NULL) {
        char aname[128] = {0};
        bool is_indices = false;
        int slice_offset = 0;
        int slice_length = -1;
        const char *b1 = strstr(items_str, "${");
        if (!b1) b1 = strstr(items_str, "$");
        if (b1) {
            b1 = (b1[1] == '{') ? b1 + 2 : b1 + 1;
            if (*b1 == '!') {
                is_indices = true;
                b1++;
            }
            const char *b2 = strstr(b1, "[@");
            if (!b2) b2 = strstr(b1, "[*");
            if (b2 && b2 - b1 < (int)sizeof(aname)) {
                strncpy(aname, b1, b2 - b1);
                aname[b2 - b1] = '\0';
                const char *close_br = strchr(b2, ']');
                if (close_br && close_br[1] == ':') {
                    const char *slice_p = close_br + 2;
                    slice_offset = atoi(slice_p);
                    const char *len_p = strchr(slice_p, ':');
                    if (len_p) {
                        slice_length = atoi(len_p + 1);
                    }
                }
            }
        }
        zbash_array_t *arr = zbash_find_array(state, aname);
        if (arr) {
            int start_idx = (slice_offset >= 0 && slice_offset < arr->count) ? slice_offset : 0;
            int end_idx = (slice_length >= 0) ? (start_idx + slice_length) : arr->count;
            if (end_idx > arr->count) end_idx = arr->count;

            int iters = 0;
            for (int m = start_idx; m < end_idx; m++) {
                iters++;
                if (is_indices) {
                    char idx_buf[32];
                    snprintf(idx_buf, sizeof(idx_buf), "%d", m);
                    setenv(var_name, idx_buf, 1);
                } else {
                    setenv(var_name, arr->elements[m], 1);
                }
                if (body_buf[0]) run_loop_body(state, body_buf, body_start_line);
                if (state->loop_continue) {
                    if (state->loop_continue_count > 1) { state->loop_continue_count--; break; }
                    else { state->loop_continue = false; state->loop_continue_count = 0; continue; }
                }
                if (state->loop_break) {
                    state->loop_break_count--;
                    if (state->loop_break_count <= 0) { state->loop_break = false; state->loop_break_count = 0; }
                    break;
                }
                if (state->should_exit || state->return_called) break;
            }
            if (iters == 0) state->last_status = 0;
            goto done_for;
        }
    }

    extract_for_items(state, items_str, &words, &word_count);

    int iters = 0;
    for (int i = 0; i < word_count; i++) {
        iters++;
        zbash_trigger_debug_trap(state, for_start_line);
        setenv(var_name, words[i], 1);
        if (body_buf[0]) run_loop_body(state, body_buf, body_start_line);
        if (state->loop_continue) {
            if (state->loop_continue_count > 1) { state->loop_continue_count--; break; }
            else { state->loop_continue = false; state->loop_continue_count = 0; continue; }
        }
        if (state->loop_break) {
            state->loop_break_count--;
            if (state->loop_break_count <= 0) { state->loop_break = false; state->loop_break_count = 0; }
            break;
        }
        if (state->should_exit || state->return_called) break;
    }

    if (iters == 0) state->last_status = 0;

done_for:
    state->loop_depth--;
    if (words) {
        for (int i = 0; i < word_count; i++) free(words[i]);
        free(words);
    }
    if (has_redir) {
        fflush(stdout);
        fflush(stderr);
        zbash_restore_redirections(state, saved_fds, saved_count);
        free_cmd_strings(&redir_cmd);
    }
    return state->last_status;
}

int exec_case(zbash_state_t *state, const char *cmd) {
    if (!cmd) return 0;
    while (*cmd && isspace((unsigned char)*cmd)) cmd++;
    if (strncmp(cmd, "case", 4) != 0 || !isspace((unsigned char)cmd[4])) return 0;
    const char *orig_cmd = cmd;
    int case_start_line = state->lineno;
    const char *p = cmd + 4;
    while (*p && isspace((unsigned char)*p)) p++;

    /* Extract word before 'in' */
    char word_buf[1024] = {0};
    size_t wlen = 0;
    bool in_s = false, in_d = false;

    while (*p) {
        char c = *p;
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;

        if (!in_s && !in_d && isspace((unsigned char)c)) {
            const char *next = p;
            while (*next && isspace((unsigned char)*next)) next++;
            if (is_word_match(next, "in", 2)) {
                p = next + 2;
                break;
            }
        }
        if (wlen + 1 < sizeof(word_buf)) word_buf[wlen++] = *p;
        p++;
    }
    while (wlen > 0 && isspace((unsigned char)word_buf[wlen - 1])) {
        word_buf[--wlen] = '\0';
    }

    char *target_word = zbash_expand_vars(state, word_buf);
    if (!target_word) target_word = strdup(word_buf);
    size_t tw_len = strlen(target_word);
    if ((tw_len >= 2) && ((target_word[0] == '"' && target_word[tw_len - 1] == '"') ||
                         (target_word[0] == '\'' && target_word[tw_len - 1] == '\''))) {
        target_word[tw_len - 1] = '\0';
        memmove(target_word, target_word + 1, tw_len - 1);
    }

    bool matched = false;
    int inner_case = 0;

    zbash_cmd_t redir_cmd;
    memset(&redir_cmd, 0, sizeof(redir_cmd));
    bool has_redir = false;
    int saved_fds[ZBASH_MAX_REDIRS][2];
    int saved_count = 0;

    /* Check for redirections after the final esac */
    const char *last_esac = NULL;
    bool sc_in_s = false, sc_in_d = false;
    for (const char *sp = cmd; *sp; sp++) {
        if (*sp == '\'' && !sc_in_d) sc_in_s = !sc_in_s;
        else if (*sp == '"' && !sc_in_s) sc_in_d = !sc_in_d;
        else if (!sc_in_s && !sc_in_d) {
            if (is_word_match(sp, "esac", 4)) last_esac = sp + 4;
        }
    }
    if (last_esac) {
        const char *rp = last_esac;
        while (*rp && isspace((unsigned char)*rp)) rp++;
        if (*rp) {
            zbash_parse_simple_cmd(state, rp, &redir_cmd);
            if (redir_cmd.redir_count > 0 || redir_cmd.in_file || redir_cmd.out_file || redir_cmd.err_file || redir_cmd.err_to_out || redir_cmd.out_to_err) {
                has_redir = true;
                if (zbash_apply_redirections_saved(state, &redir_cmd, saved_fds, &saved_count) != 0) {
                    zbash_restore_redirections(state, saved_fds, saved_count);
                    free_cmd_strings(&redir_cmd);
                    free(target_word);
                    return 1;
                }
            }
        }
    }

    while (*p) {
        while (*p) {
            while (*p && (isspace((unsigned char)*p) || *p == ';')) p++;
            if (*p == '#') {
                while (*p && *p != '\n') p++;
                continue;
            }
            break;
        }
        if (!*p) break;

        /* Check for esac */
        if (is_word_match(p, "esac", 4)) {
            p += 4;
            break;
        }

        /* Skip optional leading '(' */
        if (*p == '(') {
            p++;
            while (*p && isspace((unsigned char)*p)) p++;
        }

        /* Read patterns until ')' */
        char pattern_buf[1024] = {0};
        size_t pat_len = 0;
        in_s = false; in_d = false;
        int p_depth = 0;

        while (*p) {
            char c = *p;
            if (c == '\\' && !in_s && p[1] != '\0') {
                if (pat_len + 1 < sizeof(pattern_buf)) pattern_buf[pat_len++] = *p++;
                if (pat_len + 1 < sizeof(pattern_buf)) pattern_buf[pat_len++] = *p++;
                continue;
            }
            if (c == '\'' && !in_d) {
                in_s = !in_s;
            } else if (c == '"' && !in_s) {
                in_d = !in_d;
            } else if (!in_s && !in_d) {
                if (c == '(') {
                    p_depth++;
                } else if (c == ')') {
                    if (p_depth > 0) {
                        p_depth--;
                    } else {
                        /* Closing parenthesis for the case clause pattern */
                        p++;
                        break;
                    }
                }
            }
            if (pat_len + 1 < sizeof(pattern_buf)) pattern_buf[pat_len++] = *p;
            p++;
        }
        while (pat_len > 0 && isspace((unsigned char)pattern_buf[pat_len - 1])) {
            pattern_buf[--pat_len] = '\0';
        }

        while (*p && (*p == ' ' || *p == '\t')) p++;
        if (*p == '\r') p++;
        if (*p == '\n') p++;
        const char *clause_code_start = p;
        int nl_before_clause = 0;
        for (const char *tc = orig_cmd; tc < clause_code_start; tc++) { if (*tc == '\n') nl_before_clause++; }
        int clause_start_line = case_start_line + nl_before_clause;

        /* Read clause body until ';;' or 'esac' */
        char body_buf[16384] = {0};
        size_t blen = 0;
        in_s = false; in_d = false;
        inner_case = 0;

        while (*p) {
            char c = *p;
            if (!in_s && !in_d && c == '#' && (p == cmd || isspace((unsigned char)p[-1]) || p[-1] == ';')) {
                while (*p && *p != '\n') {
                    if (blen + 1 < sizeof(body_buf)) body_buf[blen++] = *p;
                    p++;
                }
                if (*p == '\n') {
                    if (blen + 1 < sizeof(body_buf)) body_buf[blen++] = *p;
                    p++;
                }
                continue;
            }

            if (c == '\\' && !in_s && p[1] != '\0') {
                if (blen + 1 < sizeof(body_buf)) body_buf[blen++] = *p++;
                if (blen + 1 < sizeof(body_buf)) body_buf[blen++] = *p++;
                continue;
            }

            if (c == '\'' && !in_d) in_s = !in_s;
            else if (c == '"' && !in_s) in_d = !in_d;

            if (!in_s && !in_d) {
                if (is_word_match(p, "case", 4)) {
                    inner_case++;
                } else if (is_word_match(p, "esac", 4)) {
                    if (inner_case > 0) {
                        inner_case--;
                    } else {
                        break;
                    }
                } else if (inner_case == 0 && p[0] == ';' && p[1] == ';') {
                    p += 2;
                    break;
                }
            }
            if (blen + 1 < sizeof(body_buf)) body_buf[blen++] = *p;
            p++;
        }
        while (blen > 0 && isspace((unsigned char)body_buf[blen - 1])) {
            body_buf[--blen] = '\0';
        }

        if (!matched) {
            bool this_clause_match = false;
            const char *scan = pattern_buf;

            while (*scan) {
                while (*scan && isspace((unsigned char)*scan)) scan++;
                if (!*scan) break;

                const char *subpat_start = scan;
                int alt_p_depth = 0;
                bool alt_in_s = false;
                bool alt_in_d = false;

                while (*scan) {
                    char sc = *scan;
                    if (sc == '\\' && !alt_in_s && scan[1] != '\0') {
                        scan += 2;
                        continue;
                    }
                    if (sc == '\'' && !alt_in_d) {
                        alt_in_s = !alt_in_s;
                    } else if (sc == '"' && !alt_in_s) {
                        alt_in_d = !alt_in_d;
                    } else if (!alt_in_s && !alt_in_d) {
                        if (sc == '(') {
                            alt_p_depth++;
                        } else if (sc == ')' && alt_p_depth > 0) {
                            alt_p_depth--;
                        } else if (sc == '|' && alt_p_depth == 0) {
                            break;
                        }
                    }
                    scan++;
                }

                size_t sub_len = (size_t)(scan - subpat_start);
                char subpat[1024];
                if (sub_len >= sizeof(subpat)) sub_len = sizeof(subpat) - 1;
                strncpy(subpat, subpat_start, sub_len);
                subpat[sub_len] = '\0';

                if (*scan == '|') scan++;

                /* Trim trailing spaces from subpat */
                char *end_sp = subpat + strlen(subpat);
                while (end_sp > subpat && isspace((unsigned char)*(end_sp - 1))) {
                    *(--end_sp) = '\0';
                }

                char *exp_pat = NULL;
                if (strpbrk(subpat, "$`~") != NULL) {
                    exp_pat = zbash_expand_vars(state, subpat);
                }
                const char *actual_pat = exp_pat ? exp_pat : subpat;
                size_t aplen = strlen(actual_pat);
                char clean_pat[1024];
                if ((aplen >= 2) && ((actual_pat[0] == '"' && actual_pat[aplen - 1] == '"') ||
                                    (actual_pat[0] == '\'' && actual_pat[aplen - 1] == '\''))) {
                    strncpy(clean_pat, actual_pat + 1, aplen - 2);
                    clean_pat[aplen - 2] = '\0';
                } else {
                    strncpy(clean_pat, actual_pat, sizeof(clean_pat) - 1);
                    clean_pat[sizeof(clean_pat) - 1] = '\0';
                }

                int fnm_flags = (state && state->extglob) ? ZBASH_FNM_EXTMATCH : 0;
                if (zbash_fnmatch(clean_pat, target_word, fnm_flags) == 0) {
                    this_clause_match = true;
                }
                if (exp_pat) free(exp_pat);
                if (this_clause_match) break;
            }

            if (this_clause_match) {
                matched = true;
                if (blen > 0) {
                    run_loop_body(state, body_buf, clause_start_line);
                } else {
                    state->last_status = 0;
                }
            }
        }
    }

    if (!matched) {
        state->last_status = 0;
    }

    if (has_redir) {
        fflush(stdout);
        fflush(stderr);
        zbash_restore_redirections(state, saved_fds, saved_count);
        free_cmd_strings(&redir_cmd);
    }

    free(target_word);
    return state->last_status;
}
