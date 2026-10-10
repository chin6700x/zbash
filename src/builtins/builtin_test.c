/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"
#include <limits.h>

/* ========================================================================= */
/* Helper Functions for POSIX test / [ and [[ ... ]]                         */
/* ========================================================================= */

static const char *get_script_name(zbash_state_t *state) {
    if (state && state->source_depth > 0) return state->source_stack[state->source_depth - 1];
    if (state && state->argv && state->argv[0]) return state->argv[0];
    return "zbash";
}

static int get_lineno(zbash_state_t *state) {
    return (state && state->lineno > 0) ? state->lineno : 1;
}

static bool is_valid_int(const char *s) {
    if (!s || !*s) return false;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '-' || *s == '+') s++;
    if (!*s || !isdigit((unsigned char)*s)) return false;
    while (*s && isdigit((unsigned char)*s)) s++;
    while (*s && isspace((unsigned char)*s)) s++;
    return (*s == '\0');
}

static bool eval_shell_option(zbash_state_t *state, const char *opt) {
    if (!state || !opt) return false;
    if (strcmp(opt, "errexit") == 0) return state->errexit;
    if (strcmp(opt, "xtrace") == 0) return state->xtrace;
    if (strcmp(opt, "noexec") == 0) return state->noexec;
    if (strcmp(opt, "verbose") == 0) return state->verbose;
    if (strcmp(opt, "posix") == 0) return state->posix;
    if (strcmp(opt, "nounset") == 0) return state->nounset;
    return false;
}

static bool is_unary_op(const char *op) {
    if (!op || op[0] != '-' || op[1] == '\0' || op[2] != '\0') return false;
    char c = op[1];
    return (c == 'z' || c == 'n' || c == 'e' || c == 'a' || c == 'b' || c == 'c' ||
            c == 'd' || c == 'f' || c == 'g' || c == 'h' || c == 'L' || c == 'k' ||
            c == 'p' || c == 'r' || c == 's' || c == 't' || c == 'u' || c == 'w' ||
            c == 'x' || c == 'O' || c == 'G' || c == 'S' || c == 'N' || c == 'o' ||
            c == 'v');
}

static int eval_unary_op(zbash_state_t *state, const char *op, const char *arg) {
    struct stat st;
    if (strcmp(op, "-z") == 0) return strlen(arg) == 0;
    if (strcmp(op, "-n") == 0) return strlen(arg) > 0;
    if (strcmp(op, "-e") == 0 || strcmp(op, "-a") == 0) return stat(arg, &st) == 0;
    if (strcmp(op, "-f") == 0) return stat(arg, &st) == 0 && S_ISREG(st.st_mode);
    if (strcmp(op, "-d") == 0) return stat(arg, &st) == 0 && S_ISDIR(st.st_mode);
    if (strcmp(op, "-b") == 0) return stat(arg, &st) == 0 && S_ISBLK(st.st_mode);
    if (strcmp(op, "-c") == 0) return stat(arg, &st) == 0 && S_ISCHR(st.st_mode);
    if (strcmp(op, "-p") == 0) return stat(arg, &st) == 0 && S_ISFIFO(st.st_mode);
    if (strcmp(op, "-S") == 0) return stat(arg, &st) == 0 && S_ISSOCK(st.st_mode);
    if (strcmp(op, "-L") == 0 || strcmp(op, "-h") == 0) return lstat(arg, &st) == 0 && S_ISLNK(st.st_mode);
    if (strcmp(op, "-s") == 0) return stat(arg, &st) == 0 && st.st_size > 0;
    if (strcmp(op, "-r") == 0) return access(arg, R_OK) == 0;
    if (strcmp(op, "-w") == 0) return access(arg, W_OK) == 0;
    if (strcmp(op, "-x") == 0) return access(arg, X_OK) == 0;
    if (strcmp(op, "-u") == 0) return stat(arg, &st) == 0 && (st.st_mode & S_ISUID);
    if (strcmp(op, "-g") == 0) return stat(arg, &st) == 0 && (st.st_mode & S_ISGID);
    if (strcmp(op, "-k") == 0) return stat(arg, &st) == 0 && (st.st_mode & S_ISVTX);
    if (strcmp(op, "-O") == 0) return stat(arg, &st) == 0 && st.st_uid == geteuid();
    if (strcmp(op, "-G") == 0) return stat(arg, &st) == 0 && st.st_gid == getegid();
    if (strcmp(op, "-N") == 0) return stat(arg, &st) == 0 && st.st_mtime >= st.st_atime;
    if (strcmp(op, "-t") == 0) {
        if (!is_valid_int(arg)) return 0;
        long long fd = atoll(arg);
        if (fd < 0 || fd > INT_MAX) return 0;
        return isatty((int)fd);
    }
    if (strcmp(op, "-v") == 0) return getenv(arg) != NULL;
    if (strcmp(op, "-o") == 0) return eval_shell_option(state, arg);
    return 0;
}

