/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "core_internal.h"

/* ========================================================================= */
/* Recursive-Descent Arithmetic Evaluator ($((...)) and ((...)))              */
/* ========================================================================= */

static long long eval_expr(zbash_state_t *state, const char **p, bool active);
static long long eval_assign(zbash_state_t *state, const char **p, bool active);
static long long eval_ternary(zbash_state_t *state, const char **p, bool active);

static void skip_ws(const char **p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

static const char *get_script_name(zbash_state_t *state) {
    if (state && state->source_depth > 0) return state->source_stack[state->source_depth - 1];
    if (state && state->argv && state->argv[0]) return state->argv[0];
    return "zbash";
}

/* Retrieve variable or array element, evaluating recursively if expression */
static long long get_var_val(zbash_state_t *state, const char *vname, bool is_array, long long sub_idx, bool active) {
    if (!active) return 0;
    const char *val_str = NULL;
    if (is_array) {
        val_str = zbash_get_array_elem(state, vname, (int)sub_idx);
    } else {
        val_str = getenv(vname);
    }
    if (!val_str || !*val_str) return 0;

    char *end = NULL;
    long long num = strtoll(val_str, &end, 0);
    while (*end && isspace((unsigned char)*end)) end++;
    if (*end == '\0') {
        return num;
    }
    if (strcmp(vname, "_") == 0) return 0;
    return zbash_eval_arithmetic(state, val_str);
}

/* Set variable or array element */
static void set_var_val(zbash_state_t *state, const char *vname, bool is_array, long long sub_idx, long long val, bool active) {
    if (!active) return;
    char nbuf[32];
    snprintf(nbuf, sizeof(nbuf), "%lld", val);
    if (is_array) {
        zbash_set_array_elem(state, vname, (int)sub_idx, nbuf);
    } else {
        setenv(vname, nbuf, 1);
        zbash_assign_var(state, vname, nbuf, false);
    }
}

/* Parse number supporting base 2..64 */
static long long parse_number(zbash_state_t *state, const char **p, bool active) {
    skip_ws(p);
    const char *start = *p;
    char *end = NULL;
    long long first = strtoll(start, &end, 0);

    /* Check for base#number: 16#FF, 64#@, 2#1010 */
    if (*end == '#') {
        long long base = strtoll(start, &end, 10);
        *p = end + 1; /* Skip '#' */
        if (base < 2 || base > 64) {
            if (active) {
                fprintf(stderr, "%s: line %d: %lld: invalid arithmetic base\n",
                        get_script_name(state),
                        state->lineno > 0 ? state->lineno : 1, base);
                state->last_status = 1;
                state->expansion_error = true;
            }
            while (**p && !isspace((unsigned char)**p) && **p != ')' && **p != ';' && **p != ',') (*p)++;
            return 0;
        }
        skip_ws(p);
        const char *num_start = *p;
        long long res = 0;
        int digits_seen = 0;
        while (**p) {
            char c = **p;
            int digit = -1;
            if (base <= 36) {
                if (c >= '0' && c <= '9') digit = c - '0';
                else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
                else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
            } else {
                if (c >= '0' && c <= '9') digit = c - '0';
                else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
                else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 36;
                else if (c == '@') digit = 62;
                else if (c == '_') digit = 63;
            }
            if (digit < 0 || digit >= base) {
                if (isalnum((unsigned char)c) || c == '@' || c == '_') {
                    if (active) {
                        fprintf(stderr, "%s: line %d: %lld#%s: value too great for base\n",
                                get_script_name(state),
                                state->lineno > 0 ? state->lineno : 1, base, num_start);
                        state->last_status = 1;
                        state->expansion_error = true;
                    }
                    while (**p && !isspace((unsigned char)**p) && **p != ')' && **p != ';' && **p != ',') (*p)++;
                    return 0;
                }
                break;
            }
            res = res * base + digit;
            digits_seen++;
            (*p)++;
        }
        if (digits_seen == 0) {
            /* Missing number after base: treated as 0 in Bash */
            return 0;
        }
        return res;
    }

    *p = end;

    /* Check invalid postfix operator on constant: 4--, 4++, 4 --, 4 ++
     * Note: 4+++a is binary + followed by prefix ++a, so 3 consecutive signs are valid! */
    skip_ws(p);
    if ((**p == '+' && *(*p + 1) == '+' && *(*p + 2) != '+') ||
        (**p == '-' && *(*p + 1) == '-' && *(*p + 2) != '-')) {
        if (active) {
            fprintf(stderr, "%s: line %d: syntax error: operand expected\n",
                    get_script_name(state),
                    state->lineno > 0 ? state->lineno : 1);
            state->last_status = 1;
            state->expansion_error = true;
        }
        *p += 2;
        return 0;
    }

    return first;
}

static long long eval_primary(zbash_state_t *state, const char **p, bool active) {
    skip_ws(p);
    if (**p == '\0') return 0;

    /* ( expr ) */
    if (**p == '(') {
        (*p)++;
        long long val = eval_expr(state, p, active);
        skip_ws(p);
        if (**p == ')') (*p)++;
        return val;
    }

    /* Quoted sub-expression: "expr" or 'expr' */
    if (**p == '"' || **p == '\'') {
        char q = **p;
        (*p)++;
        const char *qstart = *p;
        while (**p && **p != q) (*p)++;
        size_t qlen = *p - qstart;
        if (**p == q) (*p)++;
        char qbuf[2048];
        if (qlen >= sizeof(qbuf)) qlen = sizeof(qbuf) - 1;
        strncpy(qbuf, qstart, qlen);
        qbuf[qlen] = '\0';
        const char *qp = qbuf;
        return eval_expr(state, &qp, active);
    }

    /* Prefix ++var or --var (only if followed by variable or array element) */
    if ((**p == '+' && *(*p + 1) == '+') || (**p == '-' && *(*p + 1) == '-')) {
        char op = **p;
        const char *look = *p + 2;
        while (*look && isspace((unsigned char)*look)) look++;
        if (isalpha((unsigned char)*look) || *look == '_') {
            *p = look;
            const char *vstart = *p;
            while (isalnum((unsigned char)**p) || **p == '_') (*p)++;
            char vname[128];
            size_t vlen = *p - vstart;
            if (vlen >= sizeof(vname)) vlen = sizeof(vname) - 1;
            strncpy(vname, vstart, vlen); vname[vlen] = '\0';

            bool is_array = false;
            long long sub_idx = 0;
            skip_ws(p);
            if (**p == '[') {
                is_array = true;
                (*p)++;
                sub_idx = eval_expr(state, p, active);
                skip_ws(p);
                if (**p == ']') (*p)++;
            }

            long long cur = get_var_val(state, vname, is_array, sub_idx, active);
            long long res = (op == '+') ? (cur + 1) : (cur - 1);
            set_var_val(state, vname, is_array, sub_idx, res, active);
            return res;
        }
    }

    /* Unary +, -, ~, ! */
    if (**p == '+') { (*p)++; return eval_primary(state, p, active); }
    if (**p == '-') { (*p)++; return -eval_primary(state, p, active); }
    if (**p == '~') { (*p)++; return ~eval_primary(state, p, active); }
    if (**p == '!') { (*p)++; return !eval_primary(state, p, active); }

    /* Number */
    if (isdigit((unsigned char)**p)) {
        return parse_number(state, p, active);
    }

    /* Variable (bare identifier) */
    if (isalpha((unsigned char)**p) || **p == '_') {
        const char *vstart = *p;
        while (isalnum((unsigned char)**p) || **p == '_') (*p)++;
        char vname[128];
        size_t vlen = *p - vstart;
        if (vlen >= sizeof(vname)) vlen = sizeof(vname) - 1;
        strncpy(vname, vstart, vlen); vname[vlen] = '\0';

        bool is_array = false;
        long long sub_idx = 0;
        skip_ws(p);
        if (**p == '[') {
            is_array = true;
            (*p)++;
            sub_idx = eval_expr(state, p, active);
            skip_ws(p);
            if (**p == ']') (*p)++;
        }

        long long cur = get_var_val(state, vname, is_array, sub_idx, active);

        skip_ws(p);
        /* Postfix var++ or var-- */
        if (**p == '+' && *(*p + 1) == '+') {
            *p += 2;
            set_var_val(state, vname, is_array, sub_idx, cur + 1, active);
            return cur;
        }
        if (**p == '-' && *(*p + 1) == '-') {
            *p += 2;
            set_var_val(state, vname, is_array, sub_idx, cur - 1, active);
            return cur;
        }
        return cur;
    }

    if (**p != '\0' && **p != ')' && **p != ',' && **p != ';') {
        if (active) {
            const char *expr_str = (state && state->current_arith_expr) ? state->current_arith_expr : *p;
            if (state && state->in_cond_cmd) {
                fprintf(stderr, "%s: line %d: [[: %s: syntax error: operand expected (error token is \"%s\")\n",
                        get_script_name(state),
                        state->lineno > 0 ? state->lineno : 1,
                        expr_str, *p);
            } else {
                fprintf(stderr, "%s: line %d: %s: syntax error: operand expected (error token is \"%s\")\n",
                        get_script_name(state),
                        state->lineno > 0 ? state->lineno : 1,
                        expr_str, *p);
            }
            state->last_status = 1;
            state->expansion_error = true;
        }
        while (**p && !isspace((unsigned char)**p) && **p != ')' && **p != ',' && **p != ';') (*p)++;
        return 0;
    }

    return 0;
}

/* Power ** (right-associative) */
static long long eval_power(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_primary(state, p, active);
    skip_ws(p);
    if (**p == '*' && *(*p + 1) == '*') {
        *p += 2;
        long long right = eval_power(state, p, active);
        if (right < 0) return 0;
        long long res = 1;
        for (long long k = 0; k < right; k++) res *= left;
        return res;
    }
    return left;
}

/* Multiplicative *, /, % */
static long long eval_mul_div(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_power(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '*' && *(*p + 1) != '*' && *(*p + 1) != '=') {
            (*p)++;
            long long right = eval_power(state, p, active);
            left *= right;
        } else if (**p == '/' && *(*p + 1) != '=') {
            (*p)++;
            long long right = eval_power(state, p, active);
            if (right == 0) {
                if (active) {
                    fprintf(stderr, "%s: line %d: division by 0\n",
                            get_script_name(state),
                            state->lineno > 0 ? state->lineno : 1);
                    state->last_status = 1;
                    state->expansion_error = true;
                }
                left = 0;
            } else {
                left /= right;
            }
        } else if (**p == '%' && *(*p + 1) != '=') {
            (*p)++;
            long long right = eval_power(state, p, active);
            if (right == 0) {
                if (active) {
                    fprintf(stderr, "%s: line %d: division by 0\n",
                            get_script_name(state),
                            state->lineno > 0 ? state->lineno : 1);
                    state->last_status = 1;
                    state->expansion_error = true;
                }
                left = 0;
            } else {
                left %= right;
            }
        } else {
            break;
        }
    }
    return left;
}

