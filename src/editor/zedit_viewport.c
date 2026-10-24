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
#include <sys/ioctl.h>
#include <termios.h>

void zedit_viewport_detect_size(zedit_session_t *sess) {
    if (!sess) return;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
        sess->term_rows = ws.ws_row;
        sess->term_cols = ws.ws_col;
    } else {
        sess->term_rows = 24;
        sess->term_cols = 80;
    }
}

void zedit_viewport_setup_geometry(zedit_session_t *sess, int preferred_editor_height) {
    if (!sess) return;
    if (sess->term_rows <= 0 || sess->term_cols <= 0) {
        zedit_viewport_detect_size(sess);
    }

    if (preferred_editor_height <= 0) {
        /* Default to 55% of terminal height */
        preferred_editor_height = (sess->term_rows * 55) / 100;
    }

    /* Clamp constraints */
    if (preferred_editor_height < 6) preferred_editor_height = 6;
    if (preferred_editor_height > sess->term_rows - 5) {
        preferred_editor_height = sess->term_rows - 5;
    }
    if (preferred_editor_height < 6) preferred_editor_height = 6;

    sess->editor_top = 1;
    sess->editor_height = preferred_editor_height;
    sess->divider_row = preferred_editor_height;
    sess->terminal_top = preferred_editor_height + 1;
    sess->terminal_bottom = sess->term_rows;
}

void zedit_viewport_enable(zedit_session_t *sess) {
    if (!sess) return;
    zedit_viewport_setup_geometry(sess, sess->editor_height);
    sess->is_active = true;

    /* 1. Enable SGR mouse tracking so clicks switch focus and trigger buttons */
    printf("\033[?1000h\033[?1006h");

    /* 2. Configure DECSTBM scrolling margin for lower terminal viewport */
    printf("\033[%d;%dr", sess->terminal_top, sess->terminal_bottom);

    /* 3. Render all editor surfaces */
    zedit_viewport_render_all(sess);

    /* 4. Flush to terminal */
    fflush(stdout);
}

void zedit_viewport_disable(zedit_session_t *sess) {
    if (!sess) return;

    /* 1. Disable mouse tracking */
    printf("\033[?1000l\033[?1006l");

    /* 2. Reset DECSTBM margins to full screen */
    printf("\033[r");

    /* 3. Move cursor to lower terminal region */
    printf("\033[%d;1H", sess->term_rows);
    fflush(stdout);

    sess->is_active = false;
}

void zedit_viewport_resize(zedit_session_t *sess, int delta_lines) {
    if (!sess || !sess->is_active) return;
    int target_height = sess->editor_height + delta_lines;
    zedit_viewport_setup_geometry(sess, target_height);

    /* Update DECSTBM margin */
    printf("\033[%d;%dr", sess->terminal_top, sess->terminal_bottom);

    /* Redraw surfaces */
    zedit_viewport_render_all(sess);
    fflush(stdout);
}

void zedit_viewport_render_header(zedit_session_t *sess) {
    if (!sess) return;

    /* Move to Row 1, Col 1 */
    printf("\033[1;1H");

    /* Top Header Bar styling (Dark slate gray background) */
    printf("\033[48;2;25;30;40m");

    /* 1. File icon & Name */
    printf(" \033[1;36m📄 %s\033[0m\033[48;2;25;30;40m", sess->filename[0] ? sess->filename : "untitled");

    /* 2. Dirty badge */
    if (sess->is_dirty) {
        printf(" \033[38;2;255;165;0m● [Unsaved]\033[0m\033[48;2;25;30;40m");
    } else {
        printf(" \033[38;2;46;213;115m✓ [Saved]\033[0m\033[48;2;25;30;40m");
    }

    /* 3. Action Buttons */
    printf("   ");
    for (int i = 0; i < sess->button_count; i++) {
        zedit_btn_t *btn = &sess->buttons[i];
        printf("\033[1;37;48;2;45;55;72m %s \033[0m\033[48;2;25;30;40m ", btn->label);
    }

    /* 4. Focus indicator & Cursor coordinate on far right using exact ANSI positioning */
    char right_info[256];
    const char *focus_str = (sess->focus == ZEDIT_FOCUS_EDITOR) 
        ? "\033[1;32m● FOCUS: EDITOR (Ctrl-T: Switch Window)\033[0m\033[48;2;25;30;40m" 
        : "\033[38;2;150;160;175m○ FOCUS: TERMINAL (Ctrl-T: Switch Window)\033[0m\033[48;2;25;30;40m";

    /* Visual length of right text without ANSI escapes is ~50 chars */
    int right_visual_len = 50;
    if (sess->term_cols > right_visual_len + 35) {
        snprintf(right_info, sizeof(right_info), "%s  Ln %zu, Col %zu ",
                 focus_str, sess->cursor_row + 1, sess->cursor_col + 1);
        int target_col = sess->term_cols - right_visual_len;
        if (target_col < 1) target_col = 1;
        printf("\033[1;%dH%s", target_col, right_info);
    } else if (sess->term_cols > 25) {
        /* Compact coordinate display on narrower terminals */
        int target_col = sess->term_cols - 18;
        if (target_col < 1) target_col = 1;
        printf("\033[1;%dH\033[1;37m Ln %zu, Col %zu \033[0m", target_col, sess->cursor_row + 1, sess->cursor_col + 1);
    }

    /* Reset styling */
    printf("\033[0m");
}

