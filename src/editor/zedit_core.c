/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "zedit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>

zedit_session_t g_zedit_session = {0};

void zedit_session_init(zedit_session_t *sess) {
    if (!sess) return;
    memset(sess, 0, sizeof(*sess));
    sess->focus = ZEDIT_FOCUS_EDITOR;
}

bool zedit_open(zedit_session_t *sess, const char *filepath) {
    if (!sess || !filepath || !*filepath) return false;

    zedit_session_init(sess);
    strncpy(sess->filepath, filepath, sizeof(sess->filepath) - 1);

    const char *slash = strrchr(sess->filepath, '/');
    const char *fname = slash ? slash + 1 : sess->filepath;
    strncpy(sess->filename, fname, sizeof(sess->filename) - 1);

    const char *dot = strrchr(fname, '.');
    if (dot) {
        strncpy(sess->file_ext, dot + 1, sizeof(sess->file_ext) - 1);
    } else {
        sess->file_ext[0] = '\0';
    }

    if (access(filepath, F_OK) != 0) {
        sess->is_new_file = true;
    }

    if (!zedit_buf_load_file(&sess->buf, filepath)) {
        return false;
    }

    zedit_init_buttons(sess);
    sess->focus = ZEDIT_FOCUS_EDITOR;
    zedit_viewport_enable(sess);
    return true;
}

void zedit_set_focus(zedit_session_t *sess, zedit_focus_t new_focus) {
    if (!sess || !sess->is_active) return;
    sess->focus = new_focus;
    zedit_viewport_render_header(sess);
    zedit_viewport_render_body(sess);
    zedit_viewport_restore_terminal_cursor(sess);
    fflush(stdout);
}

void zedit_toggle_focus(zedit_session_t *sess) {
    if (!sess || !sess->is_active) return;
    zedit_focus_t next = (sess->focus == ZEDIT_FOCUS_EDITOR) ? ZEDIT_FOCUS_TERMINAL : ZEDIT_FOCUS_EDITOR;
    zedit_set_focus(sess, next);
}

bool zedit_run_shortcut(zedit_session_t *sess, zbash_state_t *state) {
    if (!sess || !sess->is_active) return false;

    /* 1. Auto-save current modifications */
    zedit_action_save(sess);

    /* 2. Switch focus to terminal */
    zedit_set_focus(sess, ZEDIT_FOCUS_TERMINAL);

    /* 3. Build command to run */
    char cmd[512] = {0};
    const char *ext = sess->file_ext;

    if (strcmp(ext, "py") == 0) {
        snprintf(cmd, sizeof(cmd), "python3 %s", sess->filepath);
    } else if (strcmp(ext, "sh") == 0 || strcmp(ext, "bash") == 0) {
        if (access(sess->filepath, X_OK) == 0) {
            snprintf(cmd, sizeof(cmd), "%s", sess->filepath);
        } else {
            snprintf(cmd, sizeof(cmd), "bash %s", sess->filepath);
        }
    } else if (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0) {
        if (access("Makefile", F_OK) == 0) {
            snprintf(cmd, sizeof(cmd), "make");
        } else {
            snprintf(cmd, sizeof(cmd), "cc -O3 %s && ./a.out", sess->filepath);
        }
    } else if (strcmp(ext, "rs") == 0) {
        if (access("Cargo.toml", F_OK) == 0) {
            snprintf(cmd, sizeof(cmd), "cargo run");
        } else {
            snprintf(cmd, sizeof(cmd), "rustc %s && ./main", sess->filepath);
        }
    } else if (strcmp(ext, "go") == 0) {
        snprintf(cmd, sizeof(cmd), "go run %s", sess->filepath);
    } else {
        snprintf(cmd, sizeof(cmd), "cat %s", sess->filepath);
    }

    if (state && cmd[0]) {
        printf("\033[%d;1H\n\033[1;36m⚡ [zedit: F5 Run-in-Terminal]\033[0m %s\n", sess->terminal_top, cmd);
        fflush(stdout);
        zbash_run_line(state, cmd);
        return true;
    }

    return false;
}