static bool is_binary_op(const char *op) {
    if (!op) return false;
    return (strcmp(op, "=") == 0 || strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
            strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
            strcmp(op, "-eq") == 0 || strcmp(op, "-ne") == 0 || strcmp(op, "-lt") == 0 ||
            strcmp(op, "-le") == 0 || strcmp(op, "-gt") == 0 || strcmp(op, "-ge") == 0 ||
            strcmp(op, "-nt") == 0 || strcmp(op, "-ot") == 0 || strcmp(op, "-ef") == 0);
}

static int eval_binary_op(zbash_state_t *state, const char *prog, const char *left, const char *op, const char *right) {
    if (strcmp(op, "=") == 0 || strcmp(op, "==") == 0) {
        return strcmp(left, right) == 0 ? 0 : 1;
    }
    if (strcmp(op, "!=") == 0) {
        return strcmp(left, right) != 0 ? 0 : 1;
    }
    if (strcmp(op, "<") == 0) {
        return strcmp(left, right) < 0 ? 0 : 1;
    }
    if (strcmp(op, ">") == 0) {
        return strcmp(left, right) > 0 ? 0 : 1;
    }
    if (strcmp(op, "-ef") == 0) {
        struct stat st1, st2;
        if (stat(left, &st1) == 0 && stat(right, &st2) == 0) {
            return (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino) ? 0 : 1;
        }
        return 1;
    }
    if (strcmp(op, "-nt") == 0) {
        struct stat st1, st2;
        if (stat(left, &st1) != 0) return 1;
        if (stat(right, &st2) != 0) return 0;
        return (st1.st_mtime > st2.st_mtime) ? 0 : 1;
    }
    if (strcmp(op, "-ot") == 0) {
        struct stat st1, st2;
        if (stat(right, &st2) != 0) return 1;
        if (stat(left, &st1) != 0) return 0;
        return (st1.st_mtime < st2.st_mtime) ? 0 : 1;
    }
    if (strcmp(op, "-eq") == 0 || strcmp(op, "-ne") == 0 ||
        strcmp(op, "-lt") == 0 || strcmp(op, "-le") == 0 ||
        strcmp(op, "-gt") == 0 || strcmp(op, "-ge") == 0) {
        if (!is_valid_int(left)) {
            fprintf(stderr, "%s: line %d: %s: %s: integer expression expected\n",
                    get_script_name(state), get_lineno(state), prog, left);
            return 2;
        }
        if (!is_valid_int(right)) {
            fprintf(stderr, "%s: line %d: %s: %s: integer expression expected\n",
                    get_script_name(state), get_lineno(state), prog, right);
            return 2;
        }
        long long l = atoll(left);
        long long r = atoll(right);
        bool res = false;
        if (strcmp(op, "-eq") == 0) res = (l == r);
        else if (strcmp(op, "-ne") == 0) res = (l != r);
        else if (strcmp(op, "-lt") == 0) res = (l < r);
        else if (strcmp(op, "-le") == 0) res = (l <= r);
        else if (strcmp(op, "-gt") == 0) res = (l > r);
        else if (strcmp(op, "-ge") == 0) res = (l >= r);
        return res ? 0 : 1;
    }
    return 1;
}

