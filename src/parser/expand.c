/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "parser_internal.h"
#include <pwd.h>
#include <glob.h>

static const char *get_script_name(zbash_state_t *state) {
    if (state && state->source_depth > 0) return state->source_stack[state->source_depth - 1];
    if (state && state->argv && state->argv[0]) return state->argv[0];
    return "zbash";
}

static void get_shell_flags(zbash_state_t *state, char *out, size_t max_len) {
    size_t pos = 0;
    if (state && state->errexit && pos < max_len - 1) out[pos++] = 'e';
    if (pos < max_len - 1) out[pos++] = 'h';
    if (state && state->nounset && pos < max_len - 1) out[pos++] = 'u';
    if (state && state->verbose && pos < max_len - 1) out[pos++] = 'v';
    if (state && state->xtrace && pos < max_len - 1) out[pos++] = 'x';
    if (pos < max_len - 1) out[pos++] = 'B';
    if (state && state->functrace && pos < max_len - 1) out[pos++] = 'T';
    out[pos] = '\0';
}

static bool is_array_subscript_expr(const char *var_name) {
    if (!var_name) return false;
    const char *v_ident = (var_name[0] == '!') ? var_name + 1 : var_name;
    size_t id_len = 0;
    if (isalpha((unsigned char)v_ident[0]) || v_ident[0] == '_') {
        while (isalnum((unsigned char)v_ident[id_len]) || v_ident[id_len] == '_') id_len++;
    } else if (isdigit((unsigned char)v_ident[0])) {
        while (isdigit((unsigned char)v_ident[id_len])) id_len++;
    }
    return (id_len > 0 && v_ident[id_len] == '[');
}

static inline void get_ifs_first_char(const char *ifs, char *sep_buf, size_t *sep_len) {
    if (!ifs) {
        sep_buf[0] = ' ';
        sep_buf[1] = '\0';
        *sep_len = 1;
        return;
    }
    if (*ifs == '\0') {
        sep_buf[0] = '\0';
        *sep_len = 0;
        return;
    }
    unsigned char c = (unsigned char)ifs[0];
    size_t clen = 1;
    if ((c & 0x80) == 0) {
        clen = 1;
    } else if ((c & 0xE0) == 0xC0) {
        clen = 2;
    } else if ((c & 0xF0) == 0xE0) {
        clen = 3;
    } else if ((c & 0xF8) == 0xF0) {
        clen = 4;
    }
    if (clen > strlen(ifs)) clen = strlen(ifs);
    memcpy(sep_buf, ifs, clen);
    sep_buf[clen] = '\0';
    *sep_len = clen;
}

bool zbash_get_param_value(zbash_state_t *state, const char *vname, char *buf, size_t buf_sz, const char **out_val, char **out_dynamic) {
#define get_param_value zbash_get_param_value
    *out_val = NULL;
    *out_dynamic = NULL;

    if (!vname || !*vname) return false;

    if (strcmp(vname, "-") == 0) {
        get_shell_flags(state, buf, buf_sz);
        *out_val = buf;
        return true;
    }
    if (strcmp(vname, "$") == 0) {
        snprintf(buf, buf_sz, "%d", state->shell_pid);
        *out_val = buf;
        return true;
    }
    if (strcmp(vname, "?") == 0) {
        snprintf(buf, buf_sz, "%d", state->last_status);
        *out_val = buf;
        return true;
    }
    if (strcmp(vname, "!") == 0) {
        if (state->last_bg_pid > 0) {
            snprintf(buf, buf_sz, "%d", state->last_bg_pid);
            *out_val = buf;
            return true;
        }
        *out_val = "";
        return false;
    }
    if (strcmp(vname, "#") == 0) {
        snprintf(buf, buf_sz, "%d", (state->argc > 1 ? state->argc - 1 : 0));
        *out_val = buf;
        return true;
    }
    if (strcmp(vname, "*") == 0 || strcmp(vname, "@") == 0) {
        if (state->argc > 1) {
            char sep_buf[8] = " ";
            size_t sep_len = 1;
            if (strcmp(vname, "*") == 0) {
                const char *ifs = getenv("IFS");
                get_ifs_first_char(ifs, sep_buf, &sep_len);
            }
            size_t total_sz = 0;
            for (int a = 1; a < state->argc; a++) total_sz += strlen(state->argv[a]) + sep_len;
            char *dyn = malloc(total_sz + 1);
            if (dyn) {
                dyn[0] = '\0';
                for (int a = 1; a < state->argc; a++) {
                    strcat(dyn, state->argv[a]);
                    if (a < state->argc - 1 && sep_len > 0) {
                        strcat(dyn, sep_buf);
                    }
                }
                *out_dynamic = dyn;
                *out_val = dyn;
            } else {
                *out_val = "";
            }
            return true;
        } else {
            *out_val = "";
            return false;
        }
    }
    if (isdigit((unsigned char)vname[0])) {
        int arg_idx = atoi(vname);
        if (arg_idx < state->argc && state->argv) {
            *out_val = state->argv[arg_idx];
            return true;
        }
        *out_val = "";
        return false;
    }
    if (strcmp(vname, "LINENO") == 0) {
        snprintf(buf, buf_sz, "%d", state->lineno > 0 ? state->lineno : 0);
        *out_val = buf;
        return true;
    }

    zbash_array_t *arr = zbash_find_array(state, vname);
    if (arr) {
        if (arr->count > 0 && arr->elements[0]) {
            *out_val = arr->elements[0];
            return true;
        }
        *out_val = "";
        return false;
    }

    const char *ev = getenv(vname);
    if (ev != NULL) {
        *out_val = ev;
        return true;
    }

    *out_val = NULL;
    return false;
}

static const char *try_expand_tilde(zbash_state_t *state, const char *s, size_t *consumed) {
    if (!s || s[0] != '~') return NULL;

    /* Case 1: ~ or ~/ or ~: */
    if (s[1] == '\0' || s[1] == '/' || s[1] == ':') {
        const char *home = getenv("HOME");
        *consumed = 1;
        return home ? home : "";
    }

    /* Case 2: ~+ or ~+/ or ~+: */
    if (s[1] == '+' && (s[2] == '\0' || s[2] == '/' || s[2] == ':')) {
        const char *pwd = getenv("PWD");
        *consumed = 2;
        return pwd ? pwd : "";
    }

    /* Case 3: ~- or ~-N or ~-/ or ~-: */
    if (s[1] == '-') {
        if (isdigit((unsigned char)s[2])) {
            char *endp = NULL;
            long val = strtol(s + 2, &endp, 10);
            if (endp && (*endp == '\0' || *endp == '/' || *endp == ':')) {
                zbash_array_t *arr = state ? zbash_find_array(state, "DIRSTACK") : NULL;
                if (arr && val >= 0 && val < arr->count) {
                    int idx = (arr->count - 1) - (int)val;
                    *consumed = endp - s;
                    return arr->elements[idx] ? arr->elements[idx] : "";
                }
                return NULL;
            }
        } else if (s[2] == '\0' || s[2] == '/' || s[2] == ':') {
            const char *oldpwd = getenv("OLDPWD");
            *consumed = 2;
            return oldpwd ? oldpwd : "";
        }
    }

    /* Case 4: ~N or ~+N */
    const char *digits = NULL;
    if (isdigit((unsigned char)s[1])) {
        digits = s + 1;
    } else if (s[1] == '+' && isdigit((unsigned char)s[2])) {
        digits = s + 2;
    }
    if (digits) {
        char *endp = NULL;
        long val = strtol(digits, &endp, 10);
        if (endp && (*endp == '\0' || *endp == '/' || *endp == ':')) {
            zbash_array_t *arr = state ? zbash_find_array(state, "DIRSTACK") : NULL;
            if (arr && val >= 0 && val < arr->count) {
                *consumed = endp - s;
                return arr->elements[val] ? arr->elements[val] : "";
            }
            return NULL;
        }
    }

    /* Case 5: ~user */
    size_t ulen = 1;
    while (s[ulen] && s[ulen] != '/' && s[ulen] != ':') ulen++;
    char ubuf[128];
    if (ulen - 1 < sizeof(ubuf)) {
        strncpy(ubuf, s + 1, ulen - 1);
        ubuf[ulen - 1] = '\0';
        struct passwd *pw = getpwnam(ubuf);
        if (pw && pw->pw_dir) {
            *consumed = ulen;
            return pw->pw_dir;
        }
    }

    return NULL;
}

static char *expand_rhs(zbash_state_t *state, const char *rhs, bool in_double) {
    if (!rhs) return strdup("");
    bool prev_in_rhs = state ? state->in_rhs : false;
    if (state) state->in_rhs = true;
    if (!in_double) {
        char *res = zbash_expand_vars(state, rhs);
        if (state) state->in_rhs = prev_in_rhs;
        return res;
    }

    size_t cap = strlen(rhs) * 2 + 128;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) return strdup(rhs);

    bool inner_d = false;

    for (size_t i = 0; rhs[i] != '\0'; i++) {
        char c = rhs[i];

        if (c == '"') {
            inner_d = !inner_d;
            continue;
        }

        if (c == '\\') {
            char next = rhs[i + 1];
            if (next == '\0') {
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = '\\';
                break;
            }

            if (inner_d) {
                if (next == '$' || next == '`' || next == '"' || next == '\\') {
                    i++;
                    c = next;
                } else if (next == '\'') {
                    i++;
                    c = '\'';
                } else {
                    i++;
                    c = next;
                }
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = c;
                continue;
            } else {
                if (next == '$') {
                    i++;
                    if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    buf[len++] = '$';
                    continue;
                } else if (next == '\\') {
                    i++;
                    if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    buf[len++] = '\\';
                    continue;
                } else if (next == '"') {
                    i++;
                    if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    buf[len++] = '"';
                    continue;
                } else if (next == '\'') {
                    i++;
                    if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    buf[len++] = '\\';
                    buf[len++] = '\'';
                    continue;
                } else {
                    i++;
                    if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    buf[len++] = '\\';
                    buf[len++] = next;
                    continue;
                }
            }
        }

        if (c == '$') {
            size_t vstart = i;
            size_t vend = i + 1;
            if (rhs[vend] == '{') {
                int depth = 1;
                vend++;
                while (rhs[vend] && depth > 0) {
                    if (rhs[vend] == '\\' && rhs[vend + 1]) { vend += 2; continue; }
                    if (rhs[vend] == '{') depth++;
                    else if (rhs[vend] == '}') { depth--; if (depth == 0) { vend++; break; } }
                    vend++;
                }
            } else if (rhs[vend] == '\'') {
                vend++;
                while (rhs[vend] && rhs[vend] != '\'') {
                    if (rhs[vend] == '\\' && rhs[vend + 1]) vend += 2;
                    else vend++;
                }
                if (rhs[vend] == '\'') vend++;
            } else if (isalpha((unsigned char)rhs[vend]) || rhs[vend] == '_') {
                while (isalnum((unsigned char)rhs[vend]) || rhs[vend] == '_') vend++;
            } else if (isdigit((unsigned char)rhs[vend]) || rhs[vend] == '?' || rhs[vend] == '$' || rhs[vend] == '!' || rhs[vend] == '#' || rhs[vend] == '@' || rhs[vend] == '*') {
                vend++;
            }

            if (vend > vstart + 1) {
                size_t vlen = vend - vstart;
                char *vstr = malloc(vlen + 1);
                if (vstr) {
                    memcpy(vstr, rhs + vstart, vlen);
                    vstr[vlen] = '\0';
                    char *exp_v = zbash_expand_vars(state, vstr);
                    free(vstr);
                    if (exp_v) {
                        size_t elen = strlen(exp_v);
                        while (len + elen >= cap) { cap = (cap + elen) * 2; buf = realloc(buf, cap); }
                        memcpy(buf + len, exp_v, elen);
                        len += elen;
                        free(exp_v);
                    }
                }
                i = vend - 1;
                continue;
            }
        }

        if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        buf[len++] = c;
    }

    buf[len] = '\0';
    if (state) state->in_rhs = prev_in_rhs;
    return buf;
}