int zedit_read_key(int fd, int *out_btn, int *out_col, int *out_row) {
    char c = 0;
    ssize_t n = read(fd, &c, 1);
    if (n <= 0) return -1;

    if (c != 27) {
        return (unsigned char)c;
    }

    /* Escape sequence check: poll with 40ms timeout */
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    if (poll(&pfd, 1, 40) <= 0) {
        return ZEDIT_KEY_ESC;
    }

    char c2 = 0;
    if (read(fd, &c2, 1) <= 0) return ZEDIT_KEY_ESC;

    if (c2 == '[') {
        char c3 = 0;
        if (read(fd, &c3, 1) <= 0) return ZEDIT_KEY_ESC;

        if (c3 == 'A') return ZEDIT_KEY_UP;
        if (c3 == 'B') return ZEDIT_KEY_DOWN;
        if (c3 == 'C') return ZEDIT_KEY_RIGHT;
        if (c3 == 'D') return ZEDIT_KEY_LEFT;
        if (c3 == 'H') return ZEDIT_KEY_HOME;
        if (c3 == 'F') return ZEDIT_KEY_END;

        if (c3 == '<') {
            /* SGR Mouse report: \033[<btn;col;rowM or m */
            char seq[64];
            int idx = 0;
            char last = 0;
            while (idx < (int)sizeof(seq) - 1) {
                if (read(fd, &seq[idx], 1) <= 0) break;
                last = seq[idx];
                if (last == 'M' || last == 'm') {
                    seq[idx] = '\0';
                    break;
                }
                idx++;
            }
            if (last == 'M' && out_btn && out_col && out_row) {
                int btn = 0, col = 0, row = 0;
                if (sscanf(seq, "%d;%d;%d", &btn, &col, &row) == 3) {
                    *out_btn = btn;
                    *out_col = col;
                    *out_row = row;
                    return -2; /* Mouse click */
                }
            }
            return -1; /* Mouse release or malformed */
        }

        if (c3 >= '0' && c3 <= '9') {
            char num_str[16] = {0};
            num_str[0] = c3;
            int n_idx = 1;
            while (n_idx < (int)sizeof(num_str) - 1) {
                char ch = 0;
                if (read(fd, &ch, 1) <= 0) break;
                if (ch == '~') break;
                num_str[n_idx++] = ch;
            }
            int num = atoi(num_str);
            if (num == 1) return ZEDIT_KEY_HOME;
            if (num == 3) return ZEDIT_KEY_DEL;
            if (num == 4) return ZEDIT_KEY_END;
            if (num == 5) return ZEDIT_KEY_PGUP;
            if (num == 6) return ZEDIT_KEY_PGDN;
            if (num == 15) return ZEDIT_KEY_F5;
            if (num == 17) return ZEDIT_KEY_F6;
            return -1;
        }
    } else if (c2 == 'O') {
        char c3 = 0;
        if (read(fd, &c3, 1) <= 0) return ZEDIT_KEY_ESC;
        if (c3 == 'H') return ZEDIT_KEY_HOME;
        if (c3 == 'F') return ZEDIT_KEY_END;
        if (c3 == 'P' || c3 == 'Q' || c3 == 'R' || c3 == 'S') return -1;
    }

    return -1;
}