/* ========================================================================= */
/* POSIX test and [ ... ] Evaluator                                          */
/* ========================================================================= */

static int eval_test_expr(zbash_state_t *state, bool is_bracket, const char *prog, int argc, char **argv) {
    if (argc <= 0) return 1;

    if (argc == 1) {
        return (argv[0][0] != '\0') ? 0 : 1;
    }

    if (argc == 2) {
        if (strcmp(argv[0], "!") == 0) {
            return (argv[1][0] == '\0') ? 0 : 1;
        }
        if (is_unary_op(argv[0])) {
            return eval_unary_op(state, argv[0], argv[1]) ? 0 : 1;
        }
        fprintf(stderr, "%s: line %d: %s: %s: unary operator expected\n",
                get_script_name(state), get_lineno(state), prog, argv[0]);
        return 2;
    }

    if (argc == 3) {
        if (is_binary_op(argv[1])) {
            return eval_binary_op(state, prog, argv[0], argv[1], argv[2]);
        }
        if (strcmp(argv[0], "!") == 0) {
            int rc = eval_test_expr(state, is_bracket, prog, 2, argv + 1);
            if (rc == 2) return 2;
            return (rc == 0) ? 1 : 0;
        }
        if (strcmp(argv[0], "(") == 0 && strcmp(argv[2], ")") == 0) {
            return (argv[1][0] != '\0') ? 0 : 1;
        }
        if (strcmp(argv[0], "(") == 0) {
            fprintf(stderr, "%s: line %d: %s: `)' expected%s\n",
                    get_script_name(state), get_lineno(state), prog, is_bracket ? ", found ]" : "");
            return 2;
        }
        if (strcmp(argv[1], "-a") == 0) {
            int l = (argv[0][0] != '\0') ? 0 : 1;
            int r = (argv[2][0] != '\0') ? 0 : 1;
            return (l == 0 && r == 0) ? 0 : 1;
        }
        if (strcmp(argv[1], "-o") == 0) {
            int l = (argv[0][0] != '\0') ? 0 : 1;
            int r = (argv[2][0] != '\0') ? 0 : 1;
            return (l == 0 || r == 0) ? 0 : 1;
        }
        fprintf(stderr, "%s: line %d: %s: %s: binary operator expected\n",
                get_script_name(state), get_lineno(state), prog, argv[1]);
        return 2;
    }

    if (argc == 4) {
        if (strcmp(argv[0], "!") == 0) {
            int rc = eval_test_expr(state, is_bracket, prog, 3, argv + 1);
            if (rc == 2) return 2;
            return (rc == 0) ? 1 : 0;
        }
        if (strcmp(argv[0], "(") == 0 && strcmp(argv[3], ")") == 0) {
            return eval_test_expr(state, is_bracket, prog, 2, argv + 1);
        }
        if (strcmp(argv[0], "(") == 0) {
            fprintf(stderr, "%s: line %d: %s: `)' expected%s\n",
                    get_script_name(state), get_lineno(state), prog, is_bracket ? ", found ]" : "");
            return 2;
        }
    }

    /* Check for -o at depth 0 */
    int pdepth = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "(") == 0) pdepth++;
        else if (strcmp(argv[i], ")") == 0) { if (pdepth > 0) pdepth--; }
        else if (pdepth == 0 && strcmp(argv[i], "-o") == 0 && i > 0 && i < argc - 1) {
            int left_rc = eval_test_expr(state, is_bracket, prog, i, argv);
            if (left_rc == 2) return 2;
            int right_rc = eval_test_expr(state, is_bracket, prog, argc - i - 1, argv + i + 1);
            if (right_rc == 2) return 2;
            return (left_rc == 0 || right_rc == 0) ? 0 : 1;
        }
    }

    /* Check for -a at depth 0 */
    pdepth = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "(") == 0) pdepth++;
        else if (strcmp(argv[i], ")") == 0) { if (pdepth > 0) pdepth--; }
        else if (pdepth == 0 && strcmp(argv[i], "-a") == 0 && i > 0 && i < argc - 1) {
            int left_rc = eval_test_expr(state, is_bracket, prog, i, argv);
            if (left_rc == 2) return 2;
            int right_rc = eval_test_expr(state, is_bracket, prog, argc - i - 1, argv + i + 1);
            if (right_rc == 2) return 2;
            return (left_rc == 0 && right_rc == 0) ? 0 : 1;
        }
    }

    /* Check outer parentheses ( expr ) */
    if (argc >= 2 && strcmp(argv[0], "(") == 0 && strcmp(argv[argc - 1], ")") == 0) {
        int d = 0;
        bool wraps_all = true;
        for (int i = 0; i < argc - 1; i++) {
            if (strcmp(argv[i], "(") == 0) d++;
            else if (strcmp(argv[i], ")") == 0) {
                d--;
                if (d == 0) { wraps_all = false; break; }
            }
        }
        if (wraps_all) {
            return eval_test_expr(state, is_bracket, prog, argc - 2, argv + 1);
        }
    }

    /* Unary ! at start */
    if (strcmp(argv[0], "!") == 0 && argc > 1) {
        int rc = eval_test_expr(state, is_bracket, prog, argc - 1, argv + 1);
        if (rc == 2) return 2;
        return (rc == 0) ? 1 : 0;
    }

    fprintf(stderr, "%s: line %d: %s: too many arguments\n",
            get_script_name(state), get_lineno(state), prog);
    return 2;
}

