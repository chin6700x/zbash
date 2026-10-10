/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"
#include <poll.h>
#include <sys/select.h>

static inline bool is_ifs_char(char c, const char *ifs) {
    if (!ifs) return (c == ' ' || c == '\t' || c == '\n');
    return strchr(ifs, c) != NULL;
}

static inline bool is_ifs_ws(char c, const char *ifs) {
    if (!ifs) return (c == ' ' || c == '\t' || c == '\n');
    return (c == ' ' || c == '\t' || c == '\n') && strchr(ifs, c) != NULL;
}

static void assign_read_var(zbash_state_t *state, const char *var_name, const char *val) {
    const char *bracket = strchr(var_name, '[');
    if (bracket) {
        char aname[128];
        size_t nlen = bracket - var_name;
        if (nlen >= sizeof(aname)) nlen = sizeof(aname) - 1;
        strncpy(aname, var_name, nlen);
        aname[nlen] = '\0';
        const char *close_b = strrchr(bracket + 1, ']');
        if (close_b) {
            char sub_str[128];
            size_t slen = close_b - (bracket + 1);
            if (slen >= sizeof(sub_str)) slen = sizeof(sub_str) - 1;
            strncpy(sub_str, bracket + 1, slen);
            sub_str[slen] = '\0';
            int idx = (int)zbash_eval_arithmetic(state, sub_str);
            zbash_set_array_elem(state, aname, idx, val);
            return;
        }
    }
    zbash_assign_var(state, var_name, val ? val : "", false);
}