void zedit_viewport_render_body(zedit_session_t *sess) {
    if (!sess) return;
    int body_start_row = 2;
    int body_end_row = sess->divider_row - 1;
    int visible_lines = body_end_row - body_start_row + 1;
    if (visible_lines <= 0) return;

    size_t total_lines = zedit_buf_line_count(&sess->buf);
    char line_buf[2048];
    char ansi_buf[4096];

    for (int i = 0; i < visible_lines; i++) {
        size_t line_idx = sess->scroll_row + i;
        int screen_row = body_start_row + i;

        printf("\033[%d;1H\033[K", screen_row);

        if (line_idx < total_lines) {
            /* Active line highlight */
            bool is_curr = (line_idx == sess->cursor_row && sess->focus == ZEDIT_FOCUS_EDITOR);
            if (is_curr) {
                printf("\033[48;2;30;38;50m");
            } else {
                printf("\033[48;2;18;22;30m");
            }

            /* Line number gutter */
            printf("\033[38;2;100;115;140m%4zu │ \033[0m", line_idx + 1);
            if (is_curr) {
                printf("\033[48;2;30;38;50m");
            } else {
                printf("\033[48;2;18;22;30m");
            }

            /* Line text with syntax highlighting */
            if (zedit_buf_get_line(&sess->buf, line_idx, line_buf, sizeof(line_buf))) {
                zedit_syntax_highlight_line(line_buf, sess->file_ext, ansi_buf, sizeof(ansi_buf));
                printf("%s", ansi_buf);
            }

            printf("\033[0m");
        } else {
            /* Empty lines beyond EOF */
            printf("\033[48;2;18;22;30m\033[38;2;60;70;90m   ~ │ \033[0m");
        }
    }
}

void zedit_viewport_render_divider(zedit_session_t *sess) {
    if (!sess) return;
    printf("\033[%d;1H\033[K", sess->divider_row);

    const char *label = " [ ⇕ Ctrl-T: สลับหน้าต่าง | F5: รัน | Ctrl-S: บันทึก | Ctrl-Q: ปิด ] ";
    int label_len = 54; /* Visual approximation */
    int half = (sess->term_cols - label_len) / 2;
    if (half < 2) half = 2;

    printf("\033[38;2;0;210;211m");
    for (int i = 0; i < half; i++) printf("─");
    printf("\033[1;36;48;2;30;40;55m%s\033[0m\033[38;2;0;210;211m", label);
    for (int i = 0; i < half; i++) printf("─");
    printf("\033[0m");
}

void zedit_viewport_render_all(zedit_session_t *sess) {
    if (!sess) return;
    /* Save current cursor */
    printf("\033[?25l"); /* Hide cursor while rendering */
    zedit_viewport_render_header(sess);
    zedit_viewport_render_body(sess);
    zedit_viewport_render_divider(sess);
    zedit_viewport_restore_terminal_cursor(sess);
    printf("\033[?25h"); /* Show cursor */
    fflush(stdout);
}

void zedit_viewport_restore_terminal_cursor(zedit_session_t *sess) {
    if (!sess) return;
    if (sess->focus == ZEDIT_FOCUS_EDITOR) {
        /* Position cursor in Editor body */
        int row_on_screen = 2 + (int)(sess->cursor_row - sess->scroll_row);
        int col_on_screen = 7 + (int)(sess->cursor_col - sess->scroll_col);
        if (row_on_screen < 2) row_on_screen = 2;
        if (row_on_screen >= sess->divider_row) row_on_screen = sess->divider_row - 1;
        if (col_on_screen < 7) col_on_screen = 7;
        printf("\033[%d;%dH", row_on_screen, col_on_screen);
    } else {
        /* Position cursor in Terminal bottom region */
        printf("\033[%d;1H", sess->terminal_top);
    }
}