int builtin_test(zbash_state_t *state, int argc, char **argv) {
    bool is_bracket = false;
    const char *prog = "test";

    if (strcmp(argv[0], "[") == 0) {
        is_bracket = true;
        prog = "[";
        if (argc < 2 || strcmp(argv[argc - 1], "]") != 0) {
            fprintf(stderr, "%s: line %d: [: missing `]'\n", get_script_name(state), get_lineno(state));
            return 2;
        }
        argc--; /* Omit trailing ']' */
    } else if (strcmp(argv[0], "[[") == 0) {
        if (argc < 2 || strcmp(argv[argc - 1], "]]") != 0) {
            fprintf(stderr, "%s: line %d: [[: missing `]]'\n", get_script_name(state), get_lineno(state));
            return 2;
        }
        /* Join arguments into a single expression string */
        char buf[4096] = {0};
        for (int i = 1; i < argc - 1; i++) {
            strncat(buf, argv[i], sizeof(buf) - strlen(buf) - 2);
            if (i < argc - 2) strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
        }
        return zbash_eval_cond(state, buf);
    }

    if (argc <= 1) return 1;
    return eval_test_expr(state, is_bracket, prog, argc - 1, argv + 1);
}

int builtin_let(zbash_state_t *state, int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "zbash: let: expression expected\n");
        return 1;
    }
    long long val = 0;
    for (int i = 1; i < argc; i++) {
        val = zbash_eval_arithmetic(state, argv[i]);
    }
    return (val != 0) ? 0 : 1;
}

/* ========================================================================= */
/* Bash [[ ... ]] Conditional Expression Evaluator                           */
/* ========================================================================= */

typedef enum {
    CTOK_WORD,
    CTOK_AND,     /* && */
    CTOK_OR,      /* || */
    CTOK_NOT,     /* ! */
    CTOK_LPAREN,  /* ( */
    CTOK_RPAREN,  /* ) */
    CTOK_EOF
} cond_tok_type_t;

typedef struct {
    cond_tok_type_t type;
    char text[1024];
    bool is_quoted;
} cond_tok_t;

static char *strip_cond_quotes(const char *s) {
    if (!s) return strdup("");
    size_t len = strlen(s);
    char *out = malloc(len + 1);
    size_t j = 0;
    bool in_s = false, in_d = false;
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\\' && !in_s && i + 1 < len) {
            out[j++] = s[++i];
        } else if (c == '\'' && !in_d) {
            in_s = !in_s;
        } else if (c == '"' && !in_s) {
            in_d = !in_d;
        } else {
            out[j++] = c;
        }
    }
    out[j] = '\0';
    return out;
}

