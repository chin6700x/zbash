/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "autocomplete.h"
#include "flag_db.h"
#include "highlight.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

/* ========================================================================= */
/* 1. Sorted Prefix Index Table                                              */
/* ========================================================================= */

static char **s_sorted_cmds = NULL;
static size_t s_cmd_count = 0;
static size_t s_cmd_capacity = 0;
static bool s_prefix_initialized = false;

/* Standard Builtins List */
static const char *s_builtin_names[] = {
    "alias", "bg", "bind", "break", "builtin", "caller", "cd", "command",
    "compgen", "complete", "continue", "declare", "dirs", "disown", "echo",
    "enable", "eval", "exec", "exit", "export", "fc", "fg", "getopts",
    "hash", "help", "history", "jobs", "kill", "let", "local", "logout",
    "popd", "printf", "pushd", "pwd", "read", "readonly", "return", "set",
    "shift", "shopt", "source", "suspend", "test", "times", "trap", "type",
    "typeset", "ulimit", "umask", "unalias", "unset", "wait", "which",
    NULL
};

static int compare_strings(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

static void add_cmd_to_index(const char *name) {
    if (!name || !*name) return;

    if (s_cmd_count >= s_cmd_capacity) {
        size_t new_cap = s_cmd_capacity == 0 ? 512 : s_cmd_capacity * 2;
        char **new_arr = realloc(s_sorted_cmds, new_cap * sizeof(char *));
        if (!new_arr) return;
        s_sorted_cmds = new_arr;
        s_cmd_capacity = new_cap;
    }
    s_sorted_cmds[s_cmd_count++] = strdup(name);
}

void zbash_prefix_index_free(void) {
    if (s_sorted_cmds) {
        for (size_t i = 0; i < s_cmd_count; i++) {
            if (s_sorted_cmds[i]) free(s_sorted_cmds[i]);
        }
        free(s_sorted_cmds);
        s_sorted_cmds = NULL;
    }
    s_cmd_count = 0;
    s_cmd_capacity = 0;
    s_prefix_initialized = false;
}

void zbash_prefix_index_init(void) {
    if (s_prefix_initialized) return;

    /* 1. Add all builtins */
    for (size_t i = 0; s_builtin_names[i] != NULL; i++) {
        add_cmd_to_index(s_builtin_names[i]);
    }

    /* 2. Scan executables in $PATH */
    const char *path = getenv("PATH");
    if (!path) path = "/bin:/usr/bin:/usr/local/bin:/opt/homebrew/bin";

    char *path_copy = strdup(path);
    if (path_copy) {
        char *saveptr = NULL;
        char *dir = strtok_r(path_copy, ":", &saveptr);
        while (dir) {
            DIR *dp = opendir(dir);
            if (dp) {
                struct dirent *de;
                while ((de = readdir(dp)) != NULL) {
                    if (de->d_name[0] == '.') continue;
                    char fullpath[1024];
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, de->d_name);
                    if (zbash_check_exec(fullpath)) {
                        add_cmd_to_index(de->d_name);
                    }
                }
                closedir(dp);
            }
            dir = strtok_r(NULL, ":", &saveptr);
        }
        free(path_copy);
    }

    /* 3. Sort all command names */
    if (s_cmd_count > 1) {
        qsort(s_sorted_cmds, s_cmd_count, sizeof(char *), compare_strings);

        /* Deduplicate in-place */
        size_t unique = 0;
        for (size_t i = 1; i < s_cmd_count; i++) {
            if (strcmp(s_sorted_cmds[unique], s_sorted_cmds[i]) != 0) {
                unique++;
                s_sorted_cmds[unique] = s_sorted_cmds[i];
            } else {
                free(s_sorted_cmds[i]);
            }
        }
        s_cmd_count = unique + 1;
    }

    s_prefix_initialized = true;
}

