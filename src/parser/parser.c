/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "parser_internal.h"

/* Parse pipeline: splits cmd string by '|' into zbash_pipeline_t */
static void parse_pipeline(zbash_state_t *state, const char *pipe_str, zbash_pipeline_t *pipeline) {
    pipeline->cmd_count = 0;
    size_t len = strlen(pipe_str);
    size_t start = 0;
    bool in_s = false;
    bool in_d = false;
    int brace_d = 0;
    int paren_d = 0;
    int cond_d = 0;

    for (size_t i = 0; i <= len; i++) {
        char c = (i < len) ? pipe_str[i] : '\0';
        if (c == '\\' && !in_s && i + 1 < len) { i++; continue; }
        if (c == '$' && i + 1 < len && pipe_str[i + 1] == '(' && !in_s) {
            int sdepth = 1;
            bool sub_s = false;
            bool sub_d = false;
            i += 2;
            while (i < len && sdepth > 0) {
                char sc = pipe_str[i];
                if (sc == '\\' && i + 1 < len) {
                    i += 2;
                    continue;
                }
                if (sc == '\'' && !sub_d) {
                    sub_s = !sub_s;
                } else if (sc == '"' && !sub_s) {
                    sub_d = !sub_d;
                } else if (!sub_s && !sub_d) {
                    if (sc == '$' && i + 1 < len && pipe_str[i + 1] == '(') {
                        sdepth++;
                        i += 2;
                        continue;
                    } else if (sc == '(') {
                        sdepth++;
                    } else if (sc == ')') {
                        sdepth--;
                        if (sdepth == 0) break;
                    }
                }
                i++;
            }
            continue;
        }
        if (c == '`' && !in_s) {
            i++;
            while (i < len && pipe_str[i] != '`') {
                if (pipe_str[i] == '\\' && i + 1 < len) i += 2;
                else i++;
            }
            continue;
        }
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;
        else if (c == '{' && !in_s && !in_d) brace_d++;
        else if (c == '}' && !in_s && !in_d) { if (brace_d > 0) brace_d--; }
        else if (c == '(' && !in_s && !in_d) paren_d++;
        else if (c == ')' && !in_s && !in_d) { if (paren_d > 0) paren_d--; }
        else if (!in_s && !in_d) {
            if (c == '[' && i + 1 < len && pipe_str[i + 1] == '[' && (i == 0 || isspace((unsigned char)pipe_str[i - 1]))) {
                cond_d++;
                i++;
                continue;
            } else if (c == ']' && i + 1 < len && pipe_str[i + 1] == ']' && (i + 2 == len || isspace((unsigned char)pipe_str[i + 2]) || pipe_str[i + 2] == '|' || pipe_str[i + 2] == ';')) {
                if (cond_d > 0) cond_d--;
                i++;
                continue;
            }
        }

        bool is_pipe = (!in_s && !in_d && brace_d == 0 && paren_d == 0 && cond_d == 0 && c == '|' &&
                        (i == 0 || pipe_str[i - 1] != '>') &&
                        (i + 1 >= len || pipe_str[i + 1] != '|') &&
                        (i == 0 || pipe_str[i - 1] != '|'));
        bool is_end = (c == '\0');
        if ((is_pipe || is_end) && (i > start || is_end)) {
            size_t slen = i - start;
            char *stage = malloc(slen + 1);
            if (stage) {
                memcpy(stage, pipe_str + start, slen);
                stage[slen] = '\0';

                /* Skip empty stage */
                size_t t = 0;
                while (stage[t] != '\0' && isspace((unsigned char)stage[t])) t++;
                if (stage[t] != '\0') {
                    if (pipeline->cmd_count < ZBASH_MAX_PIPELINE) {
                        zbash_parse_simple_cmd(state, stage, &pipeline->cmds[pipeline->cmd_count++]);
                    }
                }
                free(stage);
            }
            start = i + 1;
        }
    }
}

