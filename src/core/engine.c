/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "core_internal.h"
#include "prompt.h"

volatile int zbash_suppress_int = 0;
volatile int zbash_pending_int = 0;

void zbash_trigger_hook(zbash_state_t *state, const char *hook_name) {
    if (!state) return;
    zbash_func_t *fn = zbash_find_func(state, hook_name);
    if (fn) {
        char *argv[] = { (char *)hook_name, NULL };
        zbash_exec_func(state, fn, 1, argv);
    }
}

void zbash_trigger_preexec(zbash_state_t *state, const char *cmdline) {
    if (!state) return;
    zbash_func_t *fn = zbash_find_func(state, "preexec");
    if (fn) {
        char *argv[] = { "preexec", (char *)cmdline, NULL };
        zbash_exec_func(state, fn, 2, argv);
    }
}

/* ========================================================================= */
/* Shell Lifecycle & State Initialization                                    */
/* ========================================================================= */

void zbash_init(zbash_state_t *state, int argc, char **argv) {
    memset(state, 0, sizeof(zbash_state_t));
    setvbuf(stdout, NULL, _IOFBF, 1024);
    state->shell_pid = getpid();
    char ppid_buf[32];
    snprintf(ppid_buf, sizeof(ppid_buf), "%d", getppid());
    setenv("PPID", ppid_buf, 1);
    state->last_status = 0;
    state->argc = argc;
    state->argv = argv;
    state->source_depth = 0;
    state->func_count = 0;
    state->array_count = 0;
    state->func_depth = 0;
    state->local_var_count = 0;
    state->sourcepath = true;
    state->current_job_idx = -1;
    state->previous_job_idx = -1;
    zbash_history_init(state);
    setenv("OPTIND", "1", 1);
    setenv("IFS", " \t\n", 1);
    unsetenv("OLDPWD");

    for (int s = 1; s <= 31; s++) {
        struct sigaction sa;
        if (sigaction(s, NULL, &sa) == 0 && sa.sa_handler == SIG_IGN) {
            state->sig_inherited_ign[s] = true;
        }
    }

    /* Initialize UID and EUID */
    char uid_buf[32], euid_buf[32];
    snprintf(uid_buf, sizeof(uid_buf), "%u", (unsigned int)getuid());
    snprintf(euid_buf, sizeof(euid_buf), "%u", (unsigned int)geteuid());
    setenv("UID", uid_buf, 1);
    setenv("EUID", euid_buf, 1);
    zbash_set_var_attr(state, "UID", ZBASH_ATTR_READONLY);
    zbash_set_var_attr(state, "EUID", ZBASH_ATTR_READONLY);

    /* Initialize GROUPS array */
    int ngroups = getgroups(0, NULL);
    if (ngroups > 0) {
        gid_t *groups = malloc(sizeof(gid_t) * ngroups);
        if (groups) {
            int actual = getgroups(ngroups, groups);
            if (actual > 0) {
                char **group_strs = malloc(sizeof(char *) * actual);
                for (int g = 0; g < actual; g++) {
                    char gbuf[32];
                    snprintf(gbuf, sizeof(gbuf), "%u", (unsigned int)groups[g]);
                    group_strs[g] = strdup(gbuf);
                }
                zbash_set_array(state, "GROUPS", actual, group_strs);
                for (int g = 0; g < actual; g++) free(group_strs[g]);
                free(group_strs);
            }
            free(groups);
        }
    } else {
        char gbuf[32];
        snprintf(gbuf, sizeof(gbuf), "%u", (unsigned int)getgid());
        char *single_group = strdup(gbuf);
        zbash_set_array(state, "GROUPS", 1, &single_group);
        free(single_group);
    }

    /* Initialize DIRSTACK array */
    const char *init_pwd = getenv("PWD");
    char cwd_buf[1024];
    if (!init_pwd || init_pwd[0] != '/') {
        if (getcwd(cwd_buf, sizeof(cwd_buf))) init_pwd = cwd_buf;
        else init_pwd = "/";
    }
    char *dstack_elem = strdup(init_pwd);
    zbash_set_array(state, "DIRSTACK", 1, &dstack_elem);
    free(dstack_elem);

    /* Import functions exported in environment: BASH_FUNC_<name>%%=() { ... } */
    extern char **environ;
    if (environ) {
        for (char **env = environ; *env != NULL; env++) {
            if (strncmp(*env, "BASH_FUNC_", 10) == 0) {
                char *eq = strchr(*env, '=');
                if (eq && (eq - *env > 12) && *(eq - 2) == '%' && *(eq - 1) == '%') {
                    size_t nlen = (size_t)((eq - 2) - (*env + 10));
                    char fname[128];
                    if (nlen < sizeof(fname)) {
                        strncpy(fname, *env + 10, nlen);
                        fname[nlen] = '\0';
                        const char *val = eq + 1;
                        while (*val && isspace((unsigned char)*val)) val++;
                        if (strncmp(val, "()", 2) == 0) {
                            val += 2;
                            while (*val && isspace((unsigned char)*val)) val++;
                            if (*val == '{') {
                                char *full_def = malloc(strlen(fname) + strlen(eq + 1) + 4);
                                if (full_def) {
                                    sprintf(full_def, "%s %s", fname, eq + 1);
                                    check_func_def(state, full_def);
                                    free(full_def);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void zbash_sync_call_stack_arrays(zbash_state_t *state) {
    if (!state) return;

    /* FUNCNAME: if frame_count <= 1 (main only), FUNCNAME has count 0 */
    if (state->frame_count <= 1) {
        zbash_set_array(state, "FUNCNAME", 0, NULL);
    } else {
        int count = state->frame_count;
        char **fn_elems = malloc(sizeof(char *) * count);
        for (int i = 0; i < count; i++) {
            int frame_idx = (count - 1) - i;
            fn_elems[i] = (char *)state->frames[frame_idx].func_name;
        }
        zbash_set_array(state, "FUNCNAME", count, fn_elems);
        free(fn_elems);
    }

    /* BASH_SOURCE */
    int src_count = state->frame_count > 0 ? state->frame_count : 1;
    char **src_elems = malloc(sizeof(char *) * src_count);
    if (state->frame_count > 0) {
        for (int i = 0; i < src_count; i++) {
            int frame_idx = (src_count - 1) - i;
            src_elems[i] = (char *)state->frames[frame_idx].file_name;
        }
    } else {
        src_elems[0] = (char *)((state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "main");
    }
    zbash_set_array(state, "BASH_SOURCE", src_count, src_elems);
    free(src_elems);

    /* BASH_LINENO */
    int line_count = state->frame_count > 0 ? state->frame_count : 1;
    char **line_elems = malloc(sizeof(char *) * line_count);
    char line_bufs[ZBASH_MAX_FRAMES][32];
    if (state->frame_count > 0) {
        for (int i = 0; i < line_count; i++) {
            int frame_idx = (line_count - 1) - i;
            snprintf(line_bufs[i], sizeof(line_bufs[i]), "%d", state->frames[frame_idx].call_line);
            line_elems[i] = line_bufs[i];
        }
    } else {
        snprintf(line_bufs[0], sizeof(line_bufs[0]), "0");
        line_elems[0] = line_bufs[0];
    }
    zbash_set_array(state, "BASH_LINENO", line_count, line_elems);
    free(line_elems);
}

int zbash_trigger_debug_trap(zbash_state_t *state, int line_num) {
    if (!state || !state->debug_trap || state->in_debug_trap) return 0;

    state->in_debug_trap = true;
    int saved_lineno = state->lineno;
    state->lineno = line_num;

    int rc = zbash_run_line(state, state->debug_trap);

    state->lineno = saved_lineno;
    state->in_debug_trap = false;

    if (state->extdebug && rc == 2) {
        return 2;
    }
    return 0;
}

/* ========================================================================= */
/* Token & Statement Parsing Helpers                                         */
/* ========================================================================= */

bool is_word_match(const char *p, const char *word, size_t wlen) {
    if (strncmp(p, word, wlen) != 0) return false;
    char after = p[wlen];
    return (after == '\0' || isspace((unsigned char)after) || after == ';' || after == '\n' || after == '(' || after == ')' || after == '{' || after == '}');
}

bool is_statement_incomplete(const char *buf) {
    if (!buf) return false;
    bool in_s = false, in_d = false, in_bt = false;
    int if_d = 0, loop_needs_body = 0, do_d = 0, brace_d = 0, param_brace_d = 0, paren_d = 0, case_d = 0, func_needs_body = 0;
    bool cmd_start = true;
    char last_non_space = '\0';
    size_t len = strlen(buf);

    for (size_t i = 0; i < len; i++) {
        char c = buf[i];
        if (c == '\\' && i + 1 < len) {
            i++;
            continue;
        }
        if (c == '\'' && !in_d && !in_bt) in_s = !in_s;
        else if (c == '"' && !in_s && !in_bt) in_d = !in_d;
        else if (c == '`' && !in_s) in_bt = !in_bt;
        else if (!in_s && !in_d && !in_bt) {
            if (c == '#' && (i == 0 || isspace((unsigned char)buf[i - 1]) || buf[i - 1] == ';')) {
                while (i < len && buf[i] != '\n') i++;
                continue;
            }
            if (c == '$' && i + 1 < len && buf[i + 1] == '{') {
                param_brace_d++;
                i++;
                continue;
            }
            if (c == '{') {
                bool at_word_start = (i == 0 || isspace((unsigned char)buf[i - 1]) || buf[i - 1] == ';');
                bool is_alone = (i + 1 == len || isspace((unsigned char)buf[i + 1]) || buf[i + 1] == ';' || buf[i + 1] == '\n');
                if (loop_needs_body > 0) {
                    loop_needs_body--;
                    brace_d++;
                    cmd_start = true;
                } else if ((last_non_space == ')' || func_needs_body > 0) && at_word_start) {
                    if (func_needs_body > 0) func_needs_body = 0;
                    brace_d++;
                    cmd_start = true;
                } else if (cmd_start && at_word_start && is_alone) {
                    brace_d++;
                    cmd_start = true;
                }
            }
            else if (c == '}') {
                if (param_brace_d > 0) {
                    param_brace_d--;
                } else {
                    bool at_word_start = (i == 0 || isspace((unsigned char)buf[i - 1]) || buf[i - 1] == ';');
                    bool is_alone = (i + 1 == len || isspace((unsigned char)buf[i + 1]) || buf[i + 1] == ';' || buf[i + 1] == '&' || buf[i + 1] == '\n');
                    if (brace_d > 0 && at_word_start && is_alone) {
                        brace_d--;
                    }
                }
            }
            else if (c == '(') {
                paren_d++;
                cmd_start = true;
            }
            else if (c == ')') { if (paren_d > 0) paren_d--; }
            else if (brace_d == 0 && paren_d == 0) {
                bool at_word_start = (i == 0 || isspace((unsigned char)buf[i - 1]) || buf[i - 1] == ';');
                if (at_word_start) {
                    if (cmd_start && is_word_match(buf + i, "if", 2)) if_d++;
                    else if (is_word_match(buf + i, "fi", 2)) { if (if_d > 0) if_d--; }
                    else if (cmd_start && (is_word_match(buf + i, "for", 3) || is_word_match(buf + i, "while", 5) || is_word_match(buf + i, "until", 5))) loop_needs_body++;
                    else if (is_word_match(buf + i, "do", 2)) {
                        if (loop_needs_body > 0) loop_needs_body--;
                        do_d++;
                        cmd_start = true;
                    }
                    else if (is_word_match(buf + i, "done", 4)) { if (do_d > 0) do_d--; }
                    else if (is_word_match(buf + i, "then", 4) || is_word_match(buf + i, "else", 4) || is_word_match(buf + i, "elif", 4)) {
                        cmd_start = true;
                    }
                    else if (cmd_start && is_word_match(buf + i, "case", 4)) case_d++;
                    else if (is_word_match(buf + i, "esac", 4)) { if (case_d > 0) case_d--; }
                    else if (cmd_start && is_word_match(buf + i, "function", 8)) func_needs_body = 1;
                }
            }
            if (c == ';' || c == '\n' || c == '|' || c == '&' || c == '(' || c == ')') {
                cmd_start = true;
            } else if (!isspace((unsigned char)c) && c != '{' && c != '}') {
                cmd_start = false;
            }
            if (!isspace((unsigned char)c)) {
                last_non_space = c;
            }
        }
    }

    if (in_s || in_d || in_bt || if_d > 0 || loop_needs_body > 0 || do_d > 0 || brace_d > 0 || param_brace_d > 0 || paren_d > 0 || case_d > 0 || func_needs_body > 0) return true;

    /* Find effective end of code on the last line (ignoring trailing unquoted comment) */
    size_t last_line_start = 0;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == '\n') last_line_start = i + 1;
    }

    size_t comment_pos = len;
    bool in_s2 = false, in_d2 = false, in_bt2 = false;
    for (size_t i = last_line_start; i < len; i++) {
        char c = buf[i];
        if (c == '\\' && i + 1 < len) { i++; continue; }
        if (c == '\'' && !in_d2 && !in_bt2) in_s2 = !in_s2;
        else if (c == '"' && !in_s2 && !in_bt2) in_d2 = !in_d2;
        else if (c == '`' && !in_s2) in_bt2 = !in_bt2;
        else if (!in_s2 && !in_d2 && !in_bt2 && c == '#' && (i == last_line_start || isspace((unsigned char)buf[i - 1]) || buf[i - 1] == ';')) {
            comment_pos = i;
            break;
        }
    }

    size_t end = comment_pos;
    while (end > last_line_start && isspace((unsigned char)buf[end - 1])) end--;
    if (end <= last_line_start) return false;

    /* Check trailing operators like |, ||, &&, \, or function signature () without { */
    if (end > 0) {
        if (buf[end - 1] == '\\' && (end == 1 || buf[end - 2] != '\\')) return true;
        if (buf[end - 1] == '|') return true;
        if (end >= 2 && buf[end - 2] == '&' && buf[end - 1] == '&') return true;
        if (buf[end - 1] == ')') {
            size_t k = end - 1;
            while (k > 0 && buf[k] != '(') k--;
            if (buf[k] == '(') {
                size_t before_k = k;
                while (before_k > 0 && isspace((unsigned char)buf[before_k - 1])) before_k--;
                if (before_k > 0 && buf[before_k - 1] == '=') {
                    /* It is an array assignment like x=(), not a function definition */
                } else {
                    bool all_ws = true;
                    for (size_t m = k + 1; m < end - 1; m++) {
                        if (!isspace((unsigned char)buf[m])) { all_ws = false; break; }
                    }
                    if (all_ws) return true;
                }
            }
        }
    }

    return false;
}


bool is_newline_separator(const char *start, size_t i) {
    while (i > 0 && isspace((unsigned char)start[i - 1])) i--;
    if (i == 0) return false;
    char last = start[i - 1];
    if (last == '\\' || last == '|' || last == '&') return false;
    if (last == ')') {
        size_t k = i - 1;
        while (k > 0 && start[k] != '(') k--;
        if (start[k] == '(') {
            bool all_ws = true;
            for (size_t m = k + 1; m < i - 1; m++) {
                if (!isspace((unsigned char)start[m])) { all_ws = false; break; }
            }
            if (all_ws) return false;
        }
    }
    return true;
}

static char *expand_aliases(zbash_state_t *state, const char *line) {
    if (!state->expand_aliases || state->alias_count == 0 || !line) return NULL;

    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p || *p == '#' || *p == '(' || *p == '{' || *p == '!' || *p == '[' || *p == '$') return NULL;
    if (strncmp(p, "if ", 3) == 0 || strncmp(p, "for ", 4) == 0 ||
        strncmp(p, "while ", 6) == 0 || strncmp(p, "until ", 6) == 0 ||
        strncmp(p, "case ", 5) == 0 || strncmp(p, "function ", 9) == 0 ||
        strncmp(p, "alias ", 6) == 0 || strncmp(p, "alias\t", 6) == 0 ||
        strncmp(p, "unalias ", 8) == 0 || strncmp(p, "unalias\t", 8) == 0) {
        return NULL;
    }

    char out[8192];
    out[0] = '\0';
    size_t out_len = 0;

    size_t lead_ws = p - line;
    if (lead_ws > 0 && lead_ws < sizeof(out)) {
        strncpy(out, line, lead_ws);
        out[lead_ws] = '\0';
        out_len = lead_ws;
    }

    bool check_next_word = true;
    bool expanded_any = false;
    const char *curr = p;

    while (*curr && check_next_word) {
        while (*curr && isspace((unsigned char)*curr)) {
            if (out_len + 1 < sizeof(out)) {
                out[out_len++] = *curr;
                out[out_len] = '\0';
            }
            curr++;
        }
        if (!*curr) break;

        const char *w_start = curr;
        while (*curr && !isspace((unsigned char)*curr) && *curr != ';' && *curr != '|' && *curr != '&') curr++;
        size_t w_len = curr - w_start;
        if (w_len == 0) break;

        char word[256];
        if (w_len >= sizeof(word)) w_len = sizeof(word) - 1;
        strncpy(word, w_start, w_len);
        word[w_len] = '\0';

        char *seen[128];
        int seen_count = 0;
        char cur_val[512];
        strncpy(cur_val, word, sizeof(cur_val) - 1);
        cur_val[sizeof(cur_val) - 1] = '\0';

        bool ends_with_space = false;
        bool found_alias = false;

        while (1) {
            const char *match_val = NULL;
            for (int a = 0; a < state->alias_count; a++) {
                if (strcmp(state->aliases[a].name, cur_val) == 0) {
                    match_val = state->aliases[a].val;
                    break;
                }
            }
            if (!match_val) break;

            bool cycle = false;
            for (int s = 0; s < seen_count; s++) {
                if (strcmp(seen[s], cur_val) == 0) {
                    cycle = true;
                    break;
                }
            }
            if (cycle) break;

            if (seen_count < 128) {
                seen[seen_count++] = strdup(cur_val);
            }
            found_alias = true;
            expanded_any = true;

            size_t mv_len = strlen(match_val);
            ends_with_space = (mv_len > 0 && (match_val[mv_len - 1] == ' ' || match_val[mv_len - 1] == '\t'));

            strncpy(cur_val, match_val, sizeof(cur_val) - 1);
            cur_val[sizeof(cur_val) - 1] = '\0';

            if (ends_with_space || strchr(match_val, ' ') != NULL) {
                break;
            }
        }

        for (int s = 0; s < seen_count; s++) {
            free(seen[s]);
        }

        if (found_alias) {
            size_t val_len = strlen(cur_val);
            if (out_len + val_len < sizeof(out)) {
                strcpy(out + out_len, cur_val);
                out_len += val_len;
            }
            check_next_word = ends_with_space;
        } else {
            if (out_len + w_len < sizeof(out)) {
                strncpy(out + out_len, word, w_len);
                out_len += w_len;
                out[out_len] = '\0';
            }
            check_next_word = false;
        }
    }

    if (*curr) {
        size_t rem_len = strlen(curr);
        if (out_len + rem_len < sizeof(out)) {
            strcpy(out + out_len, curr);
            out_len += rem_len;
        }
    }

    return expanded_any ? strdup(out) : NULL;
}

/* ========================================================================= */
/* Command & Pipeline Execution Coordinator                                  */
/* ========================================================================= */

static bool is_arithmetic_command(const char *line) {
    if (!line || line[0] != '(' || line[1] != '(') return false;
    size_t len = strlen(line);
    size_t end = len;
    while (end > 0 && isspace((unsigned char)line[end - 1])) end--;
    if (end < 4 || line[end - 1] != ')' || line[end - 2] != ')') return false;

    int depth = 2;
    bool in_s = false, in_d = false;
    for (size_t i = 2; i < end - 2; i++) {
        char c = line[i];
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;
        else if (!in_s && !in_d) {
            if (c == '(') depth++;
            else if (c == ')') {
                depth--;
                if (depth < 2) return false;
            }
        }
    }
    return (depth == 2);
}

static int zbash_run_line_internal(zbash_state_t *state, const char *line);

int zbash_run_line(zbash_state_t *state, const char *line) {
    if (!line) return 0;
#if defined(ZBASH_USE_NEON)
    line += zbash_simd_skip_spaces(line, strlen(line));
#endif
    while (waitpid(-1, NULL, WNOHANG) > 0);
    while (*line && isspace((unsigned char)*line)) line++;
    if (*line == '\0') return 0;
    if (*line == '#') {
        if (state->history_enabled && state->run_line_depth == 0) {
            zbash_history_add(state, line);
        }
        return 0;
    }
    state->expansion_error = false;

    if (state->histexp_enabled && state->run_line_depth == 0) {
        char *expanded = NULL;
        int hist_rc = zbash_histexp_expand(state, line, &expanded);
        if (hist_rc < 0) {
            state->last_status = 1;
            return 1;
        }
        if (hist_rc > 0 && expanded) {
            fprintf(stderr, "%s\n", expanded);
            if (state->history_enabled && state->run_line_depth == 0) {
                zbash_history_add(state, expanded);
            }
            if (hist_rc == 2) {
                free(expanded);
                return 0;
            }
            state->run_line_depth++;
            int rc = zbash_run_line(state, expanded);
            state->run_line_depth--;
            free(expanded);
            return rc;
        }
    }

    char *alias_expanded = NULL;
    if (state->expand_aliases && state->alias_count > 0) {
        alias_expanded = expand_aliases(state, line);
    }
    const char *eff_line = alias_expanded ? alias_expanded : line;

    if (state->history_enabled && state->run_line_depth == 0) {
        zbash_history_add(state, eff_line);
    }

    state->run_line_depth++;
    int rc = zbash_run_line_internal(state, eff_line);
    state->run_line_depth--;
    if (state->run_line_depth == 0) {
        fflush(stdout);
    }
    if (alias_expanded) free(alias_expanded);
    return rc;
}

static int zbash_run_line_internal(zbash_state_t *state, const char *line) {
    /* If line contains semicolons outside quotes, braces, parens, or compound blocks, split and run sequentially */
    bool has_semi = false;
    bool in_s = false, in_d = false;
    int brace_d = 0, paren_d = 0, if_d = 0, loop_d = 0, case_d = 0;
    bool cmd_start = true;
    for (size_t i = 0; line[i] != '\0'; i++) {
        char c = line[i];
        if (c == '\\' && (in_s || in_d || line[i + 1] != '\0')) {
            if (line[i + 1] != '\0') i++;
            continue;
        }
        if (c == '`' && !in_s) {
            i++;
            while (line[i] != '\0' && line[i] != '`') {
                if (line[i] == '\\' && line[i + 1] != '\0') i += 2;
                else i++;
            }
            if (line[i] == '\0') break;
            continue;
        }
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;
        else if (!in_s && !in_d && c == '#' && (i == 0 || isspace((unsigned char)line[i - 1]) || line[i - 1] == ';')) {
            while (line[i] != '\0' && line[i] != '\n') i++;
            if (line[i] == '\0') break;
            continue;
        }
        else if (!in_s && !in_d) {
            if (c == '{') brace_d++;
            else if (c == '}') { if (brace_d > 0) brace_d--; }
            else if (c == '(') paren_d++;
            else if (c == ')') { if (paren_d > 0) paren_d--; }
            else if (brace_d == 0 && paren_d == 0) {
                bool at_word_start = (i == 0 || isspace((unsigned char)line[i - 1]) || line[i - 1] == ';');
                if (at_word_start) {
                    if (cmd_start && is_word_match(line + i, "if", 2)) if_d++;
                    else if (is_word_match(line + i, "fi", 2)) { if (if_d > 0) if_d--; }
                    else if (cmd_start && (is_word_match(line + i, "for", 3) || is_word_match(line + i, "while", 5) || is_word_match(line + i, "until", 5))) loop_d++;
                    else if (is_word_match(line + i, "done", 4)) { if (loop_d > 0) loop_d--; }
                    else if (cmd_start && is_word_match(line + i, "case", 4)) case_d++;
                    else if (is_word_match(line + i, "esac", 4)) { if (case_d > 0) case_d--; }
                    else if (is_word_match(line + i, "do", 2) || is_word_match(line + i, "then", 4) || is_word_match(line + i, "else", 4) || is_word_match(line + i, "elif", 4)) {
                        cmd_start = true;
                    }
                }
                bool is_sep = (c == ';' || (c == '\n' && is_newline_separator(line, i)));
                if (c == '&' && line[i + 1] != '&' && (i == 0 || (line[i - 1] != '>' && line[i - 1] != '<' && line[i - 1] != '&'))) {
                    is_sep = true;
                }
                if (is_sep && if_d == 0 && loop_d == 0 && case_d == 0) {
                    has_semi = true;
                    break;
                }
            }
            if (c == ';' || c == '\n' || c == '|' || c == '&' || c == '(' || c == ')') {
                cmd_start = true;
            } else if (!isspace((unsigned char)c) && c != '{' && c != '}') {
                cmd_start = false;
            }
        }
    }

    if (has_semi) {
        char *line_copy = strdup(line);
        in_s = false; in_d = false; brace_d = 0; paren_d = 0; if_d = 0; loop_d = 0; case_d = 0;
        cmd_start = true;
        char *start = line_copy;
        for (size_t i = 0; ; i++) {
            char c = line_copy[i];
            if (c == '\\' && (in_s || in_d || line_copy[i + 1] != '\0')) {
                if (line_copy[i + 1] != '\0') i++;
                continue;
            }
            if (c == '`' && !in_s) {
                i++;
                while (line_copy[i] != '\0' && line_copy[i] != '`') {
                    if (line_copy[i] == '\\' && line_copy[i + 1] != '\0') i += 2;
                    else i++;
                }
                if (line_copy[i] == '\0') break;
                continue;
            }
            if (c == '\'' && !in_d) in_s = !in_s;
            else if (c == '"' && !in_s) in_d = !in_d;
            else if (!in_s && !in_d && c == '#' && (i == 0 || isspace((unsigned char)line_copy[i - 1]) || line_copy[i - 1] == ';')) {
                while (line_copy[i] != '\0' && line_copy[i] != '\n') i++;
                c = line_copy[i];
            }
            else if (!in_s && !in_d) {
                if (c == '{') brace_d++;
                else if (c == '}') { if (brace_d > 0) brace_d--; }
                else if (c == '(') paren_d++;
                else if (c == ')') { if (paren_d > 0) paren_d--; }
                else if (brace_d == 0 && paren_d == 0) {
                    bool at_word_start = (i == 0 || isspace((unsigned char)line_copy[i - 1]) || line_copy[i - 1] == ';');
                    if (at_word_start) {
                        if (cmd_start && is_word_match(line_copy + i, "if", 2)) if_d++;
                        else if (is_word_match(line_copy + i, "fi", 2)) { if (if_d > 0) if_d--; }
                        else if (cmd_start && (is_word_match(line_copy + i, "for", 3) || is_word_match(line_copy + i, "while", 5) || is_word_match(line_copy + i, "until", 5))) loop_d++;
                        else if (is_word_match(line_copy + i, "done", 4)) { if (loop_d > 0) loop_d--; }
                        else if (cmd_start && is_word_match(line_copy + i, "case", 4)) case_d++;
                        else if (is_word_match(line_copy + i, "esac", 4)) { if (case_d > 0) case_d--; }
                        else if (is_word_match(line_copy + i, "do", 2) || is_word_match(line_copy + i, "then", 4) || is_word_match(line_copy + i, "else", 4) || is_word_match(line_copy + i, "elif", 4)) {
                            cmd_start = true;
                        }
                    }
                }
                if (c == ';' || c == '\n' || c == '|' || c == '&' || c == '(' || c == ')') {
                    cmd_start = true;
                } else if (!isspace((unsigned char)c) && c != '{' && c != '}') {
                    cmd_start = false;
                }
            }
            bool is_semi = (c == ';' || (c == '\n' && is_newline_separator(line_copy, i)));
            bool is_bg = (c == '&' && line_copy[i + 1] != '&' && (i == 0 || (line_copy[i - 1] != '>' && line_copy[i - 1] != '<' && line_copy[i - 1] != '&')));
            if (((is_semi || is_bg) && !in_s && !in_d && brace_d == 0 && paren_d == 0 && if_d == 0 && loop_d == 0 && case_d == 0) || c == '\0') {
                line_copy[i] = '\0';
                if (is_bg) {
#if !defined(__EMSCRIPTEN__)
                    fflush(stdout);
                    fflush(stderr);
                    pid_t child = fork();
                    if (child == 0) {
                        zbash_state_t child_state = *state;
                        child_state.exit_trap = NULL;
                        if (!child_state.functrace) {
                            child_state.debug_trap = NULL;
                            child_state.return_trap = NULL;
                        }
                        bool has_in_redir = (strchr(start, '<') != NULL);
                        bool stdin_is_redirected = (state && (state->stdin_redir_depth > 0 || state->stdin_permanently_redirected));
                        if (!has_in_redir && !stdin_is_redirected) {
                            int devnull = open("/dev/null", O_RDONLY);
                            if (devnull >= 0) {
                                dup2(devnull, STDIN_FILENO);
                                if (devnull != STDIN_FILENO) close(devnull);
                            }
                        }
                        int rc = zbash_run_line(&child_state, start);
                        int final_code = child_state.should_exit ? child_state.exit_code : rc;
                        if (child_state.exit_trap) {
                            char *t = child_state.exit_trap;
                            child_state.exit_trap = NULL;
                            child_state.should_exit = false;
                            zbash_run_line(&child_state, t);
                            free(t);
                            if (child_state.should_exit) final_code = child_state.exit_code;
                        }
                        _exit(final_code);
                    } else if (child > 0) {
                        state->last_bg_pid = child;
                        state->last_status = 0;
                        char full_cmd[256];
                        char *s_trim = start;
                        while (*s_trim && isspace((unsigned char)*s_trim)) s_trim++;
                        size_t slen = strlen(s_trim);
                        while (slen > 0 && isspace((unsigned char)s_trim[slen - 1])) slen--;

                        if (slen > 1 && s_trim[0] == '(' && s_trim[slen - 1] == ')') {
                            char inner[256];
                            const char *in_start = s_trim + 1;
                            while (*in_start && isspace((unsigned char)*in_start)) in_start++;
                            const char *in_end = s_trim + slen - 1;
                            while (in_end > in_start && isspace((unsigned char)*(in_end - 1))) in_end--;
                            size_t in_len = in_end - in_start;
                            if (in_len >= sizeof(inner)) in_len = sizeof(inner) - 1;
                            size_t o = 0;
                            for (size_t k = 0; k < in_len && o + 2 < sizeof(inner); k++) {
                                if (in_start[k] == ' ' && in_start[k + 1] == ';') continue;
                                inner[o++] = in_start[k];
                            }
                            inner[o] = '\0';
                            snprintf(full_cmd, sizeof(full_cmd), "( %s ) &", inner);
                        } else {
                            snprintf(full_cmd, sizeof(full_cmd), "%.*s &", (int)slen, s_trim);
                        }
                        zbash_jobs_add(state, child, full_cmd, !state->monitor_mode);
                    }
#endif
                } else {
                    zbash_run_line(state, start);
                    if (state->should_exit || state->return_called || state->loop_break || state->loop_continue) break;
                }
                if (c == '\0') break;
                start = line_copy + i + 1;
            }
        }
        free(line_copy);
        return state->last_status;
    }

    /* Check for function definition: name() { ... } or function name { ... } */
    if (check_func_def(state, line)) return 0;

    /* Check for compound statements */
    if (strncmp(line, "if", 2) == 0 && (isspace((unsigned char)line[2]) || line[2] == '[')) {
        return exec_if(state, line);
    }
    if (strncmp(line, "for", 3) == 0 && (isspace((unsigned char)line[3]) || line[3] == '(')) {
        const char *fp = line + 3;
        while (*fp && isspace((unsigned char)*fp)) fp++;
        if (fp[0] == '(' && fp[1] == '(') {
            return exec_for_c(state, line);
        }
        return exec_for_in(state, line);
    }
    if (strncmp(line, "while", 5) == 0 && isspace((unsigned char)line[5])) {
        return exec_while(state, line);
    }
    if (strncmp(line, "until", 5) == 0 && isspace((unsigned char)line[5])) {
        return exec_until(state, line);
    }

    /* If line contains conditionals && or || outside quotes, braces, parens, or [[ ]], evaluate sequentially */
    bool has_cond = false;
    in_s = false; in_d = false; brace_d = 0; paren_d = 0; int cond_d = 0;
    for (size_t i = 0; line[i] != '\0'; i++) {
        char c = line[i];
        if (c == '\\' && (in_s || in_d || line[i + 1] != '\0')) {
            if (line[i + 1] != '\0') i++;
            continue;
        }
        if (c == '`' && !in_s) {
            i++;
            while (line[i] != '\0' && line[i] != '`') {
                if (line[i] == '\\' && line[i + 1] != '\0') i += 2;
                else i++;
            }
            if (line[i] == '\0') break;
            continue;
        }
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;
        else if (!in_s && !in_d && c == '#' && (i == 0 || isspace((unsigned char)line[i - 1]) || line[i - 1] == ';')) {
            while (line[i] != '\0' && line[i] != '\n') i++;
            if (line[i] == '\0') break;
            continue;
        }
        else if (c == '{' && !in_s && !in_d) brace_d++;
        else if (line[i] == '}' && !in_s && !in_d) { if (brace_d > 0) brace_d--; }
        else if (line[i] == '(' && !in_s && !in_d) paren_d++;
        else if (line[i] == ')' && !in_s && !in_d) { if (paren_d > 0) paren_d--; }
        else if (!in_s && !in_d) {
            if (line[i] == '[' && line[i + 1] == '[' && (i == 0 || isspace((unsigned char)line[i - 1]))) {
                cond_d++;
                i++;
                continue;
            } else if (line[i] == ']' && line[i + 1] == ']' && (line[i + 2] == '\0' || isspace((unsigned char)line[i + 2]) || line[i + 2] == ';' || line[i + 2] == '&' || line[i + 2] == '|' || line[i + 2] == '\n')) {
                if (cond_d > 0) cond_d--;
                i++;
                continue;
            }
        }
        if (!in_s && !in_d && brace_d == 0 && paren_d == 0 && cond_d == 0) {
            if ((line[i] == '&' && line[i + 1] == '&') || (line[i] == '|' && line[i + 1] == '|')) {
                has_cond = true;
                break;
            }
        }
    }

    if (has_cond) {
        char *line_copy = strdup(line);
        in_s = false; in_d = false; brace_d = 0; paren_d = 0; cond_d = 0;
        char *start = line_copy;
        bool skip_next = false;

        for (size_t i = 0; ; i++) {
            char c = line_copy[i];
            if (c == '\\' && (in_s || in_d || line_copy[i + 1] != '\0')) {
                if (line_copy[i + 1] != '\0') i++;
                continue;
            }
            if (c == '`' && !in_s) {
                i++;
                while (line_copy[i] != '\0' && line_copy[i] != '`') {
                    if (line_copy[i] == '\\' && line_copy[i + 1] != '\0') i += 2;
                    else i++;
                }
                if (line_copy[i] == '\0') break;
                continue;
            }
            if (c == '\'' && !in_d) in_s = !in_s;
            else if (c == '"' && !in_s) in_d = !in_d;
            else if (!in_s && !in_d && c == '#' && (i == 0 || isspace((unsigned char)line_copy[i - 1]) || line_copy[i - 1] == ';')) {
                while (line_copy[i] != '\0' && line_copy[i] != '\n') i++;
                c = line_copy[i];
            }
            else if (c == '{' && !in_s && !in_d) brace_d++;
            else if (line_copy[i] == '}' && !in_s && !in_d) { if (brace_d > 0) brace_d--; }
            else if (c == '(' && !in_s && !in_d) paren_d++;
            else if (c == ')' && !in_s && !in_d) { if (paren_d > 0) paren_d--; }
            else if (!in_s && !in_d) {
                if (line_copy[i] == '[' && line_copy[i + 1] == '[' && (i == 0 || isspace((unsigned char)line_copy[i - 1]))) {
                    cond_d++;
                    i++;
                    continue;
                } else if (line_copy[i] == ']' && line_copy[i + 1] == ']' && (line_copy[i + 2] == '\0' || isspace((unsigned char)line_copy[i + 2]) || line_copy[i + 2] == ';' || line_copy[i + 2] == '&' || line_copy[i + 2] == '|' || line_copy[i + 2] == '\n')) {
                    if (cond_d > 0) cond_d--;
                    i++;
                    continue;
                }
            }

            int op = 0;
            if (!in_s && !in_d && brace_d == 0 && paren_d == 0 && cond_d == 0) {
                if (c == '&' && line_copy[i + 1] == '&') op = 1;
                else if (c == '|' && line_copy[i + 1] == '|') op = 2;
                else if (c == '\0') op = 3;
            } else if (c == '\0') {
                op = 3;
            }

            if (op != 0) {
                line_copy[i] = '\0';

                if (!skip_next) {
                    bool suppress_e = (op != 3);
                    bool saved_e = state->errexit;
                    if (suppress_e) state->errexit = false;

                    zbash_run_line(state, start);

                    if (suppress_e) {
                        state->errexit = saved_e;
                        state->should_exit = false;
                    }
                    if (state->should_exit || state->return_called || state->loop_break || state->loop_continue) break;
                }

                if (op == 3) break;

                if (op == 1) {
                    skip_next = (state->last_status != 0);
                } else if (op == 2) {
                    skip_next = (state->last_status == 0);
                }

                i++;
                start = line_copy + i + 1;
            }
        }
        free(line_copy);
        return state->last_status;
    }

    /* Trigger DEBUG trap */
    if (state->suppress_debug_trap) {
        state->suppress_debug_trap = false;
    } else {
        if (zbash_trigger_debug_trap(state, state->lineno) == 2) {
            return 0;
        }
    }

    /* Check for array definition: arr=( ... ) */
    if (check_array_def(state, line)) return 0;

    /* Check for standalone arithmetic command (( expr )) */
    if (is_arithmetic_command(line)) {
        size_t elen = strlen(line);
        while (elen > 0 && isspace((unsigned char)line[elen - 1])) elen--;
        char expr[4096];
        size_t inner_len = (elen - 2) - 2;
        if (inner_len >= sizeof(expr)) inner_len = sizeof(expr) - 1;
        strncpy(expr, line + 2, inner_len);
        expr[inner_len] = '\0';
        long long val = zbash_eval_arithmetic(state, expr);
        state->last_status = (val != 0) ? 0 : 1;
        return state->last_status;
    }

    if (strncmp(line, "case ", 5) == 0 || strncmp(line, "case\t", 5) == 0 || strncmp(line, "case\n", 5) == 0) {
        return exec_case(state, line);
    }

    /* Subshell command: ( cmd ) */
    if (line[0] == '(' && !is_arithmetic_command(line)) {
        size_t slen = strlen(line);
        int depth = 1;
        bool in_s_sub = false, in_d_sub = false;
        size_t j = 1;
        while (j < slen && depth > 0) {
            char c = line[j];
            if (c == '\'' && !in_d_sub) in_s_sub = !in_s_sub;
            else if (c == '"' && !in_s_sub) in_d_sub = !in_d_sub;
            else if (c == '(' && !in_s_sub && !in_d_sub) depth++;
            else if (c == ')' && !in_s_sub && !in_d_sub) {
                depth--;
                if (depth == 0) break;
            }
            j++;
        }
        if (depth == 0) {
            size_t k = j + 1;
            while (k < slen && isspace((unsigned char)line[k])) k++;
            if (k >= slen) {
                char *inner = malloc(j);
                strncpy(inner, line + 1, j - 1);
                inner[j - 1] = '\0';
                fflush(stdout);
                fflush(stderr);
                pid_t pid = fork();
                if (pid == 0) {
                    zbash_state_t sub = *state;
                    sub.exit_trap = NULL;
                    if (!sub.functrace) {
                        sub.debug_trap = NULL;
                        sub.return_trap = NULL;
                    }
                    int rc = zbash_run_line(&sub, inner);
                    int final_code = sub.should_exit ? sub.exit_code : rc;
                    if (sub.exit_trap) {
                        char *t = sub.exit_trap;
                        sub.exit_trap = NULL;
                        sub.should_exit = false;
                        zbash_run_line(&sub, t);
                        free(t);
                        if (sub.should_exit) final_code = sub.exit_code;
                    }
                    _exit(final_code);
                } else if (pid > 0) {
                    int status = 0;
                    waitpid(pid, &status, 0);
                    state->last_status = WIFEXITED(status) ? WEXITSTATUS(status) : (128 + WTERMSIG(status));
                    free(inner);
                    if (state->errexit && state->last_status != 0) {
                        state->should_exit = true;
                        state->exit_code = state->last_status;
                    }
                    return state->last_status;
                }
                free(inner);
            }
        }
    }

    /* Brace command: { cmd; } */
    if (line[0] == '{' && isspace((unsigned char)line[1])) {
        size_t slen = strlen(line);
        int depth = 1;
        bool in_s_br = false, in_d_br = false;
        size_t j = 1;
        while (j < slen && depth > 0) {
            char c = line[j];
            if (c == '\'' && !in_d_br) in_s_br = !in_s_br;
            else if (c == '"' && !in_s_br) in_d_br = !in_d_br;
            else if (c == '{' && !in_s_br && !in_d_br) depth++;
            else if (c == '}' && !in_s_br && !in_d_br) {
                depth--;
                if (depth == 0) break;
            }
            j++;
        }
        if (depth == 0) {
            size_t k = j + 1;
            while (k < slen && isspace((unsigned char)line[k])) k++;
            if (k >= slen) {
                char *inner = malloc(j);
                strncpy(inner, line + 1, j - 1);
                inner[j - 1] = '\0';
                int rc = zbash_run_line(state, inner);
                free(inner);
                if (state->errexit && rc != 0) {
                    state->should_exit = true;
                    state->exit_code = rc;
                }
                return rc;
            }
        }
    }

    /* Arithmetic command: (( expr )) */
    if (line[0] == '(' && line[1] == '(') {
        char expr[512];
        const char *ap = line + 2;
        char *end = strstr(ap, "))");
        if (end) {
            size_t elen = end - ap;
            if (elen >= sizeof(expr)) elen = sizeof(expr) - 1;
            strncpy(expr, ap, elen);
            expr[elen] = '\0';
            long long val = zbash_eval_arithmetic(state, expr);
            state->last_status = (val != 0) ? 0 : 1;
            if (state->errexit && state->last_status != 0) {
                state->should_exit = true;
                state->exit_code = state->last_status;
            }
            return state->last_status;
        }
    }

    bool invert = false;
    if (line[0] == '!' && isspace((unsigned char)line[1])) {
        invert = true;
        line += 2;
        while (*line && isspace((unsigned char)*line)) line++;
    }

    bool saved_invert_e = state->errexit;
    if (invert) state->errexit = false;

    /* Conditional compound command: [[ ... ]] */
    const char *cond_p = line;
    while (*cond_p && isspace((unsigned char)*cond_p)) cond_p++;
    if (cond_p[0] == '[' && cond_p[1] == '[' && isspace((unsigned char)cond_p[2])) {
        size_t clen = strlen(cond_p);
        while (clen > 0 && isspace((unsigned char)cond_p[clen - 1])) clen--;
        if (clen >= 4 && cond_p[clen - 2] == ']' && cond_p[clen - 1] == ']') {
            char inner[4096];
            size_t in_len = (clen - 2) - 2;
            if (in_len >= sizeof(inner)) in_len = sizeof(inner) - 1;
            strncpy(inner, cond_p + 2, in_len);
            inner[in_len] = '\0';
            int rc = zbash_eval_cond(state, inner);
            if (invert) rc = (rc == 0) ? 1 : 0;
            state->last_status = rc;
            if (state->errexit && rc != 0) {
                state->should_exit = true;
                state->exit_code = rc;
            }
            if (invert) state->errexit = saved_invert_e;
            return rc;
        }
    }

    if (state->verbose) {
        fputs(line, stderr);
        if (line[strlen(line) - 1] != '\n') fputc('\n', stderr);
    }

    zbash_pipeline_t *pipelines = calloc(ZBASH_MAX_LIST, sizeof(zbash_pipeline_t));
    if (!pipelines) {
        if (invert) state->errexit = saved_invert_e;
        return 1;
    }
    int count = zbash_parse_line(state, line, pipelines, ZBASH_MAX_LIST);
    zbash_conn_t prev_conn = CONN_NONE;

    for (int i = 0; i < count; i++) {
        if (state->expansion_error) {
            state->expansion_error = false;
            for (int k = 0; k < pipelines[i].cmd_count; k++) {
                free_cmd_strings(&pipelines[i].cmds[k]);
            }
            continue;
        }
        if (state->should_exit || state->return_called || state->loop_break || state->loop_continue) break;

        /* Conditionals */
        if (prev_conn == CONN_AND && state->last_status != 0) {
            prev_conn = pipelines[i].next_conn;
            for (int k = 0; k < pipelines[i].cmd_count; k++) {
                free_cmd_strings(&pipelines[i].cmds[k]);
            }
            continue;
        }
        if (prev_conn == CONN_OR && state->last_status == 0) {
            prev_conn = pipelines[i].next_conn;
            for (int k = 0; k < pipelines[i].cmd_count; k++) {
                free_cmd_strings(&pipelines[i].cmds[k]);
            }
            continue;
        }

        if (state->xtrace) {
            for (int k = 0; k < pipelines[i].cmd_count; k++) {
                fprintf(stderr, "+ ");
                for (int a = 0; a < pipelines[i].cmds[k].argc; a++) {
                    fprintf(stderr, "%s ", pipelines[i].cmds[k].argv[a]);
                }
                fprintf(stderr, "\n");
            }
        }

        bool suppress_errexit = (pipelines[i].next_conn != CONN_NONE);
        bool saved_pipeline_e = state->errexit;
        if (suppress_errexit) state->errexit = false;

        if (!state->noexec) {
            state->last_status = execute_pipeline(state, &pipelines[i]);
            if (invert) {
                state->last_status = (state->last_status == 0) ? 1 : 0;
            }
        }

        if (suppress_errexit) {
            state->errexit = saved_pipeline_e;
            state->should_exit = false;
        }

        prev_conn = pipelines[i].next_conn;

        for (int k = 0; k < pipelines[i].cmd_count; k++) {
            free_cmd_strings(&pipelines[i].cmds[k]);
        }

        if (state->errexit && !invert && pipelines[i].next_conn == CONN_NONE && state->last_status != 0) {
            state->should_exit = true;
            state->exit_code = state->last_status;
            break;
        }
    }

    if (invert) {
        state->errexit = saved_invert_e;
        state->should_exit = false;
    }

    free(pipelines);
    return state->last_status;
}

/* ========================================================================= */
/* Stream & Interactive Execution                                            */
/* ========================================================================= */

int zbash_run_stream(zbash_state_t *state, void *stream) {
    FILE *fp = (FILE *)stream;
    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;

    while ((nread = getline(&line, &cap, fp)) != -1) {
        state->lineno++;
        int start_l = state->lineno;
        state->stmt_start_lineno = state->lineno;
        if (nread > 0 && line[nread - 1] == '\n') line[--nread] = '\0';
        if (nread > 0 && line[nread - 1] == '\r') line[--nread] = '\0';

        /* Accumulate multi-line statements if incomplete */
        while (is_statement_incomplete(line)) {
            char *cont_line = NULL;
            size_t cont_cap = 0;
            ssize_t cn = getline(&cont_line, &cont_cap, fp);
            if (cn == -1) {
                if (cont_line) free(cont_line);
                break;
            }
            state->lineno++;
            if (cn > 0 && cont_line[cn - 1] == '\n') cont_line[--cn] = '\0';
            if (cn > 0 && cont_line[cn - 1] == '\r') cont_line[--cn] = '\0';

            size_t add_len = strlen(cont_line) + 2;
            if (strlen(line) + add_len >= cap) {
                cap = (strlen(line) + add_len) * 2;
                line = realloc(line, cap);
            }
            strcat(line, "\n");
            strcat(line, cont_line);
            free(cont_line);
        }

        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') continue;
        if (*p == '#') {
            if (state->history_enabled) {
                zbash_history_add(state, p);
            }
            continue;
        }

        /* If this is a function definition, check_func_def registers it directly */
        if (check_func_def(state, p)) continue;

        /* Here-Document scanner: supports multiple heredocs on a single line */
        typedef struct {
            char *op_start;     /* pointer where [fd]<< begins in p */
            char *op_end;       /* pointer after delimiter ends in p */
            int src_fd;         /* target fd (default 0) */
            bool strip_tabs;    /* <<- */
            bool quoted;        /* quoted delimiter */
            char delim[128];    /* delimiter word */
            char tmp_path[64];  /* /tmp/zbash_hd_XXXXXX */
        } stream_hd_t;

        stream_hd_t hd_specs[16];
        int num_hd = 0;

        if (!(p[0] == '(' && p[1] == '(')) {
            bool in_s = false, in_d = false;
            int paren_d = 0;
            for (char *c = p; *c; c++) {
                if (*c == '\'' && !in_d) in_s = !in_s;
                else if (*c == '"' && !in_s) in_d = !in_d;
                else if (*c == '(' && !in_s && !in_d) paren_d++;
                else if (*c == ')' && !in_s && !in_d) { if (paren_d > 0) paren_d--; }
                else if (!in_s && !in_d && paren_d == 0) {
                    if ((c == p || *(c - 1) != '<') && *c == '<' && *(c + 1) == '<' && *(c + 2) != '<' && *(c + 2) != '=') {
                        char *op_start = c;
                        int sfd = 0;
                        if (c > p && isdigit((unsigned char)*(c - 1))) {
                            char *d = c - 1;
                            while (d > p && isdigit((unsigned char)*(d - 1))) d--;
                            if (d == p || isspace((unsigned char)*(d - 1)) || *(d - 1) == ';') {
                                sfd = atoi(d);
                                op_start = d;
                            }
                        }
                        char *after_op = c + 2;
                        bool strip_tabs = false;
                        if (*after_op == '-') {
                            strip_tabs = true;
                            after_op++;
                        }
                        while (*after_op && (isspace((unsigned char)*after_op) || (*after_op == '\\' && *(after_op + 1) == '\n'))) {
                            if (*after_op == '\\') after_op += 2;
                            else after_op++;
                        }
                        char delim[128] = {0};
                        size_t dlen = 0;
                        bool quoted = false;
                        char *dscan = after_op;
                        while (*dscan && dlen < sizeof(delim) - 1) {
                            if (*dscan == '\\' && *(dscan + 1) == '\n') {
                                dscan += 2;
                                continue;
                            }
                            if (isspace((unsigned char)*dscan) || *dscan == '>' || *dscan == '<' || *dscan == ';' || *dscan == '|' || *dscan == '&') {
                                break;
                            }
                            if (*dscan == '\'') {
                                quoted = true;
                                dscan++;
                                while (*dscan && *dscan != '\'' && dlen < sizeof(delim) - 1) {
                                    delim[dlen++] = *dscan++;
                                }
                                if (*dscan == '\'') dscan++;
                            } else if (*dscan == '"') {
                                quoted = true;
                                dscan++;
                                while (*dscan && *dscan != '"' && dlen < sizeof(delim) - 1) {
                                    delim[dlen++] = *dscan++;
                                }
                                if (*dscan == '"') dscan++;
                            } else if (*dscan == '\\') {
                                quoted = true;
                                dscan++;
                                if (*dscan && dlen < sizeof(delim) - 1) {
                                    delim[dlen++] = *dscan++;
                                }
                            } else {
                                delim[dlen++] = *dscan++;
                            }
                        }
                        delim[dlen] = '\0';
                        if (num_hd < 16 && dlen > 0) {
                            hd_specs[num_hd].op_start = op_start;
                            hd_specs[num_hd].op_end = dscan;
                            hd_specs[num_hd].src_fd = sfd;
                            hd_specs[num_hd].strip_tabs = strip_tabs;
                            hd_specs[num_hd].quoted = quoted;
                            strncpy(hd_specs[num_hd].delim, delim, sizeof(hd_specs[num_hd].delim) - 1);
                            num_hd++;
                            c = dscan - 1;
                        }
                    }
                }
            }
        }

        if (num_hd > 0) {
            char *last_semi = NULL;
            bool s_in_s = false, s_in_d = false;
            int s_brace_d = 0, s_paren_d = 0, s_loop_d = 0, s_do_d = 0, s_if_d = 0, s_case_d = 0;
            for (char *c = p; c < hd_specs[0].op_start; c++) {
                if (*c == '\'' && !s_in_d) s_in_s = !s_in_s;
                else if (*c == '"' && !s_in_s) s_in_d = !s_in_d;
                else if (!s_in_s && !s_in_d) {
                    if (*c == '(') s_paren_d++;
                    else if (*c == ')') { if (s_paren_d > 0) s_paren_d--; }
                    else if (*c == '{') s_brace_d++;
                    else if (*c == '}') { if (s_brace_d > 0) s_brace_d--; }
                    else {
                        bool at_word_start = (c == p || isspace((unsigned char)c[-1]) || c[-1] == ';');
                        if (at_word_start) {
                            if (is_word_match(c, "for", 3) || is_word_match(c, "while", 5) || is_word_match(c, "until", 5)) s_loop_d++;
                            else if (is_word_match(c, "do", 2)) s_do_d++;
                            else if (is_word_match(c, "done", 4)) { if (s_do_d > 0) s_do_d--; if (s_loop_d > 0) s_loop_d--; }
                            else if (is_word_match(c, "if", 2)) s_if_d++;
                            else if (is_word_match(c, "fi", 2)) { if (s_if_d > 0) s_if_d--; }
                            else if (is_word_match(c, "case", 4)) s_case_d++;
                            else if (is_word_match(c, "esac", 4)) { if (s_case_d > 0) s_case_d--; }
                        }
                        if (*c == ';' && s_paren_d == 0 && s_brace_d == 0 && s_loop_d == 0 && s_do_d == 0 && s_if_d == 0 && s_case_d == 0) {
                            last_semi = c;
                        }
                    }
                }
            }
            if (last_semi) {
                *last_semi = '\0';
                zbash_run_line(state, p);
                p = last_semi + 1;
                while (*p && isspace((unsigned char)*p)) p++;
            }

            bool read_ok = true;
            for (int s = 0; s < num_hd; s++) {
                snprintf(hd_specs[s].tmp_path, sizeof(hd_specs[s].tmp_path), "/tmp/zbash_hd_XXXXXX");
                int fd = mkstemp(hd_specs[s].tmp_path);
                if (fd < 0) { read_ok = false; break; }
                FILE *hd_fp = fdopen(fd, "w");
                char *hline = NULL;
                size_t hcap = 0;
                while (getline(&hline, &hcap, fp) != -1) {
                    state->lineno++;
                    size_t hlen = strlen(hline);
                    if (hlen > 0 && hline[hlen - 1] == '\n') hline[--hlen] = '\0';
                    if (hlen > 0 && hline[hlen - 1] == '\r') hline[--hlen] = '\0';

                    char *curr_p = hline;
                    if (hd_specs[s].strip_tabs) {
                        while (*curr_p == '\t') curr_p++;
                    }

                    char *merged_alloc = NULL;
                    if (!hd_specs[s].quoted) {
                        while (1) {
                            size_t clen = strlen(curr_p);
                            if (clen > 0 && curr_p[clen - 1] == '\\' && (clen == 1 || curr_p[clen - 2] != '\\')) {
                                curr_p[clen - 1] = '\0';
                                char *next_l = NULL;
                                size_t next_c = 0;
                                if (getline(&next_l, &next_c, fp) != -1) {
                                    state->lineno++;
                                    size_t nlen = strlen(next_l);
                                    if (nlen > 0 && next_l[nlen - 1] == '\n') next_l[--nlen] = '\0';
                                    if (nlen > 0 && next_l[nlen - 1] == '\r') next_l[--nlen] = '\0';
                                    char *np = next_l;
                                    if (hd_specs[s].strip_tabs) {
                                        while (*np == '\t') np++;
                                    }
                                    size_t tot = strlen(curr_p) + strlen(np) + 1;
                                    char *merged = malloc(tot);
                                    strcpy(merged, curr_p);
                                    strcat(merged, np);
                                    free(next_l);
                                    if (merged_alloc) free(merged_alloc);
                                    merged_alloc = merged;
                                    curr_p = merged;
                                } else {
                                    if (next_l) free(next_l);
                                    break;
                                }
                            } else {
                                break;
                            }
                        }
                    }

                    if (strcmp(curr_p, hd_specs[s].delim) == 0) {
                        if (merged_alloc) free(merged_alloc);
                        break;
                    }

                    if (!hd_specs[s].quoted) {
                        char *exp_line = zbash_expand_heredoc(state, curr_p);
                        fprintf(hd_fp, "%s\n", exp_line ? exp_line : curr_p);
                        if (exp_line) free(exp_line);
                    } else {
                        fprintf(hd_fp, "%s\n", curr_p);
                    }
                    if (merged_alloc) free(merged_alloc);
                }
                if (hline) free(hline);
                fclose(hd_fp);
            }

            if (read_ok) {
                size_t cap_needed = strlen(p) + num_hd * 128 + 64;
                char *cmd_mod = malloc(cap_needed);
                if (cmd_mod) {
                    cmd_mod[0] = '\0';
                    char *src_ptr = p;
                    for (int s = 0; s < num_hd; s++) {
                        size_t chunk_len = hd_specs[s].op_start - src_ptr;
                        strncat(cmd_mod, src_ptr, chunk_len);
                        char repl[128];
                        if (hd_specs[s].src_fd == 0) {
                            snprintf(repl, sizeof(repl), "< %s ", hd_specs[s].tmp_path);
                        } else {
                            snprintf(repl, sizeof(repl), "%d< %s ", hd_specs[s].src_fd, hd_specs[s].tmp_path);
                        }
                        strcat(cmd_mod, repl);
                        src_ptr = hd_specs[s].op_end;
                    }
                    strcat(cmd_mod, src_ptr);

                    zbash_run_line(state, cmd_mod);
                    free(cmd_mod);
                }
            }
            for (int s = 0; s < num_hd; s++) {
                unlink(hd_specs[s].tmp_path);
            }
            continue;
        }

        /* Run line / statement */
        int end_l = state->lineno;
        state->lineno = start_l;
        zbash_run_line(state, p);
        state->lineno = end_l;

        if (state->should_exit || state->return_called || state->loop_break || state->loop_continue) {
            break;
        }
    }

    if (line) free(line);
    return state->should_exit ? state->exit_code : state->last_status;
}

int zbash_source_file_args(zbash_state_t *state, const char *filepath, int argc, char **argv) {
    if (!filepath || !*filepath) return 1;
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
        int lineno = state->lineno > 0 ? state->lineno : 0;
        fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, filepath, strerror(errno));
        if (state->posix) {
            state->should_exit = true;
            state->exit_code = 1;
        }
        return 1;
    }
    struct stat st;
    char *content = NULL;
    FILE *fp = NULL;

    if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
        if (st.st_size == 0) {
            close(fd);
            state->last_status = 0;
            return 0;
        }
        size_t size = (size_t)st.st_size;
        content = malloc(size + 1);
        if (content) {
            ssize_t nr = read(fd, content, size);
            close(fd);
            fd = -1;
            if (nr < 0) nr = 0;
            content[nr] = '\0';
            fp = fmemopen(content, nr, "r");
        }
    }

    if (!fp && fd >= 0) {
        int high_fd = fcntl(fd, F_DUPFD_CLOEXEC, 64);
        if (high_fd >= 0) {
            close(fd);
            fd = high_fd;
        }
        fp = fdopen(fd, "r");
    }

    if (!fp) {
        if (fd >= 0) close(fd);
        if (content) free(content);
        return 1;
    }

    if (state->source_depth < ZBASH_MAX_SOURCE_DEPTH) {
        state->source_stack[state->source_depth++] = strdup(filepath);
    }

    int saved_argc = state->argc;
    char **saved_argv = state->argv;
    bool has_args = (argc > 1);
    if (has_args) {
        state->argc = argc;
        state->argv = argv;
        state->positional_params_modified = false;
    }

    int saved_call_line = state->lineno;
    int frame_idx = state->frame_count;
    if (frame_idx < ZBASH_MAX_FRAMES) {
        state->frames[frame_idx].func_name = "source";
        state->frames[frame_idx].file_name = filepath;
        state->frames[frame_idx].call_line = saved_call_line;
        state->frames[frame_idx].def_line = 1;
        state->frame_count++;
        zbash_sync_call_stack_arrays(state);
    }

    char *saved_debug_trap = state->debug_trap ? strdup(state->debug_trap) : NULL;
    char *saved_return_trap = state->return_trap ? strdup(state->return_trap) : NULL;
    if (!state->functrace) {
        if (state->debug_trap) { free(state->debug_trap); state->debug_trap = NULL; }
        if (state->return_trap) { free(state->return_trap); state->return_trap = NULL; }
    }

    int saved_lineno = state->lineno;
    state->lineno = 0;
    int rc = zbash_run_stream(state, fp);
    fclose(fp);
    if (content) free(content);
    state->lineno = saved_lineno;

    if (!state->functrace) {
        if (state->debug_trap) free(state->debug_trap);
        state->debug_trap = saved_debug_trap;
        if (state->return_trap) free(state->return_trap);
        state->return_trap = saved_return_trap;
    } else {
        if (saved_debug_trap) free(saved_debug_trap);
        if (saved_return_trap) free(saved_return_trap);
    }

    if (has_args && !state->positional_params_modified) {
        state->argc = saved_argc;
        state->argv = saved_argv;
    }

    if (state->source_depth > 0) {
        state->source_depth--;
        free(state->source_stack[state->source_depth]);
        state->source_stack[state->source_depth] = NULL;
    }

    if (state->frame_count > 1) {
        state->frame_count--;
        zbash_sync_call_stack_arrays(state);

        if (state->return_trap && !state->in_return_trap && (state->func_depth == 0 || state->functrace)) {
            if (state->functrace) zbash_trigger_debug_trap(state, saved_call_line);
            state->in_return_trap = true;
            int prev_l = state->lineno;
            state->lineno = saved_call_line;
            bool saved_suppress = state->suppress_debug_trap;
            state->suppress_debug_trap = true;
            zbash_run_line(state, state->return_trap);
            state->suppress_debug_trap = saved_suppress;
            state->lineno = prev_l;
            state->in_return_trap = false;
        }
    }

    state->return_called = false;
    return rc;
}

int zbash_source_file(zbash_state_t *state, const char *filepath) {
    return zbash_source_file_args(state, filepath, 0, NULL);
}

int zbash_run_file(zbash_state_t *state, const char *filepath) {
    if (!filepath || !*filepath) return 1;
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "zbash: %s: %s\n", filepath, strerror(errno));
        return 127;
    }

    struct stat st;
    char *content = NULL;
    FILE *fp = NULL;

    if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && st.st_size >= 0) {
        size_t size = (size_t)st.st_size;
        content = malloc(size + 1);
        if (content) {
            ssize_t nr = read(fd, content, size);
            close(fd);
            fd = -1;
            if (nr < 0) nr = 0;
            content[nr] = '\0';
            fp = fmemopen(content, nr, "r");
        }
    }

    if (!fp && fd >= 0) {
        int high_fd = fcntl(fd, F_DUPFD_CLOEXEC, 64);
        if (high_fd >= 0) {
            close(fd);
            fd = high_fd;
        }
        fp = fdopen(fd, "r");
    }

    if (!fp) {
        if (fd >= 0) close(fd);
        if (content) free(content);
        fprintf(stderr, "zbash: %s: %s\n", filepath, strerror(errno));
        return 127;
    }

    if (state->source_depth < ZBASH_MAX_SOURCE_DEPTH) {
        state->source_stack[state->source_depth++] = strdup(filepath);
    }

    if (state->frame_count == 0) {
        state->frames[0].func_name = "main";
        state->frames[0].file_name = filepath;
        state->frames[0].call_line = 0;
        state->frames[0].def_line = 0;
        state->frame_count = 1;
        zbash_sync_call_stack_arrays(state);
    }

    int rc = zbash_run_stream(state, fp);
    fclose(fp);
    if (content) free(content);

    if (state->source_depth > 0) {
        state->source_depth--;
        free(state->source_stack[state->source_depth]);
        state->source_stack[state->source_depth] = NULL;
    }

    return state->should_exit ? state->exit_code : rc;
}

#if defined(__APPLE__) && !defined(__EMSCRIPTEN__)
#include <histedit.h>

static char *zbash_prompt_callback(EditLine *el) {
    zbash_state_t *state = NULL;
    el_get(el, EL_CLIENTDATA, &state);
    return zbash_build_prompt(state, true);
}

int zbash_run_interactive(zbash_state_t *state) {
    if (!isatty(STDIN_FILENO)) {
        return zbash_run_stream(state, stdin);
    }

    state->interactive = true;
    state->expand_aliases = true;
    zbash_load_rc(state);

    EditLine *el = el_init("zbash", stdin, stdout, stderr);
    if (!el) {
        return zbash_run_stream(state, stdin);
    }

    History *hist = history_init();
    HistEvent ev;
    if (hist) {
        history(hist, &ev, H_SETSIZE, 2000);
        history(hist, &ev, H_SETUNIQUE, 1);
    }

    const char *home = getenv("HOME");
    char hist_path[1024];
    if (home && *home) {
        snprintf(hist_path, sizeof(hist_path), "%s/.zbash_history", home);
    } else {
        snprintf(hist_path, sizeof(hist_path), ".zbash_history");
    }

    if (hist) {
        history(hist, &ev, H_LOAD, hist_path);
    }

    el_set(el, EL_CLIENTDATA, state);
    el_set(el, EL_PROMPT_ESC, zbash_prompt_callback, '\1');
    if (hist) {
        el_set(el, EL_HIST, history, hist);
    }
    el_set(el, EL_EDITOR, "emacs");
    el_set(el, EL_ADDFN, "sh-complete", "Filename and command completion", _el_fn_sh_complete);
    el_set(el, EL_BIND, "^I", "sh-complete", NULL);
    el_source(el, NULL);

    int count = 0;
    while (!state->should_exit) {
        zbash_trigger_hook(state, "precmd");

        const char *line = el_gets(el, &count);
        if (!line || count <= 0) break;

        char *trimmed = strdup(line);
        if (!trimmed) continue;

        size_t tlen = strlen(trimmed);
        while (tlen > 0 && (trimmed[tlen - 1] == '\n' || trimmed[tlen - 1] == '\r')) {
            trimmed[--tlen] = '\0';
        }

        if (tlen == 0) {
            free(trimmed);
            continue;
        }

        if (hist) {
            history(hist, &ev, H_ENTER, trimmed);
            FILE *hfp = fopen(hist_path, "a");
            if (hfp) {
                fprintf(hfp, "%s\n", trimmed);
                fclose(hfp);
            }
        }

        struct timespec ts_start, ts_end;
        clock_gettime(CLOCK_MONOTONIC, &ts_start);

        zbash_trigger_preexec(state, trimmed);
        zbash_run_line(state, trimmed);

        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        double elapsed_ms = (ts_end.tv_sec - ts_start.tv_sec) * 1000.0 +
                            (ts_end.tv_nsec - ts_start.tv_nsec) / 1000000.0;
        state->last_cmd_duration_ms = elapsed_ms;
        char dur_str[32];
        snprintf(dur_str, sizeof(dur_str), "%.2f", elapsed_ms);
        setenv("ZBASH_CMD_DURATION", dur_str, 1);

        free(trimmed);
    }

    if (hist) {
        history(hist, &ev, H_SAVE, hist_path);
        history_end(hist);
    }
    el_end(el);

    return state->should_exit ? state->exit_code : state->last_status;
}
#else
int zbash_run_interactive(zbash_state_t *state) {
    if (isatty(STDIN_FILENO)) {
        state->interactive = true;
        state->expand_aliases = true;
        zbash_load_rc(state);

        char *line = NULL;
        size_t cap = 0;
        ssize_t nread;
        while (!state->should_exit) {
            zbash_trigger_hook(state, "precmd");
            fputs(zbash_build_prompt(state, false), stdout);
            fflush(stdout);
            nread = getline(&line, &cap, stdin);
            if (nread == -1) break;
            if (nread > 0 && line[nread - 1] == '\n') line[nread - 1] = '\0';
            if (*line) {
                struct timespec ts_start, ts_end;
                clock_gettime(CLOCK_MONOTONIC, &ts_start);

                zbash_trigger_preexec(state, line);
                zbash_run_line(state, line);

                clock_gettime(CLOCK_MONOTONIC, &ts_end);
                double elapsed_ms = (ts_end.tv_sec - ts_start.tv_sec) * 1000.0 +
                                    (ts_end.tv_nsec - ts_start.tv_nsec) / 1000000.0;
                state->last_cmd_duration_ms = elapsed_ms;
                char dur_str[32];
                snprintf(dur_str, sizeof(dur_str), "%.2f", elapsed_ms);
                setenv("ZBASH_CMD_DURATION", dur_str, 1);
            }
        }
        if (line) free(line);
        return state->should_exit ? state->exit_code : state->last_status;
    } else {
        return zbash_run_stream(state, stdin);
    }
}
#endif