/* Binary search lower bound for prefix in s_sorted_cmds */
static ssize_t find_first_prefix(const char *prefix, size_t plen) {
    if (s_cmd_count == 0) return -1;

    ssize_t low = 0;
    ssize_t high = s_cmd_count - 1;
    ssize_t first = -1;

    while (low <= high) {
        ssize_t mid = low + (high - low) / 2;
        int cmp = strncmp(s_sorted_cmds[mid], prefix, plen);
        if (cmp >= 0) {
            if (cmp == 0) first = mid;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
    return first;
}

/* ========================================================================= */
/* 2. Candidate List Memory Management                                       */
/* ========================================================================= */

void zbash_candidate_list_free(zbash_candidate_list_t *list) {
    if (!list) return;
    if (list->items) {
        free(list->items);
        list->items = NULL;
    }
    list->count = 0;
    list->capacity = 0;
    list->selected_index = 0;
}

static void add_candidate(zbash_candidate_list_t *list, const char *full_text, 
                          const char *suffix, const char *summary, zbash_completion_type_t type) {
    if (!list || !full_text) return;

    /* Check duplicates */
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].full_text, full_text) == 0) return;
    }

    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity == 0 ? 32 : list->capacity * 2;
        zbash_candidate_t *new_items = realloc(list->items, new_cap * sizeof(zbash_candidate_t));
        if (!new_items) return;
        list->items = new_items;
        list->capacity = new_cap;
    }

    zbash_candidate_t *c = &list->items[list->count++];
    strncpy(c->full_text, full_text, sizeof(c->full_text) - 1);
    c->full_text[sizeof(c->full_text) - 1] = '\0';

    if (suffix) {
        strncpy(c->display_suffix, suffix, sizeof(c->display_suffix) - 1);
        c->display_suffix[sizeof(c->display_suffix) - 1] = '\0';
    } else {
        c->display_suffix[0] = '\0';
    }

    if (summary) {
        strncpy(c->summary, summary, sizeof(c->summary) - 1);
        c->summary[sizeof(c->summary) - 1] = '\0';
    } else {
        c->summary[0] = '\0';
    }
    c->type = type;
}

/* ========================================================================= */
/* 3. Multi-Source Completions Collector                                     */
/* ========================================================================= */

/*
 * Tokenize active segment before cursor
 */
static void extract_active_token(const char *line, size_t cursor_pos, 
                                char *out_tok, size_t max_tok, 
                                size_t *out_start, size_t *out_len,
                                bool *out_is_cmd_pos, char *out_cmd_name, size_t max_cmd) {
    out_tok[0] = '\0';
    out_cmd_name[0] = '\0';
    *out_start = cursor_pos;
    *out_len = 0;
    *out_is_cmd_pos = true;

    if (!line || cursor_pos == 0) return;

    /* Find start of statement (|;&) */
    size_t stmt_start = 0;
    for (size_t i = cursor_pos; i > 0; i--) {
        char c = line[i - 1];
        if (c == '|' || c == ';' || c == '&') {
            stmt_start = i;
            break;
        }
    }

    /* Skip leading whitespace in statement */
    while (stmt_start < cursor_pos && isspace((unsigned char)line[stmt_start])) stmt_start++;

    /* Extract command name of this statement */
    size_t c_idx = 0;
    size_t s_idx = stmt_start;
    while (s_idx < cursor_pos && !isspace((unsigned char)line[s_idx]) && c_idx < max_cmd - 1) {
        out_cmd_name[c_idx++] = line[s_idx++];
    }
    out_cmd_name[c_idx] = '\0';

    /* Find word boundary immediately before cursor */
    size_t tok_start = cursor_pos;
    while (tok_start > stmt_start && !isspace((unsigned char)line[tok_start - 1])) {
        tok_start--;
    }

    size_t tok_len = cursor_pos - tok_start;
    if (tok_len >= max_tok) tok_len = max_tok - 1;
    memcpy(out_tok, line + tok_start, tok_len);
    out_tok[tok_len] = '\0';

    *out_start = tok_start;
    *out_len = tok_len;
    *out_is_cmd_pos = (tok_start == stmt_start);
}