int builtin_read(zbash_state_t *state, int argc, char **argv) {
    int opt_idx = 1;
    bool raw_mode = false;
    const char *array_name = NULL;
    char delim = '\n';
    int input_fd = STDIN_FILENO;
    double timeout = -1.0;
    int nchars = -1;

    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] :
                      (state && state->argv && state->argv[0] ? state->argv[0] : "zbash");
    int lineno = state ? (state->stmt_start_lineno > 0 ? state->stmt_start_lineno : (state->lineno > 0 ? state->lineno : 1)) : 1;

    while (opt_idx < argc && argv[opt_idx][0] == '-') {
        if (strcmp(argv[opt_idx], "--") == 0) {
            opt_idx++;
            break;
        }
        if (argv[opt_idx][1] == '\0') {
            break;
        }
        char *arg = argv[opt_idx];
        for (int c = 1; arg[c]; c++) {
            if (arg[c] == 'r') {
                raw_mode = true;
            } else if (arg[c] == 'a') {
                if (arg[c + 1] != '\0') {
                    array_name = arg + c + 1;
                    break;
                } else if (opt_idx + 1 < argc) {
                    array_name = argv[++opt_idx];
                    break;
                }
            } else if (arg[c] == 'd') {
                const char *darg = NULL;
                if (arg[c + 1] != '\0') {
                    darg = arg + c + 1;
                } else if (opt_idx + 1 < argc) {
                    darg = argv[++opt_idx];
                }
                if (darg) {
                    delim = darg[0];
                }
                break;
            } else if (arg[c] == 'u') {
                if (arg[c + 1] != '\0') {
                    input_fd = atoi(arg + c + 1);
                    break;
                } else if (opt_idx + 1 < argc) {
                    input_fd = atoi(argv[++opt_idx]);
                    break;
                }
            } else if (arg[c] == 't') {
                const char *targ = NULL;
                if (arg[c + 1] != '\0') {
                    targ = arg + c + 1;
                } else if (opt_idx + 1 < argc) {
                    targ = argv[++opt_idx];
                }
                if (targ) {
                    double tval = atof(targ);
                    if (tval < 0) {
                        fprintf(stderr, "%s: line %d: read: %s: invalid timeout specification\n", src, lineno, targ);
                        return 1;
                    }
                    timeout = tval;
                }
                break;
            } else if (arg[c] == 'n') {
                const char *narg = NULL;
                if (arg[c + 1] != '\0') {
                    narg = arg + c + 1;
                } else if (opt_idx + 1 < argc) {
                    narg = argv[++opt_idx];
                }
                if (narg) {
                    int nval = atoi(narg);
                    if (nval < 0) {
                        fprintf(stderr, "%s: line %d: read: %s: invalid number\n", src, lineno, narg);
                        return 1;
                    }
                    nchars = nval;
                }
                break;
            } else if (arg[c] == 'p') {
                if (arg[c + 1] != '\0') {
                    break;
                } else if (opt_idx + 1 < argc) {
                    opt_idx++;
                    break;
                }
            }
        }
        opt_idx++;
    }

    if (input_fd != STDIN_FILENO && (input_fd < 0 || fcntl(input_fd, F_GETFL) < 0)) {
        fprintf(stderr, "%s: line %d: read: %d: invalid file descriptor: Bad file descriptor\n", src, lineno, input_fd);
        return 1;
    }

    if (timeout >= 0) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(input_fd, &rfds);
        struct timeval tv;
        tv.tv_sec = (time_t)timeout;
        tv.tv_usec = (suseconds_t)((timeout - (double)tv.tv_sec) * 1000000.0);
        int sr = select(input_fd + 1, &rfds, NULL, NULL, &tv);
        if (sr <= 0 || !FD_ISSET(input_fd, &rfds)) {
            return 1;
        }
    }

    char line[8192];
    bool is_escaped[8192];
    size_t len = 0;
    int read_any = 0;
    bool delim_seen = false;

    while (len + 1 < sizeof(line)) {
        if (nchars >= 0 && (int)len >= nchars) {
            delim_seen = true;
            break;
        }

        char ch;
        ssize_t n = read(input_fd, &ch, 1);
        if (n < 0) {
            if (errno == EBADF) {
                fprintf(stderr, "%s: line %d: read: read error: %d: Bad file descriptor\n", src, lineno, input_fd);
            }
            return 1;
        }
        if (n == 0) {
            break;
        }
        read_any = 1;

        if (!raw_mode && ch == '\\') {
            char next_c;
            ssize_t n2 = read(input_fd, &next_c, 1);
            if (n2 <= 0) {
                line[len] = '\\';
                is_escaped[len] = false;
                len++;
                break;
            }
            if (next_c == '\n') {
                /* Line continuation: both '\' and '\n' disappear */
                continue;
            }
            line[len] = next_c;
            is_escaped[len] = true;
            len++;
            continue;
        }

        if (ch == delim) {
            delim_seen = true;
            break;
        }
        if (delim == '\n' && ch == '\r') {
            continue;
        }
        line[len] = ch;
        is_escaped[len] = false;
        len++;
    }
    line[len] = '\0';
    is_escaped[len] = false;

    if (!read_any) {
        return 1;
    }

    if (opt_idx >= argc && !array_name) {
        setenv("REPLY", line, 1);
        zbash_assign_var(state, "REPLY", line, false);
        return delim_seen ? 0 : 1;
    }

    const char *ifs = getenv("IFS");

    if (array_name) {
        zbash_unset_array(state, array_name);
        size_t p = 0;
        while (p < len && !is_escaped[p] && is_ifs_ws(line[p], ifs)) p++;
        int a_idx = 0;
        while (p < len) {
            size_t tok_start = p;
            while (p < len && (is_escaped[p] || !is_ifs_char(line[p], ifs))) p++;
            char saved_delim = (p < len) ? line[p] : '\0';
            bool saved_esc = (p < len) ? is_escaped[p] : false;
            line[p] = '\0';
            zbash_set_array_elem(state, array_name, a_idx++, line + tok_start);
            if (saved_delim != '\0') {
                p++;
                if (!saved_esc && is_ifs_ws(saved_delim, ifs)) {
                    while (p < len && !is_escaped[p] && is_ifs_ws(line[p], ifs)) p++;
                    if (p < len && !is_escaped[p] && is_ifs_char(line[p], ifs) && !is_ifs_ws(line[p], ifs)) {
                        p++;
                        while (p < len && !is_escaped[p] && is_ifs_ws(line[p], ifs)) p++;
                    }
                } else {
                    while (p < len && !is_escaped[p] && is_ifs_ws(line[p], ifs)) p++;
                }
            }
        }
        return delim_seen ? 0 : 1;
    }

    size_t p = 0;
    while (p < len && !is_escaped[p] && is_ifs_ws(line[p], ifs)) p++;

    int var_count = argc - opt_idx;
    for (int k = 0; k < var_count; k++) {
        char *var_name = argv[opt_idx + k];
        if (k == var_count - 1) {
            /* 1. Strip trailing IFS whitespace */
            size_t end_idx = len;
            while (end_idx > p && (!is_escaped[end_idx - 1] || (var_count == 1 && end_idx - 1 > p)) && is_ifs_ws(line[end_idx - 1], ifs)) {
                end_idx--;
            }
            /* 2. Check if the line ends with an unescaped non-whitespace IFS delimiter */
            if (end_idx > p && !is_escaped[end_idx - 1] && is_ifs_char(line[end_idx - 1], ifs) && !is_ifs_ws(line[end_idx - 1], ifs)) {
                size_t content_end = end_idx - 1;
                while (content_end > p && !is_escaped[content_end - 1] && is_ifs_ws(line[content_end - 1], ifs)) {
                    content_end--;
                }
                bool has_internal_delim = false;
                for (size_t i = p; i < content_end; i++) {
                    if (!is_escaped[i] && is_ifs_char(line[i], ifs)) {
                        has_internal_delim = true;
                        break;
                    }
                }
                if (!has_internal_delim) {
                    end_idx = content_end;
                }
            }
            line[end_idx] = '\0';
            assign_read_var(state, var_name, line + p);
            break;
        }

        if (p >= len) {
            assign_read_var(state, var_name, "");
        } else {
            size_t tok_start = p;
            while (p < len && (is_escaped[p] || !is_ifs_char(line[p], ifs))) p++;
            char saved_delim = (p < len) ? line[p] : '\0';
            bool saved_esc = (p < len) ? is_escaped[p] : false;
            line[p] = '\0';
            assign_read_var(state, var_name, line + tok_start);

            if (saved_delim != '\0') {
                p++;
                if (!saved_esc && is_ifs_ws(saved_delim, ifs)) {
                    while (p < len && !is_escaped[p] && is_ifs_ws(line[p], ifs)) p++;
                    if (p < len && !is_escaped[p] && is_ifs_char(line[p], ifs) && !is_ifs_ws(line[p], ifs)) {
                        p++;
                        while (p < len && !is_escaped[p] && is_ifs_ws(line[p], ifs)) p++;
                    }
                } else {
                    while (p < len && !is_escaped[p] && is_ifs_ws(line[p], ifs)) p++;
                }
            }
        }
    }
    return delim_seen ? 0 : 1;
}
