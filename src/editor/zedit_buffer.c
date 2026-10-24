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
#include <fcntl.h>
#include <sys/stat.h>

bool zedit_buf_init(zedit_gap_buffer_t *buf, size_t initial_cap) {
    if (!buf) return false;
    if (initial_cap < 128) initial_cap = ZEDIT_DEFAULT_BUFFER_CAP;

    buf->data = (char *)malloc(initial_cap);
    if (!buf->data) {
        buf->gap_start = 0;
        buf->gap_end = 0;
        buf->capacity = 0;
        return false;
    }

    buf->gap_start = 0;
    buf->gap_end = initial_cap;
    buf->capacity = initial_cap;
    return true;
}

void zedit_buf_free(zedit_gap_buffer_t *buf) {
    if (!buf) return;
    if (buf->data) {
        free(buf->data);
        buf->data = NULL;
    }
    buf->gap_start = 0;
    buf->gap_end = 0;
    buf->capacity = 0;
}

size_t zedit_buf_length(const zedit_gap_buffer_t *buf) {
    if (!buf || !buf->data) return 0;
    return buf->gap_start + (buf->capacity - buf->gap_end);
}

static void zedit_buf_move_gap(zedit_gap_buffer_t *buf, size_t target_pos) {
    if (!buf || !buf->data) return;
    size_t len = zedit_buf_length(buf);
    if (target_pos > len) target_pos = len;

    if (target_pos < buf->gap_start) {
        size_t count = buf->gap_start - target_pos;
        memmove(buf->data + buf->gap_end - count, buf->data + target_pos, count);
        buf->gap_start -= count;
        buf->gap_end -= count;
    } else if (target_pos > buf->gap_start) {
        size_t count = target_pos - buf->gap_start;
        memmove(buf->data + buf->gap_start, buf->data + buf->gap_end, count);
        buf->gap_start += count;
        buf->gap_end += count;
    }
}

static bool zedit_buf_grow(zedit_gap_buffer_t *buf, size_t min_extra) {
    if (!buf || !buf->data) return false;
    size_t old_cap = buf->capacity;
    size_t new_cap = old_cap * 2;
    if (new_cap < old_cap + min_extra) {
        new_cap = old_cap + min_extra + 1024;
    }

    char *new_data = (char *)realloc(buf->data, new_cap);
    if (!new_data) return false;
    buf->data = new_data;

    /* Shift right side of the gap to the end of the new buffer */
    size_t right_len = old_cap - buf->gap_end;
    size_t new_gap_end = new_cap - right_len;
    if (right_len > 0) {
        memmove(buf->data + new_gap_end, buf->data + buf->gap_end, right_len);
    }

    buf->gap_end = new_gap_end;
    buf->capacity = new_cap;
    return true;
}

bool zedit_buf_load_file(zedit_gap_buffer_t *buf, const char *path) {
    if (!buf || !path) return false;
    zedit_buf_free(buf);

    if (access(path, F_OK) != 0) {
        return zedit_buf_init(buf, ZEDIT_DEFAULT_BUFFER_CAP);
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return zedit_buf_init(buf, ZEDIT_DEFAULT_BUFFER_CAP);
    }

    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size <= 0) {
        close(fd);
        return zedit_buf_init(buf, ZEDIT_DEFAULT_BUFFER_CAP);
    }

    size_t fsize = (size_t)st.st_size;
    size_t cap = fsize + ZEDIT_DEFAULT_BUFFER_CAP;
    if (!zedit_buf_init(buf, cap)) {
        close(fd);
        return false;
    }

    ssize_t nread = read(fd, buf->data, fsize);
    close(fd);

    if (nread < 0) nread = 0;
    buf->gap_start = (size_t)nread;
    buf->gap_end = buf->capacity;
    return true;
}

bool zedit_buf_save_file(const zedit_gap_buffer_t *buf, const char *path) {
    if (!buf || !buf->data || !path || !*path) return false;

    char tmp_path[ZEDIT_MAX_PATH + 32];
    snprintf(tmp_path, sizeof(tmp_path), "%s.zedit_tmp_%d", path, (int)getpid());

    int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;

    /* Write left portion */
    if (buf->gap_start > 0) {
        ssize_t wr = write(fd, buf->data, buf->gap_start);
        if (wr != (ssize_t)buf->gap_start) {
            close(fd);
            unlink(tmp_path);
            return false;
        }
    }

    /* Write right portion */
    size_t right_len = buf->capacity - buf->gap_end;
    if (right_len > 0) {
        ssize_t wr = write(fd, buf->data + buf->gap_end, right_len);
        if (wr != (ssize_t)right_len) {
            close(fd);
            unlink(tmp_path);
            return false;
        }
    }

    fsync(fd);
    close(fd);

    /* Atomic replace */
    if (rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        return false;
    }

    return true;
}