bool zedit_handle_key(zedit_session_t *sess, int key) {
    if (!sess || !sess->is_active || sess->focus != ZEDIT_FOCUS_EDITOR) return false;

    int visible_lines = sess->divider_row - 2;
    if (visible_lines < 1) visible_lines = 1;
    size_t total_lines = zedit_buf_line_count(&sess->buf);

    char line_buf[2048] = {0};
    zedit_buf_get_line(&sess->buf, sess->cursor_row, line_buf, sizeof(line_buf));
    size_t cur_line_len = strlen(line_buf);

    switch (key) {
        case ZEDIT_KEY_CTRL_S:
            zedit_action_save(sess);
            return true;

        case ZEDIT_KEY_CTRL_Q:
        case ZEDIT_KEY_CTRL_W:
            zedit_action_close(sess);
            return true;

        case ZEDIT_KEY_CTRL_T:
        case ZEDIT_KEY_F6:
            zedit_toggle_focus(sess);
            return true;

        case ZEDIT_KEY_UP:
            if (sess->cursor_row > 0) {
                sess->cursor_row--;
                char prev_line[2048];
                zedit_buf_get_line(&sess->buf, sess->cursor_row, prev_line, sizeof(prev_line));
                size_t prev_len = strlen(prev_line);
                if (sess->cursor_col > prev_len) sess->cursor_col = prev_len;
            }
            break;

        case ZEDIT_KEY_DOWN:
            if (sess->cursor_row + 1 < total_lines) {
                sess->cursor_row++;
                char next_line[2048];
                zedit_buf_get_line(&sess->buf, sess->cursor_row, next_line, sizeof(next_line));
                size_t next_len = strlen(next_line);
                if (sess->cursor_col > next_len) sess->cursor_col = next_len;
            }
            break;

        case ZEDIT_KEY_LEFT:
            if (sess->cursor_col > 0) {
                sess->cursor_col--;
            } else if (sess->cursor_row > 0) {
                sess->cursor_row--;
                char prev_line[2048];
                zedit_buf_get_line(&sess->buf, sess->cursor_row, prev_line, sizeof(prev_line));
                sess->cursor_col = strlen(prev_line);
            }
            break;

        case ZEDIT_KEY_RIGHT:
            if (sess->cursor_col < cur_line_len) {
                sess->cursor_col++;
            } else if (sess->cursor_row + 1 < total_lines) {
                sess->cursor_row++;
                sess->cursor_col = 0;
            }
            break;

        case ZEDIT_KEY_HOME:
            sess->cursor_col = 0;
            break;

        case ZEDIT_KEY_END:
            sess->cursor_col = cur_line_len;
            break;

        case ZEDIT_KEY_PGUP:
            if (sess->cursor_row >= (size_t)visible_lines) {
                sess->cursor_row -= visible_lines;
            } else {
                sess->cursor_row = 0;
            }
            break;

        case ZEDIT_KEY_PGDN:
            sess->cursor_row += visible_lines;
            if (sess->cursor_row >= total_lines) {
                sess->cursor_row = (total_lines > 0) ? total_lines - 1 : 0;
            }
            break;

        case ZEDIT_KEY_TAB: {
            size_t pos = zedit_buf_coord_to_pos(&sess->buf, sess->cursor_row, sess->cursor_col);
            zedit_buf_insert_str(&sess->buf, pos, "    ", 4);
            sess->cursor_col += 4;
            sess->is_dirty = true;
            break;
        }

        case ZEDIT_KEY_DEL: {
            size_t pos = zedit_buf_coord_to_pos(&sess->buf, sess->cursor_row, sess->cursor_col);
            if (pos < zedit_buf_length(&sess->buf)) {
                zedit_buf_delete_char(&sess->buf, pos);
                sess->is_dirty = true;
            }
            break;
        }

        case '\r':
        case '\n': {
            size_t pos = zedit_buf_coord_to_pos(&sess->buf, sess->cursor_row, sess->cursor_col);
            zedit_buf_insert_char(&sess->buf, pos, '\n');
            sess->cursor_row++;
            sess->cursor_col = 0;
            sess->is_dirty = true;
            break;
        }

        case 127:
        case 8: { /* Backspace */
            if (sess->cursor_col > 0) {
                size_t pos = zedit_buf_coord_to_pos(&sess->buf, sess->cursor_row, sess->cursor_col);
                if (pos > 0) {
                    zedit_buf_delete_char(&sess->buf, pos - 1);
                    sess->cursor_col--;
                    sess->is_dirty = true;
                }
            } else if (sess->cursor_row > 0) {
                /* Merge with previous line */
                size_t pos = zedit_buf_coord_to_pos(&sess->buf, sess->cursor_row, 0);
                if (pos > 0) {
                    zedit_buf_delete_char(&sess->buf, pos - 1);
                    sess->cursor_row--;
                    char pline[2048];
                    zedit_buf_get_line(&sess->buf, sess->cursor_row, pline, sizeof(pline));
                    sess->cursor_col = strlen(pline);
                    sess->is_dirty = true;
                }
            }
            break;
        }

        default:
            if (key >= 32 && key <= 126) {
                size_t pos = zedit_buf_coord_to_pos(&sess->buf, sess->cursor_row, sess->cursor_col);
                zedit_buf_insert_char(&sess->buf, pos, (char)key);
                sess->cursor_col++;
                sess->is_dirty = true;
            }
            break;
    }

    /* Scroll handling */
    if (sess->cursor_row < sess->scroll_row) {
        sess->scroll_row = sess->cursor_row;
    }
    if (sess->cursor_row >= sess->scroll_row + visible_lines) {
        sess->scroll_row = sess->cursor_row - visible_lines + 1;
    }

    zedit_viewport_render_all(sess);
    return true;
}