static bool is_glob_meta(char c) {
    return (c == '*' || c == '?' || c == '[' || c == ']' || c == '\\' ||
            c == '+' || c == '@' || c == '!' || c == '(' || c == ')' || c == '|');
}

static char *cond_prepare_pattern(const char *s) {
    if (!s) return strdup("");
    size_t len = strlen(s);
    char *out = malloc(len * 2 + 1);
    if (!out) return strdup(s);
    size_t j = 0;
    bool in_s = false, in_d = false;

    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\\' && !in_s) {
            if (i + 1 < len) {
                char next = s[++i];
                if (in_d) {
                    if (next == '$' || next == '`' || next == '"' || next == '\\') {
                        if (is_glob_meta(next)) out[j++] = '\\';
                        out[j++] = next;
                    } else {
                        out[j++] = '\\';
                        out[j++] = '\\';
                        if (is_glob_meta(next)) out[j++] = '\\';
                        out[j++] = next;
                    }
                } else {
                    out[j++] = '\\';
                    out[j++] = next;
                }
            } else {
                out[j++] = '\\';
            }
        } else if (c == '\'' && !in_d) {
            in_s = !in_s;
        } else if (c == '"' && !in_s) {
            in_d = !in_d;
        } else {
            if ((in_s || in_d) && is_glob_meta(c)) {
                out[j++] = '\\';
            }
            out[j++] = c;
        }
    }
    out[j] = '\0';
    return out;
}

static bool cond_pattern_match(zbash_state_t *state, const char *val, const char *pat) {
    if (!val) val = "";
    if (!pat) pat = "";
    int flags = (state && state->extglob) ? ZBASH_FNM_EXTMATCH : 0;
    return (zbash_fnmatch(pat, val, flags) == 0);
}

static int tokenize_cond(const char *s, cond_tok_t *tokens, int max_toks) {
    int count = 0;
    while (*s && count < max_toks - 1) {
        while (*s && isspace((unsigned char)*s)) s++;
        if (!*s) break;

        if (s[0] == '&' && s[1] == '&') {
            tokens[count].type = CTOK_AND;
            strcpy(tokens[count].text, "&&");
            tokens[count++].is_quoted = false;
            s += 2;
            continue;
        }
        if (s[0] == '|' && s[1] == '|') {
            tokens[count].type = CTOK_OR;
            strcpy(tokens[count].text, "||");
            tokens[count++].is_quoted = false;
            s += 2;
            continue;
        }
        if (s[0] == '(' && (count == 0 || tokens[count - 1].type == CTOK_AND || tokens[count - 1].type == CTOK_OR || tokens[count - 1].type == CTOK_NOT || tokens[count - 1].type == CTOK_LPAREN)) {
            tokens[count].type = CTOK_LPAREN;
            strcpy(tokens[count].text, "(");
            tokens[count++].is_quoted = false;
            s++;
            continue;
        }
        if (s[0] == ')') {
            tokens[count].type = CTOK_RPAREN;
            strcpy(tokens[count].text, ")");
            tokens[count++].is_quoted = false;
            s++;
            continue;
        }
        bool prev_is_expr_start = (count == 0 ||
                                   tokens[count - 1].type == CTOK_AND ||
                                   tokens[count - 1].type == CTOK_OR ||
                                   tokens[count - 1].type == CTOK_NOT ||
                                   tokens[count - 1].type == CTOK_LPAREN);
        if (prev_is_expr_start && s[0] == '!' && (isspace((unsigned char)s[1]) || s[1] == '(' || s[1] == '\0')) {
            tokens[count].type = CTOK_NOT;
            strcpy(tokens[count].text, "!");
            tokens[count++].is_quoted = false;
            s++;
            continue;
        }

        /* Token is a word or operator */
        char buf[1024];
        size_t b_idx = 0;
        bool in_s = false, in_d = false;
        bool was_quoted = false;

        /* If word starts with '(', split off '(' only if at expr start */
        if (prev_is_expr_start && *s == '(' && (s[1] != '\0' && !isspace((unsigned char)s[1]))) {
            tokens[count].type = CTOK_LPAREN;
            strcpy(tokens[count].text, "(");
            tokens[count++].is_quoted = false;
            s++;
            continue;
        }

        int word_p_depth = 0;
        int word_b_depth = 0;
        while (*s && b_idx < sizeof(buf) - 1) {
            char c = *s;
            if (c == '\\' && !in_s && s[1] != '\0') {
                buf[b_idx++] = *s++;
                buf[b_idx++] = *s++;
                was_quoted = true;
                continue;
            }
            if (c == '\'' && !in_d) {
                in_s = !in_s;
                was_quoted = true;
                buf[b_idx++] = *s++;
                continue;
            }
            if (c == '"' && !in_s) {
                in_d = !in_d;
                was_quoted = true;
                buf[b_idx++] = *s++;
                continue;
            }
            if (!in_s && !in_d) {
                if (c == '[') {
                    word_b_depth++;
                } else if (c == ']' && word_b_depth > 0) {
                    word_b_depth--;
                } else if (word_b_depth == 0) {
                    if (c == '(') {
                        word_p_depth++;
                    } else if (c == ')') {
                        if (word_p_depth > 0) {
                            word_p_depth--;
                        } else {
                            break;
                        }
                    }
                }
                if (word_p_depth == 0 && word_b_depth == 0) {
                    if (isspace((unsigned char)c)) break;
                    if (c == '&' && s[1] == '&') break;
                    if (c == '|' && s[1] == '|') break;
                }
            }
            buf[b_idx++] = *s++;
        }
        buf[b_idx] = '\0';

        tokens[count].type = CTOK_WORD;
        strncpy(tokens[count].text, buf, sizeof(tokens[count].text) - 1);
        tokens[count].text[sizeof(tokens[count].text) - 1] = '\0';
        tokens[count].is_quoted = was_quoted;
        count++;
    }

    tokens[count].type = CTOK_EOF;
    tokens[count].text[0] = '\0';
    tokens[count].is_quoted = false;
    return count;
}

