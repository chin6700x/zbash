/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "parser_internal.h"

bool is_utf8_mode(void) {
    const char *lc = getenv("LC_ALL");
    if (!lc || !*lc) lc = getenv("LC_CTYPE");
    if (!lc || !*lc) lc = getenv("LANG");
    if (lc) {
        if (strstr(lc, "UTF-8") || strstr(lc, "utf-8") || strstr(lc, "UTF8") || strstr(lc, "utf8")) return true;
        if (strcmp(lc, "C") == 0 || strcmp(lc, "POSIX") == 0) return false;
    }
    return true; /* Default on Darwin / modern systems */
}

size_t utf8_char_count(const char *s) {
    if (!s) return 0;
    if (!is_utf8_mode()) return strlen(s);
    size_t count = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) count++;
        s++;
    }
    return count;
}

const char *utf8_char_at(const char *s, int char_idx) {
    if (!s || char_idx <= 0) return s;
    if (!is_utf8_mode()) {
        size_t slen = strlen(s);
        return ((size_t)char_idx >= slen) ? s + slen : s + char_idx;
    }
    int cur = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) {
            if (cur == char_idx) return s;
            cur++;
        }
        s++;
    }
    return s;
}

/* ========================================================================= */
/* Complete POSIX & Ksh-Style Extended Glob Matching Engine                  */
/* ========================================================================= */

static const char *patscan(const char *p, const char *pe, char delim, int flags) {
    int pnest = 0, bnest = 0, skip = 0;
    if (p >= pe) return NULL;
    for (const char *s = p; s < pe; s++) {
        if (skip) { skip = 0; continue; }
        char c = *s;
        if (c == '\\' && !(flags & ZBASH_FNM_NOESCAPE)) {
            skip = 1;
            continue;
        }
        if (c == '[') {
            if (bnest == 0) bnest = 1;
        } else if (c == ']' && bnest) {
            bnest = 0;
        } else if (bnest == 0) {
            if (c == '(') {
                pnest++;
            } else if (c == ')') {
                if (pnest == 0) return s + 1;
                pnest--;
            } else if (c == '|' && pnest == 0 && delim == '|') {
                return s + 1;
            }
        }
    }
    return NULL;
}

#define COLL_INVALID -2

static int get_collating_elem(const char **pp, const char *pe) {
    const char *p = *pp;
    if (p >= pe) return -1;
    if (*p == '[' && p + 1 < pe) {
        if (p[1] == '.') {
            const char *end = strstr(p + 2, ".]");
            if (end && end < pe) {
                size_t len = end - (p + 2);
                int val = -1;
                if (len == 1) val = (unsigned char)p[2];
                else if (len == 6 && strncmp(p + 2, "hyphen", 6) == 0) val = '-';
                else if (len == 5 && strncmp(p + 2, "space", 5) == 0) val = ' ';
                else if (len == 3 && strncmp(p + 2, "tab", 3) == 0) val = '\t';
                else if (len == 7 && strncmp(p + 2, "newline", 7) == 0) val = '\n';
                else if (len == 12 && strncmp(p + 2, "grave-accent", 12) == 0) val = '`';
                else val = COLL_INVALID;
                *pp = end + 2;
                return val;
            }
        } else if (p[1] == '=') {
            const char *end = strstr(p + 2, "=]");
            if (end && end < pe) {
                size_t len = end - (p + 2);
                int val = -1;
                if (len == 1) val = (unsigned char)p[2];
                else val = COLL_INVALID;
                *pp = end + 2;
                return val;
            }
        }
    }
    unsigned char c = (unsigned char)*p++;
    if (c == '\\' && p < pe) {
        c = (unsigned char)*p++;
    }
    *pp = p;
    return c;
}

