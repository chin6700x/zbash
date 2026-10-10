/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"
#include <ctype.h>
#include <errno.h>

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} strbuf_t;

static void sb_init(strbuf_t *sb) {
    sb->cap = 1024;
    sb->buf = malloc(sb->cap);
    sb->buf[0] = '\0';
    sb->len = 0;
}

static void sb_free(strbuf_t *sb) {
    if (sb->buf) {
        free(sb->buf);
        sb->buf = NULL;
    }
    sb->len = 0;
    sb->cap = 0;
}

static void sb_putc(strbuf_t *sb, char c) {
    if (sb->len + 2 >= sb->cap) {
        sb->cap *= 2;
        sb->buf = realloc(sb->buf, sb->cap);
    }
    sb->buf[sb->len++] = c;
    sb->buf[sb->len] = '\0';
}

static void sb_putmem(strbuf_t *sb, const char *p, size_t n) {
    if (sb->len + n + 2 >= sb->cap) {
        while (sb->len + n + 2 >= sb->cap) sb->cap *= 2;
        sb->buf = realloc(sb->buf, sb->cap);
    }
    memcpy(sb->buf + sb->len, p, n);
    sb->len += n;
    sb->buf[sb->len] = '\0';
}

static void sb_puts(strbuf_t *sb, const char *s) {
    if (!s) return;
    sb_putmem(sb, s, strlen(s));
}

static const char *get_script_name(zbash_state_t *state) {
    if (state && state->source_depth > 0) return state->source_stack[state->source_depth - 1];
    if (state && state->argv && state->argv[0]) return state->argv[0];
    return "zbash";
}

static int get_lineno(zbash_state_t *state) {
    return (state && state->lineno > 0) ? state->lineno : 1;
}

/* Formats a quoted string suitable for shell input (%q) */
static char *quote_shell_string(const char *s) {
    if (!s || *s == '\0') return strdup("''");
    strbuf_t qb;
    sb_init(&qb);
    for (size_t i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        if (strchr(" \t\n'\"`$\\|&;()<>{}^#*?~![]", c) != NULL) {
            sb_putc(&qb, '\\');
        }
        sb_putc(&qb, c);
    }
    return qb.buf;
}

/* Expands backslash escapes in %b argument. Returns false if \c encountered (halt output). */
static bool expand_b_arg(strbuf_t *sb, const char *s, zbash_state_t *state) {
    if (!s) return true;
    for (size_t i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        if (c == '\\') {
            i++;
            if (s[i] == '\0') {
                sb_putc(sb, '\\');
                break;
            }
            char next = s[i];
            if (next == 'c') {
                return false; /* \c terminates immediately */
            } else if (next == 'a') sb_putc(sb, '\a');
            else if (next == 'b') sb_putc(sb, '\b');
            else if (next == 'e' || next == 'E') sb_putc(sb, 0x1B);
            else if (next == 'f') sb_putc(sb, '\f');
            else if (next == 'n') sb_putc(sb, '\n');
            else if (next == 'r') sb_putc(sb, '\r');
            else if (next == 't') sb_putc(sb, '\t');
            else if (next == 'v') sb_putc(sb, '\v');
            else if (next == '\\') sb_putc(sb, '\\');
            else if (next == '0') {
                int val = 0;
                int digits = 0;
                while (digits < 3 && s[i + 1] >= '0' && s[i + 1] <= '7') {
                    val = (val << 3) + (s[++i] - '0');
                    digits++;
                }
                sb_putc(sb, (char)(unsigned char)val);
            } else if (next >= '1' && next <= '7') {
                int val = next - '0';
                int digits = 1;
                while (digits < 3 && s[i + 1] >= '0' && s[i + 1] <= '7') {
                    val = (val << 3) + (s[++i] - '0');
                    digits++;
                }
                sb_putc(sb, (char)(unsigned char)val);
            } else if (next == 'x') {
                int val = 0;
                int digits = 0;
                while (digits < 2 && isxdigit((unsigned char)s[i + 1])) {
                    char h = s[++i];
                    int hex_val = isdigit((unsigned char)h) ? (h - '0') : (tolower((unsigned char)h) - 'a' + 10);
                    val = (val << 4) + hex_val;
                    digits++;
                }
                if (digits == 0) {
                    fprintf(stderr, "%s: line %d: printf: missing hex digit for \\x\n",
                            get_script_name(state), get_lineno(state));
                    sb_puts(sb, "\\x");
                } else {
                    sb_putc(sb, (char)(unsigned char)val);
                }
            } else {
                sb_putc(sb, '\\');
                sb_putc(sb, next);
            }
        } else {
            sb_putc(sb, c);
        }
    }
    return true;
}

