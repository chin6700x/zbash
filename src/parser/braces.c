/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "zbash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

typedef struct {
    char **items;
    size_t count;
    size_t cap;
} str_list_t;

static void str_list_init(str_list_t *list) {
    list->items = NULL;
    list->count = 0;
    list->cap = 0;
}

static void str_list_push(str_list_t *list, char *str) {
    if (list->count + 1 >= list->cap) {
        list->cap = (list->cap == 0) ? 16 : (list->cap * 2);
        list->items = realloc(list->items, list->cap * sizeof(char *));
    }
    list->items[list->count++] = str;
    list->items[list->count] = NULL;
}

void zbash_free_string_list(char **list) {
    if (!list) return;
    for (size_t i = 0; list[i]; i++) {
        free(list[i]);
    }
    free(list);
}

/* Check if inner content represents a valid sequence: start..end */
static bool is_valid_sequence(const char *inner, size_t in_len, long *start_num, long *end_num, char *start_ch, char *end_ch, bool *is_char_seq) {
    if (in_len < 4) return false; // Minimum sequence is "1..2" or "a..b"

    const char *dot = NULL;
    bool in_s = false, in_d = false;

    for (size_t i = 0; i + 1 < in_len; i++) {
        if (inner[i] == '\\' && i + 1 < in_len) { i++; continue; }
        if (inner[i] == '\'' && !in_d) in_s = !in_s;
        else if (inner[i] == '"' && !in_s) in_d = !in_d;
        else if (!in_s && !in_d && inner[i] == '.' && inner[i + 1] == '.') {
            dot = inner + i;
            break;
        }
    }
    if (!dot) return false;

    size_t len1 = dot - inner;
    const char *p2 = dot + 2;
    size_t len2 = (inner + in_len) - p2;
    if (len1 == 0 || len2 == 0) return false;

    /* Check if both are integers */
    bool p1_int = true;
    size_t idx1 = 0;
    if (inner[idx1] == '-' || inner[idx1] == '+') idx1++;
    if (idx1 == len1) p1_int = false;
    for (size_t i = idx1; i < len1; i++) {
        if (!isdigit((unsigned char)inner[i])) { p1_int = false; break; }
    }

    bool p2_int = true;
    size_t idx2 = 0;
    if (p2[idx2] == '-' || p2[idx2] == '+') idx2++;
    if (idx2 == len2) p2_int = false;
    for (size_t i = idx2; i < len2; i++) {
        if (!isdigit((unsigned char)p2[i])) { p2_int = false; break; }
    }

    if (p1_int && p2_int) {
        char buf1[64], buf2[64];
        if (len1 >= sizeof(buf1) || len2 >= sizeof(buf2)) return false;
        strncpy(buf1, inner, len1); buf1[len1] = '\0';
        strncpy(buf2, p2, len2); buf2[len2] = '\0';
        *start_num = strtol(buf1, NULL, 10);
        *end_num = strtol(buf2, NULL, 10);
        *is_char_seq = false;
        return true;
    }

    /* Check if both are single ASCII letters */
    if (len1 == 1 && len2 == 1 && isalpha((unsigned char)inner[0]) && isalpha((unsigned char)p2[0])) {
        *start_ch = inner[0];
        *end_ch = p2[0];
        *is_char_seq = true;
        return true;
    }

    return false;
}

/* Find first expandable brace '{...}' in word */
static bool find_expandable_brace(const char *word, size_t *out_start, size_t *out_end, int *out_comma_count,
                                  long *s_num, long *e_num, char *s_ch, char *e_ch, bool *is_char) {
    size_t len = strlen(word);
    size_t pos = 0;
    bool in_s = false, in_d = false;

    while (pos < len) {
        char c = word[pos];
        if (c == '\\' && pos + 1 < len) {
            pos += 2;
            continue;
        }
        if (c == '\'' && !in_d) { in_s = !in_s; pos++; continue; }
        if (c == '"' && !in_s) { in_d = !in_d; pos++; continue; }

        /* Skip command substitution */
        if (!in_s && c == '$' && pos + 1 < len && word[pos + 1] == '(') {
            pos += 2;
            int pd = 1;
            while (pos < len && pd > 0) {
                if (word[pos] == '\\' && pos + 1 < len) { pos += 2; continue; }
                if (word[pos] == '(') pd++;
                else if (word[pos] == ')') pd--;
                pos++;
            }
            continue;
        }
        if (!in_s && c == '`') {
            pos++;
            while (pos < len && word[pos] != '`') {
                if (word[pos] == '\\' && pos + 1 < len) pos += 2;
                else pos++;
            }
            if (pos < len && word[pos] == '`') pos++;
            continue;
        }

        if (c == '{' && !in_s && !in_d) {
            size_t b_start = pos;
            size_t j = b_start + 1;
            int depth = 0;
            bool sub_s = false, sub_d = false;
            int comma_count = 0;
            size_t match_end = 0;
            bool found_match = false;

            while (j < len) {
                char jc = word[j];
                if (jc == '\\' && j + 1 < len) {
                    j += 2;
                    continue;
                }
                if (jc == '\'' && !sub_d) { sub_s = !sub_s; j++; continue; }
                if (jc == '"' && !sub_s) { sub_d = !sub_d; j++; continue; }

                if (!sub_s && jc == '$' && j + 1 < len && word[j + 1] == '(') {
                    j += 2;
                    int pd = 1;
                    while (j < len && pd > 0) {
                        if (word[j] == '\\' && j + 1 < len) { j += 2; continue; }
                        if (word[j] == '(') pd++;
                        else if (word[j] == ')') pd--;
                        j++;
                    }
                    continue;
                }
                if (!sub_s && jc == '`') {
                    j++;
                    while (j < len && word[j] != '`') {
                        if (word[j] == '\\' && j + 1 < len) j += 2;
                        else j++;
                    }
                    if (j < len && word[j] == '`') j++;
                    continue;
                }

                if (!sub_s && !sub_d) {
                    if (jc == '{') {
                        depth++;
                    } else if (jc == '}') {
                        if (depth > 0) {
                            depth--;
                        } else {
                            match_end = j;
                            found_match = true;
                            break;
                        }
                    } else if (jc == ',' && depth == 0) {
                        comma_count++;
                    }
                }
                j++;
            }

            if (found_match) {
                size_t in_len = match_end - (b_start + 1);
                if (comma_count > 0 || is_valid_sequence(word + b_start + 1, in_len, s_num, e_num, s_ch, e_ch, is_char)) {
                    *out_start = b_start;
                    *out_end = match_end;
                    *out_comma_count = comma_count;
                    return true;
                }
            }
        }
        pos++;
    }
    return false;
}