static const char *brackmatch(const char *p, const char *pe, unsigned char test, int flags) {
    if (p >= pe) return NULL;
    bool neg = false;
    if (*p == '!' || *p == '^') {
        neg = true;
        p++;
    }
    bool matched = false;
    if (p < pe && *p == ']') {
        if (test == ']') matched = true;
        p++;
    }
    while (p < pe && *p != ']') {
        if (*p == '[' && p + 1 < pe && p[1] == ':') {
            const char *cl_end = strstr(p + 2, ":]");
            if (cl_end && cl_end < pe) {
                size_t nlen = cl_end - (p + 2);
                bool valid_class = true;
                if (nlen == 5 && strncmp(p + 2, "alnum", 5) == 0) { if (isalnum(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "alpha", 5) == 0) { if (isalpha(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "ascii", 5) == 0) { if ((unsigned char)test <= 127) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "blank", 5) == 0) { if (isblank(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "cntrl", 5) == 0) { if (iscntrl(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "digit", 5) == 0) { if (isdigit(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "graph", 5) == 0) { if (isgraph(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "lower", 5) == 0) { if (islower(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "print", 5) == 0) { if (isprint(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "punct", 5) == 0) { if (ispunct(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "space", 5) == 0) { if (isspace(test)) matched = true; }
                else if (nlen == 5 && strncmp(p + 2, "upper", 5) == 0) { if (isupper(test)) matched = true; }
                else if (nlen == 4 && strncmp(p + 2, "word", 4) == 0) { if (isalnum(test) || test == '_') matched = true; }
                else if (nlen == 6 && strncmp(p + 2, "xdigit", 6) == 0) { if (isxdigit(test)) matched = true; }
                else valid_class = false;

                if (valid_class) {
                    p = cl_end + 2;
                    continue;
                }
            }
        }

        int cstart = get_collating_elem(&p, pe);
        if (cstart == -1) break;

        if (p < pe && *p == '-' && p + 1 < pe && p[1] != ']') {
            p++; /* skip '-' */
            int cend = get_collating_elem(&p, pe);
            if (cstart >= 0 && cend >= 0 && cstart <= cend) {
                if (flags & ZBASH_FNM_CASEFOLD) {
                    if (tolower(test) >= tolower(cstart) && tolower(test) <= tolower(cend)) matched = true;
                } else {
                    if ((int)test >= cstart && (int)test <= cend) matched = true;
                }
            }
        } else if (cstart >= 0) {
            if (flags & ZBASH_FNM_CASEFOLD) {
                if (tolower(test) == tolower(cstart)) matched = true;
            } else {
                if ((int)test == cstart) matched = true;
            }
        }
    }

    /* A bracket expression MUST be closed by ']' */
    if (p >= pe || *p != ']') {
        return NULL;
    }
    p++; /* skip ']' */
    return (neg ? !matched : matched) ? p : NULL;
}

static int gmatch(const char *s, const char *se, const char *p, const char *pe, int flags);

static int extmatch(char op, const char *s, const char *se, const char *p, const char *pe, int flags) {
    const char *prest = patscan(p, pe, 0, flags);
    if (!prest) return 1;

    if (op == '*') {
        if (gmatch(s, se, prest, pe, flags) == 0) return 0;
    } else if (op == '?') {
        if (gmatch(s, se, prest, pe, flags) == 0) return 0;
    }

    if (op == '!') {
        for (const char *srest = s; srest <= se; srest++) {
            bool any_match = false;
            const char *psub = p;
            while (psub < prest) {
                const char *pnext = patscan(psub, pe, '|', flags);
                if (!pnext) break;
                const char *sub_end = (pnext < prest) ? (pnext - 1) : (prest - 1);
                if (gmatch(s, srest, psub, sub_end, flags) == 0) {
                    any_match = true;
                    break;
                }
                if (pnext >= prest) break;
                psub = pnext;
            }
            if (!any_match) {
                if (gmatch(srest, se, prest, pe, flags) == 0) return 0;
            }
        }
        return 1;
    }

    if (op == '*' || op == '+') {
        for (const char *srest = s; srest <= se; srest++) {
            const char *psub = p;
            while (psub < prest) {
                const char *pnext = patscan(psub, pe, '|', flags);
                if (!pnext) break;
                const char *sub_end = (pnext < prest) ? (pnext - 1) : (prest - 1);
                if (gmatch(s, srest, psub, sub_end, flags) == 0) {
                    if (gmatch(srest, se, prest, pe, flags) == 0) return 0;
                    if (srest > s && gmatch(srest, se, p - 2, pe, flags) == 0) return 0;
                }
                if (pnext >= prest) break;
                psub = pnext;
            }
        }
        return 1;
    }

    if (op == '?' || op == '@') {
        for (const char *srest = s; srest <= se; srest++) {
            const char *psub = p;
            while (psub < prest) {
                const char *pnext = patscan(psub, pe, '|', flags);
                if (!pnext) break;
                const char *sub_end = (pnext < prest) ? (pnext - 1) : (prest - 1);
                if (gmatch(s, srest, psub, sub_end, flags) == 0) {
                    if (gmatch(srest, se, prest, pe, flags) == 0) return 0;
                }
                if (pnext >= prest) break;
                psub = pnext;
            }
        }
        return 1;
    }

    return 1;
}

static int gmatch(const char *s, const char *se, const char *p, const char *pe, int flags) {
    while (p < pe) {
        if ((flags & ZBASH_FNM_EXTMATCH) && p + 1 < pe && p[1] == '(' &&
            (*p == '*' || *p == '+' || *p == '?' || *p == '@' || *p == '!')) {
            const char *scan_end = patscan(p + 2, pe, 0, flags);
            if (scan_end) {
                return extmatch(*p, s, se, p + 2, pe, flags);
            }
        }

        char c = *p;
        if (c == '?') {
            if (s >= se) return 1;
            if ((flags & ZBASH_FNM_PATHNAME) && *s == '/') return 1;
            s++; p++;
            continue;
        }

        if (c == '\\' && !(flags & ZBASH_FNM_NOESCAPE)) {
            p++;
            if (p >= pe) return 1;
            if (s >= se || *s != *p) return 1;
            s++; p++;
            continue;
        }

        if (c == '*') {
            while (p + 1 < pe && p[1] == '*') {
                if ((flags & ZBASH_FNM_EXTMATCH) && p + 2 < pe && p[2] == '(') break;
                p++;
            }
            if (p + 1 == pe) {
                if (flags & ZBASH_FNM_PATHNAME) {
                    for (const char *k = s; k < se; k++) {
                        if (*k == '/') return 1;
                    }
                }
                return 0;
            }
            for (const char *try_s = s; try_s <= se; try_s++) {
                if ((flags & ZBASH_FNM_PATHNAME) && try_s > s && *(try_s - 1) == '/') break;
                if (gmatch(try_s, se, p + 1, pe, flags) == 0) return 0;
            }
            return 1;
        }

        if (c == '[') {
            if (s >= se) return 1;
            if ((flags & ZBASH_FNM_PATHNAME) && *s == '/') return 1;
            const char *next_p = brackmatch(p + 1, pe, (unsigned char)*s, flags);
            if (next_p) {
                p = next_p;
                s++;
                continue;
            }
            if (*s != '[') return 1;
            s++; p++;
            continue;
        }

        if (s >= se) return 1;
        char sc = *s;
        if (flags & ZBASH_FNM_CASEFOLD) {
            c = tolower((unsigned char)c);
            sc = tolower((unsigned char)sc);
        }
        if (c != sc) return 1;
        s++; p++;
    }

    return (s == se) ? 0 : 1;
}

int zbash_fnmatch(const char *pattern, const char *string, int flags) {
    if (!pattern || !string) return 1;
    size_t plen = strlen(pattern);
    size_t slen = strlen(string);
    return gmatch(string, string + slen, pattern, pattern + plen, flags);
}

/* ========================================================================= */
/* Variable Expansion Pattern Operations (${var#pat}, ${var%pat}, etc.)     */
/* ========================================================================= */

char *apply_pattern_removal(const char *env_v, const char *pat, bool is_prefix, bool longest) {
    if (!env_v) env_v = "";
    if (!pat) pat = "";
    size_t vlen = strlen(env_v);
    int flags = ZBASH_FNM_EXTMATCH;
    if (is_prefix) {
        if (longest) {
            char pbuf[2048];
            for (ssize_t k = (ssize_t)vlen; k >= 0; k--) {
                char *p = (k < (ssize_t)sizeof(pbuf)) ? pbuf : malloc(k + 1);
                if (p) {
                    memcpy(p, env_v, k);
                    p[k] = '\0';
                    if (zbash_fnmatch(pat, p, flags) == 0) {
                        char *res = strdup(env_v + k);
                        if (p != pbuf) free(p);
                        return res;
                    }
                    if (p != pbuf) free(p);
                }
            }
        } else {
            char pbuf[2048];
            for (size_t k = 0; k <= vlen; k++) {
                char *p = (k < sizeof(pbuf)) ? pbuf : malloc(k + 1);
                if (p) {
                    memcpy(p, env_v, k);
                    p[k] = '\0';
                    if (zbash_fnmatch(pat, p, flags) == 0) {
                        char *res = strdup(env_v + k);
                        if (p != pbuf) free(p);
                        return res;
                    }
                    if (p != pbuf) free(p);
                }
            }
        }
    } else {
        /* Suffix removal */
        if (longest) {
            for (ssize_t k = (ssize_t)vlen; k >= 0; k--) {
                const char *suf = env_v + (vlen - k);
                if (zbash_fnmatch(pat, suf, flags) == 0) {
                    size_t cut_len = vlen - k;
                    char *res = malloc(cut_len + 1);
                    if (res) {
                        memcpy(res, env_v, cut_len);
                        res[cut_len] = '\0';
                        return res;
                    }
                }
            }
        } else {
            for (size_t k = 0; k <= vlen; k++) {
                const char *suf = env_v + (vlen - k);
                if (zbash_fnmatch(pat, suf, flags) == 0) {
                    size_t cut_len = vlen - k;
                    char *res = malloc(cut_len + 1);
                    if (res) {
                        memcpy(res, env_v, cut_len);
                        res[cut_len] = '\0';
                        return res;
                    }
                }
            }
        }
    }
    return strdup(env_v);
}

char *apply_pattern_subst(const char *env_v, const char *raw_pat, const char *replacement, bool replace_all) {
    if (!env_v) env_v = "";
    if (!raw_pat) raw_pat = "";
    if (!replacement) replacement = "";

    bool anchor_start = false;
    bool anchor_end = false;
    const char *pattern = raw_pat;
    if (pattern[0] == '#') {
        anchor_start = true;
        pattern++;
    } else if (pattern[0] == '%') {
        anchor_end = true;
        pattern++;
    }

    char unesc_rep[1024];
    char *ur = unesc_rep;
    const char *rp = replacement;
    while (*rp && (size_t)(ur - unesc_rep) < sizeof(unesc_rep) - 1) {
        if (*rp == '\\' && *(rp + 1) == '\\') rp++;
        *ur++ = *rp++;
    }
    *ur = '\0';
    const char *actual_rep = unesc_rep;

    size_t plen = strlen(pattern);
    size_t rlen = strlen(actual_rep);
    size_t vlen = strlen(env_v);
    int flags = ZBASH_FNM_EXTMATCH;

    if (plen == 0) {
        if (anchor_start) {
            char *res = malloc(rlen + vlen + 1);
            if (res) {
                memcpy(res, actual_rep, rlen);
                memcpy(res + rlen, env_v, vlen);
                res[rlen + vlen] = '\0';
                return res;
            }
        } else if (anchor_end) {
            char *res = malloc(vlen + rlen + 1);
            if (res) {
                memcpy(res, env_v, vlen);
                memcpy(res + vlen, actual_rep, rlen);
                res[vlen + rlen] = '\0';
                return res;
            }
        }
        return strdup(env_v);
    }

    bool has_glob = (strpbrk(pattern, "*?[+@!\\") != NULL);

    if (anchor_start) {
        size_t matched_len = 0;
        if (has_glob) {
            char temp[1024];
            for (size_t try_len = vlen; try_len >= 1; try_len--) {
                if (try_len < sizeof(temp)) {
                    memcpy(temp, env_v, try_len);
                    temp[try_len] = '\0';
                    if (zbash_fnmatch(pattern, temp, flags) == 0) {
                        matched_len = try_len;
                        break;
                    }
                }
            }
        } else {
            if (strncmp(env_v, pattern, plen) == 0) matched_len = plen;
        }
        if (matched_len > 0) {
            size_t rem = vlen - matched_len;
            char *res = malloc(rlen + rem + 1);
            if (res) {
                memcpy(res, actual_rep, rlen);
                memcpy(res + rlen, env_v + matched_len, rem);
                res[rlen + rem] = '\0';
                return res;
            }
        }
        return strdup(env_v);
    }

    if (anchor_end) {
        size_t match_offset = (size_t)-1;
        if (has_glob) {
            for (size_t off = 0; off <= vlen; off++) {
                if (zbash_fnmatch(pattern, env_v + off, flags) == 0) {
                    match_offset = off;
                    break;
                }
            }
        } else {
            if (vlen >= plen && strcmp(env_v + vlen - plen, pattern) == 0) {
                match_offset = vlen - plen;
            }
        }
        if (match_offset != (size_t)-1) {
            char *res = malloc(match_offset + rlen + 1);
            if (res) {
                memcpy(res, env_v, match_offset);
                memcpy(res + match_offset, actual_rep, rlen);
                res[match_offset + rlen] = '\0';
                return res;
            }
        }
        return strdup(env_v);
    }

    size_t cap = strlen(env_v) * 2 + rlen + 64;
    char *res = malloc(cap);
    if (!res) return strdup(env_v);
    res[0] = '\0';

    const char *src = env_v;
    char *dst = res;
    size_t dlen = 0;

    while (*src) {
        size_t matched_len = 0;
        if (has_glob) {
            char temp[1024];
            for (size_t try_len = strlen(src); try_len >= 1; try_len--) {
                if (try_len < sizeof(temp)) {
                    memcpy(temp, src, try_len);
                    temp[try_len] = '\0';
                    if (zbash_fnmatch(pattern, temp, flags) == 0) {
                        matched_len = try_len;
                        break;
                    }
                }
            }
        } else {
            if (strncmp(src, pattern, plen) == 0) matched_len = plen;
        }

        if (matched_len > 0) {
            if (dlen + rlen + 1 >= cap) {
                cap = (cap + rlen + 64) * 2;
                res = realloc(res, cap);
                dst = res + dlen;
            }
            memcpy(dst, actual_rep, rlen);
            dst += rlen;
            dlen += rlen;
            src += matched_len;
            if (!replace_all) {
                size_t rem = strlen(src);
                if (dlen + rem + 1 >= cap) {
                    cap = (cap + rem + 64) * 2;
                    res = realloc(res, cap);
                    dst = res + dlen;
                }
                memcpy(dst, src, rem);
                dst += rem;
                dlen += rem;
                break;
            }
        } else {
            if (dlen + 2 >= cap) {
                cap *= 2;
                res = realloc(res, cap);
                dst = res + dlen;
            }
            *dst++ = *src++;
            dlen++;
        }
    }
    *dst = '\0';
    return res;
}

bool zbash_has_glob(zbash_state_t *state, const char *word) {
    if (!word) return false;
    bool in_s = false, in_d = false;
    for (size_t i = 0; word[i] != '\0'; i++) {
        char c = word[i];
        if (c == '\\' && !in_s && word[i + 1] != '\0') {
            i++;
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
        if (!in_s && !in_d) {
            if (c == '*' || c == '?') return true;
            if (c == '[') {
                const char *end_b = strchr(word + i + 1, ']');
                if (end_b) return true;
            }
            if (state && state->extglob && (c == '@' || c == '!' || c == '+' || c == '*' || c == '?') && word[i + 1] == '(') {
                return true;
            }
        }
    }
    return false;
}

static void glob_helper(zbash_state_t *state, const char *dir_path, const char *subpat, char ***out_list, int *out_cnt, int *out_cap) {
    if (!subpat) return;

    const char *slash = strchr(subpat, '/');
    if (!slash) {
        /* Leaf component */
        const char *open_path = (dir_path && dir_path[0]) ? dir_path : ".";
        DIR *d = opendir(open_path);
        if (!d) return;

        struct dirent *ent;
        int fnm_flags = (state && state->extglob) ? ZBASH_FNM_EXTMATCH : 0;

        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                if (subpat[0] != '.') continue;
            }
            if (ent->d_name[0] == '.' && subpat[0] != '.') continue;

            if (zbash_fnmatch(subpat, ent->d_name, fnm_flags) == 0) {
                char full[1024];
                if (!dir_path || !dir_path[0]) {
                    snprintf(full, sizeof(full), "%s", ent->d_name);
                } else if (strcmp(dir_path, "/") == 0) {
                    snprintf(full, sizeof(full), "/%s", ent->d_name);
                } else {
                    snprintf(full, sizeof(full), "%s/%s", dir_path, ent->d_name);
                }

                if (*out_cnt >= *out_cap) {
                    *out_cap = (*out_cap == 0) ? 32 : (*out_cap * 2);
                    *out_list = realloc(*out_list, *out_cap * sizeof(char *));
                }
                (*out_list)[(*out_cnt)++] = strdup(full);
            }
        }
        closedir(d);
        return;
    }

    /* There is a slash: subpat = seg + '/' + rest */
    size_t seg_len = (size_t)(slash - subpat);
    char seg[512];
    if (seg_len >= sizeof(seg)) seg_len = sizeof(seg) - 1;
    strncpy(seg, subpat, seg_len);
    seg[seg_len] = '\0';
    const char *rest = slash + 1;

    if (!zbash_has_glob(state, seg)) {
        char next_dir[1024];
        if (!dir_path || !dir_path[0]) {
            snprintf(next_dir, sizeof(next_dir), "%s", seg);
        } else if (strcmp(dir_path, "/") == 0) {
            snprintf(next_dir, sizeof(next_dir), "/%s", seg);
        } else {
            snprintf(next_dir, sizeof(next_dir), "%s/%s", dir_path, seg);
        }
        glob_helper(state, next_dir, rest, out_list, out_cnt, out_cap);
        return;
    }

    const char *open_path = (dir_path && dir_path[0]) ? dir_path : ".";
    DIR *d = opendir(open_path);
    if (!d) return;

    struct dirent *ent;
    int fnm_flags = (state && state->extglob) ? ZBASH_FNM_EXTMATCH : 0;

    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            if (seg[0] != '.') continue;
        }
        if (ent->d_name[0] == '.' && seg[0] != '.') continue;

        if (zbash_fnmatch(seg, ent->d_name, fnm_flags) == 0) {
            char next_dir[1024];
            if (!dir_path || !dir_path[0]) {
                snprintf(next_dir, sizeof(next_dir), "%s", ent->d_name);
            } else if (strcmp(dir_path, "/") == 0) {
                snprintf(next_dir, sizeof(next_dir), "/%s", ent->d_name);
            } else {
                snprintf(next_dir, sizeof(next_dir), "%s/%s", dir_path, ent->d_name);
            }

            struct stat st;
            if (stat(next_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
                glob_helper(state, next_dir, rest, out_list, out_cnt, out_cap);
            }
        }
    }
    closedir(d);
}

static int compare_glob_matches(const void *a, const void *b) {
    return strcoll(*(const char **)a, *(const char **)b);
}

int zbash_expand_path_glob(zbash_state_t *state, const char *pattern, char ***out_matches, int *out_count) {
    if (!pattern || !out_matches || !out_count) return 0;
    *out_matches = NULL;
    *out_count = 0;

    if (!zbash_has_glob(state, pattern)) {
        return 0;
    }

    int cap = 0;
    if (pattern[0] == '/') {
        glob_helper(state, "/", pattern + 1, out_matches, out_count, &cap);
    } else {
        glob_helper(state, "", pattern, out_matches, out_count, &cap);
    }

    if (*out_count > 1) {
        qsort(*out_matches, *out_count, sizeof(char *), compare_glob_matches);
    }

    return *out_count;
}
