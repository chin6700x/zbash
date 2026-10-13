/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "highlight.h"
#include "prompt.h"
#include "flag_db.h"
#include "hover_box.h"
#include "autocomplete.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

/* ========================================================================= */
/* 1. Fast O(1) PATH Cache                                                  */
/* ========================================================================= */

#define PATH_HASH_BUCKETS 8192

typedef struct path_node {
    char *name;
    struct path_node *next;
} path_node_t;

static path_node_t *s_path_table[PATH_HASH_BUCKETS];
static bool s_path_cache_initialized = false;
static char *s_cached_path_env = NULL;

static inline uint32_t hash_str(const char *str) {
    uint32_t h = 5381;
    unsigned char c;
    while ((c = (unsigned char)*str++)) {
        h = ((h << 5) + h) + c; /* h * 33 + c */
    }
    return h % PATH_HASH_BUCKETS;
}

void zbash_path_cache_free(void) {
    for (int i = 0; i < PATH_HASH_BUCKETS; i++) {
        path_node_t *curr = s_path_table[i];
        while (curr) {
            path_node_t *tmp = curr->next;
            if (curr->name) free(curr->name);
            free(curr);
            curr = tmp;
        }
        s_path_table[i] = NULL;
    }
    if (s_cached_path_env) {
        free(s_cached_path_env);
        s_cached_path_env = NULL;
    }
    s_path_cache_initialized = false;
}

static void add_to_path_cache(const char *name) {
    if (!name || !*name) return;
    uint32_t bucket = hash_str(name);
    path_node_t *curr = s_path_table[bucket];
    while (curr) {
        if (strcmp(curr->name, name) == 0) return;
        curr = curr->next;
    }
    path_node_t *node = malloc(sizeof(path_node_t));
    if (node) {
        node->name = strdup(name);
        node->next = s_path_table[bucket];
        s_path_table[bucket] = node;
    }
}

void zbash_path_cache_init(void) {
    if (s_path_cache_initialized) return;

    const char *path = getenv("PATH");
    if (!path) path = "/bin:/usr/bin:/usr/local/bin:/opt/homebrew/bin";

    s_cached_path_env = strdup(path);

    char *path_copy = strdup(path);
    if (!path_copy) return;

    char *saveptr = NULL;
    char *dir = strtok_r(path_copy, ":", &saveptr);
    while (dir) {
        DIR *dp = opendir(dir);
        if (dp) {
            struct dirent *de;
            while ((de = readdir(dp)) != NULL) {
                if (de->d_name[0] == '.') continue;
                /* Check if executable */
                char fullpath[1024];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, de->d_name);
                if (zbash_check_exec(fullpath)) {
                    add_to_path_cache(de->d_name);
                }
            }
            closedir(dp);
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }
    free(path_copy);
    s_path_cache_initialized = true;
}

void zbash_path_cache_refresh(void) {
    zbash_path_cache_free();
    zbash_path_cache_init();
}

bool zbash_path_cache_lookup(const char *cmd_name) {
    if (!cmd_name || !*cmd_name) return false;

    /* If contains slash, check file system directly */
    if (strchr(cmd_name, '/')) {
        return zbash_check_exec(cmd_name);
    }

    if (!s_path_cache_initialized) {
        zbash_path_cache_init();
    }

    uint32_t bucket = hash_str(cmd_name);
    path_node_t *curr = s_path_table[bucket];
    while (curr) {
        if (strcmp(curr->name, cmd_name) == 0) return true;
        curr = curr->next;
    }
    return false;
}

/* ========================================================================= */
/* 2. Monaco-Grade Highlighting Engine                                      */
/* ========================================================================= */

