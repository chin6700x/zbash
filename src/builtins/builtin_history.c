/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"
#include <fnmatch.h>
#include <libgen.h>

/* ========================================================================= */
/* History Lifecycle & Storage                                               */
/* ========================================================================= */

void zbash_history_init(zbash_state_t *state) {
    if (!state) return;
    state->history_cap = 128;
    state->history_count = 0;
    state->history_base = 1;
    state->history_last_written = 0;
    state->history_file_loaded = false;
    state->history_lines = malloc(state->history_cap * sizeof(char *));
}

void zbash_history_free(zbash_state_t *state) {
    if (!state || !state->history_lines) return;
    for (int i = 0; i < state->history_count; i++) {
        if (state->history_lines[i]) free(state->history_lines[i]);
    }
    free(state->history_lines);
    state->history_lines = NULL;
    state->history_count = 0;
    state->history_cap = 0;
}

static char *fold_multiline(const char *line) {
    if (!line) return NULL;
    if (!strchr(line, '\n')) return strdup(line);

    size_t len = strlen(line);
    char *out = malloc(len * 2 + 1);
    size_t o = 0;
    bool in_s = false, in_d = false;

    for (size_t i = 0; i < len; i++) {
        char c = line[i];
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;

        if (c == '\n' && !in_s && !in_d) {
            bool needs_semi = true;
            if (o > 0 && (out[o - 1] == ';' || out[o - 1] == '&' || out[o - 1] == '|' || out[o - 1] == '{' || out[o - 1] == '(')) {
                needs_semi = false;
            } else {
                size_t p = o;
                while (p > 0 && isspace((unsigned char)out[p - 1])) p--;
                size_t wend = p;
                while (p > 0 && !isspace((unsigned char)out[p - 1]) && out[p - 1] != ';') p--;
                size_t wlen = wend - p;
                if ((wlen == 2 && strncmp(out + p, "do", 2) == 0) ||
                    (wlen == 4 && strncmp(out + p, "then", 4) == 0) ||
                    (wlen == 4 && strncmp(out + p, "else", 4) == 0) ||
                    (wlen == 4 && strncmp(out + p, "elif", 4) == 0) ||
                    (wlen == 2 && strncmp(out + p, "in", 2) == 0)) {
                    needs_semi = false;
                }
            }
            if (needs_semi) {
                out[o++] = ';';
                out[o++] = ' ';
            } else {
                out[o++] = ' ';
            }
            while (i + 1 < len && (line[i + 1] == ' ' || line[i + 1] == '\t')) {
                out[o++] = line[++i];
            }
        } else {
            out[o++] = c;
        }
    }
    out[o] = '\0';
    return out;
}

void zbash_history_add(zbash_state_t *state, const char *line) {
    if (!state || !line || !*line) return;
    while (*line && isspace((unsigned char)*line)) line++;
    if (*line == '\0') return;

    /* Check HISTCONTROL */
    const char *hc = getenv("HISTCONTROL");
    if (hc) {
        bool ign_space = (strstr(hc, "ignorespace") != NULL || strstr(hc, "ignoreboth") != NULL);
        bool ign_dups = (strstr(hc, "ignoredups") != NULL || strstr(hc, "ignoreboth") != NULL);
        if (ign_space && (line[0] == ' ' || line[0] == '\t')) return;
        if (ign_dups && state->history_count > 0 && strcmp(state->history_lines[state->history_count - 1], line) == 0) {
            return;
        }
    }

    /* Check HISTIGNORE */
    const char *hi = getenv("HISTIGNORE");
    if (hi) {
        char hi_buf[512];
        strncpy(hi_buf, hi, sizeof(hi_buf) - 1);
        hi_buf[sizeof(hi_buf) - 1] = '\0';
        char *tok = strtok(hi_buf, ":");
        while (tok) {
            if (strcmp(tok, "&") == 0) {
                if (state->history_count > 0 && strcmp(state->history_lines[state->history_count - 1], line) == 0) {
                    return;
                }
            } else {
                if (fnmatch(tok, line, 0) == 0) {
                    return;
                }
            }
            tok = strtok(NULL, ":");
        }
    }

    if (!state->history_lines) zbash_history_init(state);

    char *entry = fold_multiline(line);
    if (!entry) return;

    if (state->history_count >= state->history_cap) {
        state->history_cap *= 2;
        state->history_lines = realloc(state->history_lines, state->history_cap * sizeof(char *));
    }
    state->history_lines[state->history_count++] = entry;

    /* Check HISTSIZE */
    const char *hs = getenv("HISTSIZE");
    if (hs) {
        int max_size = atoi(hs);
        if (max_size > 0 && state->history_count > max_size) {
            free(state->history_lines[0]);
            memmove(&state->history_lines[0], &state->history_lines[1], (state->history_count - 1) * sizeof(char *));
            state->history_count--;
            state->history_base++;
            if (state->history_last_written > 0) state->history_last_written--;
        }
    }
}

