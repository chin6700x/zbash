/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZEDIT_H
#define ZEDIT_H

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include "zbash.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* 1. Gap Buffer Structures & Constants                                      */
/* ========================================================================= */

#define ZEDIT_DEFAULT_BUFFER_CAP 4096
#define ZEDIT_MAX_PATH 1024
#define ZEDIT_MAX_BUTTONS 4

typedef struct {
    char *data;             /* Total allocated buffer memory */
    size_t gap_start;       /* Beginning index of the editable gap */
    size_t gap_end;         /* Ending index of the editable gap */
    size_t capacity;        /* Total allocated capacity */
} zedit_gap_buffer_t;

/* ========================================================================= */
/* 2. Focus Modes & Action Buttons                                           */
/* ========================================================================= */

typedef enum {
    ZEDIT_FOCUS_TERMINAL = 0,     /* Lower terminal viewport active */
    ZEDIT_FOCUS_EDITOR   = 1      /* Upper code editor viewport active */
} zedit_focus_t;

typedef enum {
    ZEDIT_BTN_SAVE = 0,
    ZEDIT_BTN_SAVE_AS,
    ZEDIT_BTN_CANCEL,
    ZEDIT_BTN_CLOSE
} zedit_btn_type_t;

typedef struct {
    zedit_btn_type_t type;
    const char *label;
    int col_start;          /* 1-based start column on row 2 */
    int col_end;            /* 1-based end column on row 2 */
} zedit_btn_t;

enum {
    ZEDIT_KEY_UP = 1000,
    ZEDIT_KEY_DOWN,
    ZEDIT_KEY_LEFT,
    ZEDIT_KEY_RIGHT,
    ZEDIT_KEY_HOME,
    ZEDIT_KEY_END,
    ZEDIT_KEY_PGUP,
    ZEDIT_KEY_PGDN,
    ZEDIT_KEY_DEL,
    ZEDIT_KEY_F5,
    ZEDIT_KEY_F6,
    ZEDIT_KEY_TAB = 9,
    ZEDIT_KEY_BACKSPACE = 127,
    ZEDIT_KEY_CTRL_S = 19,
    ZEDIT_KEY_CTRL_Q = 17,
    ZEDIT_KEY_CTRL_W = 23,
    ZEDIT_KEY_CTRL_T = 20,
    ZEDIT_KEY_CTRL_C = 3,
    ZEDIT_KEY_CTRL_D = 4,
    ZEDIT_KEY_ESC = 27
};

/* ========================================================================= */
/* 3. Session State & Dual-Surface Viewport                                   */
/* ========================================================================= */

typedef struct {
    char filepath[ZEDIT_MAX_PATH];
    char filename[256];
    char file_ext[32];
    bool is_active;         /* Editor split surface currently active */
    bool is_dirty;          /* Has unsaved modifications */
    bool is_new_file;       /* File did not exist on disk originally */
    
    zedit_focus_t focus;    /* Active input focus */

    /* Terminal window dimensions */
    int term_rows;          /* Full terminal height */
    int term_cols;          /* Full terminal width */

    /* Viewport partition lines (1-based ANSI lines) */
    int editor_top;         /* Row 1: Header */
    int editor_height;      /* Total lines dedicated to editor (e.g. 15-25) */
    int divider_row;        /* Row editor_height: Splitter line */
    int terminal_top;       /* Row editor_height + 1: DECSTBM top margin */
    int terminal_bottom;    /* Row term_rows: DECSTBM bottom margin */

    /* Editor cursor & scroll coordinates */
    size_t cursor_row;      /* 0-based logical line index in file */
    size_t cursor_col;      /* 0-based column index in current line */
    size_t scroll_row;      /* 0-based top visible line */
    size_t scroll_col;      /* 0-based left visible column */

    /* Action buttons geometry */
    zedit_btn_t buttons[ZEDIT_MAX_BUTTONS];
    int button_count;

    /* Message / Status notification bar */
    char status_msg[256];
    int status_msg_timer;

    /* Buffer engine */
    zedit_gap_buffer_t buf;
} zedit_session_t;

