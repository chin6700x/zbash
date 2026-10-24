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

static int s_pass = 0;
static int s_fail = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        s_pass++; \
        printf("  [\033[1;32mPASS\033[0m] %s\n", msg); \
    } else { \
        s_fail++; \
        printf("  [\033[1;31mFAIL\033[0m] %s (Line %d)\n", msg, __LINE__); \
    } \
} while (0)

int zbash_run_line(zbash_state_t *state, const char *line) {
    (void)state;
    (void)line;
    return 0;
}

int main(void) {
    printf("\033[1;36m=== Testing zedit In-Shell Dual-Surface Editor Suite ===\033[0m\n\n");

    /* 1. Gap Buffer Initialization */
    zedit_gap_buffer_t buf;
    bool init_ok = zedit_buf_init(&buf, 256);
    TEST_ASSERT(init_ok == true, "Gap buffer initialized");
    TEST_ASSERT(zedit_buf_length(&buf) == 0, "Initial buffer length is 0");

    /* 2. Insert characters & strings */
    zedit_buf_insert_str(&buf, 0, "Hello World!\nLine 2 is here.", 28);
    TEST_ASSERT(zedit_buf_length(&buf) == 28, "Buffer length after insert is 28");
    TEST_ASSERT(zedit_buf_line_count(&buf) == 2, "Line count is 2");

    char line0[128];
    char line1[128];
    zedit_buf_get_line(&buf, 0, line0, sizeof(line0));
    zedit_buf_get_line(&buf, 1, line1, sizeof(line1));
    TEST_ASSERT(strcmp(line0, "Hello World!") == 0, "Line 0 extracted: 'Hello World!'");
    TEST_ASSERT(strcmp(line1, "Line 2 is here.") == 0, "Line 1 extracted: 'Line 2 is here.'");

    /* 3. Delete character */
    zedit_buf_delete_char(&buf, 11); /* Delete '!' from "Hello World!" */
    zedit_buf_get_line(&buf, 0, line0, sizeof(line0));
    TEST_ASSERT(strcmp(line0, "Hello World") == 0, "Line 0 after delete: 'Hello World'");

    /* 4. Atomic Save & Load from Disk */
    const char *test_path = "/tmp/zedit_test_file.txt";
    unlink(test_path);

    bool save_ok = zedit_buf_save_file(&buf, test_path);
    TEST_ASSERT(save_ok == true, "Atomic save buffer to disk");
    TEST_ASSERT(access(test_path, F_OK) == 0, "Saved file exists on disk");

    zedit_gap_buffer_t load_buf = {0};
    bool load_ok = zedit_buf_load_file(&load_buf, test_path);
    TEST_ASSERT(load_ok == true, "Load file into new gap buffer");
    TEST_ASSERT(zedit_buf_length(&load_buf) == zedit_buf_length(&buf), "Loaded length matches original");

    zedit_buf_free(&buf);
    zedit_buf_free(&load_buf);
    unlink(test_path);

    /* 5. Syntax Highlighting Engine */
    char ansi[1024];

    /* C Preprocessor */
    zedit_syntax_highlight_line("#include <stdio.h>", "c", ansi, sizeof(ansi));
    TEST_ASSERT(strstr(ansi, "\033[1;35m") != NULL, "C #include highlighted in Neon Magenta");

    /* C Keyword */
    zedit_syntax_highlight_line("int main(void) { return 0; }", "c", ansi, sizeof(ansi));
    TEST_ASSERT(strstr(ansi, "\033[1;36m") != NULL, "C keywords (int, void, return) highlighted in Cyan");

    /* Python Keyword */
    zedit_syntax_highlight_line("def test_fn(): return True", "py", ansi, sizeof(ansi));
    TEST_ASSERT(strstr(ansi, "\033[1;36m") != NULL, "Python keywords (def, return, True) in Cyan");

    /* Shell Variables & Keywords */
    zedit_syntax_highlight_line("if [ $STATUS -eq 0 ]; then", "sh", ansi, sizeof(ansi));
    TEST_ASSERT(strstr(ansi, "\033[1;36m") != NULL, "Shell keyword (if, then) in Cyan");
    TEST_ASSERT(strstr(ansi, "\033[38;2;100;190;255m") != NULL, "Shell variable $STATUS in Sky Blue");

    /* Comments */
    zedit_syntax_highlight_line("// Titanium Gray comment", "c", ansi, sizeof(ansi));
    TEST_ASSERT(strstr(ansi, "\033[38;2;120;135;155m") != NULL, "C comment in Titanium Gray");

    /* 6. Viewport Geometry & Setup */
    zedit_session_t sess = {0};
    sess.term_rows = 40;
    sess.term_cols = 120;

    zedit_viewport_setup_geometry(&sess, 20);
    TEST_ASSERT(sess.editor_height == 20, "Editor height is 20");
    TEST_ASSERT(sess.divider_row == 20, "Divider row is 20");
    TEST_ASSERT(sess.terminal_top == 21, "Terminal top margin is 21");
    TEST_ASSERT(sess.terminal_bottom == 40, "Terminal bottom margin is 40");

    /* 7. Action Buttons Setup */
    strncpy(sess.filename, "main.c", sizeof(sess.filename));
    zedit_init_buttons(&sess);
    TEST_ASSERT(sess.button_count == 4, "4 Action buttons initialized");
    TEST_ASSERT(sess.buttons[0].type == ZEDIT_BTN_SAVE, "Button 0 is Save");
    TEST_ASSERT(sess.buttons[1].type == ZEDIT_BTN_SAVE_AS, "Button 1 is Save As");
    TEST_ASSERT(sess.buttons[2].type == ZEDIT_BTN_CANCEL, "Button 2 is Cancel");
    TEST_ASSERT(sess.buttons[3].type == ZEDIT_BTN_CLOSE, "Button 3 is Close");

    /* 8. SGR Mouse Click on Buttons */
    sess.is_active = true;
    strncpy(sess.filepath, "/tmp/zedit_mouse_test.c", sizeof(sess.filepath));
    zedit_buf_init(&sess.buf, 512);
    zedit_buf_insert_str(&sess.buf, 0, "int x = 42;\n", 12);
    sess.is_dirty = true;

    /* Click on Save Button */
    int save_col = sess.buttons[0].col_start + 2;
    bool clicked_save = zedit_handle_mouse_click(&sess, save_col, 1);
    TEST_ASSERT(clicked_save == true, "Mouse click on Save button handled");
    TEST_ASSERT(sess.is_dirty == false, "Buffer marked clean after Save click");
    unlink(sess.filepath);

    /* Click on Editor Body -> Focus shifts to Editor */
    sess.focus = ZEDIT_FOCUS_TERMINAL;
    bool clicked_body = zedit_handle_mouse_click(&sess, 15, 3);
    TEST_ASSERT(clicked_body == true, "Mouse click on Editor body handled");
    TEST_ASSERT(sess.focus == ZEDIT_FOCUS_EDITOR, "Focus shifted to EDITOR");
    TEST_ASSERT(sess.cursor_row == 1, "Cursor row updated to clicked line");

    /* Click on Terminal Region -> Focus shifts to Terminal */
    bool clicked_term = zedit_handle_mouse_click(&sess, 10, 25);
    TEST_ASSERT(clicked_term == true, "Mouse click on Terminal handled");
    TEST_ASSERT(sess.focus == ZEDIT_FOCUS_TERMINAL, "Focus shifted to TERMINAL");

    /* 9. Focus Toggle */
    zedit_toggle_focus(&sess);
    TEST_ASSERT(sess.focus == ZEDIT_FOCUS_EDITOR, "Toggle focus switches back to EDITOR");

    /* 10. Key Handling */
    sess.cursor_row = 0;
    sess.cursor_col = 0;
    zedit_handle_key(&sess, 'A');
    TEST_ASSERT(sess.is_dirty == true, "Typing 'A' marks buffer dirty");
    zedit_handle_key(&sess, 127); /* Backspace */

    /* Test Ctrl-S (Save) key handling */
    sess.is_dirty = true;
    bool ctrl_s_handled = zedit_handle_key(&sess, ZEDIT_KEY_CTRL_S);
    TEST_ASSERT(ctrl_s_handled == true, "Ctrl-S handled by zedit_handle_key");
    TEST_ASSERT(sess.is_dirty == false, "Ctrl-S saves file and clears dirty flag");

    /* 11. Run Shortcut (F5) */
    strncpy(sess.file_ext, "py", sizeof(sess.file_ext));
    strncpy(sess.filepath, "test_app.py", sizeof(sess.filepath));
    /* Test runner without state executes save and returns */
    bool run_ok = zedit_run_shortcut(&sess, NULL);
    TEST_ASSERT(run_ok == false, "Run shortcut handled cleanly without live state");
    TEST_ASSERT(sess.focus == ZEDIT_FOCUS_TERMINAL, "Run shortcut switches focus to TERMINAL");

    /* 12. Test Ctrl-Q (Close) key handling */
    sess.focus = ZEDIT_FOCUS_EDITOR;
    bool ctrl_q_handled = zedit_handle_key(&sess, ZEDIT_KEY_CTRL_Q);
    TEST_ASSERT(ctrl_q_handled == true, "Ctrl-Q handled by zedit_handle_key");
    TEST_ASSERT(sess.is_active == false, "Ctrl-Q deactivates session");

    /* 13. Close Action */
    zedit_action_close(&sess);
    TEST_ASSERT(sess.is_active == false, "Close action deactivates session");

    printf("\n========================================================\n");
    printf("Total Tests: %d | Passed: %d | Failed: %d\n", s_pass + s_fail, s_pass, s_fail);
    if (s_fail == 0) {
        printf("\033[1;32m⚡ ALL ZEDIT TESTS PASSED!\033[0m\n");
        return 0;
    }
    return 1;
}