#define COLOR_RESET        "\033[0m"
#define COLOR_CMD_VALID    "\033[1;38;5;48m"   /* Neon Emerald Green (Bold) */
#define COLOR_CMD_INVALID  "\033[4;38;5;196m"  /* Vivid Ruby Red (Underline) */
#define COLOR_BUILTIN      "\033[1;38;5;51m"   /* Electric Aqua / Cyan */
#define COLOR_FUNC         "\033[38;5;81m"     /* Ice Blue */
#define COLOR_FLAG         "\033[38;5;221m"    /* Warm Amber Yellow */
#define COLOR_STRING       "\033[38;5;214m"    /* Warm Gold/Orange */
#define COLOR_UNCLOSED     "\033[4;38;5;244m"  /* Dim Titanium Gray (Alert) */
#define COLOR_OPERATOR     "\033[1;38;5;201m"  /* Vivid Neon Magenta */
#define COLOR_VARIABLE     "\033[1;38;5;75m"   /* Sky Blue */
#define COLOR_COMMENT      "\033[38;5;242m"    /* Subtle Titanium Gray */
#define COLOR_BRACKET      "\033[38;5;228m"    /* Light Gold */

static inline bool is_operator_char(char c) {
    return (c == '|' || c == '&' || c == ';' || c == '<' || c == '>' || c == '(' || c == ')');
}

static void buf_append(char *dst, size_t *pos, size_t max_sz, const char *src) {
    if (!src) return;
    size_t slen = strlen(src);
    if (*pos + slen < max_sz) {
        memcpy(dst + *pos, src, slen);
        *pos += slen;
        dst[*pos] = '\0';
    }
}

static void buf_append_c(char *dst, size_t *pos, size_t max_sz, char c) {
    if (*pos + 1 < max_sz) {
        dst[(*pos)++] = c;
        dst[*pos] = '\0';
    }
}

static bool check_cmd_valid(zbash_state_t *state, const char *token) {
    if (!token || !*token) return false;
    if (zbash_is_builtin(token)) return true;
    if (state && zbash_find_func(state, token)) return true;
    if (zbash_path_cache_lookup(token)) return true;
    return false;
}