/* Additive +, - */
static long long eval_add_sub(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_mul_div(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '+' && *(*p + 1) != '=') {
            (*p)++;
            skip_ws(p);
            if (**p == '\0' || **p == ')') {
                if (active) {
                    const char *expr_str = (state && state->current_arith_expr) ? state->current_arith_expr : "";
                    if (state && state->in_cond_cmd) {
                        fprintf(stderr, "%s: line %d: [[: %s: syntax error: operand expected (error token is \"+\")\n",
                                get_script_name(state),
                                state->lineno > 0 ? state->lineno : 1,
                                expr_str);
                    } else {
                        fprintf(stderr, "%s: line %d: %s: syntax error: operand expected (error token is \"+\")\n",
                                get_script_name(state),
                                state->lineno > 0 ? state->lineno : 1,
                                expr_str);
                    }
                    state->last_status = 1;
                    state->expansion_error = true;
                }
                return left;
            }
            long long right = eval_mul_div(state, p, active);
            left += right;
        } else if (**p == '-' && *(*p + 1) != '=') {
            (*p)++;
            skip_ws(p);
            if (**p == '\0' || **p == ')') {
                if (active) {
                    const char *expr_str = (state && state->current_arith_expr) ? state->current_arith_expr : "";
                    if (state && state->in_cond_cmd) {
                        fprintf(stderr, "%s: line %d: [[: %s: syntax error: operand expected (error token is \"-\")\n",
                                get_script_name(state),
                                state->lineno > 0 ? state->lineno : 1,
                                expr_str);
                    } else {
                        fprintf(stderr, "%s: line %d: %s: syntax error: operand expected (error token is \"-\")\n",
                                get_script_name(state),
                                state->lineno > 0 ? state->lineno : 1,
                                expr_str);
                    }
                    state->last_status = 1;
                    state->expansion_error = true;
                }
                return left;
            }
            long long right = eval_mul_div(state, p, active);
            left -= right;
        } else {
            break;
        }
    }
    return left;
}