/* Recursive core for brace expansion */
static void expand_braces_rec(const char *word, str_list_t *results) {
    size_t b_start = 0, b_end = 0;
    int comma_count = 0;
    long s_num = 0, e_num = 0;
    char s_ch = '\0', e_ch = '\0';
    bool is_char = false;

    if (!find_expandable_brace(word, &b_start, &b_end, &comma_count, &s_num, &e_num, &s_ch, &e_ch, &is_char)) {
        str_list_push(results, strdup(word));
        return;
    }

    /* Extract preamble */
    char *preamble = malloc(b_start + 1);
    strncpy(preamble, word, b_start);
    preamble[b_start] = '\0';

    /* Extract postscript */
    const char *postscript = word + b_end + 1;

    str_list_t items;
    str_list_init(&items);

    if (comma_count > 0) {
        /* Split on depth-0 commas */
        size_t cur_start = b_start + 1;
        int depth = 0;
        bool sub_s = false, sub_d = false;

        for (size_t k = b_start + 1; k <= b_end; k++) {
            char kc = (k < b_end) ? word[k] : ',';
            if (k < b_end) {
                if (kc == '\\' && k + 1 < b_end) { k++; continue; }
                if (kc == '\'' && !sub_d) { sub_s = !sub_s; continue; }
                if (kc == '"' && !sub_s) { sub_d = !sub_d; continue; }
                if (!sub_s && kc == '$' && k + 1 < b_end && word[k + 1] == '(') {
                    k += 2;
                    int pd = 1;
                    while (k < b_end && pd > 0) {
                        if (word[k] == '\\' && k + 1 < b_end) { k += 2; continue; }
                        if (word[k] == '(') pd++;
                        else if (word[k] == ')') pd--;
                        k++;
                    }
                    k--;
                    continue;
                }
                if (!sub_s && kc == '`') {
                    k++;
                    while (k < b_end && word[k] != '`') {
                        if (word[k] == '\\' && k + 1 < b_end) k += 2;
                        else k++;
                    }
                    continue;
                }
                if (!sub_s && !sub_d) {
                    if (kc == '{') depth++;
                    else if (kc == '}') { if (depth > 0) depth--; }
                }
            }
            if ((k == b_end || kc == ',') && depth == 0 && !sub_s && !sub_d) {
                size_t item_len = k - cur_start;
                char *item = malloc(item_len + 1);
                strncpy(item, word + cur_start, item_len);
                item[item_len] = '\0';
                str_list_push(&items, item);
                cur_start = k + 1;
            }
        }
    } else {
        /* Sequence expansion */
        if (is_char) {
            if (s_ch <= e_ch) {
                for (int c = (int)s_ch; c <= (int)e_ch; c++) {
                    char s[2] = { (char)c, '\0' };
                    str_list_push(&items, strdup(s));
                }
            } else {
                for (int c = (int)s_ch; c >= (int)e_ch; c--) {
                    char s[2] = { (char)c, '\0' };
                    str_list_push(&items, strdup(s));
                }
            }
        } else {
            if (s_num <= e_num) {
                for (long n = s_num; n <= e_num; n++) {
                    char s[64];
                    snprintf(s, sizeof(s), "%ld", n);
                    str_list_push(&items, strdup(s));
                }
            } else {
                for (long n = s_num; n >= e_num; n--) {
                    char s[64];
                    snprintf(s, sizeof(s), "%ld", n);
                    str_list_push(&items, strdup(s));
                }
            }
        }
    }

    /* For each item, concatenate preamble + item + postscript and recurse */
    for (size_t i = 0; i < items.count; i++) {
        size_t nlen = strlen(preamble) + strlen(items.items[i]) + strlen(postscript);
        char *comb = malloc(nlen + 1);
        snprintf(comb, nlen + 1, "%s%s%s", preamble, items.items[i], postscript);

        expand_braces_rec(comb, results);
        free(comb);
    }

    free(preamble);
    zbash_free_string_list(items.items);
}

/* Public entry point: returns a NULL-terminated array of strings */
char **zbash_expand_braces(const char *word) {
    if (!word) return NULL;

    str_list_t results;
    str_list_init(&results);

    expand_braces_rec(word, &results);

    if (results.count == 0) {
        str_list_push(&results, strdup(""));
    }

    return results.items;
}
