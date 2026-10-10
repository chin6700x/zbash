/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "parser_internal.h"

static inline size_t skip_cmd_whitespace(const char *cmd_str, size_t i, size_t len) {
    while (i < len) {
        if (isspace((unsigned char)cmd_str[i])) {
            i++;
        } else if (cmd_str[i] == '\\' && i + 1 < len && cmd_str[i + 1] == '\n') {
            i += 2;
        } else {
            break;
        }
    }
    return i;
}

static size_t scan_word(const char *cmd_str, size_t i, size_t len) {
    bool in_s = false;
    bool in_d = false;
    int bdepth = 0;

    while (i < len) {
        char c = cmd_str[i];
        if (c == '\\' && !in_s && i + 1 < len) {
            i += 2;
            continue;
        }
        if (c == '`' && !in_s) {
            i++;
            while (i < len && cmd_str[i] != '`') {
                if (cmd_str[i] == '\\' && i + 1 < len) i += 2;
                else i++;
            }
            if (i < len && cmd_str[i] == '`') i++;
            continue;
        }
        if ((c == '<' || c == '>') && i + 1 < len && cmd_str[i + 1] == '(' && !in_s) {
            int pdepth = 1;
            bool q_stack_s[64] = {false};
            bool q_stack_d[64] = {false};
            i += 2;
            while (i < len && pdepth > 0) {
                char sc = cmd_str[i];
                if (sc == '\\' && i + 1 < len) {
                    i += 2;
                    continue;
                }
                if (sc == '\'' && !q_stack_d[pdepth]) {
                    q_stack_s[pdepth] = !q_stack_s[pdepth];
                } else if (sc == '"' && !q_stack_s[pdepth]) {
                    q_stack_d[pdepth] = !q_stack_d[pdepth];
                } else if (!q_stack_s[pdepth] && !q_stack_d[pdepth]) {
                    if (sc == '(') {
                        if (pdepth < 63) {
                            pdepth++;
                            q_stack_s[pdepth] = false;
                            q_stack_d[pdepth] = false;
                        }
                    } else if (sc == ')') {
                        pdepth--;
                        if (pdepth == 0) { i++; break; }
                    }
                }
                i++;
            }
            continue;
        }
        if (c == '$' && i + 1 < len && cmd_str[i + 1] == '(' && !in_s) {
            int sdepth = 1;
            bool q_stack_s[64] = {false};
            bool q_stack_d[64] = {false};
            i += 2;
            while (i < len && sdepth > 0) {
                char sc = cmd_str[i];
                if (sc == '\\' && i + 1 < len) {
                    i += 2;
                    continue;
                }
                if (sc == '\'' && !q_stack_d[sdepth]) {
                    q_stack_s[sdepth] = !q_stack_s[sdepth];
                } else if (sc == '"' && !q_stack_s[sdepth]) {
                    q_stack_d[sdepth] = !q_stack_d[sdepth];
                } else if (!q_stack_s[sdepth] && !q_stack_d[sdepth]) {
                    if (sc == '$' && i + 1 < len && cmd_str[i + 1] == '(') {
                        if (sdepth < 63) {
                            sdepth++;
                            q_stack_s[sdepth] = false;
                            q_stack_d[sdepth] = false;
                        }
                        i += 2;
                        continue;
                    } else if (sc == '(') {
                        if (sdepth < 63) {
                            sdepth++;
                            q_stack_s[sdepth] = false;
                            q_stack_d[sdepth] = false;
                        }
                    } else if (sc == ')') {
                        sdepth--;
                        if (sdepth == 0) { i++; break; }
                    }
                } else if (q_stack_d[sdepth] && sc == '$' && i + 1 < len && cmd_str[i + 1] == '(') {
                    if (sdepth < 63) {
                        sdepth++;
                        q_stack_s[sdepth] = false;
                        q_stack_d[sdepth] = false;
                    }
                    i += 2;
                    continue;
                }
                i++;
            }
            continue;
        }
        if (c == '$' && i + 1 < len && cmd_str[i + 1] == '{' && !in_s) {
            int b_depth = 1;
            bool br_s = false, br_d = false;
            i += 2;
            while (i < len && b_depth > 0) {
                char bc = cmd_str[i];
                if (bc == '\\' && i + 1 < len) {
                    i += 2;
                    continue;
                }
                if (bc == '`' && !br_s) {
                    i++;
                    while (i < len && cmd_str[i] != '`') {
                        if (cmd_str[i] == '\\' && i + 1 < len) i += 2;
                        else i++;
                    }
                    if (i < len && cmd_str[i] == '`') i++;
                    continue;
                }
                if (bc == '$' && i + 1 < len && cmd_str[i + 1] == '(' && !br_s) {
                    int pdepth = 1;
                    i += 2;
                    while (i < len && pdepth > 0) {
                        if (cmd_str[i] == '\\' && i + 1 < len) { i += 2; continue; }
                        if (cmd_str[i] == '(') pdepth++;
                        else if (cmd_str[i] == ')') { pdepth--; if (pdepth == 0) { i++; break; } }
                        i++;
                    }
                    continue;
                }
                if (bc == '\'' && !br_d) br_s = !br_s;
                else if (bc == '"' && !br_s) br_d = !br_d;
                else if (!br_s && !br_d) {
                    if (bc == '{' && i > 0 && cmd_str[i - 1] == '$') b_depth++;
                    else if (bc == '}') {
                        b_depth--;
                        if (b_depth == 0) { i++; break; }
                    }
                }
                i++;
            }
            continue;
        }
        if (c == '(' && !in_s && !in_d && i > 0 && (cmd_str[i - 1] == '=' || (i > 1 && cmd_str[i - 2] == '='))) {
            int pdepth = 1;
            bool sub_s = false;
            bool sub_d = false;
            i++;
            while (i < len && pdepth > 0) {
                char sc = cmd_str[i];
                if (sc == '\\' && i + 1 < len) {
                    i += 2;
                    continue;
                }
                if (sc == '\'' && !sub_d) sub_s = !sub_s;
                else if (sc == '"' && !sub_s) sub_d = !sub_d;
                else if (!sub_s && !sub_d) {
                    if (sc == '(') pdepth++;
                    else if (sc == ')') {
                        pdepth--;
                        if (pdepth == 0) { i++; break; }
                    }
                }
                i++;
            }
            continue;
        }
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;
        else if (!in_s && !in_d) {
            if (c == '[' && i + 1 < len && !isspace((unsigned char)cmd_str[i + 1])) {
                bdepth++;
            } else if (c == ']' && bdepth > 0) {
                bdepth--;
            } else if (bdepth == 0 && (isspace((unsigned char)c) || c == '>' || c == '<' || (c == '&' && i + 1 < len && cmd_str[i + 1] == '>'))) {
                break;
            }
        }
        i++;

#if defined(ZBASH_USE_NEON)
        if (!in_s && !in_d && bdepth == 0 && i < len) {
            size_t skip = zbash_simd_scan_token(cmd_str + i, len - i);
            if (skip > 0) {
                for (size_t s = 0; s < skip; s++) {
                    if (cmd_str[i + s] == '=' || cmd_str[i + s] == '(' || cmd_str[i + s] == ')' || cmd_str[i + s] == '`' || cmd_str[i + s] == '[') {
                        skip = s;
                        break;
                    }
                }
                i += skip;
            }
        }
#endif
    }
    return i;
}