/* Shift <<, >> */
static long long eval_shift(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_add_sub(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '<' && *(*p + 1) == '<' && *(*p + 2) != '=') {
            *p += 2;
            long long right = eval_add_sub(state, p, active);
            left <<= right;
        } else if (**p == '>' && *(*p + 1) == '>' && *(*p + 2) != '=') {
            *p += 2;
            long long right = eval_add_sub(state, p, active);
            left >>= right;
        } else {
            break;
        }
    }
    return left;
}

/* Relational <, <=, >, >= */
static long long eval_relational(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_shift(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '<' && *(*p + 1) == '=') {
            *p += 2;
            long long right = eval_shift(state, p, active);
            left = (left <= right) ? 1 : 0;
        } else if (**p == '>' && *(*p + 1) == '=') {
            *p += 2;
            long long right = eval_shift(state, p, active);
            left = (left >= right) ? 1 : 0;
        } else if (**p == '<' && *(*p + 1) != '<') {
            (*p)++;
            long long right = eval_shift(state, p, active);
            left = (left < right) ? 1 : 0;
        } else if (**p == '>' && *(*p + 1) != '>') {
            (*p)++;
            long long right = eval_shift(state, p, active);
            left = (left > right) ? 1 : 0;
        } else {
            break;
        }
    }
    return left;
}