static int parse_cond_or(zbash_state_t *state, cond_tok_t *tokens, int *pos, bool eval);
static int parse_cond_and(zbash_state_t *state, cond_tok_t *tokens, int *pos, bool eval);
static int parse_cond_term(zbash_state_t *state, cond_tok_t *tokens, int *pos, bool eval);
static int parse_cond_primary(zbash_state_t *state, cond_tok_t *tokens, int *pos, bool eval);

static int parse_cond_or(zbash_state_t *state, cond_tok_t *tokens, int *pos, bool eval) {
    int left = parse_cond_and(state, tokens, pos, eval);
    while (tokens[*pos].type == CTOK_OR) {
        (*pos)++;
        bool next_eval = eval && (left != 0); /* short-circuit if left was true (0) */
        int right = parse_cond_and(state, tokens, pos, next_eval);
        if (eval) {
            left = (left == 0 || right == 0) ? 0 : 1;
        }
    }
    return left;
}

static int parse_cond_and(zbash_state_t *state, cond_tok_t *tokens, int *pos, bool eval) {
    int left = parse_cond_term(state, tokens, pos, eval);
    while (tokens[*pos].type == CTOK_AND) {
        (*pos)++;
        bool next_eval = eval && (left == 0); /* short-circuit if left was false (!= 0) */
        int right = parse_cond_term(state, tokens, pos, next_eval);
        if (eval) {
            left = (left == 0 && right == 0) ? 0 : 1;
        }
    }
    return left;
}

static int parse_cond_term(zbash_state_t *state, cond_tok_t *tokens, int *pos, bool eval) {
    if (tokens[*pos].type == CTOK_NOT) {
        (*pos)++;
        int res = parse_cond_term(state, tokens, pos, eval);
        return eval ? ((res == 0) ? 1 : 0) : 0;
    }
    if (tokens[*pos].type == CTOK_LPAREN) {
        (*pos)++;
        int res = parse_cond_or(state, tokens, pos, eval);
        if (tokens[*pos].type == CTOK_RPAREN) {
            (*pos)++;
        }
        return res;
    }
    return parse_cond_primary(state, tokens, pos, eval);
}

