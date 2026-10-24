/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "zedit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void zedit_init_buttons(zedit_session_t *sess) {
    if (!sess) return;
    sess->button_count = 4;

    /* Base column offset on Row 1 (after file name and status badge) */
    int cur_col = 3 + (int)strlen(sess->filename) + 14;

    /* 1. [ 💾 Save ] */
    sess->buttons[0].type = ZEDIT_BTN_SAVE;
    sess->buttons[0].label = "[ 💾 Save ]";
    sess->buttons[0].col_start = cur_col;
    sess->buttons[0].col_end = cur_col + 11;
    cur_col += 13;

    /* 2. [ 📝 Save As ] */
    sess->buttons[1].type = ZEDIT_BTN_SAVE_AS;
    sess->buttons[1].label = "[ 📝 Save As ]";
    sess->buttons[1].col_start = cur_col;
    sess->buttons[1].col_end = cur_col + 14;
    cur_col += 16;

    /* 3. [ ↺ Cancel ] */
    sess->buttons[2].type = ZEDIT_BTN_CANCEL;
    sess->buttons[2].label = "[ ↺ Cancel ]";
    sess->buttons[2].col_start = cur_col;
    sess->buttons[2].col_end = cur_col + 12;
    cur_col += 14;

    /* 4. [ ✖ Close ] */
    sess->buttons[3].type = ZEDIT_BTN_CLOSE;
    sess->buttons[3].label = "[ ✖ Close ]";
    sess->buttons[3].col_start = cur_col;
    sess->buttons[3].col_end = cur_col + 11;
}

bool zedit_action_save(zedit_session_t *sess) {
    if (!sess || !sess->is_active) return false;
    if (!sess->filepath[0]) {
        snprintf(sess->status_msg, sizeof(sess->status_msg), "Error: No file path specified");
        zedit_viewport_render_header(sess);
        fflush(stdout);
        return false;
    }

    if (zedit_buf_save_file(&sess->buf, sess->filepath)) {
        sess->is_dirty = false;
        sess->is_new_file = false;
        snprintf(sess->status_msg, sizeof(sess->status_msg), "✓ Saved successfully");
        zedit_viewport_render_header(sess);
        fflush(stdout);
        return true;
    } else {
        snprintf(sess->status_msg, sizeof(sess->status_msg), "❌ Error saving file");
        zedit_viewport_render_header(sess);
        fflush(stdout);
        return false;
    }
}

bool zedit_action_save_as(zedit_session_t *sess, const char *new_path) {
    if (!sess || !new_path || !*new_path) return false;
    strncpy(sess->filepath, new_path, sizeof(sess->filepath) - 1);
    sess->filepath[sizeof(sess->filepath) - 1] = '\0';

    const char *slash = strrchr(sess->filepath, '/');
    const char *fname = slash ? slash + 1 : sess->filepath;
    strncpy(sess->filename, fname, sizeof(sess->filename) - 1);

    const char *dot = strrchr(fname, '.');
    if (dot) {
        strncpy(sess->file_ext, dot + 1, sizeof(sess->file_ext) - 1);
    } else {
        sess->file_ext[0] = '\0';
    }

    zedit_init_buttons(sess);
    return zedit_action_save(sess);
}

void zedit_action_cancel(zedit_session_t *sess) {
    if (!sess || !sess->is_active) return;
    /* Discard modifications and reload original disk content */
    if (sess->filepath[0]) {
        zedit_buf_load_file(&sess->buf, sess->filepath);
    }
    sess->is_dirty = false;
    sess->cursor_row = 0;
    sess->cursor_col = 0;
    sess->scroll_row = 0;
    sess->scroll_col = 0;
    snprintf(sess->status_msg, sizeof(sess->status_msg), "Changes discarded");
    zedit_viewport_render_all(sess);
}

void zedit_action_close(zedit_session_t *sess) {
    if (!sess || !sess->is_active) return;
    zedit_viewport_disable(sess);
    zedit_buf_free(&sess->buf);
    sess->is_dirty = false;
    sess->filepath[0] = '\0';
    sess->filename[0] = '\0';
}

bool zedit_handle_mouse_click(zedit_session_t *sess, int col, int row) {
    if (!sess || !sess->is_active) return false;

    /* 1. Click on Header row (Buttons) */
    if (row == 1) {
        for (int i = 0; i < sess->button_count; i++) {
            zedit_btn_t *btn = &sess->buttons[i];
            if (col >= btn->col_start && col <= btn->col_end) {
                switch (btn->type) {
                    case ZEDIT_BTN_SAVE:
                        zedit_action_save(sess);
                        return true;
                    case ZEDIT_BTN_SAVE_AS:
                        /* For mouse click, trigger save or status message */
                        zedit_action_save(sess);
                        return true;
                    case ZEDIT_BTN_CANCEL:
                        zedit_action_cancel(sess);
                        return true;
                    case ZEDIT_BTN_CLOSE:
                        zedit_action_close(sess);
                        return true;
                }
            }
        }
        return true;
    }

    /* 2. Click on Editor Body */
    if (row >= 2 && row < sess->divider_row) {
        zedit_set_focus(sess, ZEDIT_FOCUS_EDITOR);
        size_t target_row = sess->scroll_row + (row - 2);
        size_t total_lines = zedit_buf_line_count(&sess->buf);
        if (target_row < total_lines) {
            sess->cursor_row = target_row;
            int target_col = col - 7;
            if (target_col < 0) target_col = 0;
            sess->cursor_col = (size_t)target_col;
        }
        zedit_viewport_render_all(sess);
        return true;
    }

    /* 3. Click on Divider */
    if (row == sess->divider_row) {
        return true;
    }

    /* 4. Click on Lower Terminal */
    if (row >= sess->terminal_top) {
        zedit_set_focus(sess, ZEDIT_FOCUS_TERMINAL);
        zedit_viewport_render_header(sess);
        zedit_viewport_restore_terminal_cursor(sess);
        fflush(stdout);
        return true;
    }

    return false;
}