size_t zbash_get_completions(zbash_state_t *state, const char *line, size_t len, 
                             size_t cursor_pos, zbash_candidate_list_t *out_list) {
    (void)len;
    if (!out_list) return 0;
    memset(out_list, 0, sizeof(zbash_candidate_list_t));

    if (!s_prefix_initialized) {
        zbash_prefix_index_init();
    }

    char token[128];
    char cmd_name[64];
    size_t tok_start = 0;
    size_t tok_len = 0;
    bool is_cmd_pos = true;

    extract_active_token(line, cursor_pos, token, sizeof(token), 
                         &tok_start, &tok_len, &is_cmd_pos, cmd_name, sizeof(cmd_name));

    out_list->replace_start = tok_start;
    out_list->replace_len = tok_len;

    /* --------------------------------------------------------------------- */
    /* SOURCE 1: Command Completion (Builtins, $PATH, Functions, Aliases)    */
    /* --------------------------------------------------------------------- */
    if (is_cmd_pos) {
        if (tok_len == 0) return 0;

        /* Check User-Defined Functions */
        if (state && state->func_count > 0) {
            for (int i = 0; i < state->func_count; i++) {
                if (state->funcs[i].name && strncmp(state->funcs[i].name, token, tok_len) == 0) {
                    add_candidate(out_list, state->funcs[i].name, state->funcs[i].name + tok_len, "shell function", COMPLETE_CMD_FUNC);
                }
            }
        }

        /* Check User-Defined Aliases */
        if (state && state->alias_count > 0) {
            for (int i = 0; i < state->alias_count; i++) {
                if (state->aliases[i].name && strncmp(state->aliases[i].name, token, tok_len) == 0) {
                    add_candidate(out_list, state->aliases[i].name, state->aliases[i].name + tok_len, "alias", COMPLETE_CMD_ALIAS);
                }
            }
        }

        /* Check Sorted Index (Builtins + $PATH) */
        ssize_t first = find_first_prefix(token, tok_len);
        if (first >= 0) {
            for (size_t i = (size_t)first; i < s_cmd_count; i++) {
                if (strncmp(s_sorted_cmds[i], token, tok_len) != 0) break;
                const char *cmd = s_sorted_cmds[i];
                const zbash_cmd_desc_t *desc = zbash_find_cmd_desc(cmd);
                const char *summary = desc ? desc->summary : "system executable";
                zbash_completion_type_t t = zbash_is_builtin(cmd) ? COMPLETE_CMD_BUILTIN : COMPLETE_CMD_PATH;
                add_candidate(out_list, cmd, cmd + tok_len, summary, t);
                if (out_list->count >= 64) break; /* Cap to top 64 */
            }
        }
        return out_list->count;
    }

    /* --------------------------------------------------------------------- */
    /* SOURCE 2: Flag & Option Completion (from Phase 2 flag_db)             */
    /* --------------------------------------------------------------------- */
    if (token[0] == '-' && cmd_name[0] != '\0') {
        const zbash_cmd_desc_t *desc = zbash_find_cmd_desc(cmd_name);
        if (desc && desc->flags) {
            for (size_t i = 0; desc->flags[i].description != NULL; i++) {
                const zbash_flag_entry_t *f = &desc->flags[i];
                /* Check long opt: e.g. "--create" */
                if (f->long_opt && strncmp(f->long_opt, token, tok_len) == 0) {
                    add_candidate(out_list, f->long_opt, f->long_opt + tok_len, f->description, COMPLETE_FLAG);
                }
                /* Check short opt: e.g. "-c" */
                if (f->short_opt && tok_len <= 2) {
                    char sopt[4];
                    snprintf(sopt, sizeof(sopt), "-%c", f->short_opt);
                    if (strncmp(sopt, token, tok_len) == 0) {
                        add_candidate(out_list, sopt, sopt + tok_len, f->description, COMPLETE_FLAG);
                    }
                }
            }
            if (out_list->count > 0) return out_list->count;
        }
    }

    /* --------------------------------------------------------------------- */
    /* SOURCE 3: File & Directory Path Completion (APFS Traversal)           */
    /* --------------------------------------------------------------------- */
    char dir_path[512] = ".";
    const char *file_prefix = token;

    char *last_slash = strrchr(token, '/');
    if (last_slash) {
        size_t dlen = last_slash - token;
        if (dlen == 0) {
            strcpy(dir_path, "/");
        } else {
            strncpy(dir_path, token, dlen);
            dir_path[dlen] = '\0';
        }
        file_prefix = last_slash + 1;
    }

    size_t fplen = strlen(file_prefix);
    DIR *dp = opendir(dir_path);
    if (dp) {
        struct dirent *de;
        while ((de = readdir(dp)) != NULL && out_list->count < 64) {
            if (de->d_name[0] == '.' && file_prefix[0] != '.') continue;
            if (strncmp(de->d_name, file_prefix, fplen) == 0) {
                char full_match[512];
                if (last_slash) {
                    snprintf(full_match, sizeof(full_match), "%.*s/%s", (int)(last_slash - token), token, de->d_name);
                } else {
                    snprintf(full_match, sizeof(full_match), "%s", de->d_name);
                }

                /* Check if directory */
                char check_path[1024];
                snprintf(check_path, sizeof(check_path), "%s/%s", dir_path, de->d_name);
                struct stat st;
                bool is_dir = (stat(check_path, &st) == 0 && S_ISDIR(st.st_mode));
                if (is_dir) {
                    strncat(full_match, "/", sizeof(full_match) - strlen(full_match) - 1);
                }

                const char *suffix = full_match + tok_len;
                add_candidate(out_list, full_match, suffix, is_dir ? "directory" : "file", is_dir ? COMPLETE_DIR : COMPLETE_FILE);
            }
        }
        closedir(dp);
    }

    return out_list->count;
}