int zbash_history_load(zbash_state_t *state, const char *filename) {
    if (!state) return 1;
    const char *fn = filename ? filename : getenv("HISTFILE");
    if (!fn) fn = "history.list";

    FILE *fp = fopen(fn, "r");
    if (!fp) return 1;

    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;
    while ((nread = getline(&line, &cap, fp)) != -1) {
        if (nread > 0 && line[nread - 1] == '\n') line[--nread] = '\0';
        if (nread > 0 && line[nread - 1] == '\r') line[--nread] = '\0';
        if (nread > 0) {
            if (!state->history_lines) zbash_history_init(state);
            if (state->history_count >= state->history_cap) {
                state->history_cap *= 2;
                state->history_lines = realloc(state->history_lines, state->history_cap * sizeof(char *));
            }
            state->history_lines[state->history_count++] = strdup(line);
        }
    }
    if (line) free(line);
    fclose(fp);
    state->history_file_loaded = true;
    state->history_last_written = state->history_count;
    return 0;
}

/* ========================================================================= */
/* Builtin: history                                                          */
/* ========================================================================= */

int builtin_history(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state->lineno > 0 ? state->lineno : 1;

    bool opt_c = false, opt_d = false, opt_a = false, opt_w = false, opt_r = false, opt_n = false, opt_s = false, opt_p = false;
    const char *d_arg = NULL;
    int opt_cnt_awrn = 0;
    int first_nonopt = argc;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            for (int c = 1; argv[i][c]; c++) {
                char ch = argv[i][c];
                if (ch == 'c') opt_c = true;
                else if (ch == 'd') {
                    opt_d = true;
                    if (argv[i][c + 1] != '\0') {
                        d_arg = &argv[i][c + 1];
                        break;
                    } else if (i + 1 < argc) {
                        d_arg = argv[++i];
                        break;
                    }
                } else if (ch == 'a') { opt_a = true; opt_cnt_awrn++; }
                else if (ch == 'w') { opt_w = true; opt_cnt_awrn++; }
                else if (ch == 'r') { opt_r = true; opt_cnt_awrn++; }
                else if (ch == 'n') { opt_n = true; opt_cnt_awrn++; }
                else if (ch == 's') { opt_s = true; first_nonopt = i + 1; goto done_opts; }
                else if (ch == 'p') { opt_p = true; first_nonopt = i + 1; goto done_opts; }
                else {
                    fprintf(stderr, "%s: line %d: history: %s: invalid option\nhistory: usage: history [-c] [-d offset] [n] or history -awrn [filename] or history -ps arg [arg...]\n", src, lineno, argv[i]);
                    return 2;
                }
            }
        } else {
            first_nonopt = i;
            break;
        }
    }