static char *find_slice_separator(char *s) {
    if (!s) return NULL;
    int p_depth = 0;
    int q_depth = 0;
    for (char *p = s; *p; p++) {
        if (*p == '(') p_depth++;
        else if (*p == ')') { if (p_depth > 0) p_depth--; }
        else if (*p == '?' && p_depth == 0) q_depth++;
        else if (*p == ':') {
            if (p_depth > 0) continue;
            if (q_depth > 0) { q_depth--; continue; }
            return p;
        }
    }
    return NULL;
}

static bool get_indirect_param_value(zbash_state_t *state, const char *vname_id, char *buf, size_t buf_sz, const char **out_val, char **out_dynamic) {
    char tgt_buf[128];
    const char *tgt_name = NULL;
    char *dyn_tgt = NULL;
    bool found = zbash_get_param_value(state, vname_id, tgt_buf, sizeof(tgt_buf), &tgt_name, &dyn_tgt);
    if (!found || !tgt_name || !*tgt_name) {
        if (dyn_tgt) free(dyn_tgt);
        *out_val = NULL;
        *out_dynamic = NULL;
        return false;
    }
    char real_name[128];
    strncpy(real_name, tgt_name, sizeof(real_name) - 1);
    real_name[sizeof(real_name) - 1] = '\0';
    if (dyn_tgt) free(dyn_tgt);

    /* Check if target name is an array subscript like arr[@] or arr[*] */
    if (strstr(real_name, "[@]") || strstr(real_name, "[*]")) {
        char *br = strchr(real_name, '[');
        if (br) *br = '\0';
        zbash_array_t *arr = zbash_find_array(state, real_name);
        if (arr && arr->count > 0) {
            size_t cap = 1024;
            char *dyn = malloc(cap);
            if (dyn) {
                dyn[0] = '\0';
                for (int a = 0; a < arr->count; a++) {
                    if (!arr->elements[a]) continue;
                    if (dyn[0] != '\0') strcat(dyn, " ");
                    size_t elen = strlen(arr->elements[a]);
                    while (strlen(dyn) + elen + 2 >= cap) {
                        cap *= 2;
                        dyn = realloc(dyn, cap);
                    }
                    strcat(dyn, arr->elements[a]);
                }
                *out_dynamic = dyn;
                *out_val = dyn;
                return true;
            }
        }
    }
    return zbash_get_param_value(state, real_name, buf, buf_sz, out_val, out_dynamic);
}

static bool is_valid_tilde_pos(zbash_state_t *state, const char *input, size_t pos) {
    if (pos == 0) return true;
    if (isspace((unsigned char)input[pos - 1])) return true;

    /* Inside parameter expansion default/alternative (${var:-default}),
     * tilde expansion only happens at the start of the default value (pos == 0 or after space),
     * never after = or : because it is not an assignment statement. */
    if (state && state->in_rhs) return false;

    /* If preceded by '=': check if this is the assignment operator of a variable assignment */
    if (input[pos - 1] == '=') {
        /* In POSIX mode, assignments that are arguments to commands do NOT expand tilde */
        if (state && state->posix && state->in_cmd_args) return false;

        /* Find start of current word */
        size_t w = pos - 1;
        while (w > 0 && !isspace((unsigned char)input[w - 1])) {
            w--;
        }
        /* Verify that input[w ... pos-2] is a valid variable identifier (optionally with array index or +=) */
        size_t k = w;
        if (!isalpha((unsigned char)input[k]) && input[k] != '_') return false;
        k++;
        while (k < pos - 1 && (isalnum((unsigned char)input[k]) || input[k] == '_')) {
            k++;
        }
        if (k < pos - 1 && input[k] == '[') {
            int depth = 1;
            k++;
            while (k < pos - 1 && depth > 0) {
                if (input[k] == '[') depth++;
                else if (input[k] == ']') depth--;
                k++;
            }
        }
        if (k < pos - 1 && input[k] == '+' && k + 1 == pos - 1) {
            k++;
        }
        return (k == pos - 1);
    }

    /* If preceded by ':': only expands if we are inside the value portion of an assignment statement */
    if (input[pos - 1] == ':') {
        if (state && state->posix && state->in_cmd_args) return false;

        /* Find start of current word */
        size_t w = pos - 1;
        while (w > 0 && !isspace((unsigned char)input[w - 1])) {
            w--;
        }
        /* Check if word begins with a valid identifier followed by '=' */
        size_t k = w;
        if (!isalpha((unsigned char)input[k]) && input[k] != '_') return false;
        k++;
        while (k < pos - 1 && (isalnum((unsigned char)input[k]) || input[k] == '_')) {
            k++;
        }
        if (k < pos - 1 && input[k] == '[') {
            int depth = 1;
            k++;
            while (k < pos - 1 && depth > 0) {
                if (input[k] == '[') depth++;
                else if (input[k] == ']') depth--;
                k++;
            }
        }
        if (k < pos - 1 && input[k] == '+' && k + 1 < pos - 1 && input[k + 1] == '=') {
            k++;
        }
        if (k < pos - 1 && input[k] == '=') {
            return true;
        }
        return false;
    }

    return false;
}