/* ========================================================================= */
/* 4. Sub-50ns Inline Ghost Text Finder                                      */
/* ========================================================================= */

bool zbash_find_inline_ghost(zbash_state_t *state, const char *line, size_t len, 
                            size_t cursor_pos, zbash_candidate_t *out_ghost) {
    if (!line || len == 0 || cursor_pos == 0 || cursor_pos != len) return false;

    /* 1. History Recency Priority: check if full line matches recent history */
    if (state && state->history_count > 0) {
        int max_check = state->history_count > 50 ? state->history_count - 50 : 0;
        for (int i = state->history_count - 1; i >= max_check; i--) {
            const char *hline = state->history_lines[i];
            if (hline && strncmp(hline, line, len) == 0 && strlen(hline) > len) {
                strncpy(out_ghost->full_text, hline, sizeof(out_ghost->full_text) - 1);
                strncpy(out_ghost->display_suffix, hline + len, sizeof(out_ghost->display_suffix) - 1);
                out_ghost->type = COMPLETE_HISTORY;
                out_ghost->summary[0] = '\0';
                return true;
            }
        }
    }

    /* 2. Prefix Lookup */
    zbash_candidate_list_t list;
    size_t count = zbash_get_completions(state, line, len, cursor_pos, &list);
    if (count > 0 && list.items) {
        /* Choose the best candidate: exact or shortest */
        size_t best_idx = 0;
        size_t min_len = strlen(list.items[0].full_text);

        for (size_t i = 1; i < count; i++) {
            size_t clen = strlen(list.items[i].full_text);
            if (clen < min_len) {
                min_len = clen;
                best_idx = i;
            }
        }

        *out_ghost = list.items[best_idx];
        zbash_candidate_list_free(&list);
        return (out_ghost->display_suffix[0] != '\0');
    }

    zbash_candidate_list_free(&list);
    return false;
}

/* ========================================================================= */
/* 5. Monaco Visual Mini-Palette Menu (Tab Dropdown)                         */
/* ========================================================================= */