done_opts:

    if (opt_cnt_awrn > 1) {
        fprintf(stderr, "%s: line %d: history: cannot use more than one of -anrw\n", src, lineno);
        return 1;
    }

    if (opt_c) {
        if (state->history_lines) {
            for (int i = 0; i < state->history_count; i++) {
                if (state->history_lines[i]) free(state->history_lines[i]);
            }
            state->history_count = 0;
            state->history_base = 1;
            state->history_last_written = 0;
        }
        return 0;
    }

    if (opt_d) {
        if (!d_arg) {
            fprintf(stderr, "%s: line %d: history: -d: option requires an argument\n", src, lineno);
            return 2;
        }
        int pos = atoi(d_arg);
        int idx = (pos >= state->history_base) ? (pos - state->history_base) : (pos < 0 ? state->history_count + pos : -1);
        if (idx < 0 || idx >= state->history_count) {
            fprintf(stderr, "%s: line %d: history: %s: history position out of range\n", src, lineno, d_arg);
            return 1;
        }
        free(state->history_lines[idx]);
        memmove(&state->history_lines[idx], &state->history_lines[idx + 1], (state->history_count - idx - 1) * sizeof(char *));
        state->history_count--;
        return 0;
    }

    if (opt_s) {
        /* Store arguments into history as a single entry */
        size_t total_len = 0;
        for (int i = first_nonopt; i < argc; i++) {
            total_len += strlen(argv[i]) + 1;
        }
        if (total_len > 0) {
            char *buf = malloc(total_len + 1);
            buf[0] = '\0';
            for (int i = first_nonopt; i < argc; i++) {
                if (i > first_nonopt) strcat(buf, " ");
                strcat(buf, argv[i]);
            }
            if (!state->history_lines) zbash_history_init(state);
            if (state->history_count >= state->history_cap) {
                state->history_cap *= 2;
                state->history_lines = realloc(state->history_lines, state->history_cap * sizeof(char *));
            }
            state->history_lines[state->history_count++] = buf;
        }
        return 0;
    }

    if (opt_p) {
        /* Perform history expansion on args and print without adding to history */
        int rc = 0;
        for (int i = first_nonopt; i < argc; i++) {
            char *expanded = NULL;
            int ret = zbash_histexp_expand(state, argv[i], &expanded);
            if (ret < 0) {
                rc = 1;
            } else if (ret > 0 && expanded) {
                printf("%s\n", expanded);
                free(expanded);
            } else {
                printf("%s\n", argv[i]);
            }
        }
        return rc;
    }

    const char *target_file = (first_nonopt < argc) ? argv[first_nonopt] : getenv("HISTFILE");
    if (!target_file) target_file = "history.list";

    if (opt_a) {
        FILE *fp = fopen(target_file, "a");
        if (fp) {
            for (int i = state->history_last_written; i < state->history_count; i++) {
                fprintf(fp, "%s\n", state->history_lines[i]);
            }
            fclose(fp);
            state->history_last_written = state->history_count;
        }
        return 0;
    }

    if (opt_w) {
        FILE *fp = fopen(target_file, "w");
        if (fp) {
            for (int i = 0; i < state->history_count; i++) {
                fprintf(fp, "%s\n", state->history_lines[i]);
            }
            fclose(fp);
            state->history_last_written = state->history_count;
        }
        return 0;
    }

    if (opt_r || opt_n) {
        return zbash_history_load(state, target_file);
    }

    /* Print history */
    int num_to_show = state->history_count;
    if (first_nonopt < argc && isdigit((unsigned char)argv[first_nonopt][0])) {
        num_to_show = atoi(argv[first_nonopt]);
        if (num_to_show > state->history_count) num_to_show = state->history_count;
    }

    int start_i = state->history_count - num_to_show;
    if (start_i < 0) start_i = 0;

    for (int i = start_i; i < state->history_count; i++) {
        printf("%5d  %s\n", state->history_base + i, state->history_lines[i]);
    }
    return 0;
}

/* ========================================================================= */
/* Builtin: fc                                                               */
/* ========================================================================= */