char *zbash_highlight_line(zbash_state_t *state, const char *line, size_t len, size_t cursor_pos) {
    (void)cursor_pos;
    if (!line) return strdup("");

    size_t max_out = len * 16 + 256;
    char *out = malloc(max_out);
    if (!out) return strdup(line);

    size_t opos = 0;
    out[0] = '\0';

    bool is_cmd_pos = true;
    size_t i = 0;

    while (i < len) {
        char c = line[i];

        /* 1. Skip Whitespace */
        if (c == ' ' || c == '\t') {
            buf_append_c(out, &opos, max_out, c);
            i++;
            continue;
        }

        /* 2. Comments: '#' starts comment if at boundary */
        if (c == '#' && (i == 0 || isspace((unsigned char)line[i - 1]))) {
            buf_append(out, &opos, max_out, COLOR_COMMENT);
            while (i < len) {
                buf_append_c(out, &opos, max_out, line[i++]);
            }
            buf_append(out, &opos, max_out, COLOR_RESET);
            break;
        }

        /* 3. Operators */
        if (is_operator_char(c)) {
            buf_append(out, &opos, max_out, COLOR_OPERATOR);
            while (i < len && is_operator_char(line[i])) {
                buf_append_c(out, &opos, max_out, line[i++]);
            }
            buf_append(out, &opos, max_out, COLOR_RESET);
            is_cmd_pos = true; /* After operator, next word is a command */
            continue;
        }

        /* 4. Single Quotes: '...' */
        if (c == '\'') {
            size_t qstart = i;
            i++;
            bool closed = false;
            while (i < len) {
                if (line[i] == '\'') {
                    closed = true;
                    i++;
                    break;
                }
                i++;
            }
            if (closed) {
                buf_append(out, &opos, max_out, COLOR_STRING);
                for (size_t k = qstart; k < i; k++) {
                    buf_append_c(out, &opos, max_out, line[k]);
                }
                buf_append(out, &opos, max_out, COLOR_RESET);
            } else {
                buf_append(out, &opos, max_out, COLOR_UNCLOSED);
                for (size_t k = qstart; k < i; k++) {
                    buf_append_c(out, &opos, max_out, line[k]);
                }
                buf_append(out, &opos, max_out, COLOR_RESET);
            }
            is_cmd_pos = false;
            continue;
        }

        /* 5. Double Quotes: "..." */
        if (c == '"') {
            size_t qstart = i;
            i++;
            bool closed = false;
            while (i < len) {
                if (line[i] == '\\' && i + 1 < len) {
                    i += 2;
                    continue;
                }
                if (line[i] == '"') {
                    closed = true;
                    i++;
                    break;
                }
                i++;
            }
            if (closed) {
                buf_append(out, &opos, max_out, COLOR_STRING);
                for (size_t k = qstart; k < i; k++) {
                    buf_append_c(out, &opos, max_out, line[k]);
                }
                buf_append(out, &opos, max_out, COLOR_RESET);
            } else {
                buf_append(out, &opos, max_out, COLOR_UNCLOSED);
                for (size_t k = qstart; k < i; k++) {
                    buf_append_c(out, &opos, max_out, line[k]);
                }
                buf_append(out, &opos, max_out, COLOR_RESET);
            }
            is_cmd_pos = false;
            continue;
        }

        /* 6. Variables: $VAR or ${VAR} */
        if (c == '$') {
            buf_append(out, &opos, max_out, COLOR_VARIABLE);
            buf_append_c(out, &opos, max_out, line[i++]);
            if (i < len && line[i] == '{') {
                buf_append_c(out, &opos, max_out, line[i++]);
                while (i < len && line[i] != '}') {
                    buf_append_c(out, &opos, max_out, line[i++]);
                }
                if (i < len && line[i] == '}') {
                    buf_append_c(out, &opos, max_out, line[i++]);
                }
            } else {
                while (i < len && (isalnum((unsigned char)line[i]) || line[i] == '_' || line[i] == '?' || line[i] == '!' || line[i] == '#' || line[i] == '$' || line[i] == '@' || line[i] == '*')) {
                    buf_append_c(out, &opos, max_out, line[i++]);
                }
            }
            buf_append(out, &opos, max_out, COLOR_RESET);
            is_cmd_pos = false;
            continue;
        }

        /* 7. Regular Word (Command, Flag, or Argument) */
        size_t wstart = i;
        while (i < len && !isspace((unsigned char)line[i]) && !is_operator_char(line[i]) && line[i] != '\'' && line[i] != '"') {
            i++;
        }
        size_t wlen = i - wstart;
        char word[256];
        if (wlen >= sizeof(word)) wlen = sizeof(word) - 1;
        memcpy(word, line + wstart, wlen);
        word[wlen] = '\0';

        /* Check for variable assignment: e.g. FOO=bar */
        char *eq = strchr(word, '=');
        if (is_cmd_pos && eq && eq != word) {
            /* Assignment prefix: highlight name in variable color */
            buf_append(out, &opos, max_out, COLOR_VARIABLE);
            for (char *p = word; p < eq; p++) buf_append_c(out, &opos, max_out, *p);
            buf_append(out, &opos, max_out, COLOR_RESET);
            buf_append_c(out, &opos, max_out, '=');
            buf_append(out, &opos, max_out, COLOR_STRING);
            buf_append(out, &opos, max_out, eq + 1);
            buf_append(out, &opos, max_out, COLOR_RESET);
            /* Next word can still be command */
            continue;
        }

        if (is_cmd_pos) {
            if (zbash_is_builtin(word)) {
                buf_append(out, &opos, max_out, COLOR_BUILTIN);
                buf_append(out, &opos, max_out, word);
                buf_append(out, &opos, max_out, COLOR_RESET);
            } else if (state && zbash_find_func(state, word)) {
                buf_append(out, &opos, max_out, COLOR_FUNC);
                buf_append(out, &opos, max_out, word);
                buf_append(out, &opos, max_out, COLOR_RESET);
            } else if (check_cmd_valid(state, word)) {
                buf_append(out, &opos, max_out, COLOR_CMD_VALID);
                buf_append(out, &opos, max_out, word);
                buf_append(out, &opos, max_out, COLOR_RESET);
            } else {
                buf_append(out, &opos, max_out, COLOR_CMD_INVALID);
                buf_append(out, &opos, max_out, word);
                buf_append(out, &opos, max_out, COLOR_RESET);
            }
            is_cmd_pos = false;
        } else if (word[0] == '-') {
            /* Flag/Option */
            buf_append(out, &opos, max_out, COLOR_FLAG);
            buf_append(out, &opos, max_out, word);
            buf_append(out, &opos, max_out, COLOR_RESET);
        } else {
            /* Normal Argument / Path */
            buf_append(out, &opos, max_out, word);
        }
    }

    return out;
}