static bool parse_redir(zbash_state_t *state, const char *cmd_str, size_t *pos, size_t len, zbash_cmd_t *cmd) {
    size_t i = *pos;
    int explicit_fd = -1;

    /* Check optional leading file descriptor digits */
    if (isdigit((unsigned char)cmd_str[i])) {
        size_t d = i;
        while (d < len && isdigit((unsigned char)cmd_str[d])) d++;
        if (d < len && (cmd_str[d] == '<' || cmd_str[d] == '>')) {
            explicit_fd = 0;
            for (size_t k = i; k < d; k++) {
                explicit_fd = explicit_fd * 10 + (cmd_str[k] - '0');
            }
            i = d;
        } else {
            return false;
        }
    }

    if (cmd_str[i] != '<' && cmd_str[i] != '>' && cmd_str[i] != '&') {
        return false;
    }

    /* &> or &>> */
    if (cmd_str[i] == '&' && i + 1 < len && cmd_str[i + 1] == '>') {
        i += 2;
        bool append = false;
        if (i < len && cmd_str[i] == '>') {
            append = true;
            i++;
        }
        i = skip_cmd_whitespace(cmd_str, i, len);
        size_t start = i;
        i = scan_word(cmd_str, i, len);
        char tmp[512];
        size_t flen = (i - start < sizeof(tmp) - 1) ? (i - start) : sizeof(tmp) - 1;
        strncpy(tmp, cmd_str + start, flen);
        tmp[flen] = '\0';
        char *exp = zbash_expand_vars(state, tmp);
        char *final_fn = exp ? exp : strdup(tmp);

        if (cmd->redir_count + 2 <= ZBASH_MAX_REDIRS) {
            cmd->redirs[cmd->redir_count].type = append ? REDIR_APPEND : REDIR_OUT;
            cmd->redirs[cmd->redir_count].src_fd = 1;
            cmd->redirs[cmd->redir_count].target_fd = -1;
            cmd->redirs[cmd->redir_count].filename = strdup(final_fn);
            cmd->redir_count++;

            cmd->redirs[cmd->redir_count].type = REDIR_DUP_OUT;
            cmd->redirs[cmd->redir_count].src_fd = 2;
            cmd->redirs[cmd->redir_count].target_fd = 1;
            cmd->redirs[cmd->redir_count].filename = NULL;
            cmd->redir_count++;
        }
        if (cmd->out_file) free(cmd->out_file);
        cmd->out_file = strdup(final_fn);
        cmd->append_out = append;
        cmd->err_to_out = true;
        free(final_fn);
        *pos = i;
        return true;
    }

    /* Here-string <<< */
    if (cmd_str[i] == '<' && i + 2 < len && cmd_str[i + 1] == '<' && cmd_str[i + 2] == '<') {
        i += 3;
        i = skip_cmd_whitespace(cmd_str, i, len);
        size_t start = i;
        i = scan_word(cmd_str, i, len);
        char tmp[2048];
        size_t flen = (i - start < sizeof(tmp) - 1) ? (i - start) : sizeof(tmp) - 1;
        strncpy(tmp, cmd_str + start, flen);
        tmp[flen] = '\0';
        char *exp = zbash_expand_vars(state, tmp);
        const char *content = exp ? exp : tmp;
        char hs_template[] = "/tmp/zbash_hs_XXXXXX";
        int fd = mkstemp(hs_template);
        if (fd >= 0) {
            ssize_t nw = write(fd, content, strlen(content));
            (void)nw;
            nw = write(fd, "\n", 1);
            (void)nw;
            close(fd);
            int sfd = (explicit_fd >= 0 ? explicit_fd : 0);
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_IN;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(hs_template);
                cmd->redir_count++;
            }
            if (sfd == 0) {
                if (cmd->in_file) {
                    if (strncmp(cmd->in_file, "/tmp/zbash_hs_", 14) == 0) unlink(cmd->in_file);
                    free(cmd->in_file);
                }
                cmd->in_file = strdup(hs_template);
            }
        }
        if (exp) free(exp);
        *pos = i;
        return true;
    }

    /* <& dup / close input */
    if (cmd_str[i] == '<' && i + 1 < len && cmd_str[i + 1] == '&') {
        i += 2;
        i = skip_cmd_whitespace(cmd_str, i, len);
        int sfd = (explicit_fd >= 0 ? explicit_fd : 0);
        size_t start = i;
        i = scan_word(cmd_str, i, len);
        char tmp[512];
        size_t flen = (i - start < sizeof(tmp) - 1) ? (i - start) : sizeof(tmp) - 1;
        strncpy(tmp, cmd_str + start, flen);
        tmp[flen] = '\0';

        char *exp = zbash_expand_vars(state, tmp);
        const char *val = exp ? exp : tmp;
        bool has_var = (strchr(tmp, '$') != NULL);

        if (has_var && *val == '\0') {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_AMBIGUOUS;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(tmp);
                cmd->redir_count++;
            }
        } else if (strcmp(val, "-") == 0) {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_CLOSE_IN;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = NULL;
                cmd->redir_count++;
            }
        } else {
            bool all_digits = true;
            bool digit_minus = false;
            size_t vlen = strlen(val);
            if (vlen > 1 && val[vlen - 1] == '-') {
                digit_minus = true;
                for (size_t k = 0; k < vlen - 1; k++) {
                    if (!isdigit((unsigned char)val[k])) { all_digits = false; break; }
                }
            } else if (vlen > 0) {
                for (size_t k = 0; k < vlen; k++) {
                    if (!isdigit((unsigned char)val[k])) { all_digits = false; break; }
                }
            } else {
                all_digits = false;
            }

            if (all_digits) {
                int tfd = atoi(val);
                if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                    cmd->redirs[cmd->redir_count].type = REDIR_DUP_IN;
                    cmd->redirs[cmd->redir_count].src_fd = sfd;
                    cmd->redirs[cmd->redir_count].target_fd = tfd;
                    cmd->redirs[cmd->redir_count].filename = NULL;
                    cmd->redir_count++;
                }
                if (digit_minus && cmd->redir_count < ZBASH_MAX_REDIRS) {
                    cmd->redirs[cmd->redir_count].type = REDIR_CLOSE_IN;
                    cmd->redirs[cmd->redir_count].src_fd = tfd;
                    cmd->redirs[cmd->redir_count].target_fd = -1;
                    cmd->redirs[cmd->redir_count].filename = NULL;
                    cmd->redir_count++;
                }
            } else {
                if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                    cmd->redirs[cmd->redir_count].type = REDIR_AMBIGUOUS;
                    cmd->redirs[cmd->redir_count].src_fd = sfd;
                    cmd->redirs[cmd->redir_count].target_fd = -1;
                    cmd->redirs[cmd->redir_count].filename = strdup(tmp);
                    cmd->redir_count++;
                }
            }
        }
        if (exp) free(exp);
        *pos = i;
        return true;
    }

    /* <> open read-write */
    if (cmd_str[i] == '<' && i + 1 < len && cmd_str[i + 1] == '>') {
        i += 2;
        i = skip_cmd_whitespace(cmd_str, i, len);
        size_t start = i;
        i = scan_word(cmd_str, i, len);
        char tmp[512];
        size_t flen = (i - start < sizeof(tmp) - 1) ? (i - start) : sizeof(tmp) - 1;
        strncpy(tmp, cmd_str + start, flen);
        tmp[flen] = '\0';
        char *exp = zbash_expand_vars(state, tmp);
        char *fn = exp ? exp : strdup(tmp);
        int sfd = (explicit_fd >= 0 ? explicit_fd : 0);
        bool has_var = (strchr(tmp, '$') != NULL);
        if (has_var && tmp[0] != '"' && tmp[0] != '\'' && (fn[0] == '\0' || strpbrk(fn, " \t\n") != NULL)) {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_AMBIGUOUS;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(tmp);
                cmd->redir_count++;
            }
        } else {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_RDWR;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(fn);
                cmd->redir_count++;
            }
        }
        free(fn);
        *pos = i;
        return true;
    }

    /* < open read */
    if (cmd_str[i] == '<') {
        if (!state->posix && i + 1 < len && cmd_str[i + 1] == '(') {
            return false;
        }
        i++;
        i = skip_cmd_whitespace(cmd_str, i, len);
        size_t start = i;
        i = scan_word(cmd_str, i, len);
        char tmp[512];
        size_t flen = (i - start < sizeof(tmp) - 1) ? (i - start) : sizeof(tmp) - 1;
        strncpy(tmp, cmd_str + start, flen);
        tmp[flen] = '\0';
        char *exp = zbash_expand_vars(state, tmp);
        char *fn = exp ? exp : strdup(tmp);
        int sfd = (explicit_fd >= 0 ? explicit_fd : 0);
        bool has_var = (strchr(tmp, '$') != NULL);
        if (has_var && tmp[0] != '"' && tmp[0] != '\'' && (fn[0] == '\0' || strpbrk(fn, " \t\n") != NULL)) {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_AMBIGUOUS;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(tmp);
                cmd->redir_count++;
            }
        } else {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_IN;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(fn);
                cmd->redir_count++;
            }
            if (sfd == 0) {
                if (cmd->in_file) {
                    if (strncmp(cmd->in_file, "/tmp/zbash_hs_", 14) == 0) unlink(cmd->in_file);
                    free(cmd->in_file);
                }
                cmd->in_file = strdup(fn);
            }
        }
        free(fn);
        *pos = i;
        return true;
    }

    /* >& dup / close output, or >& file */
    if (cmd_str[i] == '>' && i + 1 < len && cmd_str[i + 1] == '&') {
        i += 2;
        i = skip_cmd_whitespace(cmd_str, i, len);
        int sfd = (explicit_fd >= 0 ? explicit_fd : 1);
        size_t start = i;
        i = scan_word(cmd_str, i, len);
        char tmp[512];
        size_t flen = (i - start < sizeof(tmp) - 1) ? (i - start) : sizeof(tmp) - 1;
        strncpy(tmp, cmd_str + start, flen);
        tmp[flen] = '\0';

        char *exp = zbash_expand_vars(state, tmp);
        const char *val = exp ? exp : tmp;
        bool has_var = (strchr(tmp, '$') != NULL);

        if (has_var && *val == '\0') {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_AMBIGUOUS;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(tmp);
                cmd->redir_count++;
            }
        } else if (strcmp(val, "-") == 0) {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_CLOSE_OUT;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = NULL;
                cmd->redir_count++;
            }
        } else {
            bool all_digits = true;
            bool digit_minus = false;
            size_t vlen = strlen(val);
            if (vlen > 1 && val[vlen - 1] == '-') {
                digit_minus = true;
                for (size_t k = 0; k < vlen - 1; k++) {
                    if (!isdigit((unsigned char)val[k])) { all_digits = false; break; }
                }
            } else if (vlen > 0) {
                for (size_t k = 0; k < vlen; k++) {
                    if (!isdigit((unsigned char)val[k])) { all_digits = false; break; }
                }
            } else {
                all_digits = false;
            }

            if (all_digits) {
                int tfd = atoi(val);
                if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                    cmd->redirs[cmd->redir_count].type = REDIR_DUP_OUT;
                    cmd->redirs[cmd->redir_count].src_fd = sfd;
                    cmd->redirs[cmd->redir_count].target_fd = tfd;
                    cmd->redirs[cmd->redir_count].filename = NULL;
                    cmd->redir_count++;
                }
                if (digit_minus && cmd->redir_count < ZBASH_MAX_REDIRS) {
                    cmd->redirs[cmd->redir_count].type = REDIR_CLOSE_OUT;
                    cmd->redirs[cmd->redir_count].src_fd = tfd;
                    cmd->redirs[cmd->redir_count].target_fd = -1;
                    cmd->redirs[cmd->redir_count].filename = NULL;
                    cmd->redir_count++;
                }
                if (sfd == 2 && tfd == 1) cmd->err_to_out = true;
                if (sfd == 1 && tfd == 2) cmd->out_to_err = true;
            } else if (explicit_fd < 0 || explicit_fd == 1) {
                /* >& file : redirect both stdout and stderr */
                if (has_var && tmp[0] != '"' && tmp[0] != '\'' && strpbrk(val, " \t\n") != NULL) {
                    if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                        cmd->redirs[cmd->redir_count].type = REDIR_AMBIGUOUS;
                        cmd->redirs[cmd->redir_count].src_fd = 1;
                        cmd->redirs[cmd->redir_count].target_fd = -1;
                        cmd->redirs[cmd->redir_count].filename = strdup(tmp);
                        cmd->redir_count++;
                    }
                } else {
                    if (cmd->redir_count + 2 <= ZBASH_MAX_REDIRS) {
                        cmd->redirs[cmd->redir_count].type = REDIR_OUT;
                        cmd->redirs[cmd->redir_count].src_fd = 1;
                        cmd->redirs[cmd->redir_count].target_fd = -1;
                        cmd->redirs[cmd->redir_count].filename = strdup(val);
                        cmd->redir_count++;
                        cmd->redirs[cmd->redir_count].type = REDIR_DUP_OUT;
                        cmd->redirs[cmd->redir_count].src_fd = 2;
                        cmd->redirs[cmd->redir_count].target_fd = 1;
                        cmd->redirs[cmd->redir_count].filename = NULL;
                        cmd->redir_count++;
                    }
                    if (cmd->out_file) free(cmd->out_file);
                    cmd->out_file = strdup(val);
                    cmd->err_to_out = true;
                }
            } else {
                if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                    cmd->redirs[cmd->redir_count].type = REDIR_AMBIGUOUS;
                    cmd->redirs[cmd->redir_count].src_fd = sfd;
                    cmd->redirs[cmd->redir_count].target_fd = -1;
                    cmd->redirs[cmd->redir_count].filename = strdup(tmp);
                    cmd->redir_count++;
                }
            }
        }
        if (exp) free(exp);
        *pos = i;
        return true;
    }

    /* >| clobber */
    if (cmd_str[i] == '>' && i + 1 < len && cmd_str[i + 1] == '|') {
        i += 2;
        i = skip_cmd_whitespace(cmd_str, i, len);
        size_t start = i;
        i = scan_word(cmd_str, i, len);
        char tmp[512];
        size_t flen = (i - start < sizeof(tmp) - 1) ? (i - start) : sizeof(tmp) - 1;
        strncpy(tmp, cmd_str + start, flen);
        tmp[flen] = '\0';
        char *exp = zbash_expand_vars(state, tmp);
        char *fn = exp ? exp : strdup(tmp);
        int sfd = (explicit_fd >= 0 ? explicit_fd : 1);
        bool has_var = (strchr(tmp, '$') != NULL);
        if (has_var && tmp[0] != '"' && tmp[0] != '\'' && (fn[0] == '\0' || strpbrk(fn, " \t\n") != NULL)) {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_AMBIGUOUS;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(tmp);
                cmd->redir_count++;
            }
        } else {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_CLOBBER;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(fn);
                cmd->redir_count++;
            }
            if (sfd == 1) {
                if (cmd->out_file) free(cmd->out_file);
                cmd->out_file = strdup(fn);
                cmd->append_out = false;
            }
        }
        free(fn);
        *pos = i;
        return true;
    }

    /* >> append or > truncate */
    if (cmd_str[i] == '>') {
        if (!state->posix && i + 1 < len && cmd_str[i + 1] == '(') {
            return false;
        }
        i++;
        bool append = false;
        if (i < len && cmd_str[i] == '>') {
            append = true;
            i++;
        }
        i = skip_cmd_whitespace(cmd_str, i, len);
        size_t start = i;
        i = scan_word(cmd_str, i, len);
        char tmp[512];
        size_t flen = (i - start < sizeof(tmp) - 1) ? (i - start) : sizeof(tmp) - 1;
        strncpy(tmp, cmd_str + start, flen);
        tmp[flen] = '\0';
        char *exp = zbash_expand_vars(state, tmp);
        char *fn = exp ? exp : strdup(tmp);
        int sfd = (explicit_fd >= 0 ? explicit_fd : 1);
        bool has_var = (strchr(tmp, '$') != NULL);
        if (has_var && tmp[0] != '"' && tmp[0] != '\'' && (fn[0] == '\0' || strpbrk(fn, " \t\n") != NULL)) {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = REDIR_AMBIGUOUS;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(tmp);
                cmd->redir_count++;
            }
        } else {
            if (cmd->redir_count < ZBASH_MAX_REDIRS) {
                cmd->redirs[cmd->redir_count].type = append ? REDIR_APPEND : REDIR_OUT;
                cmd->redirs[cmd->redir_count].src_fd = sfd;
                cmd->redirs[cmd->redir_count].target_fd = -1;
                cmd->redirs[cmd->redir_count].filename = strdup(fn);
                cmd->redir_count++;
            }
            if (sfd == 1) {
                if (cmd->out_file) free(cmd->out_file);
                cmd->out_file = strdup(fn);
                cmd->append_out = append;
            } else if (sfd == 2) {
                if (cmd->err_file) free(cmd->err_file);
                cmd->err_file = strdup(fn);
                cmd->append_err = append;
            }
        }
        free(fn);
        *pos = i;
        return true;
    }

    return false;
}