int builtin_fc(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state->lineno > 0 ? state->lineno : 1;

    bool opt_l = false, opt_n = false, opt_r = false, opt_s = false;
    int first_arg = argc;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            for (int c = 1; argv[i][c]; c++) {
                char ch = argv[i][c];
                if (ch == 'l') opt_l = true;
                else if (ch == 'n') opt_n = true;
                else if (ch == 'r') opt_r = true;
                else if (ch == 's') opt_s = true;
                else if (ch == 'e') {
                    if (i + 1 < argc) { i++; break; }
                } else {
                    fprintf(stderr, "%s: line %d: fc: %s: invalid option\nfc: usage: fc [-e ename] [-nlr] [first] [last] or fc -s [pat=rep] [cmd]\n", src, lineno, argv[i]);
                    return 2;
                }
            }
        } else {
            first_arg = i;
            break;
        }
    }

    if (opt_s) {
        /* fc -s [pat=rep ...] [cmd] */
        const char *cmd_spec = NULL;
        typedef struct { const char *pat; const char *rep; } subst_t;
        subst_t substs[16];
        int num_substs = 0;

        char pat_buf[16][128];
        for (int i = first_arg; i < argc; i++) {
            char *eq = strchr(argv[i], '=');
            if (eq && num_substs < 16) {
                size_t plen = eq - argv[i];
                if (plen >= sizeof(pat_buf[num_substs])) plen = sizeof(pat_buf[num_substs]) - 1;
                strncpy(pat_buf[num_substs], argv[i], plen);
                pat_buf[num_substs][plen] = '\0';
                substs[num_substs].pat = pat_buf[num_substs];
                substs[num_substs].rep = eq + 1;
                num_substs++;
            } else {
                cmd_spec = argv[i];
            }
        }

        /* Find target command */
        int target_idx = -1;
        if (cmd_spec) {
            if (isdigit((unsigned char)cmd_spec[0]) || (cmd_spec[0] == '-' && isdigit((unsigned char)cmd_spec[1]))) {
                int n = atoi(cmd_spec);
                target_idx = (n >= state->history_base) ? (n - state->history_base) : (state->history_count + n);
            } else {
                for (int k = state->history_count - 1; k >= 0; k--) {
                    if (strncmp(state->history_lines[k], cmd_spec, strlen(cmd_spec)) == 0) {
                        target_idx = k;
                        break;
                    }
                }
            }
        } else {
            target_idx = state->history_count - 1;
            while (target_idx >= 0) {
                const char *h = state->history_lines[target_idx];
                while (*h && isspace((unsigned char)*h)) h++;
                if (strncmp(h, "fc ", 3) != 0 && strcmp(h, "fc") != 0 &&
                    strncmp(h, "r ", 2) != 0 && strcmp(h, "r") != 0) {
                    break;
                }
                target_idx--;
            }
        }

        if (target_idx < 0 || target_idx >= state->history_count) {
            fflush(stdout);
            fprintf(stderr, "%s: line %d: fc: no command found\n", src, lineno);
            return 1;
        }

        char work[4096];
        strncpy(work, state->history_lines[target_idx], sizeof(work) - 1);
        work[sizeof(work) - 1] = '\0';

        /* Apply substitutions (all occurrences) */
        for (int s = 0; s < num_substs; s++) {
            const char *p = substs[s].pat;
            const char *r = substs[s].rep;
            size_t plen = strlen(p);
            char tmp[4096];
            tmp[0] = '\0';
            char *pos = work;
            char *match;
            while ((match = strstr(pos, p)) != NULL) {
                strncat(tmp, pos, match - pos);
                strcat(tmp, r);
                pos = match + plen;
            }
            strcat(tmp, pos);
            strncpy(work, tmp, sizeof(work) - 1);
            work[sizeof(work) - 1] = '\0';
        }

        fprintf(stderr, "%s\n", work);
        fflush(stdout);
        if (state->history_enabled) {
            zbash_history_add(state, work);
        }
        return zbash_run_line(state, work);
    }

    if (opt_l) {
        int first_idx = -1;
        int last_idx = -1;

        if (first_arg < argc) {
            const char *fa = argv[first_arg];
            if (isdigit((unsigned char)fa[0]) || (fa[0] == '-' && isdigit((unsigned char)fa[1]))) {
                int n = atoi(fa);
                first_idx = (n >= state->history_base) ? (n - state->history_base) : (state->history_count + n);
            } else {
                for (int k = state->history_count - 1; k >= 0; k--) {
                    if (strncmp(state->history_lines[k], fa, strlen(fa)) == 0) {
                        first_idx = k;
                        break;
                    }
                }
            }
            if (first_arg + 1 < argc) {
                const char *la = argv[first_arg + 1];
                if (isdigit((unsigned char)la[0]) || (la[0] == '-' && isdigit((unsigned char)la[1]))) {
                    int n = atoi(la);
                    last_idx = (n >= state->history_base) ? (n - state->history_base) : (state->history_count + n);
                } else {
                    for (int k = state->history_count - 1; k >= 0; k--) {
                        if (strncmp(state->history_lines[k], la, strlen(la)) == 0) {
                            last_idx = k;
                            break;
                        }
                    }
                }
            } else {
                last_idx = state->history_count - 1;
            }
        } else {
            first_idx = (state->history_count > 16) ? (state->history_count - 16) : 0;
            last_idx = state->history_count - 1;
        }

        if (first_idx < 0 || first_idx >= state->history_count || last_idx < 0 || last_idx >= state->history_count) {
            fflush(stdout);
            fprintf(stderr, "%s: line %d: fc: history specification out of range\n", src, lineno);
            return 1;
        }

        int step = (first_idx <= last_idx) ? 1 : -1;
        if (opt_r) {
            int tmp = first_idx;
            first_idx = last_idx;
            last_idx = tmp;
            step = -step;
        }

        for (int i = first_idx; ; i += step) {
            if (opt_n) {
                printf("\t %s\n", state->history_lines[i]);
            } else {
                printf("%d\t %s\n", state->history_base + i, state->history_lines[i]);
            }
            if (i == last_idx) break;
        }
        return 0;
    }

    return 0;
}