/* Equality ==, != */
static long long eval_equality(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_relational(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '=' && *(*p + 1) == '=') {
            *p += 2;
            long long right = eval_relational(state, p, active);
            left = (left == right) ? 1 : 0;
        } else if (**p == '!' && *(*p + 1) == '=') {
            *p += 2;
            long long right = eval_relational(state, p, active);
            left = (left != right) ? 1 : 0;
        } else {
            break;
        }
    }
    return left;
}

/* Bitwise AND & */
static long long eval_bit_and(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_equality(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '&' && *(*p + 1) != '&' && *(*p + 1) != '=') {
            (*p)++;
            long long right = eval_equality(state, p, active);
            left &= right;
        } else {
            break;
        }
    }
    return left;
}

/* Bitwise XOR ^ */
static long long eval_bit_xor(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_bit_and(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '^' && *(*p + 1) != '=') {
            (*p)++;
            long long right = eval_bit_and(state, p, active);
            left ^= right;
        } else {
            break;
        }
    }
    return left;
}

/* Bitwise OR | */
static long long eval_bit_or(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_bit_xor(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '|' && *(*p + 1) != '|' && *(*p + 1) != '=') {
            (*p)++;
            long long right = eval_bit_xor(state, p, active);
            left |= right;
        } else {
            break;
        }
    }
    return left;
}