char *zbash_expand_vars(zbash_state_t *state, const char *input) {
    if (!input) return NULL;
    size_t cap = 2048;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) return NULL;

    bool in_single = false;
    bool in_double = false;

    for (size_t i = 0; input[i] != '\0'; i++) {
        char c = input[i];

        /* Tilde expansion at word start or after = / : */
        if (c == '~' && !in_single && !in_double) {
            bool at_tilde_pos = is_valid_tilde_pos(state, input, i);
            if (at_tilde_pos) {
                size_t consumed = 0;
                const char *exp = try_expand_tilde(state, input + i, &consumed);
                if (exp) {
                    size_t elen = strlen(exp);
                    while (len + elen >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    memcpy(buf + len, exp, elen);
                    len += elen;
                    i += consumed - 1;
                    continue;
                }
            }
        }

        if (c == '\\' && !in_single) {
            if (input[i + 1] == '\0') {
                if (!in_double) break; /* Lone trailing backslash in unquoted word is stripped */
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = '\\';
                break;
            }
            char next = input[i + 1];
            if (next == '\n') {
                i++;
                continue;
            }
            if (in_double) {
                if (next == '$' || next == '`' || next == '"' || next == '\\') {
                    i++;
                    c = next;
                } else {
                    if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    buf[len++] = '\\';
                    continue;
                }
            } else {
                i++;
                c = next;
            }
            if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
            buf[len++] = c;
            continue;
        }

        /* Locale Translation Quoting $"..." in default locale (acts like "...") */
        if (c == '$' && input[i + 1] == '"' && !in_single && !in_double) {
            continue;
        }

        /* ANSI-C Quoting $'...' */
        if (c == '$' && input[i + 1] == '\'' && !in_single && !in_double) {
            i += 2;
            while (input[i] != '\0' && input[i] != '\'') {
                char ch = input[i];
                if (ch == '\\' && input[i + 1] != '\0') {
                    i++;
                    char esc = input[i];
                    if (esc == 'n') ch = '\n';
                    else if (esc == 't') ch = '\t';
                    else if (esc == 'r') ch = '\r';
                    else if (esc == 'a') ch = '\a';
                    else if (esc == 'b') ch = '\b';
                    else if (esc == 'v') ch = '\v';
                    else if (esc == 'f') ch = '\f';
                    else if (esc == 'e' || esc == 'E') ch = '\033';
                    else if (esc == '\\') ch = '\\';
                    else if (esc == '\'') ch = '\'';
                    else if (esc == '"') ch = '"';
                    else if (esc >= '0' && esc <= '7') {
                        int val = esc - '0';
                        int count = 1;
                        while (count < 3 && input[i + 1] >= '0' && input[i + 1] <= '7') {
                            i++; count++;
                            val = val * 8 + (input[i] - '0');
                        }
                        ch = (char)val;
                    } else if (esc == 'c') {
                        if (input[i + 1] != '\0' && input[i + 1] != '\'') {
                            i++;
                            char c2 = input[i];
                            if (c2 == '?') ch = 127;
                            else ch = (char)(toupper((unsigned char)c2) ^ 0x40);
                        } else {
                            if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                            buf[len++] = '\\';
                            ch = 'c';
                        }
                    } else if (esc == 'x') {
                        if (input[i + 1] == '{') {
                            i++; /* consume '{' */
                            unsigned int val = 0;
                            while (isxdigit((unsigned char)input[i + 1])) {
                                i++;
                                char h = input[i];
                                int digit = (h >= '0' && h <= '9') ? (h - '0') :
                                            (h >= 'a' && h <= 'f') ? (h - 'a' + 10) :
                                            (h - 'A' + 10);
                                val = (val * 16 + digit) & 0xFF;
                            }
                            if (input[i + 1] == '}') {
                                i++; /* consume '}' */
                            }
                            ch = (char)(val & 0xFF);
                        } else {
                            int val = 0, count = 0;
                            while (count < 2 && isxdigit((unsigned char)input[i + 1])) {
                                i++; count++;
                                char h = input[i];
                                int digit = (h >= '0' && h <= '9') ? (h - '0') :
                                            (h >= 'a' && h <= 'f') ? (h - 'a' + 10) :
                                            (h - 'A' + 10);
                                val = val * 16 + digit;
                            }
                            ch = (char)val;
                        }
                    } else {
                        if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                        buf[len++] = '\\';
                        ch = esc;
                    }
                }
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = ch;
                i++;
            }
            continue;
        }

        if (c == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }

        if (c == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }

        /* Backtick command substitution `cmd` */
        if (c == '`' && !in_single) {
            size_t start = i + 1;
            size_t j = start;
            while (input[j] != '\0' && input[j] != '`') {
                if (input[j] == '\\' && input[j + 1] != '\0') j += 2;
                else j++;
            }
            if (input[j] == '`') {
                size_t clen = j - start;
                char *sub_cmd = malloc(clen + 1);
                if (sub_cmd) {
                    size_t sidx = 0;
                    for (size_t k = start; k < j; k++) {
                        if (input[k] == '\\' && k + 1 < j) {
                            char next = input[k + 1];
                            if (next == '$' || next == '`' || next == '\\') {
                                sub_cmd[sidx++] = next;
                                k++;
                                continue;
                            } else if (next == '\n') {
                                k++;
                                continue;
                            } else if (in_double && next == '"') {
                                sub_cmd[sidx++] = next;
                                k++;
                                continue;
                            }
                        }
                        sub_cmd[sidx++] = input[k];
                    }
                    sub_cmd[sidx] = '\0';

                    char *out = zbash_subshell_exec(state, sub_cmd);
                    if (state) state->cmd_subst_occurred = true;
                    if (out) {
                        size_t olen = strlen(out);
                        while (len + olen >= cap) { cap = (cap + olen) * 2; buf = realloc(buf, cap); }
                        memcpy(buf + len, out, olen);
                        len += olen;
                        free(out);
                    }
                    free(sub_cmd);
                }
                i = j;
                continue;
            }
        }

        if (!state->posix && (c == '<' || c == '>') && input[i + 1] == '(' && !in_single) {
            bool is_in = (c == '<');
            i += 2;
            size_t start = i;
            int pdepth = 1;
            bool q_stack_s[64] = {false};
            bool q_stack_d[64] = {false};

            while (input[i] != '\0') {
                char sc = input[i];
                if (sc == '\\' && input[i + 1] != '\0') {
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
                        if (pdepth == 0) break;
                    }
                }
                i++;
            }

            size_t clen = (i >= start) ? (i - start) : 0;
            char *sub_cmd = malloc(clen + 1);
            if (sub_cmd) {
                memcpy(sub_cmd, input + start, clen);
                sub_cmd[clen] = '\0';

                int pfd[2];
                if (pipe(pfd) == 0) {
                    pid_t pid = fork();
                    if (pid == 0) {
                        if (is_in) {
                            close(pfd[0]);
                            dup2(pfd[1], STDOUT_FILENO);
                            close(pfd[1]);
                        } else {
                            close(pfd[1]);
                            dup2(pfd[0], STDIN_FILENO);
                            close(pfd[0]);
                        }
                        zbash_run_line(state, sub_cmd);
                        fflush(stdout);
                        fflush(stderr);
                        _exit(state->last_status);
                    } else if (pid > 0) {
                        char fd_path[64];
                        if (is_in) {
                            close(pfd[1]);
                            snprintf(fd_path, sizeof(fd_path), "/dev/fd/%d", pfd[0]);
                        } else {
                            close(pfd[0]);
                            snprintf(fd_path, sizeof(fd_path), "/dev/fd/%d", pfd[1]);
                        }
                        size_t fplen = strlen(fd_path);
                        while (len + fplen >= cap) { cap = (cap + fplen) * 2; buf = realloc(buf, cap); }
                        memcpy(buf + len, fd_path, fplen);
                        len += fplen;
                    }
                }
                free(sub_cmd);
            }
            continue;
        }

        if (c == '$' && !in_single) {
            i++;
            if (input[i] == '\0') {
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = '$';
                break;
            }

            /* 1. Arithmetic $(( expr )) or $[ expr ] */
            if ((input[i] == '(' && input[i + 1] == '(') || input[i] == '[') {
                bool is_bracket = (input[i] == '[');
                i += is_bracket ? 1 : 2;
                size_t start = i;
                int pdepth = 0;
                while (input[i] != '\0') {
                    if (is_bracket) {
                        if (input[i] == '[') { pdepth++; i++; continue; }
                        if (input[i] == ']') {
                            if (pdepth > 0) { pdepth--; i++; continue; }
                            break;
                        }
                    } else {
                        if (input[i] == '(') {
                            pdepth++;
                            i++;
                            continue;
                        }
                        if (input[i] == ')') {
                            if (pdepth > 0) {
                                pdepth--;
                                i++;
                                continue;
                            } else if (input[i + 1] == ')') {
                                break;
                            }
                        }
                    }
                    i++;
                }
                size_t elen = (i >= start) ? (i - start) : 0;
                char expr[512];
                if (elen >= sizeof(expr)) elen = sizeof(expr) - 1;
                strncpy(expr, input + start, elen);
                expr[elen] = '\0';

                if (!is_bracket && input[i] == ')' && input[i + 1] == ')') i++; /* skip to second ')' */

                long long num = zbash_eval_arithmetic(state, expr);
                char num_buf[32];
                snprintf(num_buf, sizeof(num_buf), "%lld", num);
                size_t nlen = strlen(num_buf);
                while (len + nlen >= cap) { cap = (cap + nlen) * 2; buf = realloc(buf, cap); }
                memcpy(buf + len, num_buf, nlen);
                len += nlen;
                continue;
            }

            /* 2. Command substitution $( cmd ) */
            if (input[i] == '(') {
                i++;
                size_t start = i;
                int pdepth = 1;
                bool q_stack_s[64] = {false};
                bool q_stack_d[64] = {false};

                while (input[i] != '\0') {
                    char sc = input[i];
                    if (sc == '\\' && input[i + 1] != '\0') {
                        i += 2;
                        continue;
                    }
                    if (sc == '\'' && !q_stack_d[pdepth]) {
                        q_stack_s[pdepth] = !q_stack_s[pdepth];
                    } else if (sc == '"' && !q_stack_s[pdepth]) {
                        q_stack_d[pdepth] = !q_stack_d[pdepth];
                    } else if (!q_stack_s[pdepth] && !q_stack_d[pdepth]) {
                        if (sc == '$' && input[i + 1] == '(') {
                            if (pdepth < 63) {
                                pdepth++;
                                q_stack_s[pdepth] = false;
                                q_stack_d[pdepth] = false;
                            }
                            i += 2;
                            continue;
                        } else if (sc == '(') {
                            if (pdepth < 63) {
                                pdepth++;
                                q_stack_s[pdepth] = false;
                                q_stack_d[pdepth] = false;
                            }
                        } else if (sc == ')') {
                            pdepth--;
                            if (pdepth == 0) break;
                        }
                    } else if (q_stack_d[pdepth] && sc == '$' && input[i + 1] == '(') {
                        if (pdepth < 63) {
                            pdepth++;
                            q_stack_s[pdepth] = false;
                            q_stack_d[pdepth] = false;
                        }
                        i += 2;
                        continue;
                    }
                    i++;
                }

                size_t clen = (i >= start) ? (i - start) : 0;
                char *sub_cmd = malloc(clen + 1);
                if (sub_cmd) {
                    memcpy(sub_cmd, input + start, clen);
                    sub_cmd[clen] = '\0';

                    char *out = NULL;
                    const char *trimmed = sub_cmd;
                    while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
                    if (*trimmed == '<') {
                        trimmed++;
                        while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
                        char *exp_file = zbash_expand_vars(state, trimmed);
                        const char *target_file = exp_file ? exp_file : trimmed;
                        char resolved_file[1024];
                        resolved_file[0] = '\0';
                        if (!state->posix && (strchr(target_file, '*') || strchr(target_file, '?'))) {
                            glob_t g;
                            if (glob(target_file, 0, NULL, &g) == 0) {
                                if (g.gl_pathc > 1) {
                                    const char *src = get_script_name(state);
                                    int lineno = state->lineno > 0 ? state->lineno : 1;
                                    fprintf(stderr, "%s: line %d: %s: ambiguous redirect\n", src, lineno, target_file);
                                    globfree(&g);
                                    state->last_status = 1;
                                    out = strdup("");
                                    if (exp_file) free(exp_file);
                                    goto after_fast_file_read;
                                } else if (g.gl_pathc == 1) {
                                    strncpy(resolved_file, g.gl_pathv[0], sizeof(resolved_file) - 1);
                                    resolved_file[sizeof(resolved_file) - 1] = '\0';
                                    target_file = resolved_file;
                                }
                                globfree(&g);
                            }
                        }
                        FILE *fp = fopen(target_file, "r");
                        if (fp) {
                            fseek(fp, 0, SEEK_END);
                            long fsize = ftell(fp);
                            fseek(fp, 0, SEEK_SET);
                            if (fsize >= 0) {
                                out = malloc(fsize + 1);
                                if (out) {
                                    size_t nread = fread(out, 1, fsize, fp);
                                    out[nread] = '\0';
                                    while (nread > 0 && out[nread - 1] == '\n') {
                                        out[--nread] = '\0';
                                    }
                                }
                            }
                            fclose(fp);
                            state->last_status = 0;
                        } else {
                            const char *src = get_script_name(state);
                            int lineno = state->lineno > 0 ? state->lineno : 1;
                            fprintf(stderr, "%s: line %d: %s: No such file or directory\n", src, lineno, target_file);
                            state->last_status = 1;
                            out = strdup("");
                        }
                        if (exp_file) free(exp_file);
                    } else {
                        out = zbash_subshell_exec(state, sub_cmd);
                    }
after_fast_file_read:
                    if (state) state->cmd_subst_occurred = true;
                    if (out) {
                        size_t olen = strlen(out);
                        while (len + olen >= cap) { cap = (cap + olen) * 2; buf = realloc(buf, cap); }
                        memcpy(buf + len, out, olen);
                        len += olen;
                        free(out);
                    }
                    free(sub_cmd);
                }
                continue;
            }

            /* 3. Parameter expansion ${ ... } */
            char var_name[256];
            size_t vn_len = 0;
            const char *val = NULL;
            char num_buf[64];
            char *dynamic_val = NULL;

            if (input[i] == '{') {
                i++;
                int b_depth = 1;
                bool in_s = false, in_d = false;
                while (input[i] != '\0') {
                    if (input[i] == '\\' && input[i + 1] != '\0') {
                        if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                        if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                        continue;
                    }
                    if (input[i] == '`' && !in_s) {
                        if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                        while (input[i] != '\0' && input[i] != '`') {
                            if (input[i] == '\\' && input[i + 1] != '\0') {
                                if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                                if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                            } else {
                                if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                            }
                        }
                        if (input[i] == '`') {
                            if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                        }
                        continue;
                    }
                    if (input[i] == '$' && input[i + 1] == '(' && !in_s) {
                        if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                        if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                        int p_depth = 1;
                        bool p_s = false, p_d = false;
                        while (input[i] != '\0' && p_depth > 0) {
                            if (input[i] == '\\' && input[i + 1] != '\0') {
                                if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                                if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                                continue;
                            }
                            if (input[i] == '\'' && !p_d) p_s = !p_s;
                            else if (input[i] == '"' && !p_s) p_d = !p_d;
                            else if (!p_s && !p_d) {
                                if (input[i] == '(') p_depth++;
                                else if (input[i] == ')') {
                                    p_depth--;
                                    if (p_depth == 0) {
                                        if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                                        break;
                                    }
                                }
                            }
                            if (vn_len < sizeof(var_name) - 1) var_name[vn_len++] = input[i++];
                        }
                        continue;
                    }
                    if (input[i] == '\'' && !in_d) in_s = !in_s;
                    else if (input[i] == '"' && !in_s) in_d = !in_d;
                    else if (!in_s && !in_d) {
                        if (input[i] == '{' && i > 0 && input[i - 1] == '$') b_depth++;
                        else if (input[i] == '}') {
                            b_depth--;
                            if (b_depth == 0) break;
                        }
                    }
                    if (vn_len < sizeof(var_name) - 1) {
                        var_name[vn_len++] = input[i];
                    }
                    i++;
                }
                var_name[vn_len] = '\0';

                if (var_name[0] == '$' && (var_name[1] == '(' || var_name[1] == '{')) {
                    const char *src = get_script_name(state);
                    int lineno = state->lineno > 0 ? state->lineno : 1;
                    fprintf(stderr, "%s: line %d: ${%s}: bad substitution\n", src, lineno, var_name);
                    state->last_status = 1;
                    state->expansion_error = true;
                    val = "";
                    goto finish_var_eval;
                }

                /* Check for length: ${#VAR} or ${#arr[@]} or ${#arr[idx]} */
                if (var_name[0] == '#' && var_name[1] != '\0' &&
                    ((var_name[2] == '\0' && strchr("?#!$*@-0123456789", var_name[1])) ||
                     (!strchr(":=+?/%", var_name[1]) && !(var_name[1] == '-' && var_name[2] != '\0')))) {
                    if (isdigit((unsigned char)var_name[1])) {
                        const char *dp = var_name + 1;
                        while (isdigit((unsigned char)*dp)) dp++;
                        if (*dp != '\0') {
                            const char *src = get_script_name(state);
                            int lineno = state->lineno > 0 ? state->lineno : 1;
                            fprintf(stderr, "%s: line %d: ${%s}: bad substitution\n", src, lineno, var_name);
                            state->last_status = 1;
                            state->expansion_error = true;
                            val = "";
                            goto finish_var_eval;
                        }
                    }
                    if (strstr(var_name, "[@]") != NULL || strstr(var_name, "[*]") != NULL) {
                        char *b_start = strchr(var_name, '[');
                        *b_start = '\0';
                        zbash_array_t *arr = zbash_find_array(state, var_name + 1);
                        int cnt = 0;
                        if (arr) {
                            for (int k = 0; k < arr->count; k++) {
                                if (arr->elements[k]) cnt++;
                            }
                        } else if (getenv(var_name + 1)) {
                            cnt = 1;
                        }
                        snprintf(num_buf, sizeof(num_buf), "%d", cnt);
                        val = num_buf;
                    } else if (strchr(var_name, '[') != NULL) {
                        const char *vname_chk = var_name + 1;
                        size_t chk_len = 0;
                        if (isalpha((unsigned char)vname_chk[0]) || vname_chk[0] == '_') {
                            while (isalnum((unsigned char)vname_chk[chk_len]) || vname_chk[chk_len] == '_') chk_len++;
                        } else if (isdigit((unsigned char)vname_chk[0])) {
                            while (isdigit((unsigned char)vname_chk[chk_len])) chk_len++;
                        }
                        if (chk_len > 0 && vname_chk[chk_len] == '[') {
                            char *b_start = strchr(var_name, '[');
                            char *b_end = strrchr(b_start, ']');
                            if (b_end) {
                                *b_start = '\0';
                                *b_end = '\0';
                                const char *vname = var_name + 1;
                                if (state->nounset && !zbash_find_array(state, vname) && !getenv(vname)) {
                                    fprintf(stderr, "%s: line %d: %s: unbound variable\n", get_script_name(state), state->lineno > 0 ? state->lineno : 1, vname);
                                    state->last_status = 1;
                                    state->expansion_error = true;
                                    val = "";
                                } else {
                                    int idx = (int)zbash_eval_arithmetic(state, b_start + 1);
                                    const char *elem = zbash_get_array_elem(state, vname, idx);
                                    snprintf(num_buf, sizeof(num_buf), "%zu", elem ? utf8_char_count(elem) : 0);
                                    val = num_buf;
                                }
                            }
                        } else {
                            char *dyn_v = NULL;
                            const char *env_v = NULL;
                            bool is_set = get_param_value(state, var_name + 1, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                            if (state->nounset && !is_set) {
                                const char *src = get_script_name(state);
                                int lineno = state->lineno > 0 ? state->lineno : 1;
                                fprintf(stderr, "%s: line %d: %s: unbound variable\n", src, lineno, var_name + 1);
                                state->last_status = 1;
                                state->expansion_error = true;
                                state->should_exit = true;
                                state->exit_code = 1;
                                val = "";
                            } else {
                                snprintf(num_buf, sizeof(num_buf), "%zu", env_v ? utf8_char_count(env_v) : 0);
                                val = num_buf;
                            }
                            if (dyn_v) free(dyn_v);
                        }
                    } else {
                        char *dyn_v = NULL;
                        const char *env_v = NULL;
                        bool is_set = get_param_value(state, var_name + 1, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                        if (state->nounset && !is_set) {
                            const char *src = get_script_name(state);
                            int lineno = state->lineno > 0 ? state->lineno : 1;
                            fprintf(stderr, "%s: line %d: %s: unbound variable\n", src, lineno, var_name + 1);
                            state->last_status = 1;
                            state->expansion_error = true;
                            state->should_exit = true;
                            state->exit_code = 1;
                            val = "";
                        } else {
                            snprintf(num_buf, sizeof(num_buf), "%zu", env_v ? utf8_char_count(env_v) : 0);
                            val = num_buf;
                        }
                        if (dyn_v) free(dyn_v);
                    }
                }
                else if (var_name[0] == '#' && (var_name[1] == '/' || var_name[1] == '%' || var_name[1] == '=' || var_name[1] == '+')) {
                    const char *src = get_script_name(state);
                    int lineno = state->lineno > 0 ? state->lineno : 1;
                    fprintf(stderr, "%s: line %d: ${%s}: bad substitution\n", src, lineno, var_name);
                    state->last_status = 1;
                    state->expansion_error = true;
                    val = "";
                    goto finish_var_eval;
                }
                else if (var_name[0] == '!' && !strchr(var_name, '[') && (strchr(var_name, '*') || strchr(var_name, '@'))) {
                    bool ends_star_at = (vn_len >= 2 && (var_name[vn_len - 1] == '*' || var_name[vn_len - 1] == '@'));
                    char pfix[128] = {0};
                    size_t plen = ends_star_at ? (vn_len - 2) : 0;
                    if (plen >= sizeof(pfix)) plen = sizeof(pfix) - 1;
                    if (ends_star_at) strncpy(pfix, var_name + 1, plen);
                    pfix[plen] = '\0';

                    bool valid = (ends_star_at && plen > 0 && (isalpha((unsigned char)pfix[0]) || pfix[0] == '_'));
                    for (size_t k = 1; k < plen && valid; k++) {
                        if (!isalnum((unsigned char)pfix[k]) && pfix[k] != '_') valid = false;
                    }
                    if (!valid) {
                        const char *src = get_script_name(state);
                        int lineno = state->lineno > 0 ? state->lineno : 1;
                        fprintf(stderr, "%s: line %d: ${%s}: bad substitution\n", src, lineno, var_name);
                        state->last_status = 1;
                        state->expansion_error = true;
                        val = "";
                        goto finish_var_eval;
                    }
                    char sep_char = (var_name[vn_len - 1] == '*' && in_double) ? (getenv("IFS") ? getenv("IFS")[0] : ' ') : ' ';

                    extern char **environ;
                    char *matches[512];
                    int mcount = 0;
                    for (char **ep = environ; *ep && mcount < 512; ep++) {
                        char *eq = strchr(*ep, '=');
                        if (!eq) continue;
                        size_t klen = eq - *ep;
                        if (klen >= plen && strncmp(*ep, pfix, plen) == 0) {
                            char kname[128];
                            if (klen >= sizeof(kname)) klen = sizeof(kname) - 1;
                            strncpy(kname, *ep, klen);
                            kname[klen] = '\0';
                            bool dup = false;
                            for (int m = 0; m < mcount; m++) {
                                if (strcmp(matches[m], kname) == 0) { dup = true; break; }
                            }
                            if (!dup) matches[mcount++] = strdup(kname);
                        }
                    }
                    for (int a = 0; a < mcount - 1; a++) {
                        for (int b = a + 1; b < mcount; b++) {
                            if (strcmp(matches[a], matches[b]) > 0) {
                                char *tmp = matches[a];
                                matches[a] = matches[b];
                                matches[b] = tmp;
                            }
                        }
                    }
                    size_t total_sz = 0;
                    for (int m = 0; m < mcount; m++) total_sz += strlen(matches[m]) + 1;
                    dynamic_val = malloc(total_sz + 1);
                    if (dynamic_val) {
                        dynamic_val[0] = '\0';
                        for (int m = 0; m < mcount; m++) {
                            if (m > 0 && sep_char != '\0') {
                                size_t clen = strlen(dynamic_val);
                                dynamic_val[clen] = sep_char;
                                dynamic_val[clen + 1] = '\0';
                            }
                            strcat(dynamic_val, matches[m]);
                            free(matches[m]);
                        }
                        val = dynamic_val;
                    } else {
                        val = "";
                    }
                }
                /* Check for array indices ${!arr[@]} or elements ${arr[idx]} / ${arr[@]} */
                else if (is_array_subscript_expr(var_name)) {
                    char *b_start = strchr(var_name, '[');
                    char *scan_b = b_start + 1;
                    int bk_d = 1;
                    bool bk_s = false, bk_dq = false;
                    while (*scan_b && bk_d > 0) {
                        if (*scan_b == '\'' && !bk_dq) bk_s = !bk_s;
                        else if (*scan_b == '"' && !bk_s) bk_dq = !bk_dq;
                        else if (!bk_s && !bk_dq) {
                            if (*scan_b == '[') bk_d++;
                            else if (*scan_b == ']') {
                                bk_d--;
                                if (bk_d == 0) break;
                            }
                        }
                        scan_b++;
                    }
                    char *b_end = (bk_d == 0) ? scan_b : NULL;
                    if (b_end) {
                        char *after_b = b_end + 1;
                        *b_start = '\0';
                        *b_end = '\0';
                        bool is_indices = (var_name[0] == '!');
                        const char *aname = is_indices ? var_name + 1 : var_name;
                        const char *idx_str = b_start + 1;

                        bool all_ws = true;
                        for (const char *sp = idx_str; *sp; sp++) {
                            if (!isspace((unsigned char)*sp)) { all_ws = false; break; }
                        }
                        if (all_ws) {
                            const char *src = get_script_name(state);
                            int lineno = state->lineno > 0 ? state->lineno : 1;
                            fprintf(stderr, "%s: line %d: ${%s[%s]}: bad substitution\n", src, lineno, aname, idx_str);
                            state->last_status = 1;
                            state->expansion_error = true;
                            val = "";
                        } else if (strcmp(idx_str, "@") == 0 || strcmp(idx_str, "*") == 0) {
                            zbash_array_t *arr = zbash_find_array(state, aname);
                            if (is_indices) {
                                if (arr) {
                                    size_t buf_sz = (arr->count + 1) * 16 + 1;
                                    dynamic_val = malloc(buf_sz);
                                    if (dynamic_val) {
                                        dynamic_val[0] = '\0';
                                        bool first = true;
                                        for (int a = 0; a < arr->count; a++) {
                                            if (!arr->elements[a]) continue;
                                            if (!first) strcat(dynamic_val, " ");
                                            first = false;
                                            char num_s[16];
                                            snprintf(num_s, sizeof(num_s), "%d", a);
                                            strcat(dynamic_val, num_s);
                                        }
                                        val = dynamic_val;
                                    }
                                } else if (getenv(aname)) {
                                    val = "0";
                                } else {
                                    val = "";
                                }
                            } else {
                                bool has_slice = (*after_b == ':');
                                int a_offset = 0;
                                int a_len = -1;
                                if (has_slice) {
                                    char *slice_sep = find_slice_separator(after_b + 1);
                                    if (slice_sep) {
                                        *slice_sep = '\0';
                                        a_offset = (int)zbash_eval_arithmetic(state, after_b + 1);
                                        a_len = (int)zbash_eval_arithmetic(state, slice_sep + 1);
                                    } else {
                                        a_offset = (int)zbash_eval_arithmetic(state, after_b + 1);
                                    }
                                }

                                bool is_pat_rem = (*after_b == '#' || *after_b == '%');
                                bool is_prefix = (*after_b == '#');
                                bool longest = (is_pat_rem && *(after_b + 1) == *after_b);
                                char *pat_str = is_pat_rem ? (after_b + (longest ? 2 : 1)) : NULL;

                                bool is_pat_sub = (*after_b == '/');
                                bool replace_all = (is_pat_sub && *(after_b + 1) == '/');
                                char *sub_pat = NULL;
                                char *sub_rep = NULL;
                                if (is_pat_sub) {
                                    char *pat_start = after_b + (replace_all ? 2 : 1);
                                    char *slash2 = NULL;
                                    bool s_quote = false, d_quote = false;
                                    for (char *p = pat_start; *p; p++) {
                                        if (*p == '\\' && *(p + 1)) { p++; continue; }
                                        if (*p == '\'' && !d_quote) { s_quote = !s_quote; continue; }
                                        if (*p == '"' && !s_quote) { d_quote = !d_quote; continue; }
                                        if (*p == '/' && !s_quote && !d_quote) { slash2 = p; break; }
                                    }
                                    if (slash2) {
                                        *slash2 = '\0';
                                        sub_rep = slash2 + 1;
                                    } else {
                                        sub_rep = "";
                                    }
                                    sub_pat = pat_start;
                                }

                                char *exp_pat_str = pat_str ? zbash_expand_pattern(state, pat_str) : NULL;
                                const char *act_pat_str = exp_pat_str ? exp_pat_str : pat_str;
                                char *exp_sub_pat = sub_pat ? zbash_expand_pattern(state, sub_pat) : NULL;
                                const char *act_sub_pat = exp_sub_pat ? exp_sub_pat : sub_pat;
                                char *exp_sub_rep = (sub_rep && (strchr(sub_rep, '$') || strchr(sub_rep, '\'') || strchr(sub_rep, '"'))) ? zbash_expand_vars(state, sub_rep) : NULL;
                                const char *act_sub_rep = exp_sub_rep ? exp_sub_rep : sub_rep;

                                const char *elem_sep = " ";
                                char ifs_sep[2] = { ' ', '\0' };
                                if (strcmp(idx_str, "*") == 0) {
                                    const char *ifs = getenv("IFS");
                                    if (ifs) {
                                        ifs_sep[0] = ifs[0];
                                        elem_sep = ifs_sep;
                                    }
                                }

                                if (arr && arr->count > 0) {
                                    int start_idx = 0;
                                    if (has_slice) {
                                        if (a_offset < 0) {
                                            int max_idx = -1;
                                            for (int k = arr->count - 1; k >= 0; k--) {
                                                if (arr->elements[k]) { max_idx = k; break; }
                                            }
                                            start_idx = (max_idx >= 0 ? max_idx + 1 : 0) + a_offset;
                                        } else {
                                            start_idx = a_offset;
                                        }
                                        if (start_idx < 0) start_idx = 0;
                                    }

                                    size_t cap = 1024;
                                    dynamic_val = malloc(cap);
                                    if (dynamic_val) {
                                        dynamic_val[0] = '\0';
                                        size_t cur_len = 0;
                                        bool first = true;
                                        int collected = 0;

                                        for (int a = start_idx; a < arr->count && (a_len < 0 || collected < a_len); a++) {
                                            if (!arr->elements[a]) continue;
                                            collected++;

                                            char *transformed = NULL;
                                            if (is_pat_rem) {
                                                transformed = apply_pattern_removal(arr->elements[a], act_pat_str, is_prefix, longest);
                                            } else if (is_pat_sub) {
                                                transformed = apply_pattern_subst(arr->elements[a], act_sub_pat, act_sub_rep, replace_all);
                                            }
                                            const char *item = transformed ? transformed : arr->elements[a];
                                            if (!in_double && item[0] == '\0') {
                                                if (transformed) free(transformed);
                                                continue;
                                            }
                                            size_t ilen = strlen(item);
                                            size_t sep_len = strlen(elem_sep);

                                            while (cur_len + ilen + sep_len + 4 >= cap) {
                                                cap = (cap + ilen + sep_len + 64) * 2;
                                                dynamic_val = realloc(dynamic_val, cap);
                                            }

                                            if (!first && sep_len > 0) {
                                                memcpy(dynamic_val + cur_len, elem_sep, sep_len);
                                                cur_len += sep_len;
                                            }
                                            first = false;
                                            memcpy(dynamic_val + cur_len, item, ilen);
                                            cur_len += ilen;
                                            dynamic_val[cur_len] = '\0';

                                            if (transformed) free(transformed);
                                        }
                                        val = dynamic_val;
                                    }
                                } else {
                                    const char *val0 = getenv(aname);
                                    if (val0) {
                                        if (has_slice) {
                                            size_t vlen = strlen(val0);
                                            int offset = a_offset;
                                            int slen = a_len;
                                            if (offset < 0) offset = vlen + offset;
                                            if (offset < 0) offset = 0;
                                            if (offset > (int)vlen) offset = vlen;
                                            if (slen < 0 || offset + slen > (int)vlen) slen = vlen - offset;
                                            const char *p_start = utf8_char_at(val0, offset);
                                            const char *p_end = utf8_char_at(val0, offset + slen);
                                            size_t sub_len = p_end - p_start;
                                            dynamic_val = malloc(sub_len + 1);
                                            if (dynamic_val) {
                                                strncpy(dynamic_val, p_start, sub_len);
                                                dynamic_val[sub_len] = '\0';
                                                val = dynamic_val;
                                            } else {
                                                val = "";
                                            }
                                        } else {
                                            char *transformed = NULL;
                                            if (is_pat_rem) {
                                                transformed = apply_pattern_removal(val0, act_pat_str, is_prefix, longest);
                                            } else if (is_pat_sub) {
                                                transformed = apply_pattern_subst(val0, act_sub_pat, act_sub_rep, replace_all);
                                            }
                                            if (transformed) {
                                                dynamic_val = transformed;
                                                val = dynamic_val;
                                            } else {
                                                val = val0;
                                            }
                                        }
                                    } else {
                                        val = "";
                                    }
                                }
                                if (exp_pat_str) free(exp_pat_str);
                                if (exp_sub_pat) free(exp_sub_pat);
                                if (exp_sub_rep) free(exp_sub_rep);
                            }
                        } else {
                            /* Single element subscript: ${arr[idx]} */
                            int idx = (int)zbash_eval_arithmetic(state, idx_str);
                            if (idx < 0 && *after_b != ':') {
                                int lineno = state->lineno > 0 ? state->lineno : 1;
                                fprintf(stderr, "%s: line %d: %s: bad array subscript\n", get_script_name(state), lineno, aname);
                                val = "";
                            } else {
                                const char *elem_val = zbash_get_array_elem(state, aname, idx);
                                if (!elem_val) elem_val = "";

                                if (*after_b == ':') {
                                if (after_b[1] == '-') {
                                    /* ${arr[idx]:-default} */
                                    const char *def_val = after_b + 2;
                                    if (elem_val && *elem_val) {
                                        val = elem_val;
                                    } else {
                                        dynamic_val = expand_rhs(state, def_val, in_double);
                                        val = dynamic_val;
                                    }
                                } else if (after_b[1] == '=') {
                                    /* ${arr[idx]:=default} */
                                    const char *def_val = after_b + 2;
                                    if (elem_val && *elem_val) {
                                        val = elem_val;
                                    } else {
                                        char *exp_def = expand_rhs(state, def_val, in_double);
                                        zbash_set_array_elem(state, aname, idx, exp_def);
                                        dynamic_val = exp_def;
                                        val = exp_def;
                                    }
                                } else if (after_b[1] == '+') {
                                    /* ${arr[idx]:+alt} */
                                    const char *alt_val = after_b + 2;
                                    if (elem_val && *elem_val) {
                                        dynamic_val = expand_rhs(state, alt_val, in_double);
                                        val = dynamic_val;
                                    } else {
                                        val = "";
                                    }
                                } else {
                                    int elen = (int)utf8_char_count(elem_val);
                                    char *sep2 = find_slice_separator(after_b + 1);
                                    int offset = 0, slen = -1;
                                    if (sep2) {
                                        *sep2 = '\0';
                                        offset = (int)zbash_eval_arithmetic(state, after_b + 1);
                                        slen = (int)zbash_eval_arithmetic(state, sep2 + 1);
                                    } else {
                                        offset = (int)zbash_eval_arithmetic(state, after_b + 1);
                                    }
                                    if (offset < 0) offset = elen + offset;
                                    if (offset < 0) offset = 0;
                                    if (offset > elen) offset = elen;
                                    if (slen < 0 || offset + slen > elen) slen = elen - offset;
                                    if (slen < 0) slen = 0;

                                    const char *p_start = utf8_char_at(elem_val, offset);
                                    const char *p_end = utf8_char_at(p_start, slen);
                                    size_t byte_len = (size_t)(p_end - p_start);
                                    dynamic_val = malloc(byte_len + 1);
                                    if (dynamic_val) {
                                        memcpy(dynamic_val, p_start, byte_len);
                                        dynamic_val[byte_len] = '\0';
                                        val = dynamic_val;
                                    } else {
                                        val = "";
                                    }
                                }
                            } else if (*after_b == '-') {
                                const char *def_val = after_b + 1;
                                if (elem_val && *elem_val) {
                                    val = elem_val;
                                } else {
                                    dynamic_val = expand_rhs(state, def_val, in_double);
                                    val = dynamic_val;
                                }
                            } else if (*after_b == '#' || *after_b == '%') {
                                bool is_prefix = (*after_b == '#');
                                bool longest = (*(after_b + 1) == *after_b);
                                char *pat = after_b + (longest ? 2 : 1);
                                char *exp_p = pat ? zbash_expand_pattern(state, pat) : NULL;
                                dynamic_val = apply_pattern_removal(elem_val, exp_p ? exp_p : pat, is_prefix, longest);
                                val = dynamic_val ? dynamic_val : elem_val;
                                if (exp_p) free(exp_p);
                            } else if (*after_b == '/') {
                                bool replace_all = (*(after_b + 1) == '/');
                                char *pat_start = after_b + (replace_all ? 2 : 1);
                                char *slash2 = NULL;
                                bool s_quote = false, d_quote = false;
                                for (char *p = pat_start; *p; p++) {
                                    if (*p == '\\' && *(p + 1)) { p++; continue; }
                                    if (*p == '\'' && !d_quote) { s_quote = !s_quote; continue; }
                                    if (*p == '"' && !s_quote) { d_quote = !d_quote; continue; }
                                    if (*p == '/' && !s_quote && !d_quote) { slash2 = p; break; }
                                }
                                const char *rep = "";
                                if (slash2) {
                                    *slash2 = '\0';
                                    rep = slash2 + 1;
                                }
                                char *exp_p = pat_start ? zbash_expand_pattern(state, pat_start) : NULL;
                                char *exp_r = (strchr(rep, '$') || strchr(rep, '\'') || strchr(rep, '"')) ? zbash_expand_vars(state, rep) : NULL;
                                dynamic_val = apply_pattern_subst(elem_val, exp_p ? exp_p : pat_start, exp_r ? exp_r : rep, replace_all);
                                val = dynamic_val ? dynamic_val : elem_val;
                                if (exp_p) free(exp_p);
                                if (exp_r) free(exp_r);
                            } else {
                                val = elem_val;
                            }
                        }
                    }
                }
            }
            else {
                    bool is_indirect = false;
                    const char *actual_vname = var_name;
                    if (var_name[0] == '!' && var_name[1] != '\0' && !strchr("-+=?:/%", var_name[1])) {
                        is_indirect = true;
                        actual_vname = var_name + 1;
                    }

                    /* Parse leading variable name identifier [a-zA-Z_][a-zA-Z0-9_]* or special param / digit */
                    size_t vid_len = 0;
                    if (actual_vname[0] == '$' || actual_vname[0] == '?' || actual_vname[0] == '!' ||
                        actual_vname[0] == '#' || actual_vname[0] == '@' || actual_vname[0] == '*' ||
                        actual_vname[0] == '-') {
                        vid_len = 1;
                    } else if (isdigit((unsigned char)actual_vname[0])) {
                        while (isdigit((unsigned char)actual_vname[vid_len])) vid_len++;
                    } else if (isalpha((unsigned char)actual_vname[0]) || actual_vname[0] == '_') {
                        while (isalnum((unsigned char)actual_vname[vid_len]) || actual_vname[vid_len] == '_') vid_len++;
                    }

                    char op_char = actual_vname[vid_len];
                    char vname_id[128];
                    if (vid_len >= sizeof(vname_id)) vid_len = sizeof(vname_id) - 1;
                    strncpy(vname_id, actual_vname, vid_len);
                    vname_id[vid_len] = '\0';
                    char *rest = (char *)actual_vname + vid_len;

#define FETCH_PARAM(name, buf, sz, oval, odyn) \
    (is_indirect ? get_indirect_param_value(state, name, buf, sz, oval, odyn) : get_param_value(state, name, buf, sz, oval, odyn))

                    if (op_char == '\0') {
                        /* Simple ${VAR} or ${1} or special ${$}, ${?}, ${*}, ${@}, etc. */
                        char *dyn_v = NULL;
                        const char *pv = NULL;
                        bool is_set = FETCH_PARAM(vname_id, num_buf, sizeof(num_buf), &pv, &dyn_v);
                        if (state->nounset && !is_set && strcmp(vname_id, "*") != 0 && strcmp(vname_id, "@") != 0 && strcmp(vname_id, "#") != 0 && strcmp(vname_id, "?") != 0 && strcmp(vname_id, "$") != 0 && strcmp(vname_id, "!") != 0 && strcmp(vname_id, "-") != 0) {
                            const char *src = get_script_name(state);
                            int lineno = state->lineno > 0 ? state->lineno : 1;
                            fprintf(stderr, "%s: line %d: %s: unbound variable\n", src, lineno, vname_id);
                            state->last_status = 1;
                            state->expansion_error = true;
                            state->should_exit = true;
                            state->exit_code = 1;
                            val = "";
                        } else if (dyn_v) {
                            dynamic_val = dyn_v;
                            val = dyn_v;
                        } else {
                            val = is_set ? pv : "";
                        }
                    } else if (op_char == ':') {
                        char *dyn_v = NULL;
                        const char *env_v = NULL;
                        bool is_set = FETCH_PARAM(vname_id, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                        if (rest[1] == '-') {
                            /* ${VAR:-default} */
                            const char *def_val = rest + 2;
                            if (is_set && env_v && *env_v) {
                                val = env_v;
                                if (dyn_v) dynamic_val = dyn_v;
                            } else {
                                if (dyn_v) free(dyn_v);
                                dynamic_val = expand_rhs(state, def_val, in_double);
                                val = dynamic_val;
                            }
                        } else if (rest[1] == '=') {
                            /* ${VAR:=default} */
                            if (isdigit((unsigned char)vname_id[0]) || strchr("*@#?!-$", vname_id[0])) {
                                const char *src = get_script_name(state);
                                int lineno = state->lineno > 0 ? state->lineno : 1;
                                fprintf(stderr, "%s: line %d: $%s: cannot assign in this way\n", src, lineno, vname_id);
                                state->last_status = 1;
                                state->expansion_error = true;
                                val = "";
                                goto finish_var_eval;
                            }
                            const char *def_val = rest + 2;
                            if (!is_set || !env_v || *env_v == '\0') {
                                if (dyn_v) free(dyn_v);
                                char *exp_def = expand_rhs(state, def_val, in_double);
                                setenv(vname_id, exp_def, 1);
                                dynamic_val = exp_def;
                                val = exp_def;
                            } else {
                                val = env_v;
                                if (dyn_v) dynamic_val = dyn_v;
                            }
                        } else if (rest[1] == '+') {
                            /* ${VAR:+alternate} */
                            const char *alt_val = rest + 2;
                            if (dyn_v) free(dyn_v);
                            if (is_set && env_v && *env_v) {
                                dynamic_val = expand_rhs(state, alt_val, in_double);
                                val = dynamic_val;
                            } else {
                                val = "";
                            }
                        } else if (rest[1] == '?') {
                            /* ${VAR:?error_message} */
                            const char *err_msg = rest + 2;
                            if (is_set && env_v && *env_v != '\0') {
                                val = env_v;
                                if (dyn_v) dynamic_val = dyn_v;
                            } else {
                                if (dyn_v) free(dyn_v);
                                char *exp_msg = expand_rhs(state, err_msg, in_double);
                                const char *src = get_script_name(state);
                                int lineno = state->lineno > 0 ? state->lineno : 1;
                                if (exp_msg && *exp_msg) {
                                    fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, vname_id, exp_msg);
                                } else {
                                    fprintf(stderr, "%s: line %d: %s: parameter null or not set\n", src, lineno, vname_id);
                                }
                                if (exp_msg) free(exp_msg);
                                state->last_status = 1;
                                state->expansion_error = true;
                                val = "";
                                goto finish_var_eval;
                            }
                        } else {
                            /* Positional parameters / substring slicing */
                            if (strcmp(vname_id, "*") == 0 || strcmp(vname_id, "@") == 0) {
                                if (dyn_v) free(dyn_v);
                                char *sep2 = find_slice_separator(rest + 1);
                                int offset = 0, slen = -1;
                                if (sep2) {
                                    *sep2 = '\0';
                                    offset = (int)zbash_eval_arithmetic(state, rest + 1);
                                    slen = (int)zbash_eval_arithmetic(state, sep2 + 1);
                                    if (slen < 0) {
                                        const char *src = get_script_name(state);
                                        int lineno = state->lineno > 0 ? state->lineno : 1;
                                        fprintf(stderr, "%s: line %d: %s: substring expression < 0\n", src, lineno, sep2 + 1);
                                        state->last_status = 1;
                                        state->expansion_error = true;
                                        val = "";
                                        goto finish_var_eval;
                                    }
                                } else {
                                    offset = (int)zbash_eval_arithmetic(state, rest + 1);
                                }
                                int start_idx = offset;
                                if (start_idx < 0) start_idx = state->argc + start_idx;
                                if (start_idx < 0) start_idx = 0;

                                char sep_buf[8] = " ";
                                size_t sep_len = 1;
                                if (strcmp(vname_id, "*") == 0) {
                                    const char *ifs = getenv("IFS");
                                    get_ifs_first_char(ifs, sep_buf, &sep_len);
                                }
                                size_t total_sz = 0;
                                int collected = 0;
                                for (int a = start_idx; a < state->argc && (slen < 0 || collected < slen); a++) {
                                    total_sz += strlen(state->argv[a]) + sep_len;
                                    collected++;
                                }
                                dynamic_val = malloc(total_sz + 1);
                                if (dynamic_val) {
                                    dynamic_val[0] = '\0';
                                    collected = 0;
                                    for (int a = start_idx; a < state->argc && (slen < 0 || collected < slen); a++) {
                                        if (collected > 0 && sep_len > 0) {
                                            strcat(dynamic_val, sep_buf);
                                        }
                                        strcat(dynamic_val, state->argv[a]);
                                        collected++;
                                    }
                                    val = dynamic_val;
                                } else {
                                    val = "";
                                }
                            } else {
                                if (rest[1] == '\0') {
                                    const char *src = get_script_name(state);
                                    int lineno = state->lineno > 0 ? state->lineno : 1;
                                    fprintf(stderr, "%s: line %d: ${%s}: bad substitution\n", src, lineno, var_name);
                                    state->last_status = 1;
                                    state->expansion_error = true;
                                    val = "";
                                    goto finish_var_eval;
                                }
                                if (!env_v) env_v = "";
                                int vlen = (int)utf8_char_count(env_v);
                                char *sep2 = find_slice_separator(rest + 1);
                                int offset = 0, slen = -1;
                                if (sep2) {
                                    *sep2 = '\0';
                                    offset = (int)zbash_eval_arithmetic(state, rest + 1);
                                    slen = (int)zbash_eval_arithmetic(state, sep2 + 1);
                                    if (slen < 0) {
                                        const char *src = get_script_name(state);
                                        int lineno = state->lineno > 0 ? state->lineno : 1;
                                        fprintf(stderr, "%s: line %d: %s: substring expression < 0\n", src, lineno, sep2 + 1);
                                        state->last_status = 1;
                                        state->expansion_error = true;
                                        val = "";
                                        goto finish_var_eval;
                                    }
                                } else {
                                    offset = (int)zbash_eval_arithmetic(state, rest + 1);
                                }
                                if (state->expansion_error) {
                                    val = "";
                                    goto finish_var_eval;
                                }
                                if (offset < 0) offset = vlen + offset;
                                if (offset < 0) offset = 0;
                                if (offset > vlen) offset = vlen;
                                if (slen < 0 || offset + slen > vlen) slen = vlen - offset;
                                if (slen < 0) slen = 0;

                                const char *p_start = utf8_char_at(env_v, offset);
                                const char *p_end = utf8_char_at(p_start, slen);
                                size_t byte_len = (size_t)(p_end - p_start);
                                if (dyn_v) free(dyn_v);
                                dynamic_val = malloc(byte_len + 1);
                                if (dynamic_val) {
                                    memcpy(dynamic_val, p_start, byte_len);
                                    dynamic_val[byte_len] = '\0';
                                    val = dynamic_val;
                                } else {
                                    val = "";
                                }
                            }
                        }
                    } else if (op_char == '-') {
                        /* ${VAR-default} without colon */
                        char *dyn_v = NULL;
                        const char *env_v = NULL;
                        bool is_set = FETCH_PARAM(vname_id, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                        const char *def_val = rest + 1;
                        if (is_set && env_v != NULL) {
                            val = env_v;
                            if (dyn_v) dynamic_val = dyn_v;
                        } else {
                            if (dyn_v) free(dyn_v);
                            dynamic_val = expand_rhs(state, def_val, in_double);
                            val = dynamic_val;
                        }
                    } else if (op_char == '=') {
                        /* ${VAR=default} without colon */
                        if (isdigit((unsigned char)vname_id[0]) || strchr("*@#?!-$", vname_id[0])) {
                            const char *src = get_script_name(state);
                            int lineno = state->lineno > 0 ? state->lineno : 1;
                            fprintf(stderr, "%s: line %d: $%s: cannot assign in this way\n", src, lineno, vname_id);
                            state->last_status = 1;
                            state->expansion_error = true;
                            val = "";
                            goto finish_var_eval;
                        }
                        char *dyn_v = NULL;
                        const char *env_v = NULL;
                        bool is_set = FETCH_PARAM(vname_id, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                        const char *def_val = rest + 1;
                        if (!is_set) {
                            if (dyn_v) free(dyn_v);
                            char *exp_def = expand_rhs(state, def_val, in_double);
                            setenv(vname_id, exp_def, 1);
                            dynamic_val = exp_def;
                            val = exp_def;
                        } else {
                            val = env_v ? env_v : "";
                            if (dyn_v) dynamic_val = dyn_v;
                        }
                    } else if (op_char == '+') {
                        /* ${VAR+alternate} without colon */
                        char *dyn_v = NULL;
                        const char *env_v = NULL;
                        bool is_set = FETCH_PARAM(vname_id, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                        const char *alt_val = rest + 1;
                        if (dyn_v) free(dyn_v);
                        if (is_set && env_v != NULL) {
                            dynamic_val = expand_rhs(state, alt_val, in_double);
                            val = dynamic_val;
                        } else {
                            val = "";
                        }
                    } else if (op_char == '?') {
                        /* ${VAR?error_message} without colon */
                        char *dyn_v = NULL;
                        const char *env_v = NULL;
                        bool is_set = FETCH_PARAM(vname_id, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                        const char *err_msg = rest + 1;
                        if (is_set && env_v != NULL) {
                            val = env_v;
                            if (dyn_v) dynamic_val = dyn_v;
                        } else {
                            if (dyn_v) free(dyn_v);
                            char *exp_msg = expand_rhs(state, err_msg, in_double);
                            const char *src = get_script_name(state);
                            int lineno = state->lineno > 0 ? state->lineno : 1;
                            if (exp_msg && *exp_msg) {
                                fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, vname_id, exp_msg);
                            } else {
                                fprintf(stderr, "%s: line %d: %s: parameter null or not set\n", src, lineno, vname_id);
                            }
                            if (exp_msg) free(exp_msg);
                            state->last_status = 1;
                            state->expansion_error = true;
                            val = "";
                            goto finish_var_eval;
                        }
                    } else if (op_char == '/') {
                        /* Pattern replacement: ${VAR/pat/rep} or ${VAR//pat/rep} */
                        bool replace_all = (rest[1] == '/');
                        char *pat_start = replace_all ? (rest + 2) : (rest + 1);
                        char *slash2 = NULL;
                        bool s_quote = false, d_quote = false;
                        for (char *p = pat_start; *p; p++) {
                            if (*p == '\\' && *(p + 1)) { p++; continue; }
                            if (*p == '\'' && !d_quote) { s_quote = !s_quote; continue; }
                            if (*p == '"' && !s_quote) { d_quote = !d_quote; continue; }
                            if (*p == '/' && !s_quote && !d_quote) { slash2 = p; break; }
                        }
                        const char *rep = "";
                        if (slash2) {
                            *slash2 = '\0';
                            rep = slash2 + 1;
                        }
                        char *exp_p = pat_start ? zbash_expand_pattern(state, pat_start) : NULL;
                        if (state->expansion_error) {
                            if (exp_p) free(exp_p);
                            val = "";
                            goto finish_var_eval;
                        }
                        const char *actual_pat = exp_p ? exp_p : pat_start;
                        char *exp_r = (strchr(rep, '$') || strchr(rep, '\'') || strchr(rep, '"')) ? zbash_expand_vars(state, rep) : NULL;
                        const char *actual_rep = exp_r ? exp_r : rep;

                        if (strcmp(vname_id, "@") == 0 || strcmp(vname_id, "*") == 0) {
                            if (state->argc > 1) {
                                size_t cap = 1024;
                                dynamic_val = malloc(cap);
                                if (dynamic_val) {
                                    dynamic_val[0] = '\0';
                                    const char *ifs = getenv("IFS");
                                    char sep = (ifs ? ifs[0] : ' ');
                                    if (strcmp(vname_id, "@") == 0) sep = ' ';
                                    for (int a = 1; a < state->argc; a++) {
                                        char *trans = apply_pattern_subst(state->argv[a], actual_pat, actual_rep, replace_all);
                                        const char *s = trans ? trans : state->argv[a];
                                        size_t slen = strlen(s);
                                        while (strlen(dynamic_val) + slen + 2 >= cap) {
                                            cap *= 2;
                                            dynamic_val = realloc(dynamic_val, cap);
                                        }
                                        if (a > 1 && sep != '\0') {
                                            size_t cur = strlen(dynamic_val);
                                            dynamic_val[cur] = sep;
                                            dynamic_val[cur + 1] = '\0';
                                        }
                                        strcat(dynamic_val, s);
                                        if (trans) free(trans);
                                    }
                                    val = dynamic_val;
                                } else {
                                    val = "";
                                }
                            } else {
                                val = "";
                            }
                        } else {
                            char *dyn_v = NULL;
                            const char *env_v = NULL;
                            FETCH_PARAM(vname_id, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                            if (!env_v) env_v = "";
                            dynamic_val = apply_pattern_subst(env_v, actual_pat, actual_rep, replace_all);
                            val = dynamic_val ? dynamic_val : env_v;
                            if (dyn_v) free(dyn_v);
                        }
                        if (exp_p) free(exp_p);
                        if (exp_r) free(exp_r);
                    } else if (op_char == '#') {
                        /* Prefix removal: ${VAR#pat} or ${VAR##pat} */
                        bool longest = (*(rest + 1) == '#');
                        char *pat = rest + (longest ? 2 : 1);
                        char *exp_p = pat ? zbash_expand_pattern(state, pat) : NULL;
                        const char *actual_pat = exp_p ? exp_p : pat;

                        if (strcmp(vname_id, "@") == 0 || strcmp(vname_id, "*") == 0) {
                            if (state->argc > 1) {
                                size_t cap = 1024;
                                dynamic_val = malloc(cap);
                                if (dynamic_val) {
                                    dynamic_val[0] = '\0';
                                    const char *ifs = getenv("IFS");
                                    char sep = (ifs ? ifs[0] : ' ');
                                    if (strcmp(vname_id, "@") == 0) sep = ' ';
                                    for (int a = 1; a < state->argc; a++) {
                                        char *trans = apply_pattern_removal(state->argv[a], actual_pat, true, longest);
                                        const char *s = trans ? trans : state->argv[a];
                                        size_t slen = strlen(s);
                                        while (strlen(dynamic_val) + slen + 2 >= cap) {
                                            cap *= 2;
                                            dynamic_val = realloc(dynamic_val, cap);
                                        }
                                        if (a > 1 && sep != '\0') {
                                            size_t cur = strlen(dynamic_val);
                                            dynamic_val[cur] = sep;
                                            dynamic_val[cur + 1] = '\0';
                                        }
                                        strcat(dynamic_val, s);
                                        if (trans) free(trans);
                                    }
                                    val = dynamic_val;
                                } else {
                                    val = "";
                                }
                            } else {
                                val = "";
                            }
                        } else {
                            char *dyn_v = NULL;
                            const char *env_v = NULL;
                            FETCH_PARAM(vname_id, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                            if (!env_v) env_v = "";
                            dynamic_val = apply_pattern_removal(env_v, actual_pat, true, longest);
                            val = dynamic_val ? dynamic_val : env_v;
                            if (dyn_v) free(dyn_v);
                        }
                        if (exp_p) free(exp_p);
                    } else if (op_char == '%') {
                        /* Suffix removal: ${VAR%pat} or ${VAR%%pat} */
                        bool longest = (*(rest + 1) == '%');
                        char *pat = rest + (longest ? 2 : 1);
                        char *exp_p = pat ? zbash_expand_pattern(state, pat) : NULL;
                        const char *actual_pat = exp_p ? exp_p : pat;

                        if (strcmp(vname_id, "@") == 0 || strcmp(vname_id, "*") == 0) {
                            if (state->argc > 1) {
                                size_t cap = 1024;
                                dynamic_val = malloc(cap);
                                if (dynamic_val) {
                                    dynamic_val[0] = '\0';
                                    const char *ifs = getenv("IFS");
                                    char sep = (ifs ? ifs[0] : ' ');
                                    if (strcmp(vname_id, "@") == 0) sep = ' ';
                                    for (int a = 1; a < state->argc; a++) {
                                        char *trans = apply_pattern_removal(state->argv[a], actual_pat, false, longest);
                                        const char *s = trans ? trans : state->argv[a];
                                        size_t slen = strlen(s);
                                        while (strlen(dynamic_val) + slen + 2 >= cap) {
                                            cap *= 2;
                                            dynamic_val = realloc(dynamic_val, cap);
                                        }
                                        if (a > 1 && sep != '\0') {
                                            size_t cur = strlen(dynamic_val);
                                            dynamic_val[cur] = sep;
                                            dynamic_val[cur + 1] = '\0';
                                        }
                                        strcat(dynamic_val, s);
                                        if (trans) free(trans);
                                    }
                                    val = dynamic_val;
                                } else {
                                    val = "";
                                }
                            } else {
                                val = "";
                            }
                        } else {
                            char *dyn_v = NULL;
                            const char *env_v = NULL;
                            FETCH_PARAM(vname_id, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                            if (!env_v) env_v = "";
                            dynamic_val = apply_pattern_removal(env_v, actual_pat, false, longest);
                            val = dynamic_val ? dynamic_val : env_v;
                            if (dyn_v) free(dyn_v);
                        }
                        if (exp_p) free(exp_p);
                    } else {
                        char *dyn_v = NULL;
                        const char *env_v = NULL;
                        get_param_value(state, var_name, num_buf, sizeof(num_buf), &env_v, &dyn_v);
                        if (dyn_v) {
                            dynamic_val = dyn_v;
                            val = dyn_v;
                        } else {
                            val = env_v ? env_v : "";
                        }
                    }
                }
            }
            /* Positional parameter $0..$9 */
            else if (isdigit((unsigned char)input[i])) {
                int arg_idx = input[i] - '0';
                if (state->nounset && arg_idx >= state->argc) {
                    const char *src = get_script_name(state);
                    int lineno = state->lineno > 0 ? state->lineno : 1;
                    fprintf(stderr, "%s: line %d: $%c: unbound variable\n", src, lineno, input[i]);
                    state->last_status = 1;
                    state->expansion_error = true;
                    state->should_exit = true;
                    state->exit_code = 1;
                    val = "";
                } else {
                    val = (arg_idx < state->argc) ? state->argv[arg_idx] : "";
                }
            }
            /* Special params: $?, $$, $!, $#, $@, $*, $- */
            else if (input[i] == '-') {
                get_shell_flags(state, num_buf, sizeof(num_buf));
                val = num_buf;
            } else if (input[i] == '?') {
                snprintf(num_buf, sizeof(num_buf), "%d", state->last_status);
                val = num_buf;
            } else if (input[i] == '$') {
                snprintf(num_buf, sizeof(num_buf), "%d", state->shell_pid);
                val = num_buf;
            } else if (input[i] == '!') {
                if (state->last_bg_pid > 0) {
                    snprintf(num_buf, sizeof(num_buf), "%d", state->last_bg_pid);
                    val = num_buf;
                } else {
                    val = "";
                }
            } else if (input[i] == '#') {
                snprintf(num_buf, sizeof(num_buf), "%d", (state->argc > 1 ? state->argc - 1 : 0));
                val = num_buf;
            } else if (input[i] == '@' || input[i] == '*') {
                if (state->argc > 1) {
                    char sep_buf[8] = " ";
                    size_t sep_len = 1;
                    if (input[i] == '*') {
                        const char *ifs = getenv("IFS");
                        get_ifs_first_char(ifs, sep_buf, &sep_len);
                    }
                    size_t total_sz = 0;
                    for (int a = 1; a < state->argc; a++) total_sz += strlen(state->argv[a]) + sep_len;
                    dynamic_val = malloc(total_sz + 1);
                    if (dynamic_val) {
                        dynamic_val[0] = '\0';
                        for (int a = 1; a < state->argc; a++) {
                            strcat(dynamic_val, state->argv[a]);
                            if (a < state->argc - 1 && sep_len > 0) {
                                strcat(dynamic_val, sep_buf);
                            }
                        }
                        val = dynamic_val;
                    }
                } else {
                    val = "";
                }
            }
            /* Variable name identifier $VAR or $BASH_SOURCE */
            else if (isalpha((unsigned char)input[i]) || input[i] == '_') {
                while (isalnum((unsigned char)input[i]) || input[i] == '_') {
                    if (vn_len < sizeof(var_name) - 1) {
                        var_name[vn_len++] = input[i];
                    }
                    i++;
                }
                i--; /* Rewind one step so loop increments correctly */
                var_name[vn_len] = '\0';
                if (strcmp(var_name, "LINENO") == 0) {
                    snprintf(num_buf, sizeof(num_buf), "%d", state->lineno > 0 ? state->lineno : 0);
                    val = num_buf;
                } else {
                    zbash_array_t *arr = zbash_find_array(state, var_name);
                    if (arr) {
                        val = (arr->count > 0 && arr->elements[0]) ? arr->elements[0] : "";
                    } else {
                        val = getenv(var_name);
                    }
                    if (state->nounset && !arr && !val) {
                        const char *src = get_script_name(state);
                        int lineno = state->lineno > 0 ? state->lineno : 1;
                        fprintf(stderr, "%s: line %d: %s: unbound variable\n", src, lineno, var_name);
                        state->last_status = 1;
                        state->expansion_error = true;
                        state->should_exit = true;
                        state->exit_code = 1;
                        val = "";
                    }
                }
            } else {
                /* Not a valid variable, print literal $ */
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = '$';
                i--;
                continue;
            }

finish_var_eval:
            if (val) {
                size_t vlen = strlen(val);
                while (len + vlen >= cap) {
                    cap = (cap + vlen) * 2;
                    buf = realloc(buf, cap);
                }
                memcpy(buf + len, val, vlen);
                len += vlen;
            }
            if (dynamic_val) {
                free(dynamic_val);
            }
            continue;
        }

        if (len + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        buf[len++] = c;
    }

    buf[len] = '\0';
    return buf;
}

static bool is_glob_char(char c) {
    return c == '*' || c == '?' || c == '[' || c == ']' || c == '\\' ||
           c == '(' || c == ')' || c == '|' || c == '+' || c == '@' || c == '!';
}

char *zbash_expand_pattern(zbash_state_t *state, const char *pat) {
    if (!pat) return NULL;
    if (!strchr(pat, '\'') && !strchr(pat, '"') && !strchr(pat, '$')) {
        return strdup(pat);
    }

    size_t cap = strlen(pat) * 2 + 32;
    char *buf = malloc(cap);
    if (!buf) return strdup(pat);
    size_t len = 0;

    size_t i = 0;
    size_t plen = strlen(pat);
    while (i < plen) {
        char c = pat[i];
        if (c == '$' && i + 1 < plen && pat[i + 1] == '\'') {
            /* ANSI-C quoting: $'...', evaluate escape sequence */
            i += 2;
            while (i < plen && pat[i] != '\'') {
                if (pat[i] == '\\' && i + 1 < plen) {
                    i++;
                    char esc = pat[i];
                    char out_c = esc;
                    if (esc == 'a') out_c = '\a';
                    else if (esc == 'b') out_c = '\b';
                    else if (esc == 'e' || esc == 'E') out_c = '\033';
                    else if (esc == 'f') out_c = '\f';
                    else if (esc == 'n') out_c = '\n';
                    else if (esc == 'r') out_c = '\r';
                    else if (esc == 't') out_c = '\t';
                    else if (esc == 'v') out_c = '\v';
                    else if (esc == '\\') out_c = '\\';
                    else if (esc == '\'') out_c = '\'';
                    else if (esc == '"') out_c = '"';
                    else if (esc >= '0' && esc <= '7') {
                        int oct = esc - '0';
                        if (i + 1 < plen && pat[i + 1] >= '0' && pat[i + 1] <= '7') {
                            oct = oct * 8 + (pat[++i] - '0');
                            if (i + 1 < plen && pat[i + 1] >= '0' && pat[i + 1] <= '7') {
                                oct = oct * 8 + (pat[++i] - '0');
                            }
                        }
                        out_c = (char)oct;
                    } else if (esc == 'x' && i + 1 < plen && isxdigit((unsigned char)pat[i + 1])) {
                        int hval = 0;
                        while (i + 1 < plen && isxdigit((unsigned char)pat[i + 1])) {
                            i++;
                            char ch = pat[i];
                            int digit = isdigit((unsigned char)ch) ? (ch - '0') : (tolower((unsigned char)ch) - 'a' + 10);
                            hval = (hval * 16 + digit) & 0xFF;
                        }
                        out_c = (char)hval;
                    }
                    if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    if (is_glob_char(out_c)) buf[len++] = '\\';
                    buf[len++] = out_c;
                    i++;
                } else {
                    if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    if (is_glob_char(pat[i])) buf[len++] = '\\';
                    buf[len++] = pat[i++];
                }
            }
            if (i < plen && pat[i] == '\'') i++;
            continue;
        }

        if (c == '\'') {
            /* Single quotes: all characters inside are literal */
            i++;
            while (i < plen && pat[i] != '\'') {
                if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                if (is_glob_char(pat[i])) buf[len++] = '\\';
                buf[len++] = pat[i++];
            }
            if (i < plen && pat[i] == '\'') i++;
            continue;
        }

        if (c == '"') {
            /* Double quotes: characters inside are literal, except variables */
            i++;
            while (i < plen && pat[i] != '"') {
                if (pat[i] == '$' && i + 1 < plen && (isalpha((unsigned char)pat[i + 1]) || pat[i + 1] == '_' || pat[i + 1] == '{')) {
                    size_t vstart = i;
                    if (pat[i + 1] == '{') {
                        i += 2;
                        while (i < plen && pat[i] != '}') i++;
                        if (i < plen) i++;
                    } else {
                        i++;
                        while (i < plen && (isalnum((unsigned char)pat[i]) || pat[i] == '_')) i++;
                    }
                    char vsub[256];
                    size_t vlen = i - vstart;
                    if (vlen < sizeof(vsub)) {
                        strncpy(vsub, pat + vstart, vlen);
                        vsub[vlen] = '\0';
                        char *vexp = zbash_expand_vars(state, vsub);
                        if (vexp) {
                            for (size_t k = 0; vexp[k]; k++) {
                                if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                                if (is_glob_char(vexp[k])) buf[len++] = '\\';
                                buf[len++] = vexp[k];
                            }
                            free(vexp);
                        }
                    }
                } else if (pat[i] == '\\' && i + 1 < plen) {
                    i++;
                    if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    if (is_glob_char(pat[i])) buf[len++] = '\\';
                    buf[len++] = pat[i++];
                } else {
                    if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                    if (is_glob_char(pat[i])) buf[len++] = '\\';
                    buf[len++] = pat[i++];
                }
            }
            if (i < plen && pat[i] == '"') i++;
            continue;
        }

        if (c == '$' && i + 1 < plen && (isalpha((unsigned char)pat[i + 1]) || pat[i + 1] == '_' || pat[i + 1] == '{')) {
            size_t vstart = i;
            if (pat[i + 1] == '{') {
                i += 2;
                while (i < plen && pat[i] != '}') i++;
                if (i < plen) i++;
            } else {
                i++;
                while (i < plen && (isalnum((unsigned char)pat[i]) || pat[i] == '_')) i++;
            }
            char vsub[256];
            size_t vlen = i - vstart;
            if (vlen < sizeof(vsub)) {
                strncpy(vsub, pat + vstart, vlen);
                vsub[vlen] = '\0';
                char *vexp = zbash_expand_vars(state, vsub);
                if (vexp) {
                    size_t v_len = strlen(vexp);
                    while (len + v_len >= cap) { cap = (cap + v_len) * 2; buf = realloc(buf, cap); }
                    memcpy(buf + len, vexp, v_len);
                    len += v_len;
                    free(vexp);
                }
            }
            continue;
        }

        if (c == '\\' && i + 1 < plen) {
            if (len + 2 >= cap) { cap *= 2; buf = realloc(buf, cap); }
            buf[len++] = pat[i++];
            buf[len++] = pat[i++];
            continue;
        }

        if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        buf[len++] = pat[i++];
    }

    buf[len] = '\0';
    return buf;
}

char *zbash_expand_heredoc(zbash_state_t *state, const char *input) {
    if (!input) return NULL;
    size_t cap = strlen(input) * 2 + 128;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) return NULL;

    for (size_t i = 0; input[i] != '\0'; i++) {
        char c = input[i];

        if (c == '\\') {
            char next = input[i + 1];
            if (next == '$' || next == '`' || next == '\\') {
                i++;
                c = next;
            } else if (next == '\n') {
                i++;
                continue;
            } else {
                if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
                buf[len++] = '\\';
                continue;
            }
            if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
            buf[len++] = c;
            continue;
        }

        if (c == '$' || c == '`') {
            size_t vstart = i;
            size_t vend = i;
            if (c == '`') {
                vend = i + 1;
                while (input[vend] && input[vend] != '`') {
                    if (input[vend] == '\\' && input[vend + 1]) vend += 2;
                    else vend++;
                }
                if (input[vend] == '`') vend++;
            } else if (input[i + 1] == '(') {
                int depth = 1;
                vend = i + 2;
                while (input[vend] && depth > 0) {
                    if (input[vend] == '\\' && input[vend + 1]) { vend += 2; continue; }
                    if (input[vend] == '(') depth++;
                    else if (input[vend] == ')') { depth--; if (depth == 0) { vend++; break; } }
                    vend++;
                }
            } else if (input[i + 1] == '{') {
                int depth = 1;
                vend = i + 2;
                while (input[vend] && depth > 0) {
                    if (input[vend] == '\\' && input[vend + 1]) { vend += 2; continue; }
                    if (input[vend] == '{') depth++;
                    else if (input[vend] == '}') { depth--; if (depth == 0) { vend++; break; } }
                    vend++;
                }
            } else if (isalpha((unsigned char)input[i + 1]) || input[i + 1] == '_') {
                vend = i + 2;
                while (isalnum((unsigned char)input[vend]) || input[vend] == '_') vend++;
            } else if (isdigit((unsigned char)input[i + 1]) || input[i + 1] == '?' || input[i + 1] == '$' || input[i + 1] == '!' || input[i + 1] == '#' || input[i + 1] == '@' || input[i + 1] == '*' || input[i + 1] == '-') {
                vend = i + 2;
            }

            if (vend > vstart) {
                size_t vlen = vend - vstart;
                char *vsub = malloc(vlen + 1);
                if (vsub) {
                    memcpy(vsub, input + vstart, vlen);
                    vsub[vlen] = '\0';
                    char *vexp = zbash_expand_vars(state, vsub);
                    free(vsub);
                    if (vexp) {
                        size_t elen = strlen(vexp);
                        while (len + elen >= cap) { cap = (cap + elen) * 2; buf = realloc(buf, cap); }
                        memcpy(buf + len, vexp, elen);
                        len += elen;
                        free(vexp);
                    }
                }
                i = vend - 1;
                continue;
            }
        }

        if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        buf[len++] = c;
    }

    buf[len] = '\0';
    return buf;
}