/* ========================================================================= */
/* 3. Silicon Line Editor (SLE) with Real-Time Terminal Viewport            */
/* ========================================================================= */

static struct termios s_orig_termios;
static bool s_raw_mode_active = false;

static void disable_raw_mode(void) {
    if (s_raw_mode_active) {
        tcsetattr(STDIN_FILENO, TCSADRAIN, &s_orig_termios);
        s_raw_mode_active = false;
    }
}

static bool enable_raw_mode(void) {
    if (!isatty(STDIN_FILENO)) return false;
    if (tcgetattr(STDIN_FILENO, &s_orig_termios) == -1) return false;

    struct termios raw = s_orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0) return false;
    s_raw_mode_active = true;
    atexit(disable_raw_mode);
    return true;
}

static char s_tab_ribbon_buf[512] = {0};

/* Repaint current line with Monaco-grade highlighting and live flag ribbon */
static void repaint_line(zbash_state_t *state, const char *line_buf, size_t line_len, size_t cursor_pos, int prompt_visual_width) {
    char *highlighted = zbash_highlight_line(state, line_buf, line_len, cursor_pos);

    /* Move to col 0 of line, clear to end of line */
    printf("\r\033[K");

    /* Render prompt prefix */
    printf("\033[1;38;5;48m╰─❯\033[0m ");

    /* Render highlighted string */
    if (highlighted) {
        fputs(highlighted, stdout);
        free(highlighted);
    }

    /* Render Inline Ghost Text (Titanium Gray) if cursor is at end of line and not in tab cycle */
    if (!zbash_tab_cycle_is_active() && cursor_pos == line_len && line_len > 0) {
        zbash_candidate_t ghost;
        if (zbash_find_inline_ghost(state, line_buf, line_len, cursor_pos, &ghost)) {
            printf("\033[38;5;242m%s\033[0m", ghost.display_suffix);
        }
    }

    /* Clear any remaining characters to right */
    printf("\033[K");

    /* Inspect ribbon for parameter hints / flag explanation or tab cycle status */
    char *ribbon = NULL;
    if (zbash_tab_cycle_is_active() && s_tab_ribbon_buf[0] != '\0') {
        ribbon = strdup(s_tab_ribbon_buf);
    } else {
        ribbon = zbash_inspect_ribbon(line_buf, line_len, cursor_pos);
    }

    /* Save cursor position (DECSC \0337) */
    printf("\0337");

    /* Move down 1 row to ribbon line */
    printf("\n\r\033[2K");
    if (ribbon && *ribbon) {
        fputs(ribbon, stdout);
        free(ribbon);
    }

    /* Restore cursor position (DECRC \0338) */
    printf("\0338");

    /* Reposition cursor horizontally to current edit position */
    int target_col = prompt_visual_width + (int)cursor_pos + 1;
    printf("\033[%dG", target_col);
    fflush(stdout);
}