bool zbash_show_completion_menu(zbash_state_t *state, char *line_buf, size_t *line_len, 
                               size_t *cursor_pos, int prompt_visual_width) {
    zbash_candidate_list_t list;
    size_t count = zbash_get_completions(state, line_buf, *line_len, *cursor_pos, &list);
    if (count == 0) {
        /* No candidates -> terminal bell */
        printf("\a");
        fflush(stdout);
        zbash_candidate_list_free(&list);
        return false;
    }

    /* Case A: Exactly 1 candidate -> Complete immediately without menu! */
    if (count == 1) {
        const char *completed = list.items[0].full_text;
        size_t rep_start = list.replace_start;
        size_t rep_len = list.replace_len;
        size_t comp_len = strlen(completed);

        /* Splice completed text into line_buf */
        size_t tail_len = *line_len - (rep_start + rep_len);
        memmove(line_buf + rep_start + comp_len, line_buf + rep_start + rep_len, tail_len);
        memcpy(line_buf + rep_start, completed, comp_len);

        *line_len = rep_start + comp_len + tail_len;
        *cursor_pos = rep_start + comp_len;

        /* If directory or file, add space if full command */
        if (list.items[0].type != COMPLETE_DIR && *cursor_pos == *line_len && *line_len + 1 < 4096) {
            line_buf[*line_len] = ' ';
            (*line_len)++;
            (*cursor_pos)++;
        }
        line_buf[*line_len] = '\0';

        zbash_candidate_list_free(&list);
        return true;
    }

    /* Case B: Multiple candidates -> Interactive Visual Palette Menu */
    struct winsize ws;
    int cols = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 30) {
        cols = ws.ws_col;
    }

    size_t sel = 0;
    size_t max_visible = count > 5 ? 5 : count;

    while (1) {
        /* Render Palette Box beneath prompt */
        printf("\0337"); /* Save cursor */

        /* Move down, clear lines */
        printf("\n\r\033[2K");
        printf("\033[1;38;5;51m╭─ \033[38;5;220m💡 \033[1;38;5;48mSuggestions (%zu matches)\033[1;38;5;51m ", count);
        for (int i = 28; i < cols - 4; i++) fputs("─", stdout);
        printf("╮\033[0m\r\n\033[2K");

        /* Visible candidates */
        size_t start_idx = sel >= max_visible ? sel - max_visible + 1 : 0;
        for (size_t i = 0; i < max_visible; i++) {
            size_t c_idx = start_idx + i;
            if (c_idx < count) {
                const zbash_candidate_t *c = &list.items[c_idx];
                bool is_sel = (c_idx == sel);
                if (is_sel) {
                    printf("\033[1;38;5;51m│ \033[1;38;5;48m▸ \033[1;38;5;221m%-20.20s\033[0m \033[38;5;252m%-*.*s\033[1;38;5;51m│\033[0m\r\n\033[2K",
                           c->full_text, cols - 30 > 10 ? cols - 30 : 10, cols - 30 > 10 ? cols - 30 : 10, c->summary);
                } else {
                    printf("\033[1;38;5;51m│   \033[38;5;250m%-20.20s\033[0m \033[38;5;244m%-*.*s\033[1;38;5;51m│\033[0m\r\n\033[2K",
                           c->full_text, cols - 30 > 10 ? cols - 30 : 10, cols - 30 > 10 ? cols - 30 : 10, c->summary);
                }
            }
        }

        /* Footer */
        printf("\033[1;38;5;51m╰─ \033[38;5;244m[Tab/↓] Next · [Enter] Select · [Esc] Dismiss ");
        for (int i = 46; i < cols - 4; i++) fputs("─", stdout);
        printf("╯\033[0m");

        /* Restore cursor */
        printf("\0338");
        printf("\033[%dG", prompt_visual_width + (int)(*cursor_pos) + 1);
        fflush(stdout);

        /* Read key */
        char k = 0;
        if (read(STDIN_FILENO, &k, 1) <= 0) break;

        /* Tab or Down Arrow -> Next Candidate */
        if (k == '\t') {
            sel = (sel + 1) % count;
            continue;
        }

        /* Enter or Space -> Select Active Candidate */
        if (k == '\r' || k == '\n' || k == ' ') {
            const char *completed = list.items[sel].full_text;
            size_t rep_start = list.replace_start;
            size_t rep_len = list.replace_len;
            size_t comp_len = strlen(completed);

            size_t tail_len = *line_len - (rep_start + rep_len);
            memmove(line_buf + rep_start + comp_len, line_buf + rep_start + rep_len, tail_len);
            memcpy(line_buf + rep_start, completed, comp_len);

            *line_len = rep_start + comp_len + tail_len;
            *cursor_pos = rep_start + comp_len;

            if (list.items[sel].type != COMPLETE_DIR && *cursor_pos == *line_len && *line_len + 1 < 4096) {
                line_buf[*line_len] = ' ';
                (*line_len)++;
                (*cursor_pos)++;
            }
            line_buf[*line_len] = '\0';
            break;
        }

        /* Esc or Ctrl-C or 'q' -> Dismiss Menu */
        if (k == 27 || k == 3 || k == 'q') {
            break;
        }

        /* If any regular char, ungetc or dismiss to let line editor handle */
        break;
    }

    /* Clean up the menu rows */
    printf("\0337");
    for (size_t i = 0; i < max_visible + 2; i++) {
        printf("\n\r\033[2K");
    }
    printf("\0338");
    printf("\033[%dG", prompt_visual_width + (int)(*cursor_pos) + 1);
    fflush(stdout);

    zbash_candidate_list_free(&list);
    return true;
}