/* Global or active zedit session instance */
extern zedit_session_t g_zedit_session;

/* ========================================================================= */
/* 4. Function Prototypes                                                    */
/* ========================================================================= */

/* Gap Buffer Engine (zedit_buffer.c) */
bool zedit_buf_init(zedit_gap_buffer_t *buf, size_t initial_cap);
void zedit_buf_free(zedit_gap_buffer_t *buf);
size_t zedit_buf_length(const zedit_gap_buffer_t *buf);
bool zedit_buf_load_file(zedit_gap_buffer_t *buf, const char *path);
bool zedit_buf_save_file(const zedit_gap_buffer_t *buf, const char *path);
bool zedit_buf_insert_char(zedit_gap_buffer_t *buf, size_t pos, char c);
bool zedit_buf_insert_str(zedit_gap_buffer_t *buf, size_t pos, const char *str, size_t len);
bool zedit_buf_delete_char(zedit_gap_buffer_t *buf, size_t pos);
size_t zedit_buf_line_count(const zedit_gap_buffer_t *buf);
bool zedit_buf_get_line(const zedit_gap_buffer_t *buf, size_t line_idx, char *out, size_t max_len);
size_t zedit_buf_coord_to_pos(const zedit_gap_buffer_t *buf, size_t row, size_t col);
void zedit_buf_pos_to_coord(const zedit_gap_buffer_t *buf, size_t pos, size_t *out_row, size_t *out_col);

/* Dual-Surface Viewport & ANSI DECSTBM Engine (zedit_viewport.c) */
void zedit_viewport_detect_size(zedit_session_t *sess);
void zedit_viewport_setup_geometry(zedit_session_t *sess, int preferred_editor_height);
void zedit_viewport_enable(zedit_session_t *sess);
void zedit_viewport_disable(zedit_session_t *sess);
void zedit_viewport_resize(zedit_session_t *sess, int delta_lines);
void zedit_viewport_render_all(zedit_session_t *sess);
void zedit_viewport_render_header(zedit_session_t *sess);
void zedit_viewport_render_body(zedit_session_t *sess);
void zedit_viewport_render_divider(zedit_session_t *sess);
void zedit_viewport_restore_terminal_cursor(zedit_session_t *sess);

/* Syntax Highlighting Engine (zedit_syntax.c) */
void zedit_syntax_highlight_line(const char *raw_line, const char *ext, char *out_ansi, size_t max_len);

/* Action Buttons & Mouse Engine (zedit_actions.c) */
void zedit_init_buttons(zedit_session_t *sess);
bool zedit_handle_mouse_click(zedit_session_t *sess, int col, int row);
bool zedit_action_save(zedit_session_t *sess);
bool zedit_action_save_as(zedit_session_t *sess, const char *new_path);
void zedit_action_cancel(zedit_session_t *sess);
void zedit_action_close(zedit_session_t *sess);

/* Dual-Focus & Execution Bridge (zedit_core.c) */
void zedit_session_init(zedit_session_t *sess);
bool zedit_open(zedit_session_t *sess, const char *filepath);
void zedit_set_focus(zedit_session_t *sess, zedit_focus_t new_focus);
void zedit_toggle_focus(zedit_session_t *sess);
bool zedit_handle_key(zedit_session_t *sess, int key);
bool zedit_run_shortcut(zedit_session_t *sess, zbash_state_t *state);

int zedit_run(zedit_session_t *sess, zbash_state_t *state);
int builtin_zedit(zbash_state_t *state, int argc, char **argv);

/* Environment Detection & Companion GUI Engine (zedit_env.c) */
typedef enum {
    ZEDIT_ENV_HEADLESS = 0,
    ZEDIT_ENV_MACOS_GUI = 1,
    ZEDIT_ENV_LINUX_GUI = 2
} zedit_env_t;

zedit_env_t zedit_detect_environment(void);
bool zedit_spawn_gui(const char *filepath);

#ifdef __cplusplus
}
#endif

#endif /* ZEDIT_H */