char *zbash_read_line_highlighted(zbash_state_t *state) {
    if (!enable_raw_mode()) {
        /* Fallback if raw mode cannot be set */
        char buf[4096];
        if (!fgets(buf, sizeof(buf), stdin)) return NULL;
        size_t l = strlen(buf);
        while (l > 0 && (buf[l - 1] == '\n' || buf[l - 1] == '\r')) buf[--l] = '\0';
        return strdup(buf);
    }

    /* Print top line of Two-Line prompt */
    const char *prompt_str = zbash_build_prompt(state, false);
    if (prompt_str) {
        const char *nl = strchr(prompt_str, '\n');
        if (nl) {
            fwrite(prompt_str, 1, nl - prompt_str, stdout);
            printf("\r\n");
        } else {
            fputs(prompt_str, stdout);
        }
    }

    char line_buf[4096];
    size_t line_len = 0;
    size_t cursor_pos = 0;
    line_buf[0] = '\0';

    int prompt_prefix_len = 4; /* "╰─❯ " is 4 visual columns */
    int hist_index = state ? state->history_count : 0;
    char *saved_draft = NULL;

    /* Ensure a row is allocated beneath prompt for parameter hint ribbon */
    printf("\r\n\033[A");

    zbash_tab_cycle_reset();
    s_tab_ribbon_buf[0] = '\0';

    repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);

    while (1) {
        char c = 0;
        ssize_t nread = read(STDIN_FILENO, &c, 1);
        if (nread <= 0) break;

        /* Enter key */
        if (c == '\r' || c == '\n') {
            /* Enter-Accept Paradigm: Auto-absorb ghost text if at end of line */
            if (!zbash_tab_cycle_is_active() && cursor_pos == line_len && line_len > 0) {
                zbash_candidate_t ghost;
                if (zbash_find_inline_ghost(state, line_buf, line_len, cursor_pos, &ghost)) {
                    size_t slen = strlen(ghost.display_suffix);
                    if (line_len + slen < sizeof(line_buf) - 1) {
                        memcpy(line_buf + line_len, ghost.display_suffix, slen);
                        line_len += slen;
                        cursor_pos = line_len;
                        line_buf[line_len] = '\0';
                        repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                    }
                }
            }
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            printf("\n\r\033[2K\r");
            fflush(stdout);
            disable_raw_mode();
            if (saved_draft) free(saved_draft);
            return strdup(line_buf);
        }

        /* Ctrl-C */
        if (c == 3) {
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            printf("^C\n\r\033[2K\r");
            fflush(stdout);
            disable_raw_mode();
            if (saved_draft) free(saved_draft);
            if (state) state->last_status = 130;
            return strdup("");
        }

        /* Ctrl-D (EOF or delete char) */
        if (c == 4) {
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            if (line_len == 0) {
                printf("\n\r\033[2K\r");
                fflush(stdout);
                disable_raw_mode();
                if (saved_draft) free(saved_draft);
                return NULL; /* EOF */
            }
            if (cursor_pos < line_len) {
                memmove(line_buf + cursor_pos, line_buf + cursor_pos + 1, line_len - cursor_pos);
                line_len--;
                repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
            }
            continue;
        }

        /* Backspace */
        if (c == 127 || c == 8) {
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            if (cursor_pos > 0) {
                memmove(line_buf + cursor_pos - 1, line_buf + cursor_pos, line_len - cursor_pos + 1);
                cursor_pos--;
                line_len--;
                repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
            }
            continue;
        }

        /* Ctrl-A: Beginning of line */
        if (c == 1) {
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            cursor_pos = 0;
            repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
            continue;
        }

        /* Ctrl-E / Ctrl-F: End of line or accept ghost text */
        if (c == 5 || c == 6) {
            if (cursor_pos == line_len) {
                zbash_candidate_t ghost;
                if (zbash_find_inline_ghost(state, line_buf, line_len, cursor_pos, &ghost)) {
                    size_t slen = strlen(ghost.display_suffix);
                    if (line_len + slen < sizeof(line_buf) - 1) {
                        memcpy(line_buf + line_len, ghost.display_suffix, slen);
                        line_len += slen;
                        cursor_pos = line_len;
                        line_buf[line_len] = '\0';
                    }
                }
            } else {
                cursor_pos = line_len;
            }
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
            continue;
        }

        /* Ctrl-U: Clear entire line */
        if (c == 21) {
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            line_len = 0;
            cursor_pos = 0;
            line_buf[0] = '\0';
            repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
            continue;
        }

        /* Ctrl-K: Clear from cursor to end of line */
        if (c == 11) {
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            line_len = cursor_pos;
            line_buf[line_len] = '\0';
            repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
            continue;
        }

        /* Ctrl-W: Delete word backward */
        if (c == 23) {
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            if (cursor_pos > 0) {
                size_t old_pos = cursor_pos;
                while (cursor_pos > 0 && isspace((unsigned char)line_buf[cursor_pos - 1])) cursor_pos--;
                while (cursor_pos > 0 && !isspace((unsigned char)line_buf[cursor_pos - 1])) cursor_pos--;
                size_t deleted = old_pos - cursor_pos;
                memmove(line_buf + cursor_pos, line_buf + old_pos, line_len - old_pos + 1);
                line_len -= deleted;
                repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
            }
            continue;
        }

        /* Ctrl-L: Clear screen */
        if (c == 12) {
            printf("\033[H\033[2J\r\n\033[A");
            repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
            continue;
        }

        /* Tab key (Circular Inline Tab Cycling) */
        if (c == '\t') {
            if (zbash_tab_cycle_next(state, line_buf, &line_len, &cursor_pos, s_tab_ribbon_buf, sizeof(s_tab_ribbon_buf))) {
                repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
            }
            continue;
        }

        /* Escape Sequences */
        if (c == 27) {
            struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
            if (poll(&pfd, 1, 50) <= 0) {
                /* Standalone Esc key: Revert Tab Cycle */
                if (zbash_tab_cycle_is_active()) {
                    zbash_tab_cycle_cancel(line_buf, &line_len, &cursor_pos);
                    s_tab_ribbon_buf[0] = '\0';
                    repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                }
                continue;
            }

            char seq[5] = {0};
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;

            /* Alt-H / Esc-h / Esc-? -> Trigger Monaco Hover Help Box */
            if (seq[0] == 'h' || seq[0] == 'H' || seq[0] == '?') {
                zbash_tab_cycle_reset();
                s_tab_ribbon_buf[0] = '\0';
                zbash_show_hover_help(line_buf, line_len, cursor_pos);
                repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                continue;
            }

            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;

            /* F1 in Xterm/macOS Terminal: \033OP */
            if (seq[0] == 'O' && seq[1] == 'P') {
                zbash_tab_cycle_reset();
                s_tab_ribbon_buf[0] = '\0';
                zbash_show_hover_help(line_buf, line_len, cursor_pos);
                repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                continue;
            }

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'D': /* Left Arrow */
                        zbash_tab_cycle_reset();
                        s_tab_ribbon_buf[0] = '\0';
                        if (cursor_pos > 0) {
                            cursor_pos--;
                            repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                        }
                        break;
                    case 'C': /* Right Arrow */
                        if (cursor_pos < line_len) {
                            cursor_pos++;
                            zbash_tab_cycle_reset();
                            s_tab_ribbon_buf[0] = '\0';
                            repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                        } else if (cursor_pos == line_len) {
                            /* Accept inline ghost text */
                            zbash_candidate_t ghost;
                            if (zbash_find_inline_ghost(state, line_buf, line_len, cursor_pos, &ghost)) {
                                size_t slen = strlen(ghost.display_suffix);
                                if (line_len + slen < sizeof(line_buf) - 1) {
                                    memcpy(line_buf + line_len, ghost.display_suffix, slen);
                                    line_len += slen;
                                    cursor_pos = line_len;
                                    line_buf[line_len] = '\0';
                                    zbash_tab_cycle_reset();
                                    s_tab_ribbon_buf[0] = '\0';
                                    repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                                }
                            }
                        }
                        break;
                    case 'A': /* Up Arrow (History Back) */
                        zbash_tab_cycle_reset();
                        s_tab_ribbon_buf[0] = '\0';
                        if (state && state->history_count > 0 && hist_index > 0) {
                            if (hist_index == state->history_count) {
                                if (saved_draft) free(saved_draft);
                                saved_draft = strdup(line_buf);
                            }
                            hist_index--;
                            const char *hline = state->history_lines[hist_index];
                            if (hline) {
                                strncpy(line_buf, hline, sizeof(line_buf) - 1);
                                line_buf[sizeof(line_buf) - 1] = '\0';
                                line_len = strlen(line_buf);
                                cursor_pos = line_len;
                                repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                            }
                        }
                        break;
                    case 'B': /* Down Arrow (History Forward) */
                        zbash_tab_cycle_reset();
                        s_tab_ribbon_buf[0] = '\0';
                        if (state && hist_index < state->history_count) {
                            hist_index++;
                            if (hist_index == state->history_count) {
                                if (saved_draft) {
                                    strncpy(line_buf, saved_draft, sizeof(line_buf) - 1);
                                    line_buf[sizeof(line_buf) - 1] = '\0';
                                    free(saved_draft);
                                    saved_draft = NULL;
                                } else {
                                    line_buf[0] = '\0';
                                }
                            } else {
                                const char *hline = state->history_lines[hist_index];
                                if (hline) {
                                    strncpy(line_buf, hline, sizeof(line_buf) - 1);
                                    line_buf[sizeof(line_buf) - 1] = '\0';
                                }
                            }
                            line_len = strlen(line_buf);
                            cursor_pos = line_len;
                            repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                        }
                        break;
                    case 'H': /* Home */
                        zbash_tab_cycle_reset();
                        s_tab_ribbon_buf[0] = '\0';
                        cursor_pos = 0;
                        repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                        break;
                    case 'F': /* End */
                        if (cursor_pos == line_len) {
                            /* Accept inline ghost text on End */
                            zbash_candidate_t ghost;
                            if (zbash_find_inline_ghost(state, line_buf, line_len, cursor_pos, &ghost)) {
                                size_t slen = strlen(ghost.display_suffix);
                                if (line_len + slen < sizeof(line_buf) - 1) {
                                    memcpy(line_buf + line_len, ghost.display_suffix, slen);
                                    line_len += slen;
                                    cursor_pos = line_len;
                                    line_buf[line_len] = '\0';
                                }
                            }
                        } else {
                            cursor_pos = line_len;
                        }
                        zbash_tab_cycle_reset();
                        s_tab_ribbon_buf[0] = '\0';
                        repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                        break;
                    case '3': /* Delete key \033[3~ */
                        if (read(STDIN_FILENO, &seq[2], 1) > 0 && seq[2] == '~') {
                            zbash_tab_cycle_reset();
                            s_tab_ribbon_buf[0] = '\0';
                            if (cursor_pos < line_len) {
                                memmove(line_buf + cursor_pos, line_buf + cursor_pos + 1, line_len - cursor_pos);
                                line_len--;
                                repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
                            }
                        }
                        break;
                }
            }
            continue;
        }

        /* Printable Characters */
        if ((unsigned char)c >= 32 && line_len + 1 < sizeof(line_buf)) {
            zbash_tab_cycle_reset();
            s_tab_ribbon_buf[0] = '\0';
            memmove(line_buf + cursor_pos + 1, line_buf + cursor_pos, line_len - cursor_pos + 1);
            line_buf[cursor_pos] = c;
            line_len++;
            cursor_pos++;
            repaint_line(state, line_buf, line_len, cursor_pos, prompt_prefix_len);
        }
    }

    disable_raw_mode();
    if (saved_draft) free(saved_draft);
    return strdup(line_buf);
}