static bool has_unquoted_expansion(zbash_state_t *state, const char *word) {
    if (!word) return false;
    bool in_s = false, in_d = false;
    for (size_t i = 0; word[i]; i++) {
        char c = word[i];
        if (c == '\\' && !in_s) {
            if (word[i + 1]) i++;
            continue;
        }
        if (c == '\'' && !in_d) {
            in_s = !in_s;
            continue;
        }
        if (c == '"' && !in_s) {
            in_d = !in_d;
            continue;
        }
        if (in_d) {
            if (c == '$' && word[i + 1] == '{') {
                int b_depth = 1;
                bool br_s = false, br_d = false;
                i += 2;
                while (word[i] && b_depth > 0) {
                    if (word[i] == '\\' && word[i + 1]) { i += 2; continue; }
                    if (word[i] == '`' && !br_s) {
                        i++;
                        while (word[i] && word[i] != '`') {
                            if (word[i] == '\\' && word[i + 1]) i += 2;
                            else i++;
                        }
                        if (word[i] == '`') i++;
                        continue;
                    }
                    if (word[i] == '\'' && !br_d) br_s = !br_s;
                    else if (word[i] == '"' && !br_s) br_d = !br_d;
                    else if (!br_s && !br_d) {
                        if (word[i] == '{' && i > 0 && word[i - 1] == '$') b_depth++;
                        else if (word[i] == '}') {
                            b_depth--;
                            if (b_depth == 0) break;
                        }
                    }
                    i++;
                }
                continue;
            }
            if (c == '$' && word[i + 1] == '(') {
                int p_depth = 1;
                i += 2;
                while (word[i] && p_depth > 0) {
                    if (word[i] == '\\' && word[i + 1]) { i += 2; continue; }
                    if (word[i] == '(') p_depth++;
                    else if (word[i] == ')') {
                        p_depth--;
                        if (p_depth == 0) break;
                    }
                    i++;
                }
                continue;
            }
            continue;
        }
        if (!in_s && !in_d) {
            if (c == '`') return true;
            if (c == '$') {
                char next = word[i + 1];
                if (next == '\'' || next == '"') continue;
                if (next == '{') {
                    const char *inner = word + i + 2;
                    char vname[128] = {0};
                    size_t vi = 0;
                    while (inner[vi] && inner[vi] != '}' && inner[vi] != ':' && inner[vi] != '-' &&
                           inner[vi] != '=' && inner[vi] != '+' && inner[vi] != '/' && inner[vi] != '%' &&
                           vi < sizeof(vname) - 1) {
                        vname[vi] = inner[vi];
                        vi++;
                    }
                    vname[vi] = '\0';
                    const char *op = inner + vi;
                    if (*op == ':' && (op[1] == '-' || op[1] == '=')) {
                        const char *rhs = op + 2;
                        size_t rlen = strlen(rhs);
                        if (rlen > 0 && rhs[rlen - 1] == '}') rlen--;
                        bool rhs_quoted = (rlen >= 2 && ((rhs[0] == '"' && rhs[rlen - 1] == '"') || (rhs[0] == '\'' && rhs[rlen - 1] == '\'')));
                        char num_buf[32];
                        const char *val = NULL;
                        char *dyn = NULL;
                        bool is_set = zbash_get_param_value(state, vname, num_buf, sizeof(num_buf), &val, &dyn);
                        if (dyn) free(dyn);
                        if ((!is_set || !val || *val == '\0') && rhs_quoted) {
                            int b_depth = 1;
                            i += 2;
                            while (word[i] && b_depth > 0) {
                                if (word[i] == '\\' && word[i + 1]) { i += 2; continue; }
                                if (word[i] == '{' && i > 0 && word[i - 1] == '$') b_depth++;
                                else if (word[i] == '}') { b_depth--; if (b_depth == 0) break; }
                                i++;
                            }
                            continue;
                        }
                    } else if (*op == '-' || *op == '=') {
                        const char *rhs = op + 1;
                        size_t rlen = strlen(rhs);
                        if (rlen > 0 && rhs[rlen - 1] == '}') rlen--;
                        bool rhs_quoted = (rlen >= 2 && ((rhs[0] == '"' && rhs[rlen - 1] == '"') || (rhs[0] == '\'' && rhs[rlen - 1] == '\'')));
                        char num_buf[32];
                        const char *val = NULL;
                        char *dyn = NULL;
                        bool is_set = zbash_get_param_value(state, vname, num_buf, sizeof(num_buf), &val, &dyn);
                        if (dyn) free(dyn);
                        if (!is_set && rhs_quoted) {
                            int b_depth = 1;
                            i += 2;
                            while (word[i] && b_depth > 0) {
                                if (word[i] == '\\' && word[i + 1]) { i += 2; continue; }
                                if (word[i] == '{' && i > 0 && word[i - 1] == '$') b_depth++;
                                else if (word[i] == '}') { b_depth--; if (b_depth == 0) break; }
                                i++;
                            }
                            continue;
                        }
                    }
                    return true;
                }
                if (isalpha((unsigned char)next) || next == '_' ||
                    next == '(' || isdigit((unsigned char)next) || next == '?' ||
                    next == '$' || next == '!' || next == '#' || next == '@' ||
                    next == '*' || next == '-') {
                    return true;
                }
            }
        }
    }
    return false;
}