static int parse_cond_primary(zbash_state_t *state, cond_tok_t *tokens, int *pos, bool eval) {
    if (tokens[*pos].type == CTOK_EOF) return 1;

    /* Check for binary operator in next token */
    if (tokens[*pos].type == CTOK_WORD && tokens[*pos + 1].type == CTOK_WORD) {
        const char *op = tokens[*pos + 1].text;
        if (is_binary_op(op) || strcmp(op, "=~") == 0) {
            cond_tok_t lhs_tok = tokens[(*pos)++];
            cond_tok_t op_tok = tokens[(*pos)++];
            cond_tok_t rhs_tok = tokens[(*pos)++];

            if (!eval) return 0;

            char *lhs_exp = zbash_expand_vars(state, lhs_tok.text);
            char *rhs_exp = NULL;
            const char *r_raw = NULL;
            bool is_pattern_op = (strcmp(op_tok.text, "==") == 0 || strcmp(op_tok.text, "=") == 0 || strcmp(op_tok.text, "!=") == 0);
            if (is_pattern_op && strchr(rhs_tok.text, '$') == NULL) {
                r_raw = rhs_tok.text;
            } else {
                rhs_exp = zbash_expand_vars(state, rhs_tok.text);
                r_raw = rhs_exp ? rhs_exp : rhs_tok.text;
            }
            const char *l_raw = lhs_exp ? lhs_exp : lhs_tok.text;

            int res = 1;

            if (strcmp(op_tok.text, "==") == 0 || strcmp(op_tok.text, "=") == 0) {
                char *l_str = strip_cond_quotes(l_raw);
                char *pat_to_use = cond_prepare_pattern(r_raw);
                res = cond_pattern_match(state, l_str, pat_to_use) ? 0 : 1;
                free(l_str);
                free(pat_to_use);
            } else if (strcmp(op_tok.text, "!=") == 0) {
                char *l_str = strip_cond_quotes(l_raw);
                char *pat_to_use = cond_prepare_pattern(r_raw);
                res = cond_pattern_match(state, l_str, pat_to_use) ? 1 : 0;
                free(l_str);
                free(pat_to_use);
            } else if (strcmp(op_tok.text, "=~") == 0) {
                char *l_str = strip_cond_quotes(l_raw);
                char *r_str = strip_cond_quotes(r_raw);
                regex_t reg;
                if (regcomp(&reg, r_str, REG_EXTENDED | REG_NOSUB) == 0) {
                    res = (regexec(&reg, l_str, 0, NULL, 0) == 0) ? 0 : 1;
                    regfree(&reg);
                } else {
                    res = 1;
                }
                free(l_str);
                free(r_str);
            } else if (strcmp(op_tok.text, "<") == 0) {
                char *l_str = strip_cond_quotes(l_raw);
                char *r_str = strip_cond_quotes(r_raw);
                res = (strcmp(l_str, r_str) < 0) ? 0 : 1;
                free(l_str);
                free(r_str);
            } else if (strcmp(op_tok.text, ">") == 0) {
                char *l_str = strip_cond_quotes(l_raw);
                char *r_str = strip_cond_quotes(r_raw);
                res = (strcmp(l_str, r_str) > 0) ? 0 : 1;
                free(l_str);
                free(r_str);
            } else if (strcmp(op_tok.text, "-ef") == 0) {
                char *l_str = strip_cond_quotes(l_raw);
                char *r_str = strip_cond_quotes(r_raw);
                struct stat st1, st2;
                if (stat(l_str, &st1) == 0 && stat(r_str, &st2) == 0) {
                    res = (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino) ? 0 : 1;
                } else {
                    res = 1;
                }
                free(l_str);
                free(r_str);
            } else if (strcmp(op_tok.text, "-nt") == 0) {
                char *l_str = strip_cond_quotes(l_raw);
                char *r_str = strip_cond_quotes(r_raw);
                struct stat st1, st2;
                if (stat(l_str, &st1) != 0) res = 1;
                else if (stat(r_str, &st2) != 0) res = 0;
                else res = (st1.st_mtime > st2.st_mtime) ? 0 : 1;
                free(l_str);
                free(r_str);
            } else if (strcmp(op_tok.text, "-ot") == 0) {
                char *l_str = strip_cond_quotes(l_raw);
                char *r_str = strip_cond_quotes(r_raw);
                struct stat st1, st2;
                if (stat(r_str, &st2) != 0) res = 1;
                else if (stat(l_str, &st1) != 0) res = 0;
                else res = (st1.st_mtime < st2.st_mtime) ? 0 : 1;
                free(l_str);
                free(r_str);
            } else if (strcmp(op_tok.text, "-eq") == 0 || strcmp(op_tok.text, "-ne") == 0 ||
                       strcmp(op_tok.text, "-lt") == 0 || strcmp(op_tok.text, "-le") == 0 ||
                       strcmp(op_tok.text, "-gt") == 0 || strcmp(op_tok.text, "-ge") == 0) {
                char *l_str = strip_cond_quotes(l_raw);
                char *r_str = strip_cond_quotes(r_raw);
                long long l = zbash_eval_arithmetic(state, l_str);
                if (state->expansion_error) {
                    free(l_str); free(r_str);
                    if (lhs_exp) free(lhs_exp);
                    if (rhs_exp) free(rhs_exp);
                    return 1;
                }
                long long r = zbash_eval_arithmetic(state, r_str);
                if (state->expansion_error) {
                    free(l_str); free(r_str);
                    if (lhs_exp) free(lhs_exp);
                    if (rhs_exp) free(rhs_exp);
                    return 1;
                }
                if (strcmp(op_tok.text, "-eq") == 0) res = (l == r) ? 0 : 1;
                else if (strcmp(op_tok.text, "-ne") == 0) res = (l != r) ? 0 : 1;
                else if (strcmp(op_tok.text, "-lt") == 0) res = (l < r) ? 0 : 1;
                else if (strcmp(op_tok.text, "-le") == 0) res = (l <= r) ? 0 : 1;
                else if (strcmp(op_tok.text, "-gt") == 0) res = (l > r) ? 0 : 1;
                else if (strcmp(op_tok.text, "-ge") == 0) res = (l >= r) ? 0 : 1;
                free(l_str);
                free(r_str);
            }

            if (lhs_exp) free(lhs_exp);
            if (rhs_exp) free(rhs_exp);
            return res;
        }
    }

    /* Check for unary operator */
    if (tokens[*pos].type == CTOK_WORD && is_unary_op(tokens[*pos].text)) {
        cond_tok_t op_tok = tokens[(*pos)++];
        cond_tok_t arg_tok = tokens[(*pos)++];
        if (!eval) return 0;
        char *arg_exp = zbash_expand_vars(state, arg_tok.text);
        char *arg_str = strip_cond_quotes(arg_exp ? arg_exp : arg_tok.text);
        int res = eval_unary_op(state, op_tok.text, arg_str) ? 0 : 1;
        free(arg_str);
        if (arg_exp) free(arg_exp);
        return res;
    }

    /* Single word term (true if non-empty string) */
    if (tokens[*pos].type == CTOK_WORD) {
        cond_tok_t w_tok = tokens[(*pos)++];
        if (!eval) return 0;
        char *exp = zbash_expand_vars(state, w_tok.text);
        char *str = strip_cond_quotes(exp ? exp : w_tok.text);
        int res = (str && str[0] != '\0') ? 0 : 1;
        free(str);
        if (exp) free(exp);
        return res;
    }

    return 1;
}

int zbash_eval_cond(zbash_state_t *state, const char *expr_str) {
    if (!expr_str) return 1;
    cond_tok_t tokens[256];
    int tok_count = tokenize_cond(expr_str, tokens, 256);
    if (tok_count <= 0) return 1;

    bool saved_in_cond = state ? state->in_cond_cmd : false;
    if (state) state->in_cond_cmd = true;

    int pos = 0;
    int rc = parse_cond_or(state, tokens, &pos, true);

    if (state) state->in_cond_cmd = saved_in_cond;
    return rc;
}