bool zedit_buf_insert_char(zedit_gap_buffer_t *buf, size_t pos, char c) {
    if (!buf || !buf->data) return false;
    zedit_buf_move_gap(buf, pos);

    if (buf->gap_start == buf->gap_end) {
        if (!zedit_buf_grow(buf, 256)) return false;
    }

    buf->data[buf->gap_start++] = c;
    return true;
}

bool zedit_buf_insert_str(zedit_gap_buffer_t *buf, size_t pos, const char *str, size_t len) {
    if (!buf || !buf->data || !str || len == 0) return false;
    zedit_buf_move_gap(buf, pos);

    if (buf->gap_end - buf->gap_start < len) {
        if (!zedit_buf_grow(buf, len + 256)) return false;
    }

    memcpy(buf->data + buf->gap_start, str, len);
    buf->gap_start += len;
    return true;
}

bool zedit_buf_delete_char(zedit_gap_buffer_t *buf, size_t pos) {
    if (!buf || !buf->data) return false;
    size_t len = zedit_buf_length(buf);
    if (pos >= len) return false;

    zedit_buf_move_gap(buf, pos);
    if (buf->gap_end < buf->capacity) {
        buf->gap_end++;
        return true;
    }
    return false;
}

size_t zedit_buf_line_count(const zedit_gap_buffer_t *buf) {
    if (!buf || !buf->data) return 1;
    size_t count = 1;

    for (size_t i = 0; i < buf->gap_start; i++) {
        if (buf->data[i] == '\n') count++;
    }
    for (size_t i = buf->gap_end; i < buf->capacity; i++) {
        if (buf->data[i] == '\n') count++;
    }
    return count;
}

bool zedit_buf_get_line(const zedit_gap_buffer_t *buf, size_t line_idx, char *out, size_t max_len) {
    if (!buf || !buf->data || !out || max_len == 0) return false;
    out[0] = '\0';

    size_t current_line = 0;
    size_t out_pos = 0;

    /* Scan left part */
    for (size_t i = 0; i < buf->gap_start; i++) {
        char ch = buf->data[i];
        if (current_line == line_idx) {
            if (ch == '\n' || ch == '\r') {
                out[out_pos] = '\0';
                return true;
            }
            if (out_pos + 1 < max_len) {
                out[out_pos++] = ch;
            }
        }
        if (ch == '\n') {
            current_line++;
            if (current_line > line_idx) {
                out[out_pos] = '\0';
                return true;
            }
        }
    }

    /* Scan right part */
    for (size_t i = buf->gap_end; i < buf->capacity; i++) {
        char ch = buf->data[i];
        if (current_line == line_idx) {
            if (ch == '\n' || ch == '\r') {
                out[out_pos] = '\0';
                return true;
            }
            if (out_pos + 1 < max_len) {
                out[out_pos++] = ch;
            }
        }
        if (ch == '\n') {
            current_line++;
            if (current_line > line_idx) {
                out[out_pos] = '\0';
                return true;
            }
        }
    }

    out[out_pos] = '\0';
    return (current_line == line_idx);
}

size_t zedit_buf_coord_to_pos(const zedit_gap_buffer_t *buf, size_t row, size_t col) {
    if (!buf || !buf->data) return 0;
    size_t cur_row = 0;
    size_t cur_col = 0;
    size_t logical_pos = 0;

    for (size_t i = 0; i < buf->gap_start; i++) {
        if (cur_row == row && cur_col == col) return logical_pos;
        if (buf->data[i] == '\n') {
            if (cur_row == row) return logical_pos;
            cur_row++;
            cur_col = 0;
        } else {
            cur_col++;
        }
        logical_pos++;
    }

    for (size_t i = buf->gap_end; i < buf->capacity; i++) {
        if (cur_row == row && cur_col == col) return logical_pos;
        if (buf->data[i] == '\n') {
            if (cur_row == row) return logical_pos;
            cur_row++;
            cur_col = 0;
        } else {
            cur_col++;
        }
        logical_pos++;
    }

    return logical_pos;
}

void zedit_buf_pos_to_coord(const zedit_gap_buffer_t *buf, size_t pos, size_t *out_row, size_t *out_col) {
    if (!out_row || !out_col) return;
    *out_row = 0;
    *out_col = 0;
    if (!buf || !buf->data) return;

    size_t logical_pos = 0;
    for (size_t i = 0; i < buf->gap_start && logical_pos < pos; i++, logical_pos++) {
        if (buf->data[i] == '\n') {
            (*out_row)++;
            *out_col = 0;
        } else {
            (*out_col)++;
        }
    }
    for (size_t i = buf->gap_end; i < buf->capacity && logical_pos < pos; i++, logical_pos++) {
        if (buf->data[i] == '\n') {
            (*out_row)++;
            *out_col = 0;
        } else {
            (*out_col)++;
        }
    }
}