/* ========================================================================= */
/* History Expansion Engine                                                  */
/* ========================================================================= */

static char *apply_modifier(zbash_state_t *state, const char *word, const char *mod_str, size_t *consumed, bool *out_only_print) {
    if (!mod_str || *mod_str != ':') return strdup(word);
    char mod = mod_str[1];
    *consumed = 2;

    if (mod == 'h') {
        /* Head (dirname) */
        char tmp[1024];
        strncpy(tmp, word, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        char *d = dirname(tmp);
        return strdup(d);
    } else if (mod == 't') {
        /* Tail (basename) */
        char tmp[1024];
        strncpy(tmp, word, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        char *b = basename(tmp);
        return strdup(b);
    } else if (mod == 'r') {
        /* Root (remove trailing suffix) */
        char *res = strdup(word);
        char *dot = strrchr(res, '.');
        if (dot && dot != res) *dot = '\0';
        return res;
    } else if (mod == 'e') {
        /* Extension (all but root) */
        const char *dot = strrchr(word, '.');
        return dot ? strdup(dot) : strdup("");
    } else if (mod == 'p') {
        if (out_only_print) *out_only_print = true;
        return strdup(word);
    } else if (mod == 'q') {
        size_t wlen = strlen(word);
        char *res = malloc(wlen + 3);
        snprintf(res, wlen + 3, "'%s'", word);
        return res;
    } else if (mod == 'x') {
        char out[4096];
        out[0] = '\0';
        char wcopy[4096];
        strncpy(wcopy, word, sizeof(wcopy) - 1);
        wcopy[sizeof(wcopy) - 1] = '\0';
        char *tok = strtok(wcopy, " \t");
        bool first = true;
        while (tok) {
            if (!first) strcat(out, " ");
            strcat(out, "'");
            strcat(out, tok);
            strcat(out, "'");
            first = false;
            tok = strtok(NULL, " \t");
        }
        return strdup(out);
    } else if (mod == 's' || (mod == 'g' && mod_str[2] == 's')) {
        bool global = (mod == 'g');
        size_t off = global ? 3 : 2;
        char delim = mod_str[off++];
        if (delim == '\0') return strdup(word);

        const char *p1 = mod_str + off;
        const char *p2 = strchr(p1, delim);
        if (!p2) return strdup(word);
        size_t pat_len = p2 - p1;
        char pat[256];
        if (pat_len >= sizeof(pat)) pat_len = sizeof(pat) - 1;
        strncpy(pat, p1, pat_len);
        pat[pat_len] = '\0';

        const char *p3 = p2 + 1;
        const char *p4 = strchr(p3, delim);
        size_t rep_len = p4 ? (size_t)(p4 - p3) : strlen(p3);
        char rep[256];
        if (rep_len >= sizeof(rep)) rep_len = sizeof(rep) - 1;
        strncpy(rep, p3, rep_len);
        rep[rep_len] = '\0';

        *consumed = (p4 ? (size_t)(p4 - mod_str + 1) : (p3 + rep_len - mod_str));

        /* Remember substitution for :& */
        strncpy(state->hist_subst_pat, pat, sizeof(state->hist_subst_pat) - 1);
        strncpy(state->hist_subst_rep, rep, sizeof(state->hist_subst_rep) - 1);

        /* Expand '&' in rep to pat */
        char act_rep[512];
        act_rep[0] = '\0';
        for (size_t k = 0; rep[k]; k++) {
            if (rep[k] == '&') strcat(act_rep, pat);
            else {
                size_t al = strlen(act_rep);
                act_rep[al] = rep[k];
                act_rep[al + 1] = '\0';
            }
        }

        /* Perform substitution */
        char out[4096];
        out[0] = '\0';
        const char *curr = word;
        const char *m;
        while ((m = strstr(curr, pat)) != NULL) {
            strncat(out, curr, m - curr);
            strcat(out, act_rep);
            curr = m + pat_len;
            if (!global) break;
        }
        strcat(out, curr);
        return strdup(out);
    } else if (mod == '&' || (mod == 'g' && mod_str[2] == '&')) {
        bool global = (mod == 'g');
        *consumed = global ? 3 : 2;
        const char *pat = state->hist_subst_pat;
        const char *rep = state->hist_subst_rep;
        size_t pat_len = strlen(pat);
        if (pat_len == 0) return strdup(word);

        char act_rep[512];
        act_rep[0] = '\0';
        for (size_t k = 0; rep[k]; k++) {
            if (rep[k] == '&') strcat(act_rep, pat);
            else {
                size_t al = strlen(act_rep);
                act_rep[al] = rep[k];
                act_rep[al + 1] = '\0';
            }
        }

        char out[4096];
        out[0] = '\0';
        const char *curr = word;
        const char *m;
        while ((m = strstr(curr, pat)) != NULL) {
            strncat(out, curr, m - curr);
            strcat(out, act_rep);
            curr = m + pat_len;
            if (!global) break;
        }
        strcat(out, curr);
        return strdup(out);
    }

    return NULL;
}

int zbash_histexp_expand(zbash_state_t *state, const char *line, char **out_expanded) {
    if (!state || !line || !out_expanded) return 0;
    *out_expanded = NULL;

    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state->lineno > 0 ? state->lineno : 1;

    /* Quick substitution: ^pat^rep^ or ^pat^rep */
    if (line[0] == '^') {
        const char *p1 = line + 1;
        const char *p2 = strchr(p1, '^');
        if (!p2) return 0;
        size_t plen = p2 - p1;
        char pat[256];
        if (plen >= sizeof(pat)) plen = sizeof(pat) - 1;
        strncpy(pat, p1, plen);
        pat[plen] = '\0';

        const char *p3 = p2 + 1;
        const char *p4 = strchr(p3, '^');
        size_t rlen = p4 ? (size_t)(p4 - p3) : strlen(p3);
        char rep[256];
        if (rlen >= sizeof(rep)) rlen = sizeof(rep) - 1;
        strncpy(rep, p3, rlen);
        rep[rlen] = '\0';

        if (state->history_count == 0) {
            fprintf(stderr, "%s: line %d: history: !!: history expansion failed\n", src, lineno);
            return -1;
        }
        const char *prev = state->history_lines[state->history_count - 1];
        char *m = strstr(prev, pat);
        if (!m) {
            fprintf(stderr, "%s: line %d: history: !!:s^%s^%s: substitution failed\n", src, lineno, pat, rep);
            return -1;
        }
        char out[4096];
        out[0] = '\0';
        strncat(out, prev, m - prev);
        strcat(out, rep);
        strcat(out, m + plen);
        *out_expanded = strdup(out);
        return 1;
    }

    if (!strchr(line, '!')) return 0;

    size_t cap = strlen(line) * 2 + 1024;
    char *res = malloc(cap);
    res[0] = '\0';
    size_t res_len = 0;
    bool only_print = false;
    bool expansion_happened = false;

    bool in_s = false, in_d = false;
    size_t len = strlen(line);

    for (size_t i = 0; i < len; i++) {
        char c = line[i];
        if (c == '\'' && !in_d) {
            in_s = !in_s;
            res[res_len++] = c;
            res[res_len] = '\0';
            continue;
        }
        if (c == '"' && !in_s) {
            in_d = !in_d;
            res[res_len++] = c;
            res[res_len] = '\0';
            continue;
        }
        if (c == '\\' && i + 1 < len) {
            res[res_len++] = c;
            res[res_len++] = line[++i];
            res[res_len] = '\0';
            continue;
        }

        /* Ignore comments */
        if (!in_s && !in_d && c == '#' && (i == 0 || isspace((unsigned char)line[i - 1]))) {
            while (i < len) res[res_len++] = line[i++];
            break;
        }

        /* Ignore [! and ${! */
        if (c == '[' && line[i + 1] == '!') {
            res[res_len++] = c;
            res[res_len++] = line[++i];
            res[res_len] = '\0';
            continue;
        }
        if (c == '$' && line[i + 1] == '{' && line[i + 2] == '!') {
            res[res_len++] = c;
            res[res_len++] = line[++i];
            res[res_len++] = line[++i];
            res[res_len] = '\0';
            continue;
        }

        if (c == '!' && !in_s) {
            char next = line[i + 1];
            if (next == '\0' || isspace((unsigned char)next) || next == '=' || next == '(' || next == ')') {
                res[res_len++] = c;
                res[res_len] = '\0';
                continue;
            }

            /* History event designator */
            const char *event_line = NULL;
            size_t ev_start = i;
            i++;

            if (line[i] == '!') {
                /* !!: last command */
                if (state->history_count > 0) {
                    event_line = state->history_lines[state->history_count - 1];
                }
                i++;
            } else if (line[i] == '-' && isdigit((unsigned char)line[i + 1])) {
                /* !-N */
                i++;
                int n = 0;
                while (isdigit((unsigned char)line[i])) {
                    n = n * 10 + (line[i] - '0');
                    i++;
                }
                int idx = state->history_count - n;
                if (idx >= 0 && idx < state->history_count) {
                    event_line = state->history_lines[idx];
                }
            } else if (isdigit((unsigned char)line[i])) {
                /* !N */
                int n = 0;
                while (isdigit((unsigned char)line[i])) {
                    n = n * 10 + (line[i] - '0');
                    i++;
                }
                int idx = n - state->history_base;
                if (idx >= 0 && idx < state->history_count) {
                    event_line = state->history_lines[idx];
                }
            } else if (line[i] == '?') {
                /* !?string? or !?string */
                i++;
                char pat[128];
                size_t plen = 0;
                while (line[i] != '\0' && line[i] != '?' && line[i] != ':' && !isspace((unsigned char)line[i])) {
                    if (plen < sizeof(pat) - 1) pat[plen++] = line[i];
                    i++;
                }
                pat[plen] = '\0';
                if (line[i] == '?') i++;

                for (int k = state->history_count - 1; k >= 0; k--) {
                    if (strstr(state->history_lines[k], pat) != NULL) {
                        event_line = state->history_lines[k];
                        strncpy(state->hist_search_pat, pat, sizeof(state->hist_search_pat) - 1);
                        /* Find matched word */
                        char wbuf[1024];
                        strncpy(wbuf, event_line, sizeof(wbuf) - 1);
                        char *wtok = strtok(wbuf, " \t");
                        while (wtok) {
                            if (strstr(wtok, pat)) {
                                strncpy(state->hist_search_word, wtok, sizeof(state->hist_search_word) - 1);
                            }
                            wtok = strtok(NULL, " \t");
                        }
                        break;
                    }
                }
            } else if (line[i] == '#') {
                /* !# current line */
                event_line = res;
                i++;
            } else if (line[i] == '$' || line[i] == '*' || line[i] == '^' || line[i] == '-' || line[i] == ':') {
                /* Default to !! */
                if (state->history_count > 0) {
                    event_line = state->history_lines[state->history_count - 1];
                }
            } else if (isalpha((unsigned char)line[i]) || line[i] == '/' || line[i] == '.') {
                /* !string */
                char prefix[128];
                size_t plen = 0;
                while (line[i] != '\0' && line[i] != ':' && !isspace((unsigned char)line[i])) {
                    if (plen < sizeof(prefix) - 1) prefix[plen++] = line[i];
                    i++;
                }
                prefix[plen] = '\0';
                for (int k = state->history_count - 1; k >= 0; k--) {
                    if (strncmp(state->history_lines[k], prefix, plen) == 0) {
                        event_line = state->history_lines[k];
                        break;
                    }
                }
            }

            if (!event_line) {
                char err_tok[64];
                size_t elen = i - ev_start;
                if (elen >= sizeof(err_tok)) elen = sizeof(err_tok) - 1;
                strncpy(err_tok, line + ev_start, elen);
                err_tok[elen] = '\0';
                fprintf(stderr, "%s: line %d: history: %s: history expansion failed\n", src, lineno, err_tok);
                free(res);
                return -1;
            }

            /* Word splitting on event_line */
            char *words[128];
            int nwords = 0;
            char *ev_copy = strdup(event_line);
            bool w_in_s = false, w_in_d = false;
            char *w_curr = ev_copy;
            while (*w_curr && isspace((unsigned char)*w_curr)) w_curr++;
            while (*w_curr && nwords < 128) {
                words[nwords++] = w_curr;
                while (*w_curr) {
                    if (*w_curr == '\'' && !w_in_d) w_in_s = !w_in_s;
                    else if (*w_curr == '"' && !w_in_s) w_in_d = !w_in_d;
                    else if (!w_in_s && !w_in_d && isspace((unsigned char)*w_curr)) {
                        *w_curr++ = '\0';
                        break;
                    }
                    w_curr++;
                }
                while (*w_curr && isspace((unsigned char)*w_curr)) w_curr++;
            }

            /* Parse optional word designator */
            int w_start = 0;
            int w_end = nwords - 1;
            bool has_word_desig = false;

            if (line[i] == ':') {
                char nc = line[i + 1];
                if (isdigit((unsigned char)nc) || nc == '^' || nc == '$' || nc == '*' || nc == '-' || nc == '%') {
                    has_word_desig = true;
                    i++;
                }
            } else if (line[i] == '^' || line[i] == '$' || line[i] == '*' || line[i] == '-') {
                has_word_desig = true;
            }

            const char *pct_word = NULL;
            if (has_word_desig) {
                char wc = line[i];
                if (wc == '^') {
                    w_start = 1;
                    i++;
                    if (line[i] == '-') {
                        i++;
                        if (line[i] == '$') { w_end = nwords - 1; i++; }
                        else if (isdigit((unsigned char)line[i])) { w_end = line[i++] - '0'; }
                        else { w_end = nwords - 2; }
                    } else if (line[i] == '*') {
                        w_end = nwords - 1;
                        i++;
                    } else {
                        w_end = 1;
                    }
                } else if (wc == '$') {
                    w_start = nwords - 1;
                    w_end = nwords - 1;
                    i++;
                } else if (wc == '*') {
                    w_start = 1;
                    w_end = nwords - 1;
                    i++;
                } else if (wc == '%') {
                    pct_word = state->hist_search_word;
                    i++;
                } else if (wc == '-') {
                    w_start = 0;
                    i++;
                    if (line[i] == '$') { w_end = nwords - 1; i++; }
                    else if (isdigit((unsigned char)line[i])) {
                        w_end = line[i++] - '0';
                    } else {
                        w_end = nwords - 2;
                    }
                } else if (isdigit((unsigned char)wc)) {
                    w_start = wc - '0';
                    i++;
                    if (line[i] == '-') {
                        i++;
                        if (line[i] == '$') { w_end = nwords - 1; i++; }
                        else if (isdigit((unsigned char)line[i])) { w_end = line[i++] - '0'; }
                        else { w_end = nwords - 2; }
                    } else if (line[i] == '*') {
                        w_end = nwords - 1;
                        i++;
                    } else {
                        w_end = w_start;
                    }
                }
            }

            if (w_start < 0) w_start = 0;
            if (w_end >= nwords) w_end = nwords - 1;

            /* Assemble selected words into a single string */
            char sel_buf[4096];
            sel_buf[0] = '\0';
            if (pct_word) {
                strncpy(sel_buf, pct_word, sizeof(sel_buf) - 1);
            } else {
                for (int w = w_start; w <= w_end && w < nwords; w++) {
                    if (w > w_start) strcat(sel_buf, " ");
                    strcat(sel_buf, words[w]);
                }
            }
            free(ev_copy);

            /* Parse optional modifiers (:h, :t, :r, :e, :p, :q, :x, :s, :gs, :&, :g&) */
            char *mod_res = strdup(sel_buf);
            while (line[i] == ':') {
                size_t consumed = 0;
                char *new_mod = apply_modifier(state, mod_res, line + i, &consumed, &only_print);
                if (!new_mod) {
                    /* Modifier failed */
                    char err_tok[64];
                    size_t elen = i - ev_start + 2;
                    if (elen >= sizeof(err_tok)) elen = sizeof(err_tok) - 1;
                    strncpy(err_tok, line + ev_start, elen);
                    err_tok[elen] = '\0';
                    fprintf(stderr, "%s: line %d: history: %s: history expansion failed\n", src, lineno, err_tok);
                    free(mod_res);
                    free(res);
                    return -1;
                }
                free(mod_res);
                mod_res = new_mod;
                i += consumed;
            }
            i--; /* step back for outer loop increment */

            size_t mlen = strlen(mod_res);
            while (res_len + mlen + 16 >= cap) { cap *= 2; res = realloc(res, cap); }
            memcpy(res + res_len, mod_res, mlen);
            res_len += mlen;
            res[res_len] = '\0';
            free(mod_res);
            expansion_happened = true;
            continue;
        }

        res[res_len++] = c;
        res[res_len] = '\0';
    }

    if (!expansion_happened) {
        free(res);
        return 0;
    }

    *out_expanded = res;
    return only_print ? 2 : 1;
}