static bool word_has_explicit_quotes(const char *word) {
    if (!word) return false;
    for (size_t i = 0; word[i]; i++) {
        if (word[i] == '\\') {
            if (word[i + 1]) i++;
            continue;
        }
        if (word[i] == '`') {
            i++;
            while (word[i] && word[i] != '`') {
                if (word[i] == '\\' && word[i + 1]) i += 2;
                else i++;
            }
            if (!word[i]) break;
            continue;
        }
        if (word[i] == '$' && word[i + 1] == '(') {
            int p_depth = 1;
            i += 2;
            while (word[i] && p_depth > 0) {
                if (word[i] == '\\' && word[i + 1]) { i += 2; continue; }
                if (word[i] == '(') p_depth++;
                else if (word[i] == ')') {
                    p_depth--;
                    if (p_depth == 0) break;
                }
                i++;
            }
            if (!word[i]) break;
            continue;
        }
        if (word[i] == '\'' || word[i] == '"') {
            return true;
        }
    }
    return false;
}

static void add_field_to_cmd(zbash_state_t *state, zbash_cmd_t *cmd, const char *field, bool was_quoted) {
    char **gmatches = NULL;
    int gcount = 0;
    if (!was_quoted && zbash_expand_path_glob(state, field, &gmatches, &gcount) > 0) {
        for (int m = 0; m < gcount; m++) {
            if (cmd->argc < ZBASH_MAX_ARGS - 1) cmd->argv[cmd->argc++] = gmatches[m];
            else free(gmatches[m]);
        }
        free(gmatches);
    } else {
        if (cmd->argc < ZBASH_MAX_ARGS - 1) cmd->argv[cmd->argc++] = strdup(field);
    }
}

static void expand_and_split_word(zbash_state_t *state, zbash_cmd_t *cmd, const char *word) {
    const char *ifs = getenv("IFS");
    if (!ifs) ifs = " \t\n";

    #define IS_IFS(c) (strchr(ifs, (c)) != NULL)
    #define IS_IFS_WS(c) (((c) == ' ' || (c) == '\t' || (c) == '\n') && IS_IFS(c))
    #define IS_IFS_NWS(c) (!IS_IFS_WS(c) && IS_IFS(c))

    size_t cap = 2048;
    char *cur = malloc(cap);
    size_t cur_len = 0;
    bool has_content = false;
    bool any_field_emitted = false;
    bool word_quoted = false;

    size_t wlen = strlen(word);
    size_t i = 0;
    while (i < wlen) {
        char c = word[i];
        if (c == '\'') {
            word_quoted = true;
            has_content = true;
            i++;
            while (i < wlen && word[i] != '\'') {
                if (cur_len + 1 >= cap) { cap *= 2; cur = realloc(cur, cap); }
                cur[cur_len++] = word[i++];
            }
            if (i < wlen && word[i] == '\'') i++;
            continue;
        }
        if (c == '"') {
            word_quoted = true;
            has_content = true;
            size_t qstart = i;
            i++;
            while (i < wlen) {
                if (word[i] == '\\' && i + 1 < wlen) { i += 2; continue; }
                if (word[i] == '"') break;
                i++;
            }
            size_t qend = (i < wlen) ? i + 1 : i;
            size_t sub_len = qend - qstart;
            char *sub = malloc(sub_len + 1);
            memcpy(sub, word + qstart, sub_len);
            sub[sub_len] = '\0';
            char *exp_sub = zbash_expand_vars(state, sub);
            free(sub);
            if (exp_sub) {
                size_t elen = strlen(exp_sub);
                while (cur_len + elen >= cap) { cap = (cap + elen) * 2; cur = realloc(cur, cap); }
                memcpy(cur + cur_len, exp_sub, elen);
                cur_len += elen;
                free(exp_sub);
            }
            if (i < wlen && word[i] == '"') i++;
            continue;
        }
        if (c == '\\' && i + 1 < wlen) {
            has_content = true;
            word_quoted = true;
            if (cur_len + 1 >= cap) { cap *= 2; cur = realloc(cur, cap); }
            cur[cur_len++] = word[i + 1];
            i += 2;
            continue;
        }
        if (c == '$' || c == '`') {
            size_t vstart = i;
            size_t vend = i;
            if (c == '`') {
                vend = i + 1;
                while (vend < wlen && word[vend] != '`') {
                    if (word[vend] == '\\' && vend + 1 < wlen) vend += 2;
                    else vend++;
                }
                if (vend < wlen && word[vend] == '`') vend++;
            } else if (word[i + 1] == '(') {
                int pdepth = 1;
                bool q_stack_s[64] = {false};
                bool q_stack_d[64] = {false};
                vend = i + 2;
                while (vend < wlen) {
                    char sc = word[vend];
                    if (sc == '\\' && vend + 1 < wlen) { vend += 2; continue; }
                    if (sc == '\'' && !q_stack_d[pdepth]) {
                        q_stack_s[pdepth] = !q_stack_s[pdepth];
                    } else if (sc == '"' && !q_stack_s[pdepth]) {
                        q_stack_d[pdepth] = !q_stack_d[pdepth];
                    } else if (sc == '`' && !q_stack_s[pdepth]) {
                        vend++;
                        while (vend < wlen && word[vend] != '`') {
                            if (word[vend] == '\\' && vend + 1 < wlen) vend += 2;
                            else vend++;
                        }
                        if (vend < wlen && word[vend] == '`') vend++;
                        continue;
                    } else if (!q_stack_s[pdepth] && !q_stack_d[pdepth]) {
                        if (sc == '$' && vend + 1 < wlen && word[vend + 1] == '(') {
                            if (pdepth < 63) {
                                pdepth++;
                                q_stack_s[pdepth] = false;
                                q_stack_d[pdepth] = false;
                            }
                            vend += 2;
                            continue;
                        } else if (sc == '(') {
                            if (pdepth < 63) {
                                pdepth++;
                                q_stack_s[pdepth] = false;
                                q_stack_d[pdepth] = false;
                            }
                        } else if (sc == ')') {
                            pdepth--;
                            if (pdepth == 0) { vend++; break; }
                        }
                    } else if (q_stack_d[pdepth] && sc == '$' && vend + 1 < wlen && word[vend + 1] == '(') {
                        if (pdepth < 63) {
                            pdepth++;
                            q_stack_s[pdepth] = false;
                            q_stack_d[pdepth] = false;
                        }
                        vend += 2;
                        continue;
                    }
                    vend++;
                }
            } else if (word[i + 1] == '{') {
                int bdepth = 1;
                bool b_stack_s[64] = {false};
                bool b_stack_d[64] = {false};
                vend = i + 2;
                while (vend < wlen) {
                    char sc = word[vend];
                    if (sc == '\\' && vend + 1 < wlen) { vend += 2; continue; }
                    if (sc == '\'' && !b_stack_d[bdepth]) {
                        b_stack_s[bdepth] = !b_stack_s[bdepth];
                    } else if (sc == '"' && !b_stack_s[bdepth]) {
                        b_stack_d[bdepth] = !b_stack_d[bdepth];
                    } else if (sc == '`' && !b_stack_s[bdepth]) {
                        vend++;
                        while (vend < wlen && word[vend] != '`') {
                            if (word[vend] == '\\' && vend + 1 < wlen) vend += 2;
                            else vend++;
                        }
                        if (vend < wlen && word[vend] == '`') vend++;
                        continue;
                    } else if (sc == '$' && vend + 1 < wlen && word[vend + 1] == '(' && !b_stack_s[bdepth]) {
                        int pdepth = 1;
                        bool p_s = false, p_d = false;
                        vend += 2;
                        while (vend < wlen && pdepth > 0) {
                            if (word[vend] == '\\' && vend + 1 < wlen) { vend += 2; continue; }
                            if (word[vend] == '\'' && !p_d) p_s = !p_s;
                            else if (word[vend] == '"' && !p_s) p_d = !p_d;
                            else if (!p_s && !p_d) {
                                if (word[vend] == '(') pdepth++;
                                else if (word[vend] == ')') pdepth--;
                            }
                            vend++;
                        }
                        continue;
                    } else if (!b_stack_s[bdepth] && !b_stack_d[bdepth]) {
                        if (sc == '$' && vend + 1 < wlen && word[vend + 1] == '{') {
                            if (bdepth < 63) {
                                bdepth++;
                                b_stack_s[bdepth] = false;
                                b_stack_d[bdepth] = false;
                            }
                            vend += 2;
                            continue;
                        } else if (sc == '{') {
                            if (bdepth < 63) {
                                bdepth++;
                                b_stack_s[bdepth] = false;
                                b_stack_d[bdepth] = false;
                            }
                        } else if (sc == '}') {
                            bdepth--;
                            if (bdepth == 0) { vend++; break; }
                        }
                    } else if (b_stack_d[bdepth] && sc == '$' && vend + 1 < wlen && word[vend + 1] == '{') {
                        if (bdepth < 63) {
                            bdepth++;
                            b_stack_s[bdepth] = false;
                            b_stack_d[bdepth] = false;
                        }
                        vend += 2;
                        continue;
                    }
                    vend++;
                }
            } else if (isalpha((unsigned char)word[i + 1]) || word[i + 1] == '_') {
                vend = i + 2;
                while (vend < wlen && (isalnum((unsigned char)word[vend]) || word[vend] == '_')) vend++;
            } else if (isdigit((unsigned char)word[i + 1]) || word[i + 1] == '?' || word[i + 1] == '$' ||
                       word[i + 1] == '!' || word[i + 1] == '#' || word[i + 1] == '@' || word[i + 1] == '*' ||
                       word[i + 1] == '-') {
                vend = i + 2;
            }

            if (vend > vstart) {
                size_t vlen = vend - vstart;
                char *vsub = malloc(vlen + 1);
                memcpy(vsub, word + vstart, vlen);
                vsub[vlen] = '\0';
                char *exp_val = zbash_expand_vars(state, vsub);
                free(vsub);

                if (exp_val) {
                    if (*ifs == '\0') {
                        size_t elen = strlen(exp_val);
                        while (cur_len + elen >= cap) { cap = (cap + elen) * 2; cur = realloc(cur, cap); }
                        memcpy(cur + cur_len, exp_val, elen);
                        cur_len += elen;
                        if (elen > 0) has_content = true;
                    } else {
                        bool has_ifs = false;
                        for (size_t k = 0; exp_val[k]; k++) {
                            if (IS_IFS(exp_val[k])) { has_ifs = true; break; }
                        }
                        if (!has_ifs) {
                            size_t elen = strlen(exp_val);
                            while (cur_len + elen >= cap) { cap = (cap + elen) * 2; cur = realloc(cur, cap); }
                            memcpy(cur + cur_len, exp_val, elen);
                            cur_len += elen;
                            if (elen > 0) has_content = true;
                        } else {
                            char *p = exp_val;
                            if (cur_len == 0 && !has_content) {
                                while (*p && IS_IFS_WS(*p)) p++;
                            }
                            while (*p) {
                                char *field_start = p;
                                while (*p && !IS_IFS(*p)) p++;
                                size_t flen = p - field_start;
                                while (cur_len + flen >= cap) { cap = (cap + flen) * 2; cur = realloc(cur, cap); }
                                memcpy(cur + cur_len, field_start, flen);
                                cur_len += flen;
                                if (flen > 0) has_content = true;

                                if (*p == '\0') break;

                                char delim = *p++;
                                bool is_nws_delim = IS_IFS_NWS(delim);
                                if (IS_IFS_WS(delim)) {
                                    while (*p && IS_IFS_WS(*p)) p++;
                                    if (*p && IS_IFS_NWS(*p)) {
                                        is_nws_delim = true;
                                        p++;
                                        while (*p && IS_IFS_WS(*p)) p++;
                                    }
                                } else {
                                    while (*p && IS_IFS_WS(*p)) p++;
                                }

                                if (is_nws_delim || has_content || cur_len > 0) {
                                    cur[cur_len] = '\0';
                                    add_field_to_cmd(state, cmd, cur, word_quoted);
                                    any_field_emitted = true;
                                    cur_len = 0;
                                    has_content = false;
                                }
                            }
                        }
                    }
                    free(exp_val);
                }
                i = vend;
                continue;
            }
        }

        has_content = true;
        if (cur_len + 1 >= cap) { cap *= 2; cur = realloc(cur, cap); }
        cur[cur_len++] = c;
        i++;
    }

    if (has_content || cur_len > 0 || (!any_field_emitted && word_has_explicit_quotes(word))) {
        cur[cur_len] = '\0';
        add_field_to_cmd(state, cmd, cur, word_quoted);
    }
    free(cur);

    #undef IS_IFS
    #undef IS_IFS_WS
    #undef IS_IFS_NWS
}