int zedit_run(zedit_session_t *sess, zbash_state_t *state) {
    if (!sess || !sess->is_active) return 0;
    if (!isatty(STDIN_FILENO)) return 0;

    struct termios orig_termios, raw;
    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) return 0;
    raw = orig_termios;
    /* Crucial: Disable IXON so Ctrl-S and Ctrl-Q are NOT intercepted by tty flow control */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    zedit_viewport_render_all(sess);

    char term_cmd[1024] = {0};
    int term_len = 0;
    int term_pos = 0;

    while (sess->is_active) {
        int btn = 0, col = 0, row = 0;
        int key = zedit_read_key(STDIN_FILENO, &btn, &col, &row);

        if (key == -2) {
            /* Mouse click */
            zedit_handle_mouse_click(sess, col, row);
            if (sess->focus == ZEDIT_FOCUS_TERMINAL) {
                printf("\033[%d;1H\033[K\033[1;36mzbash [term] ❯\033[0m %s\033[%dG",
                       sess->terminal_bottom, term_cmd, 16 + term_pos);
                fflush(stdout);
            }
            continue;
        }
        if (key == -1) continue;

        /* Universal shortcut: Ctrl-T or F6 toggles window focus */
        if (key == ZEDIT_KEY_CTRL_T || key == ZEDIT_KEY_F6) {
            zedit_toggle_focus(sess);
            if (sess->focus == ZEDIT_FOCUS_TERMINAL) {
                printf("\033[%d;1H\033[K\033[1;36mzbash [term] ❯\033[0m %s\033[%dG",
                       sess->terminal_bottom, term_cmd, 16 + term_pos);
                fflush(stdout);
            }
            continue;
        }

        /* Universal shortcut: Ctrl-S saves file immediately from any mode */
        if (key == ZEDIT_KEY_CTRL_S) {
            zedit_action_save(sess);
            zedit_viewport_render_all(sess);
            if (sess->focus == ZEDIT_FOCUS_TERMINAL) {
                printf("\033[%d;1H\033[K\033[1;36mzbash [term] ❯\033[0m %s\033[%dG",
                       sess->terminal_bottom, term_cmd, 16 + term_pos);
            }
            fflush(stdout);
            continue;
        }

        /* Universal shortcut: Ctrl-Q or Ctrl-W closes editor immediately from any mode */
        if (key == ZEDIT_KEY_CTRL_Q || key == ZEDIT_KEY_CTRL_W) {
            zedit_action_close(sess);
            break;
        }

        if (sess->focus == ZEDIT_FOCUS_EDITOR) {
            if (key == ZEDIT_KEY_F5) {
                /* F5: Auto-save and Run in Terminal */
                tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
                zedit_run_shortcut(sess, state);
                tcsetattr(STDIN_FILENO, TCSANOW, &raw);
                printf("\033[%d;%dr", sess->terminal_top, sess->terminal_bottom);
                zedit_viewport_render_all(sess);
                if (sess->focus == ZEDIT_FOCUS_TERMINAL) {
                    printf("\033[%d;1H\033[K\033[1;36mzbash [term] ❯\033[0m ", sess->terminal_bottom);
                    fflush(stdout);
                }
            } else {
                zedit_handle_key(sess, key);
            }
        } else {
            /* ZEDIT_FOCUS_TERMINAL */
            if (key == ZEDIT_KEY_ESC) {
                /* ESC in terminal mode switches back to editor */
                zedit_set_focus(sess, ZEDIT_FOCUS_EDITOR);
                continue;
            }

            if (key == '\r' || key == '\n') {
                printf("\r\n");
                fflush(stdout);
                if (strcmp(term_cmd, "exit") == 0 || strcmp(term_cmd, "quit") == 0) {
                    zedit_action_close(sess);
                    break;
                }
                if (term_len > 0) {
                    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
                    zbash_run_line(state, term_cmd);
                    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
                    term_cmd[0] = '\0';
                    term_len = 0;
                    term_pos = 0;
                    printf("\033[%d;%dr", sess->terminal_top, sess->terminal_bottom);
                    zedit_viewport_render_divider(sess);
                    zedit_viewport_render_header(sess);
                }
                printf("\r\n\033[1;36mzbash [term] ❯\033[0m ");
                fflush(stdout);
            } else if (key == ZEDIT_KEY_BACKSPACE) {
                if (term_pos > 0) {
                    memmove(term_cmd + term_pos - 1, term_cmd + term_pos, term_len - term_pos + 1);
                    term_pos--;
                    term_len--;
                    printf("\r\033[K\033[1;36mzbash [term] ❯\033[0m %s\033[%dG",
                           term_cmd, 16 + term_pos);
                    fflush(stdout);
                }
            } else if (key == ZEDIT_KEY_CTRL_C) {
                term_cmd[0] = '\0';
                term_len = 0;
                term_pos = 0;
                printf("^C\r\n\033[1;36mzbash [term] ❯\033[0m ");
                fflush(stdout);
            } else if (key == ZEDIT_KEY_LEFT) {
                if (term_pos > 0) {
                    term_pos--;
                    printf("\033[D");
                    fflush(stdout);
                }
            } else if (key == ZEDIT_KEY_RIGHT) {
                if (term_pos < term_len) {
                    term_pos++;
                    printf("\033[C");
                    fflush(stdout);
                }
            } else if (key >= 32 && key <= 126) {
                if (term_len < (int)sizeof(term_cmd) - 2) {
                    memmove(term_cmd + term_pos + 1, term_cmd + term_pos, term_len - term_pos + 1);
                    term_cmd[term_pos] = (char)key;
                    term_pos++;
                    term_len++;
                    term_cmd[term_len] = '\0';
                    printf("\r\033[K\033[1;36mzbash [term] ❯\033[0m %s\033[%dG",
                           term_cmd, 16 + term_pos);
                    fflush(stdout);
                }
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    zedit_viewport_disable(sess);
    return 0;
}