/* Logical AND && (short-circuiting) */
static long long eval_log_and(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_bit_or(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '&' && *(*p + 1) == '&') {
            *p += 2;
            long long right = eval_bit_or(state, p, active && (left != 0));
            left = (left != 0 && right != 0) ? 1 : 0;
        } else {
            break;
        }
    }
    return left;
}

/* Logical OR || (short-circuiting) */
static long long eval_log_or(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_log_and(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == '|' && *(*p + 1) == '|') {
            *p += 2;
            long long right = eval_log_and(state, p, active && (left == 0));
            left = (left != 0 || right != 0) ? 1 : 0;
        } else {
            break;
        }
    }
    return left;
}

/* Ternary cond ? true_val : false_val (short-circuiting) */
static long long eval_ternary(zbash_state_t *state, const char **p, bool active) {
    long long left = eval_log_or(state, p, active);
    skip_ws(p);
    if (**p == '?') {
        (*p)++;
        skip_ws(p);
        if (**p == ':') {
            if (active) {
                fprintf(stderr, "%s: line %d: expression expected\n",
                        get_script_name(state),
                        state->lineno > 0 ? state->lineno : 1);
                state->last_status = 1;
                state->expansion_error = true;
            }
            return 0;
        }
        long long val_true = eval_expr(state, p, active && (left != 0));
        skip_ws(p);
        if (**p == ':') {
            (*p)++;
        } else {
            if (active) {
                fprintf(stderr, "%s: line %d: `:' expected for conditional expression\n",
                        get_script_name(state),
                        state->lineno > 0 ? state->lineno : 1);
                state->last_status = 1;
                state->expansion_error = true;
            }
            return 0;
        }
        skip_ws(p);
        if (**p == '\0' || **p == ')') {
            if (active) {
                fprintf(stderr, "%s: line %d: expression expected\n",
                        get_script_name(state),
                        state->lineno > 0 ? state->lineno : 1);
                state->last_status = 1;
                state->expansion_error = true;
            }
            return 0;
        }
        long long val_false = eval_ternary(state, p, active && (left == 0));
        return (left != 0) ? val_true : val_false;
    }
    return left;
}