/* Parse a single command string (no pipes, no connectors) into zbash_cmd_t */
void zbash_parse_simple_cmd(zbash_state_t *state, const char *cmd_str, zbash_cmd_t *cmd) {
    memset(cmd, 0, sizeof(zbash_cmd_t));
    size_t len = strlen(cmd_str);
    size_t i = 0;

    i = skip_cmd_whitespace(cmd_str, i, len);
    if (i >= len) return;

    /* Handle compound brace { ... } */
    if (cmd_str[i] == '{' && (i + 1 == len || isspace((unsigned char)cmd_str[i + 1]))) {
        size_t b_start = i + 1;
        int depth = 1;
        bool in_s = false, in_d = false;
        size_t j = b_start;
        while (j < len && depth > 0) {
            char c = cmd_str[j];
            if (c == '\'' && !in_d) in_s = !in_s;
            else if (c == '"' && !in_s) in_d = !in_d;
            else if (c == '{' && !in_s && !in_d) depth++;
            else if (c == '}' && !in_s && !in_d) {
                depth--;
                if (depth == 0) break;
            }
            j++;
        }
        if (depth == 0) {
            cmd->argv[0] = strdup("{");
            size_t inner_len = j - b_start;
            char *inner = malloc(inner_len + 1);
            strncpy(inner, cmd_str + b_start, inner_len);
            inner[inner_len] = '\0';
            cmd->argv[1] = inner;
            cmd->argv[2] = NULL;
            cmd->argc = 2;
            i = j + 1;
        }
    }

    /* Handle compound subshell ( ... ) */
    if (cmd->argc == 0 && cmd_str[i] == '(' && (i + 1 < len && cmd_str[i + 1] != '(')) {
        size_t b_start = i + 1;
        int depth = 1;
        bool in_s = false, in_d = false;
        size_t j = b_start;
        while (j < len && depth > 0) {
            char c = cmd_str[j];
            if (c == '\'' && !in_d) in_s = !in_s;
            else if (c == '"' && !in_s) in_d = !in_d;
            else if (c == '(' && !in_s && !in_d) depth++;
            else if (c == ')' && !in_s && !in_d) {
                depth--;
                if (depth == 0) break;
            }
            j++;
        }
        if (depth == 0) {
            cmd->argv[0] = strdup("(");
            size_t inner_len = j - b_start;
            char *inner = malloc(inner_len + 1);
            strncpy(inner, cmd_str + b_start, inner_len);
            inner[inner_len] = '\0';
            cmd->argv[1] = inner;
            cmd->argv[2] = NULL;
            cmd->argc = 2;
            i = j + 1;
        }
    }

    while (i < len) {
#if defined(ZBASH_USE_NEON)
        i += zbash_simd_skip_spaces(cmd_str + i, len - i);
#endif
        i = skip_cmd_whitespace(cmd_str, i, len);
        if (i >= len) break;

        /* Check for redirections */
        if (parse_redir(state, cmd_str, &i, len, cmd)) {
            continue;
        }

        /* Normal argument word */
        size_t start = i;
        i = scan_word(cmd_str, i, len);

        if (i > start) {
            size_t rwlen = i - start;
            char *raw_word = malloc(rwlen + 1);
            if (!raw_word) continue;
            memcpy(raw_word, cmd_str + start, rwlen);
            raw_word[rwlen] = '\0';

            /* Preserve compound array assignments (VAR=(...) or VAR+=(...)) intact */
            if (raw_word[0] != '\'' && raw_word[0] != '"') {
                char *eq_paren = strstr(raw_word, "=(");
                if (!eq_paren) eq_paren = strstr(raw_word, "+=(");
                if (eq_paren) {
                    if (cmd->argc > 0) {
                        const char *c0 = cmd->argv[0];
                        bool is_decl = (strcmp(c0, "declare") == 0 || strcmp(c0, "typeset") == 0 ||
                                        strcmp(c0, "local") == 0 || strcmp(c0, "readonly") == 0 ||
                                        strcmp(c0, "export") == 0 || strcmp(c0, "eval") == 0 ||
                                        strcmp(c0, "let") == 0);
                        if (!is_decl) {
                            const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "./array.tests";
                            int lineno = state->lineno > 0 ? state->lineno : 1;
                            fprintf(stderr, "%s: line %d: syntax error near unexpected token `('\n", src, lineno);
                            fprintf(stderr, "%s: line %d: `%s'\n", src, lineno, cmd_str);
                            state->last_status = 2;
                            state->expansion_error = true;
                            free(raw_word);
                            return;
                        }
                    }
                    if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                        cmd->argv[cmd->argc++] = strdup(raw_word);
                    }
                    free(raw_word);
                    continue;
                }
            }

            /* Check if raw_word is an assignment at command-initial position (cmd->argc == 0) */
            bool is_initial_assign = false;
            char *eq = strchr(raw_word, '=');
            if (eq && eq > raw_word && *(eq - 1) == '\\') eq = NULL;
            if (cmd->argc == cmd->prefix_assign_count && eq && eq > raw_word) {
                bool valid = false;
                char *p = raw_word;
                if (isalpha((unsigned char)*p) || *p == '_') {
                    p++;
                    while (isalnum((unsigned char)*p) || *p == '_') p++;
                    if (*p == '[') {
                        int b_depth = 1;
                        p++;
                        while (*p && b_depth > 0 && p < eq) {
                            if (*p == '[') b_depth++;
                            else if (*p == ']') b_depth--;
                            p++;
                        }
                    }
                    if (*p == '+' && p == eq - 1) p++;
                    if (p == eq) valid = true;
                }
                if (valid) {
                    is_initial_assign = true;
                    cmd->prefix_assign_count++;
                }
            }

            char **bwords = NULL;
            if (is_initial_assign) {
                bwords = malloc(2 * sizeof(char *));
                bwords[0] = strdup(raw_word);
                bwords[1] = NULL;
            } else {
                bwords = zbash_expand_braces(raw_word);
            }
            free(raw_word);

            if (!bwords) continue;

            for (size_t b_idx = 0; bwords[b_idx]; b_idx++) {
                char *word = bwords[b_idx];
                size_t wlen = strlen(word);

                /* Check for positional parameters "$@" or "${@}" or "${@:offset...}" */
                if (strcmp(word, "\"$@\"") == 0 || strcmp(word, "\"${@}\"") == 0) {
                    for (int a = 1; a < state->argc; a++) {
                        if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                            cmd->argv[cmd->argc++] = strdup(state->argv[a]);
                        }
                    }
                    continue;
                }
                if (strcmp(word, "${1+\"$@\"}") == 0 || strcmp(word, "\"${1+\"$@\"}\"") == 0 ||
                    strcmp(word, "\"${1+$@}\"") == 0 || strcmp(word, "${1+$@}") == 0 ||
                    strcmp(word, "${1+\"${@}\"}") == 0 || strcmp(word, "\"${1+\"${@}\"}\"") == 0) {
                    if (state->argc > 1) {
                        for (int a = 1; a < state->argc; a++) {
                            if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                cmd->argv[cmd->argc++] = strdup(state->argv[a]);
                            }
                        }
                    }
                    continue;
                }
                /* Check for ${var:-"$@"} or "${var:-$@}" or "${var:-"$@"}" or variations */
                bool is_outer_quoted = (word[0] == '"' && wlen >= 2 && word[wlen - 1] == '"');
                const char *chk_word = is_outer_quoted ? (word + 1) : word;
                size_t chk_len = is_outer_quoted ? (wlen - 2) : wlen;
                if (chk_len >= 3 && chk_word[0] == '$' && chk_word[1] == '{' && chk_word[chk_len - 1] == '}' &&
                    (strstr(chk_word, "$@") || strstr(chk_word, "${@}"))) {
                    char vname[128] = {0};
                    const char *p = chk_word + 2;
                    size_t vi = 0;
                    bool is_colon = false;
                    while (*p && *p != ':' && *p != '-' && vi < sizeof(vname) - 1) {
                        vname[vi++] = *p++;
                    }
                    vname[vi] = '\0';
                    if (*p == ':') { is_colon = true; p++; }
                    if (*p == '-') {
                        char num_buf[32];
                        const char *val = NULL;
                        char *dyn = NULL;
                        bool is_set = zbash_get_param_value(state, vname, num_buf, sizeof(num_buf), &val, &dyn);
                        if (is_colon ? (is_set && val && *val != '\0') : is_set) {
                            if (is_outer_quoted) {
                                if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                    cmd->argv[cmd->argc++] = strdup(val ? val : "");
                                }
                            } else {
                                const char *ifs = getenv("IFS");
                                if (ifs && *ifs == '\0') {
                                    if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                        cmd->argv[cmd->argc++] = strdup(val ? val : "");
                                    }
                                } else {
                                    const char *delims = ifs ? ifs : " \t\n";
                                    char *vcopy = strdup(val ? val : "");
                                    char *sp = vcopy;
                                    while (*sp) {
                                        while (*sp && strchr(delims, *sp)) sp++;
                                        if (!*sp) break;
                                        char *tok = sp;
                                        while (*sp && !strchr(delims, *sp)) sp++;
                                        char saved = *sp;
                                        *sp = '\0';
                                        if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                            cmd->argv[cmd->argc++] = strdup(tok);
                                        }
                                        if (saved) {
                                            *sp = saved;
                                            sp++;
                                        }
                                    }
                                    free(vcopy);
                                }
                            }
                        } else {
                            if (state->argc > 1) {
                                for (int a = 1; a < state->argc; a++) {
                                    if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                        cmd->argv[cmd->argc++] = strdup(state->argv[a]);
                                    }
                                }
                            } else {
                                if (is_outer_quoted) {
                                    if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                        cmd->argv[cmd->argc++] = strdup("");
                                    }
                                }
                            }
                        }
                        if (dyn) free(dyn);
                        continue;
                    }
                }
                if (strcmp(word, "\"${@-}\"") == 0 || strcmp(word, "\"${@:-}\"") == 0) {
                    if (state->argc > 1) {
                        for (int a = 1; a < state->argc; a++) {
                            if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                cmd->argv[cmd->argc++] = strdup(state->argv[a]);
                            }
                        }
                    } else {
                        if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                            cmd->argv[cmd->argc++] = strdup("");
                        }
                    }
                    continue;
                }
                char *at_pos_quoted = strstr(word, "\"$@\"");
                char *open_brace_before = strstr(word, "${");
                bool is_full_quoted = (wlen >= 4 && word[0] == '"' && word[wlen - 1] == '"');
                char *at_in_full = NULL;
                bool at_is_brace = false;
                if (!at_pos_quoted && is_full_quoted) {
                    char *p = strstr(word, "$@");
                    if (!p) {
                        p = strstr(word, "${@}");
                        if (p) at_is_brace = true;
                    }
                    if (p) {
                        int b_depth = 0;
                        for (const char *s = word + 1; s < p; s++) {
                            if (*s == '$' && *(s + 1) == '{') { b_depth++; s++; }
                            else if (*s == '}' && b_depth > 0) { b_depth--; }
                        }
                        if (b_depth == 0) {
                            at_in_full = p;
                        }
                    }
                }
                if ((at_pos_quoted && (!open_brace_before || open_brace_before > at_pos_quoted) &&
                     (at_pos_quoted > word || at_pos_quoted[4] != '\0')) || at_in_full) {
                    char pfx_buf[256] = {0};
                    char sfx_buf[256] = {0};
                    bool is_inner_at = (at_in_full != NULL);
                    if (is_inner_at) {
                        size_t pfx_len = at_in_full - (word + 1);
                        if (pfx_len >= sizeof(pfx_buf)) pfx_len = sizeof(pfx_buf) - 1;
                        strncpy(pfx_buf, word + 1, pfx_len);
                        pfx_buf[pfx_len] = '\0';
                        size_t skip = at_is_brace ? 4 : 2;
                        size_t sfx_len = (word + wlen - 1) - (at_in_full + skip);
                        if (sfx_len >= sizeof(sfx_buf)) sfx_len = sizeof(sfx_buf) - 1;
                        strncpy(sfx_buf, at_in_full + skip, sfx_len);
                        sfx_buf[sfx_len] = '\0';
                    } else {
                        size_t pfx_len = at_pos_quoted - word;
                        if (pfx_len >= sizeof(pfx_buf)) pfx_len = sizeof(pfx_buf) - 1;
                        strncpy(pfx_buf, word, pfx_len);
                        pfx_buf[pfx_len] = '\0';
                        const char *sfx_raw = at_pos_quoted + 4;
                        strncpy(sfx_buf, sfx_raw, sizeof(sfx_buf) - 1);
                        sfx_buf[sizeof(sfx_buf) - 1] = '\0';
                    }
                    char *pfx_exp = (pfx_buf[0] != '\0') ? zbash_expand_vars(state, pfx_buf) : strdup("");
                    char *sfx_exp = (sfx_buf[0] != '\0') ? zbash_expand_vars(state, sfx_buf) : strdup("");
                    if (state->argc > 1) {
                        if (state->argc == 2) {
                            char comb[1024];
                            snprintf(comb, sizeof(comb), "%s%s%s", pfx_exp ? pfx_exp : "", state->argv[1], sfx_exp ? sfx_exp : "");
                            if (cmd->argc < ZBASH_MAX_ARGS - 1) cmd->argv[cmd->argc++] = strdup(comb);
                        } else {
                            if (is_inner_at && sfx_buf[0] != '\0') {
                                size_t total_sz = strlen(pfx_exp ? pfx_exp : "") + strlen(sfx_exp ? sfx_exp : "") + 2;
                                for (int a = 1; a < state->argc; a++) total_sz += strlen(state->argv[a]) + 1;
                                char *comb = malloc(total_sz);
                                if (comb) {
                                    strcpy(comb, pfx_exp ? pfx_exp : "");
                                    for (int a = 1; a < state->argc; a++) {
                                        strcat(comb, state->argv[a]);
                                        if (a < state->argc - 1) strcat(comb, " ");
                                    }
                                    strcat(comb, sfx_exp ? sfx_exp : "");
                                    if (cmd->argc < ZBASH_MAX_ARGS - 1) cmd->argv[cmd->argc++] = comb;
                                    else free(comb);
                                }
                            } else {
                                char comb_first[1024];
                                snprintf(comb_first, sizeof(comb_first), "%s%s", pfx_exp ? pfx_exp : "", state->argv[1]);
                                if (cmd->argc < ZBASH_MAX_ARGS - 1) cmd->argv[cmd->argc++] = strdup(comb_first);
                                for (int a = 2; a < state->argc - 1; a++) {
                                    if (cmd->argc < ZBASH_MAX_ARGS - 1) cmd->argv[cmd->argc++] = strdup(state->argv[a]);
                                }
                                char comb_last[1024];
                                snprintf(comb_last, sizeof(comb_last), "%s%s", state->argv[state->argc - 1], sfx_exp ? sfx_exp : "");
                                if (cmd->argc < ZBASH_MAX_ARGS - 1) cmd->argv[cmd->argc++] = strdup(comb_last);
                            }
                        }
                    } else {
                        if ((pfx_exp && pfx_exp[0] != '\0') || (sfx_exp && sfx_exp[0] != '\0')) {
                            char comb[1024];
                            snprintf(comb, sizeof(comb), "%s%s", pfx_exp ? pfx_exp : "", sfx_exp ? sfx_exp : "");
                            if (cmd->argc < ZBASH_MAX_ARGS - 1) cmd->argv[cmd->argc++] = strdup(comb);
                        }
                    }
                    if (pfx_exp) free(pfx_exp);
                    if (sfx_exp) free(sfx_exp);
                    continue;
                }
                if (wlen >= 7 && word[0] == '"' && word[1] == '$' && word[2] == '{' && word[3] == '@' && word[4] == ':' && word[wlen - 2] == '}' && word[wlen - 1] == '"') {
                    char expr_buf[128];
                    size_t elen = (wlen - 2) - 5;
                    if (elen < sizeof(expr_buf)) {
                        strncpy(expr_buf, word + 5, elen);
                        expr_buf[elen] = '\0';
                        char *sep = strchr(expr_buf, ':');
                        int offset = 0, slen = -1;
                        if (sep) {
                            *sep = '\0';
                            offset = (int)zbash_eval_arithmetic(state, expr_buf);
                            slen = (int)zbash_eval_arithmetic(state, sep + 1);
                            if (slen < 0) {
                                const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
                                int lineno = state->lineno > 0 ? state->lineno : 1;
                                fprintf(stderr, "%s: line %d: %s: substring expression < 0\n", src, lineno, sep + 1);
                                state->last_status = 1;
                                state->expansion_error = true;
                                continue;
                            }
                        } else {
                            offset = (int)zbash_eval_arithmetic(state, expr_buf);
                        }
                        int start_idx = offset;
                        if (start_idx < 0) start_idx = state->argc + start_idx;
                        if (start_idx < 0) start_idx = 0;
                        int collected = 0;
                        for (int a = start_idx; a < state->argc && (slen < 0 || collected < slen); a++) {
                            if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                cmd->argv[cmd->argc++] = strdup(state->argv[a]);
                            }
                            collected++;
                        }
                    }
                    continue;
                }

                /* Check for standalone array expansion "${arr[@]}" or ${arr[@]} with optional slice/pattern, or $@ */
                bool is_standalone_arr = false;
                char aname[128] = {0};
                int arr_offset = 0;
                int arr_len = -1;
                bool has_slice = false;

                bool is_quoted = (wlen >= 6 && word[0] == '"' && word[1] == '$' && word[2] == '{' &&
                                  word[wlen - 2] == '}' && word[wlen - 1] == '"');
                bool is_unquoted = (wlen >= 4 && word[0] == '$' && word[1] == '{' && word[wlen - 1] == '}');

                bool is_indices = false;
                bool is_pat_rem = false;
                bool is_pat_sub = false;
                bool is_prefix = false, longest = false, replace_all = false;
                char pat_buf[256] = {0};
                char rep_buf[256] = {0};

                if ((is_quoted || is_unquoted) && (is_quoted ? (wlen > 3 && word[3] != '#') : (wlen > 2 && word[2] != '#'))) {
                    char *at_pos = strstr(word, "[@]");
                    if (!at_pos && is_unquoted) {
                        at_pos = strstr(word, "[*]");
                    }
                    char *after_at = NULL;
                    char *end_brace = is_quoted ? (word + wlen - 2) : (word + wlen - 1);

                    if (at_pos) {
                        size_t name_start = is_quoted ? 3 : 2;
                        if (name_start < wlen && word[name_start] == '!') {
                            is_indices = true;
                            name_start++;
                        }
                        size_t name_len = at_pos - (word + name_start);
                        if (name_len > 0 && name_len < sizeof(aname)) {
                            strncpy(aname, word + name_start, name_len);
                            aname[name_len] = '\0';
                            after_at = at_pos + 3;
                        }
                    } else if (is_quoted && word[3] == '!') {
                        char ivar[128] = {0};
                        size_t iv_len = end_brace - (word + 4);
                        if (iv_len > 0 && iv_len < sizeof(ivar)) {
                            strncpy(ivar, word + 4, iv_len);
                            ivar[iv_len] = '\0';
                            char num_buf[32];
                            const char *rval = NULL;
                            char *rdyn = NULL;
                            if (zbash_get_param_value(state, ivar, num_buf, sizeof(num_buf), &rval, &rdyn)) {
                                if (rval) {
                                    char *iat = strstr(rval, "[@]");
                                    if (iat) {
                                        size_t alen = iat - rval;
                                        if (alen > 0 && alen < sizeof(aname)) {
                                            strncpy(aname, rval, alen);
                                            aname[alen] = '\0';
                                            after_at = end_brace;
                                        }
                                    }
                                }
                            }
                            if (rdyn) free(rdyn);
                        }
                    } else {
                        size_t name_start = is_quoted ? 3 : 2;
                        if (name_start < wlen && (word[name_start] == '@' || (is_unquoted && word[name_start] == '*'))) {
                            char next_c = word[name_start + 1];
                            if (next_c == '/' || next_c == '#' || next_c == '%' || next_c == ':' || (word + name_start + 1 == end_brace)) {
                                strcpy(aname, "@");
                                after_at = word + name_start + 1;
                            }
                        }
                    }

                    if (after_at) {
                        if (after_at == end_brace) {
                            is_standalone_arr = true;
                            arr_offset = 0;
                            arr_len = -1;
                        } else if (*after_at == ':') {
                            has_slice = true;
                            char slice_buf[128];
                            size_t sl_len = end_brace - (after_at + 1);
                            if (sl_len < sizeof(slice_buf)) {
                                strncpy(slice_buf, after_at + 1, sl_len);
                                slice_buf[sl_len] = '\0';
                                char *s2 = strchr(slice_buf, ':');
                                if (s2) {
                                    *s2 = '\0';
                                    arr_offset = (int)zbash_eval_arithmetic(state, slice_buf);
                                    arr_len = (int)zbash_eval_arithmetic(state, s2 + 1);
                                    if (arr_len < 0) {
                                        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
                                        int lineno = state->lineno > 0 ? state->lineno : 1;
                                        fprintf(stderr, "%s: line %d: %s: substring expression < 0\n", src, lineno, s2 + 1);
                                        state->last_status = 1;
                                        state->expansion_error = true;
                                        continue;
                                    }
                                } else {
                                    arr_offset = (int)zbash_eval_arithmetic(state, slice_buf);
                                }
                                is_standalone_arr = true;
                            }
                        } else if (*after_at == '#' || *after_at == '%') {
                            is_standalone_arr = true;
                            is_pat_rem = true;
                            is_prefix = (*after_at == '#');
                            longest = (*(after_at + 1) == *after_at);
                            char *pstart = after_at + (longest ? 2 : 1);
                            size_t plen = (end_brace >= pstart) ? (size_t)(end_brace - pstart) : 0;
                            if (plen >= sizeof(pat_buf)) plen = sizeof(pat_buf) - 1;
                            strncpy(pat_buf, pstart, plen);
                            pat_buf[plen] = '\0';
                        } else if (*after_at == '/') {
                            is_standalone_arr = true;
                            is_pat_sub = true;
                            replace_all = (*(after_at + 1) == '/');
                            char *pstart = after_at + (replace_all ? 2 : 1);
                            char *slash2 = NULL;
                            bool s_quote = false, d_quote = false;
                            for (char *p = pstart; p < end_brace; p++) {
                                if (*p == '\\' && p + 1 < end_brace) { p++; continue; }
                                if (*p == '\'' && !d_quote) { s_quote = !s_quote; continue; }
                                if (*p == '"' && !s_quote) { d_quote = !d_quote; continue; }
                                if (*p == '/' && !s_quote && !d_quote) { slash2 = p; break; }
                            }
                            if (slash2) {
                                size_t plen = slash2 - pstart;
                                if (plen >= sizeof(pat_buf)) plen = sizeof(pat_buf) - 1;
                                strncpy(pat_buf, pstart, plen);
                                pat_buf[plen] = '\0';

                                size_t rlen = end_brace - (slash2 + 1);
                                if (rlen >= sizeof(rep_buf)) rlen = sizeof(rep_buf) - 1;
                                strncpy(rep_buf, slash2 + 1, rlen);
                                rep_buf[rlen] = '\0';
                            } else {
                                size_t plen = (end_brace >= pstart) ? (size_t)(end_brace - pstart) : 0;
                                if (plen >= sizeof(pat_buf)) plen = sizeof(pat_buf) - 1;
                                strncpy(pat_buf, pstart, plen);
                                pat_buf[plen] = '\0';
                                rep_buf[0] = '\0';
                            }
                        }
                    }
                }

                if (is_standalone_arr) {
                    char *act_pat = NULL;
                    char *act_rep = NULL;
                    if (pat_buf[0]) {
                        act_pat = zbash_expand_pattern(state, pat_buf);
                    }
                    if (is_pat_sub && (strchr(rep_buf, '$') || strchr(rep_buf, '\'') || strchr(rep_buf, '"'))) {
                        act_rep = zbash_expand_vars(state, rep_buf);
                    }
                    const char *use_pat = act_pat ? act_pat : pat_buf;
                    const char *use_rep = act_rep ? act_rep : rep_buf;

                    if (strcmp(aname, "@") == 0) {
                        int start_pos = 1;
                        int slen = arr_len;
                        if (has_slice) {
                            start_pos = arr_offset;
                            if (start_pos < 0) start_pos = state->argc + start_pos;
                            if (start_pos < 0) start_pos = 0;
                        }
                        int collected = 0;
                        for (int a = start_pos; a < state->argc && (slen < 0 || collected < slen); a++) {
                            if (a < 0) continue;
                            const char *orig = state->argv[a];
                            collected++;
                            char *transformed = NULL;
                            if (is_pat_rem) {
                                transformed = apply_pattern_removal(orig, use_pat, is_prefix, longest);
                            } else if (is_pat_sub) {
                                transformed = apply_pattern_subst(orig, use_pat, use_rep, replace_all);
                            }
                            const char *elem = transformed ? transformed : orig;
                            if (is_unquoted) {
                                if (elem[0] == '\0') {
                                    if (transformed) free(transformed);
                                    continue;
                                }
                                const char *ifs = getenv("IFS");
                                const char *delims = ifs ? ifs : " \t\n";
                                if (!ifs || *ifs != '\0') {
                                    char *dup_elem = strdup(elem);
                                    char *ep = dup_elem;
                                    while (*ep) {
                                        while (*ep && strchr(delims, *ep)) ep++;
                                        if (!*ep) break;
                                        char *w = ep;
                                        while (*ep && !strchr(delims, *ep)) ep++;
                                        char saved = *ep;
                                        *ep = '\0';
                                        char **gmatches = NULL;
                                        int gcount = 0;
                                        if (zbash_expand_path_glob(state, w, &gmatches, &gcount) > 0) {
                                            for (int m = 0; m < gcount; m++) {
                                                if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                                    cmd->argv[cmd->argc++] = gmatches[m];
                                                } else {
                                                    free(gmatches[m]);
                                                }
                                            }
                                            free(gmatches);
                                        } else {
                                            if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                                cmd->argv[cmd->argc++] = strdup(w);
                                            }
                                        }
                                        *ep = saved;
                                    }
                                    free(dup_elem);
                                } else {
                                    if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                        cmd->argv[cmd->argc++] = strdup(elem);
                                    }
                                }
                            } else {
                                if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                    cmd->argv[cmd->argc++] = strdup(elem);
                                }
                            }
                            if (transformed) free(transformed);
                        }
                        if (act_pat) free(act_pat);
                        if (act_rep) free(act_rep);
                        continue;
                    }

                    zbash_array_t *arr = zbash_find_array(state, aname);
                    if (arr) {
                        if (arr_offset < 0) {
                            int max_idx = -1;
                            for (int k = arr->count - 1; k >= 0; k--) {
                                if (arr->elements[k]) { max_idx = k; break; }
                            }
                            arr_offset = (max_idx >= 0 ? max_idx + 1 : 0) + arr_offset;
                        }
                        if (arr_offset < 0) arr_offset = 0;
                        int collected = 0;
                        for (int a = arr_offset; a < arr->count && (arr_len < 0 || collected < arr_len); a++) {
                            if (!arr->elements[a]) continue;
                            collected++;
                            char *transformed = NULL;
                            if (is_pat_rem) {
                                transformed = apply_pattern_removal(arr->elements[a], use_pat, is_prefix, longest);
                            } else if (is_pat_sub) {
                                transformed = apply_pattern_subst(arr->elements[a], use_pat, use_rep, replace_all);
                            }
                            const char *elem = transformed ? transformed : arr->elements[a];
                            if (is_indices) {
                                char num_s[32];
                                snprintf(num_s, sizeof(num_s), "%d", a);
                                if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                    cmd->argv[cmd->argc++] = strdup(num_s);
                                }
                            } else if (is_unquoted) {
                                if (elem[0] == '\0') {
                                    if (transformed) free(transformed);
                                    continue;
                                }
                                const char *ifs = getenv("IFS");
                                const char *delims = ifs ? ifs : " \t\n";
                                if (!ifs || *ifs != '\0') {
                                    char *dup_elem = strdup(elem);
                                    char *ep = dup_elem;
                                    while (*ep) {
                                        while (*ep && strchr(delims, *ep)) ep++;
                                        if (!*ep) break;
                                        char *w = ep;
                                        while (*ep && !strchr(delims, *ep)) ep++;
                                        char saved = *ep;
                                        *ep = '\0';
                                        char **gmatches = NULL;
                                        int gcount = 0;
                                        if (zbash_expand_path_glob(state, w, &gmatches, &gcount) > 0) {
                                            for (int m = 0; m < gcount; m++) {
                                                if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                                    cmd->argv[cmd->argc++] = gmatches[m];
                                                } else {
                                                    free(gmatches[m]);
                                                }
                                            }
                                            free(gmatches);
                                        } else {
                                            if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                                cmd->argv[cmd->argc++] = strdup(w);
                                            }
                                        }
                                        *ep = saved;
                                    }
                                    free(dup_elem);
                                } else {
                                    if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                        cmd->argv[cmd->argc++] = strdup(elem);
                                    }
                                }
                            } else {
                                if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                    cmd->argv[cmd->argc++] = strdup(elem);
                                }
                            }
                            if (transformed) free(transformed);
                        }
                        if (act_pat) free(act_pat);
                        if (act_rep) free(act_rep);
                        continue;
                    } else {
                        const char *val = getenv(aname);
                        if (val) {
                            if (has_slice) {
                                size_t vlen = strlen(val);
                                int offset = arr_offset;
                                int slen = arr_len;
                                if (offset < 0) offset = vlen + offset;
                                if (offset < 0) offset = 0;
                                if (offset > (int)vlen) offset = vlen;
                                if (slen < 0 || offset + slen > (int)vlen) slen = vlen - offset;
                                const char *p_start = utf8_char_at(val, offset);
                                const char *p_end = utf8_char_at(val, offset + slen);
                                size_t sub_len = p_end - p_start;
                                char *sub_str = malloc(sub_len + 1);
                                if (sub_str) {
                                    strncpy(sub_str, p_start, sub_len);
                                    sub_str[sub_len] = '\0';
                                    if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                        cmd->argv[cmd->argc++] = sub_str;
                                    } else {
                                        free(sub_str);
                                    }
                                }
                            } else if (arr_offset <= 0 && (arr_len < 0 || arr_len > 0)) {
                                char *transformed = NULL;
                                if (is_pat_rem) {
                                    transformed = apply_pattern_removal(val, use_pat, is_prefix, longest);
                                } else if (is_pat_sub) {
                                    transformed = apply_pattern_subst(val, use_pat, use_rep, replace_all);
                                }
                                const char *elem = transformed ? transformed : val;
                                if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                                    cmd->argv[cmd->argc++] = strdup(is_indices ? "0" : elem);
                                }
                                if (transformed) free(transformed);
                            }
                        }
                        if (act_pat) free(act_pat);
                        if (act_rep) free(act_rep);
                        continue;
                    }
                }


                /* Check if word is an assignment (VAR=... or VAR[...]=...) */
                bool is_assign = false;
                char *eq2 = strchr(word, '=');
                if (eq2 && eq2 > word && *(eq2 - 1) == '\\') eq2 = NULL;
                if (eq2 && eq2 > word) {
                    bool allow_assign = is_initial_assign || (cmd->argc < cmd->prefix_assign_count);
                    if (!allow_assign && cmd->argc > 0) {
                        const char *c0 = cmd->argv[0];
                        if (strcmp(c0, "declare") == 0 || strcmp(c0, "typeset") == 0 ||
                            strcmp(c0, "local") == 0 || strcmp(c0, "readonly") == 0 ||
                            strcmp(c0, "export") == 0) {
                            allow_assign = true;
                        }
                    }
                    if (allow_assign) {
                        bool valid = false;
                        char *p = word;
                        if (isalpha((unsigned char)*p) || *p == '_') {
                            p++;
                            while (isalnum((unsigned char)*p) || *p == '_') p++;
                            if (*p == '[') {
                                int b_depth = 1;
                                p++;
                                while (*p && b_depth > 0 && p < eq2) {
                                    if (*p == '[') b_depth++;
                                    else if (*p == ']') b_depth--;
                                    p++;
                                }
                            }
                            if (*p == '+' && p == eq2 - 1) p++;
                            if (p == eq2) valid = true;
                        }
                        if (valid) is_assign = true;
                    }
                }
                bool is_decl = false;
                if (cmd->argc > 0) {
                    const char *c0 = cmd->argv[0];
                    if (strcmp(c0, "declare") == 0 || strcmp(c0, "typeset") == 0 ||
                        strcmp(c0, "local") == 0 || strcmp(c0, "readonly") == 0 ||
                        strcmp(c0, "export") == 0) {
                        is_decl = true;
                    }
                }
                bool is_unquoted_var_sub = (!is_assign && has_unquoted_expansion(state, word));
                const char *ifs = getenv("IFS");
                if (is_unquoted_var_sub && (!ifs || *ifs != '\0')) {
                    expand_and_split_word(state, cmd, word);
                    continue;
                }

                bool prev_in_args = state ? state->in_cmd_args : false;
                if (state && cmd->argc > 0 && !is_decl && !is_assign) {
                    state->in_cmd_args = true;
                }
                char *exp = zbash_expand_vars(state, word);
                if (state) state->in_cmd_args = prev_in_args;

                const char *w = exp ? exp : word;
                bool was_quoted = (word[0] == '"' || word[0] == '\'');
                char **gmatches = NULL;
                int gcount = 0;
                if (!is_assign && !was_quoted && zbash_expand_path_glob(state, w, &gmatches, &gcount) > 0) {
                    for (int m = 0; m < gcount; m++) {
                        if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                            cmd->argv[cmd->argc++] = gmatches[m];
                        } else {
                            free(gmatches[m]);
                        }
                    }
                    free(gmatches);
                    if (exp) free(exp);
                    continue;
                }

                if (cmd->argc < ZBASH_MAX_ARGS - 1) {
                    cmd->argv[cmd->argc++] = exp ? exp : strdup(word);
                }
            }
            zbash_free_string_list(bwords);
        }

    }
    cmd->argv[cmd->argc] = NULL;
}