int builtin_printf(zbash_state_t *state, int argc, char **argv) {
    const char *vname = NULL;
    int opt_idx = 1;

    while (opt_idx < argc) {
        if (strcmp(argv[opt_idx], "--") == 0) {
            opt_idx++;
            break;
        }
        if (strcmp(argv[opt_idx], "-v") == 0) {
            if (opt_idx + 1 >= argc) {
                fprintf(stderr, "printf: usage: printf [-v var] format [arguments]\n");
                return 2;
            }
            vname = argv[opt_idx + 1];
            opt_idx += 2;
            continue;
        }
        break;
    }

    if (opt_idx >= argc) {
        fprintf(stderr, "printf: usage: printf [-v var] format [arguments]\n");
        return 2;
    }

    const char *fmt_orig = argv[opt_idx++];
    int arg_idx = opt_idx;
    bool had_error = false;

    strbuf_t sb;
    sb_init(&sb);

    bool keep_running = true;

    do {
        const char *p = fmt_orig;
        int args_consumed_in_pass = 0;

        while (*p && keep_running) {
            char c = *p;

            if (c == '\\') {
                p++;
                if (*p == '\0') {
                    sb_putc(&sb, '\\');
                    break;
                }
                char next = *p++;
                if (next == 'a') sb_putc(&sb, '\a');
                else if (next == 'b') sb_putc(&sb, '\b');
                else if (next == 'e' || next == 'E') sb_putc(&sb, 0x1B);
                else if (next == 'f') sb_putc(&sb, '\f');
                else if (next == 'n') sb_putc(&sb, '\n');
                else if (next == 'r') sb_putc(&sb, '\r');
                else if (next == 't') sb_putc(&sb, '\t');
                else if (next == 'v') sb_putc(&sb, '\v');
                else if (next == '\\') sb_putc(&sb, '\\');
                else if (next == '\'') sb_putc(&sb, '\'');
                else if (next == '"') sb_putc(&sb, '"');
                else if (next == '?') sb_putc(&sb, '?');
                else if (next >= '0' && next <= '7') {
                    int val = next - '0';
                    int digits = 1;
                    while (digits < 3 && *p >= '0' && *p <= '7') {
                        val = (val << 3) + (*p++ - '0');
                        digits++;
                    }
                    sb_putc(&sb, (char)(unsigned char)val);
                } else if (next == 'x') {
                    int val = 0;
                    int digits = 0;
                    while (digits < 2 && isxdigit((unsigned char)*p)) {
                        char h = *p++;
                        int hex_val = isdigit((unsigned char)h) ? (h - '0') : (tolower((unsigned char)h) - 'a' + 10);
                        val = (val << 4) + hex_val;
                        digits++;
                    }
                    if (digits == 0) {
                        fprintf(stderr, "%s: line %d: printf: missing hex digit for \\x\n",
                                get_script_name(state), get_lineno(state));
                        sb_puts(&sb, "\\x");
                    } else {
                        sb_putc(&sb, (char)(unsigned char)val);
                    }
                } else {
                    sb_putc(&sb, '\\');
                    sb_putc(&sb, next);
                }
                continue;
            }

            if (c != '%') {
                sb_putc(&sb, *p++);
                continue;
            }

            /* '%' encountered */
            p++;
            if (*p == '%') {
                sb_putc(&sb, '%');
                p++;
                continue;
            }

            /* Parse flags */
            const char *conv_start = p - 1;
            char flags[16] = {0};
            size_t flen = 0;
            while (*p && strchr("-+ #0'", *p) != NULL) {
                if (flen + 1 < sizeof(flags) && strchr(flags, *p) == NULL) {
                    flags[flen++] = *p;
                }
                p++;
            }

            /* Parse width */
            int width = -1;
            bool has_width = false;
            if (*p == '*') {
                p++;
                has_width = true;
                args_consumed_in_pass++;
                width = (arg_idx < argc) ? atoi(argv[arg_idx++]) : 0;
                if (width < 0) {
                    if (strchr(flags, '-') == NULL && flen + 1 < sizeof(flags)) {
                        flags[flen++] = '-';
                    }
                    width = -width;
                }
            } else if (isdigit((unsigned char)*p)) {
                has_width = true;
                width = 0;
                while (isdigit((unsigned char)*p)) {
                    width = width * 10 + (*p++ - '0');
                }
            }

            /* Parse precision */
            int prec = -1;
            bool has_prec = false;
            if (*p == '.') {
                p++;
                has_prec = true;
                if (*p == '*') {
                    p++;
                    args_consumed_in_pass++;
                    prec = (arg_idx < argc) ? atoi(argv[arg_idx++]) : 0;
                    if (prec < 0) {
                        has_prec = false;
                        prec = -1;
                    }
                } else if (isdigit((unsigned char)*p)) {
                    prec = 0;
                    while (isdigit((unsigned char)*p)) {
                        prec = prec * 10 + (*p++ - '0');
                    }
                } else {
                    prec = 0;
                }
            }

            /* Optional length modifiers: l, L, h, etc. */
            bool is_long = false;
            if (*p == 'l' || *p == 'L') {
                is_long = true;
                p++;
            } else if (*p == 'h') {
                p++;
            }

            char conv = *p;
            if (conv == '\0') {
                char missing_buf[64];
                size_t mlen = p - conv_start;
                if (mlen >= sizeof(missing_buf)) mlen = sizeof(missing_buf) - 1;
                strncpy(missing_buf, conv_start, mlen);
                missing_buf[mlen] = '\0';
                fprintf(stderr, "%s: line %d: printf: `%s': missing format character\n",
                        get_script_name(state), get_lineno(state), missing_buf);
                had_error = true;
                keep_running = false;
                break;
            }

            p++; /* Advance past conversion specifier */

            /* Build printf specifier string */
            char c_spec[64];
            size_t c_idx = 0;
            c_spec[c_idx++] = '%';
            for (size_t f = 0; f < flen; f++) c_spec[c_idx++] = flags[f];
            if (has_width) {
                c_idx += snprintf(c_spec + c_idx, sizeof(c_spec) - c_idx, "%d", width);
            }
            if (has_prec) {
                c_idx += snprintf(c_spec + c_idx, sizeof(c_spec) - c_idx, ".%d", prec);
            }
            if (is_long && strchr("diuoxX", conv) != NULL) {
                c_spec[c_idx++] = 'l';
            }
            c_spec[c_idx++] = conv;
            c_spec[c_idx] = '\0';

            size_t alloc_sz = (has_width && width > 4096 ? width : 4096) +
                              (has_prec && prec > 4096 ? prec : 4096) + 256;

            if (conv == 's' || conv == 'q') {
                args_consumed_in_pass++;
                const char *raw_arg = (arg_idx < argc) ? argv[arg_idx++] : "";
                char *str_to_fmt = (conv == 's') ? strdup(raw_arg) : quote_shell_string(raw_arg);

                c_spec[c_idx - 1] = 's';
                size_t slen = strlen(str_to_fmt);
                size_t need = alloc_sz + slen;
                char *tmp = malloc(need);
                int n = snprintf(tmp, need, c_spec, str_to_fmt);
                if (n > 0) sb_putmem(&sb, tmp, (size_t)n);
                free(tmp);
                free(str_to_fmt);
            } else if (conv == 'b') {
                args_consumed_in_pass++;
                const char *raw_arg = (arg_idx < argc) ? argv[arg_idx++] : "";
                strbuf_t bsb;
                sb_init(&bsb);
                bool cont = expand_b_arg(&bsb, raw_arg, state);
                size_t blen = bsb.len;
                if (has_prec && (size_t)prec < blen) {
                    blen = (size_t)prec;
                }
                bool left = (strchr(flags, '-') != NULL);
                if (!left && has_width && (size_t)width > blen) {
                    for (size_t sp = 0; sp < (size_t)width - blen; sp++) sb_putc(&sb, ' ');
                }
                sb_putmem(&sb, bsb.buf, blen);
                if (left && has_width && (size_t)width > blen) {
                    for (size_t sp = 0; sp < (size_t)width - blen; sp++) sb_putc(&sb, ' ');
                }
                sb_free(&bsb);
                if (!cont) {
                    keep_running = false;
                    break;
                }
            } else if (conv == 'c') {
                args_consumed_in_pass++;
                const char *raw_arg = (arg_idx < argc) ? argv[arg_idx++] : "";
                if (raw_arg && raw_arg[0] != '\0') {
                    char ch = raw_arg[0];
                    char *tmp = malloc(alloc_sz);
                    int n = snprintf(tmp, alloc_sz, c_spec, ch);
                    if (n > 0) sb_putmem(&sb, tmp, (size_t)n);
                    free(tmp);
                }
            } else if (conv == 'd' || conv == 'i') {
                args_consumed_in_pass++;
                const char *val_str = (arg_idx < argc) ? argv[arg_idx++] : NULL;
                long long num = 0;
                if (val_str && *val_str) {
                    if (val_str[0] == '\'' || val_str[0] == '"') {
                        num = (long long)(unsigned char)val_str[1];
                    } else {
                        char *endp = NULL;
                        errno = 0;
                        num = strtoll(val_str, &endp, 0);
                        if (*endp != '\0') {
                            fprintf(stderr, "%s: line %d: printf: %s: invalid number\n",
                                    get_script_name(state), get_lineno(state), val_str);
                            had_error = true;
                        }
                    }
                }
                char *tmp = malloc(alloc_sz);
                /* Use %lld */
                char lspec[64];
                snprintf(lspec, sizeof(lspec), "%.*slld", (int)(c_idx - 1), c_spec);
                int n = snprintf(tmp, alloc_sz, lspec, num);
                if (n > 0) sb_putmem(&sb, tmp, (size_t)n);
                free(tmp);
            } else if (conv == 'u' || conv == 'o' || conv == 'x' || conv == 'X') {
                args_consumed_in_pass++;
                const char *val_str = (arg_idx < argc) ? argv[arg_idx++] : NULL;
                unsigned long long num = 0;
                if (val_str && *val_str) {
                    if (val_str[0] == '\'' || val_str[0] == '"') {
                        num = (unsigned long long)(unsigned char)val_str[1];
                    } else {
                        char *endp = NULL;
                        errno = 0;
                        num = strtoull(val_str, &endp, 0);
                        if (*endp != '\0') {
                            fprintf(stderr, "%s: line %d: printf: %s: invalid number\n",
                                    get_script_name(state), get_lineno(state), val_str);
                            had_error = true;
                        }
                    }
                }
                char *tmp = malloc(alloc_sz);
                char lspec[64];
                snprintf(lspec, sizeof(lspec), "%.*sll%c", (int)(c_idx - 1), c_spec, conv);
                int n = snprintf(tmp, alloc_sz, lspec, num);
                if (n > 0) sb_putmem(&sb, tmp, (size_t)n);
                free(tmp);
            } else if (conv == 'f' || conv == 'e' || conv == 'E' || conv == 'g' || conv == 'G') {
                args_consumed_in_pass++;
                const char *val_str = (arg_idx < argc) ? argv[arg_idx++] : NULL;
                double dnum = 0.0;
                if (val_str && *val_str) {
                    if (val_str[0] == '\'' || val_str[0] == '"') {
                        dnum = (double)(unsigned char)val_str[1];
                    } else {
                        char *endp = NULL;
                        errno = 0;
                        dnum = strtod(val_str, &endp);
                        if (*endp != '\0') {
                            fprintf(stderr, "%s: line %d: printf: %s: invalid number\n",
                                    get_script_name(state), get_lineno(state), val_str);
                            had_error = true;
                        }
                    }
                }
                char *tmp = malloc(alloc_sz);
                int n = snprintf(tmp, alloc_sz, c_spec, dnum);
                if (n > 0) sb_putmem(&sb, tmp, (size_t)n);
                free(tmp);
            } else if (conv == 'n') {
                args_consumed_in_pass++;
                const char *var_name = (arg_idx < argc) ? argv[arg_idx++] : NULL;
                if (var_name && *var_name) {
                    char nbuf[32];
                    snprintf(nbuf, sizeof(nbuf), "%zu", sb.len);
                    setenv(var_name, nbuf, 1);
                }
            } else {
                fprintf(stderr, "%s: line %d: printf: `%c': invalid format character\n",
                        get_script_name(state), get_lineno(state), conv);
                had_error = true;
                keep_running = false;
                break;
            }
        }

        if (args_consumed_in_pass == 0) {
            break; /* No format arguments used, do not repeat */
        }
    } while (keep_running && arg_idx < argc);

    if (vname) {
        setenv(vname, sb.buf ? sb.buf : "", 1);
    } else {
        if (sb.buf && sb.len > 0) {
            fwrite(sb.buf, 1, sb.len, stdout);
        }
    }

    sb_free(&sb);
    return had_error ? 1 : 0;
}