/* Assignment =, +=, -=, *=, /=, %=, <<=, >>=, &=, ^=, |= */
static long long eval_assign(zbash_state_t *state, const char **p, bool active) {
    skip_ws(p);
    const char *save_p = *p;
    if (isalpha((unsigned char)**p) || **p == '_') {
        const char *vstart = *p;
        while (isalnum((unsigned char)**p) || **p == '_') (*p)++;
        char vname[128];
        size_t vlen = *p - vstart;
        if (vlen >= sizeof(vname)) vlen = sizeof(vname) - 1;
        strncpy(vname, vstart, vlen); vname[vlen] = '\0';

        bool is_array = false;
        long long sub_idx = 0;
        skip_ws(p);
        if (**p == '[') {
            is_array = true;
            (*p)++;
            sub_idx = eval_expr(state, p, active);
            skip_ws(p);
            if (**p == ']') (*p)++;
        }

        skip_ws(p);
        int assign_op = 0;
        if (**p == '=' && *(*p + 1) != '=') { assign_op = 1; (*p)++; }
        else if (**p == '+' && *(*p + 1) == '=') { assign_op = 2; *p += 2; }
        else if (**p == '-' && *(*p + 1) == '=') { assign_op = 3; *p += 2; }
        else if (**p == '*' && *(*p + 1) == '=') { assign_op = 4; *p += 2; }
        else if (**p == '/' && *(*p + 1) == '=') { assign_op = 5; *p += 2; }
        else if (**p == '%' && *(*p + 1) == '=') { assign_op = 6; *p += 2; }
        else if (**p == '<' && *(*p + 1) == '<' && *(*p + 2) == '=') { assign_op = 7; *p += 3; }
        else if (**p == '>' && *(*p + 1) == '>' && *(*p + 2) == '=') { assign_op = 8; *p += 3; }
        else if (**p == '&' && *(*p + 1) == '=') { assign_op = 9; *p += 2; }
        else if (**p == '^' && *(*p + 1) == '=') { assign_op = 10; *p += 2; }
        else if (**p == '|' && *(*p + 1) == '=') { assign_op = 11; *p += 2; }

        if (assign_op > 0) {
            long long right = eval_assign(state, p, active);
            if (!active) return 0;
            long long cur = (assign_op == 1) ? 0 : get_var_val(state, vname, is_array, sub_idx, active);
            long long res = 0;
            switch (assign_op) {
                case 1: res = right; break;
                case 2: res = cur + right; break;
                case 3: res = cur - right; break;
                case 4: res = cur * right; break;
                case 5: res = (right != 0) ? (cur / right) : 0; break;
                case 6: res = (right != 0) ? (cur % right) : 0; break;
                case 7: res = cur << right; break;
                case 8: res = cur >> right; break;
                case 9: res = cur & right; break;
                case 10: res = cur ^ right; break;
                case 11: res = cur | right; break;
            }
            set_var_val(state, vname, is_array, sub_idx, res, active);
            return res;
        }
    }

    *p = save_p;
    long long left = eval_ternary(state, p, active);
    skip_ws(p);
    if ((**p == '=' && *(*p + 1) != '=') ||
        (**p == '+' && *(*p + 1) == '=') ||
        (**p == '-' && *(*p + 1) == '=') ||
        (**p == '*' && *(*p + 1) == '=') ||
        (**p == '/' && *(*p + 1) == '=') ||
        (**p == '%' && *(*p + 1) == '=')) {
        if (active) {
            fprintf(stderr, "%s: line %d: attempted assignment to non-variable\n",
                    get_script_name(state),
                    state->lineno > 0 ? state->lineno : 1);
            state->last_status = 1;
            state->expansion_error = true;
        }
        while (**p && !isspace((unsigned char)**p) && **p != ')' && **p != ';' && **p != ',') (*p)++;
        return 0;
    }
    return left;
}

/* Comma operator expr1, expr2 */
static long long eval_expr(zbash_state_t *state, const char **p, bool active) {
    long long val = eval_assign(state, p, active);
    while (1) {
        skip_ws(p);
        if (**p == ',') {
            (*p)++;
            val = eval_assign(state, p, active);
        } else {
            break;
        }
    }
    return val;
}

long long zbash_eval_arithmetic(zbash_state_t *state, const char *expr) {
    if (!expr) return 0;
    while (*expr && isspace((unsigned char)*expr)) expr++;
    if (*expr == '\0') return 0;

    const char *saved_expr = state ? state->current_arith_expr : NULL;
    if (state) state->current_arith_expr = expr;

    char *expanded = strchr(expr, '$') ? zbash_expand_vars(state, expr) : NULL;
    const char *p = expanded ? expanded : expr;
    long long res = eval_expr(state, &p, true);
    skip_ws(&p);
    if (*p != '\0') {
        if (state && state->in_cond_cmd) {
            fprintf(stderr, "%s: line %d: [[: %s: syntax error in expression (error token is \"%s\")\n",
                    get_script_name(state),
                    state->lineno > 0 ? state->lineno : 1,
                    expr, p);
        } else {
            fprintf(stderr, "%s: line %d: %s: syntax error in expression (error token is \"%s\")\n",
                    get_script_name(state),
                    state->lineno > 0 ? state->lineno : 1,
                    expr, p);
        }
        state->last_status = 1;
        state->expansion_error = true;
    }
    if (expanded) free(expanded);
    if (state) state->current_arith_expr = saved_expr;
    return res;
}