/* Splits line into pipelines separated by &&, ||, ;, \n */
int zbash_parse_line(zbash_state_t *state, const char *line, zbash_pipeline_t *pipelines, int max_pipelines) {
    int pipe_count = 0;
    size_t len = strlen(line);
    size_t start = 0;
    bool in_s = false;
    bool in_d = false;
    int brace_d = 0;
    int paren_d = 0;
    int cond_d = 0;

    for (size_t i = 0; i <= len; i++) {
        char c = (i < len) ? line[i] : '\0';
        if (c == '\\' && !in_s && i + 1 < len) { i++; continue; }
        if (c == '$' && i + 1 < len && line[i + 1] == '(' && !in_s) {
            int sdepth = 1;
            bool sub_s = false;
            bool sub_d = false;
            i += 2;
            while (i < len && sdepth > 0) {
                char sc = line[i];
                if (sc == '\\' && i + 1 < len) {
                    i += 2;
                    continue;
                }
                if (sc == '\'' && !sub_d) {
                    sub_s = !sub_s;
                } else if (sc == '"' && !sub_s) {
                    sub_d = !sub_d;
                } else if (!sub_s && !sub_d) {
                    if (sc == '$' && i + 1 < len && line[i + 1] == '(') {
                        sdepth++;
                        i += 2;
                        continue;
                    } else if (sc == '(') {
                        sdepth++;
                    } else if (sc == ')') {
                        sdepth--;
                        if (sdepth == 0) break;
                    }
                }
                i++;
            }
            continue;
        }
        if (c == '`' && !in_s) {
            i++;
            while (i < len && line[i] != '`') {
                if (line[i] == '\\' && i + 1 < len) i += 2;
                else i++;
            }
            continue;
        }
        if (c == '\'' && !in_d) in_s = !in_s;
        else if (c == '"' && !in_s) in_d = !in_d;
        else if (c == '{' && !in_s && !in_d) brace_d++;
        else if (c == '}' && !in_s && !in_d) { if (brace_d > 0) brace_d--; }
        else if (c == '(' && !in_s && !in_d) paren_d++;
        else if (c == ')' && !in_s && !in_d) { if (paren_d > 0) paren_d--; }
        else if (!in_s && !in_d) {
            if (c == '[' && i + 1 < len && line[i + 1] == '[' && (i == 0 || isspace((unsigned char)line[i - 1]))) {
                cond_d++;
                i++;
                continue;
            } else if (c == ']' && i + 1 < len && line[i + 1] == ']' && (i + 2 == len || isspace((unsigned char)line[i + 2]) || line[i + 2] == ';' || line[i + 2] == '&' || line[i + 2] == '|' || line[i + 2] == '\n')) {
                if (cond_d > 0) cond_d--;
                i++;
                continue;
            }
        }

        zbash_conn_t conn = CONN_NONE;
        size_t skip = 0;
        bool is_end = (c == '\0');

        if (!in_s && !in_d && brace_d == 0 && paren_d == 0 && cond_d == 0) {
            if (c == '&' && i + 1 < len && line[i + 1] == '&') {
                conn = CONN_AND;
                skip = 2;
            } else if (c == '|' && i + 1 < len && line[i + 1] == '|') {
                conn = CONN_OR;
                skip = 2;
            } else if (c == ';' || c == '\n' || (c == '#' && (i == 0 || isspace((unsigned char)line[i - 1])))) {
                conn = CONN_SEMI;
                skip = 1;
            }
        }

        if (conn != CONN_NONE || is_end) {
            size_t slen = i - start;
            char *segment = malloc(slen + 1);
            if (segment) {
                memcpy(segment, line + start, slen);
                segment[slen] = '\0';

                size_t t = 0;
                while (segment[t] != '\0' && isspace((unsigned char)segment[t])) t++;
                if (segment[t] != '\0') {
                    if (pipe_count < max_pipelines) {
                        parse_pipeline(state, segment, &pipelines[pipe_count]);
                        pipelines[pipe_count].next_conn = conn;
                        pipe_count++;
                    }
                }
                free(segment);
            }

            if (c == '#') {
                while (i < len && line[i] != '\n') i++;
                start = (i < len && line[i] == '\n') ? (i + 1) : i;
                continue;
            }

            i += (skip > 0 ? skip - 1 : 0);
            start = i + 1;
        }
    }

    return pipe_count;
}
