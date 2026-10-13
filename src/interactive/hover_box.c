/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "hover_box.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* ========================================================================= */
/* 1. Dynamic JIT Man-Page Extractor (Tier 3 / Phase 4)                      */
/* ========================================================================= */

typedef struct jit_flag_store {
    char flag_str[32];
    char desc_str[128];
} jit_flag_store_t;

typedef struct jit_cmd_node {
    char cmd_name[64];
    char synopsis[256];
    char summary[256];
    jit_flag_store_t flags_store[32];
    zbash_flag_entry_t flags[33];
    zbash_cmd_desc_t desc;
    struct jit_cmd_node *next;
} jit_cmd_node_t;

static jit_cmd_node_t *s_jit_cache_head = NULL;

const zbash_cmd_desc_t *zbash_jit_man_lookup(const char *cmd_name) {
    if (!cmd_name || !*cmd_name) return NULL;

    /* Check in-memory JIT cache */
    for (jit_cmd_node_t *curr = s_jit_cache_head; curr != NULL; curr = curr->next) {
        if (strcmp(curr->cmd_name, cmd_name) == 0) {
            return &curr->desc;
        }
    }

    /* Verify if manpage or executable exists */
    char check_cmd[128];
    snprintf(check_cmd, sizeof(check_cmd), "man -w %s >/dev/null 2>&1", cmd_name);
    if (system(check_cmd) != 0) {
        return NULL;
    }

    /* Run man -P cat <cmd_name> to stream text */
    char man_cmd[256];
    snprintf(man_cmd, sizeof(man_cmd), "man -P cat %s 2>/dev/null", cmd_name);
    FILE *fp = popen(man_cmd, "r");
    if (!fp) return NULL;

    jit_cmd_node_t *node = calloc(1, sizeof(jit_cmd_node_t));
    if (!node) {
        pclose(fp);
        return NULL;
    }

    strncpy(node->cmd_name, cmd_name, sizeof(node->cmd_name) - 1);

    char line_buf[512];
    bool in_name = false;
    bool in_synopsis = false;
    bool in_options = false;
    size_t flag_count = 0;

    while (fgets(line_buf, sizeof(line_buf), fp) != NULL) {
        /* Trim trailing newline */
        size_t llen = strlen(line_buf);
        while (llen > 0 && (line_buf[llen - 1] == '\n' || line_buf[llen - 1] == '\r')) {
            line_buf[--llen] = '\0';
        }

        /* Detect Major Sections */
        if (strcmp(line_buf, "NAME") == 0 || strcmp(line_buf, "DESCRIPTION") == 0) {
            in_name = true;
            in_synopsis = false;
            in_options = false;
            continue;
        }
        if (strcmp(line_buf, "SYNOPSIS") == 0) {
            in_synopsis = true;
            in_name = false;
            in_options = false;
            continue;
        }
        if (strcmp(line_buf, "OPTIONS") == 0 || strcmp(line_buf, "FLAGS") == 0) {
            in_options = true;
            in_name = false;
            in_synopsis = false;
            continue;
        }

        /* Parse NAME section: "<cmd> - <summary>" */
        if (in_name && node->summary[0] == '\0') {
            char *sep = strstr(line_buf, " - ");
            if (sep) {
                sep += 3;
                while (isspace((unsigned char)*sep)) sep++;
                strncpy(node->summary, sep, sizeof(node->summary) - 1);
            }
        }

        /* Parse SYNOPSIS section */
        if (in_synopsis && node->synopsis[0] == '\0') {
            char *p = line_buf;
            while (isspace((unsigned char)*p)) p++;
            if (*p && strncmp(p, cmd_name, strlen(cmd_name)) == 0) {
                strncpy(node->synopsis, p, sizeof(node->synopsis) - 1);
            }
        }

        /* Parse OPTIONS section: lines starting with "-" */
        if (in_options && flag_count < 32) {
            char *p = line_buf;
            while (isspace((unsigned char)*p)) p++;
            if (p[0] == '-' && (isalnum((unsigned char)p[1]) || p[1] == '-')) {
                char flag_tok[32] = {0};
                size_t flen = 0;
                while (*p && !isspace((unsigned char)*p) && *p != ',' && flen < sizeof(flag_tok) - 1) {
                    flag_tok[flen++] = *p++;
                }
                flag_tok[flen] = '\0';

                while (isspace((unsigned char)*p) || *p == ',') p++;

                if (flen > 0 && *p) {
                    strncpy(node->flags_store[flag_count].flag_str, flag_tok, sizeof(node->flags_store[flag_count].flag_str) - 1);
                    strncpy(node->flags_store[flag_count].desc_str, p, sizeof(node->flags_store[flag_count].desc_str) - 1);

                    zbash_flag_entry_t *fe = &node->flags[flag_count];
                    if (flag_tok[1] != '-') {
                        fe->short_opt = flag_tok[1];
                    } else {
                        fe->long_opt = node->flags_store[flag_count].flag_str;
                    }
                    fe->description = node->flags_store[flag_count].desc_str;
                    flag_count++;
                }
            }
        }
    }
    pclose(fp);

    /* Fallbacks if sections were empty */
    if (node->synopsis[0] == '\0') {
        snprintf(node->synopsis, sizeof(node->synopsis), "%s [options] [args...]", cmd_name);
    }
    if (node->summary[0] == '\0') {
        snprintf(node->summary, sizeof(node->summary), "system command");
    }

    /* Terminate flags array */
    node->flags[flag_count].description = NULL;

    node->desc.cmd_name = node->cmd_name;
    node->desc.synopsis = node->synopsis;
    node->desc.summary = node->summary;
    node->desc.flags = node->flags;

    /* Push into cache */
    node->next = s_jit_cache_head;
    s_jit_cache_head = node;

    return &node->desc;
}

/* ========================================================================= */
/* 2. Monaco-Grade Floating Hover Tooltip Box Renderer                       */
/* ========================================================================= */

/*
 * Count available flags for a command
 */
static size_t count_flags(const zbash_cmd_desc_t *desc) {
    if (!desc || !desc->flags) return 0;
    size_t count = 0;
    while (desc->flags[count].description != NULL) count++;
    return count;
}

/*
 * Draw the floating box on alternate screen buffer
 */
static void draw_box(const zbash_cmd_desc_t *desc, int rows, int cols, size_t scroll_offset) {
    /* Clear alternate screen */
    printf("\033[H\033[2J");

    int box_width = cols - 6;
    if (box_width > 86) box_width = 86;
    if (box_width < 40) box_width = 40;

    int box_height = rows - 6;
    if (box_height > 18) box_height = 18;
    if (box_height < 10) box_height = 10;

    int start_row = (rows - box_height) / 2;
    if (start_row < 2) start_row = 2;
    int start_col = (cols - box_width) / 2;
    if (start_col < 2) start_col = 2;

    int inner_w = box_width - 2;

    /* Move to start_row, start_col */
    printf("\033[%d;%dH", start_row, start_col);

    /* 1. Header Frame: ╭─ 💡 [cmd] · [summary] ───────────────╮ */
    printf("\033[1;38;5;51m╭─ \033[38;5;220m💡 \033[1;38;5;48m%s\033[0m \033[38;5;244m·\033[0m \033[38;5;252m%.32s\033[1;38;5;51m ",
           desc->cmd_name, desc->summary);
    
    int header_vis_len = 5 + (int)strlen(desc->cmd_name) + 3 + (int)strlen(desc->summary > 0 ? desc->summary : "");
    if (header_vis_len > inner_w - 4) header_vis_len = inner_w - 4;
    for (int i = header_vis_len; i < inner_w; i++) {
        fputs("─", stdout);
    }
    printf("╮\033[0m\r\n");

    /* 2. Synopsis Section */
    printf("\033[%d;%dH", start_row + 1, start_col);
    printf("\033[1;38;5;51m│\033[0m  \033[1;38;5;221mSynopsis:\033[0m \033[38;5;250m%-*.*s\033[1;38;5;51m│\033[0m\r\n",
           inner_w - 13, inner_w - 13, desc->synopsis);

    /* 3. Section Divider: ├─────────────────────────────────────────┤ */
    printf("\033[%d;%dH", start_row + 2, start_col);
    printf("\033[1;38;5;51m├");
    for (int i = 0; i < inner_w; i++) fputs("─", stdout);
    printf("┤\033[0m\r\n");

    /* 4. Flags Section Header */
    printf("\033[%d;%dH", start_row + 3, start_col);
    printf("\033[1;38;5;51m│\033[0m  \033[1;38;5;141mFlags & Options (Keystroke Parameter Hints):\033[0m%*s\033[1;38;5;51m│\033[0m\r\n",
           inner_w - 46 > 0 ? inner_w - 46 : 0, "");

    /* 5. Render Flag Items with Scrolling */
    size_t total_flags = count_flags(desc);
    int flag_rows_visible = box_height - 6;
    if (flag_rows_visible < 1) flag_rows_visible = 1;

    for (int r = 0; r < flag_rows_visible; r++) {
        printf("\033[%d;%dH", start_row + 4 + r, start_col);
        size_t fidx = scroll_offset + r;
        if (fidx < total_flags) {
            const zbash_flag_entry_t *f = &desc->flags[fidx];
            char flag_badge[40];
            if (f->short_opt && f->long_opt) {
                snprintf(flag_badge, sizeof(flag_badge), "-%c, %s", f->short_opt, f->long_opt);
            } else if (f->short_opt) {
                if (f->arg_desc) snprintf(flag_badge, sizeof(flag_badge), "-%c %s", f->short_opt, f->arg_desc);
                else snprintf(flag_badge, sizeof(flag_badge), "-%c", f->short_opt);
            } else if (f->long_opt) {
                if (f->arg_desc) snprintf(flag_badge, sizeof(flag_badge), "%s %s", f->long_opt, f->arg_desc);
                else snprintf(flag_badge, sizeof(flag_badge), "%s", f->long_opt);
            } else {
                strncpy(flag_badge, "-", sizeof(flag_badge) - 1);
            }

            int desc_avail = inner_w - 28;
            if (desc_avail < 10) desc_avail = 10;

            printf("\033[1;38;5;51m│\033[0m  \033[1;38;5;221m%-24.24s\033[0m \033[38;5;252m%-*.*s\033[0m\033[1;38;5;51m│\033[0m\r\n",
                   flag_badge, desc_avail, desc_avail, f->description);
        } else {
            /* Empty row inside box */
            printf("\033[1;38;5;51m│\033[0m%*s\033[1;38;5;51m│\033[0m\r\n", inner_w, "");
        }
    }

    /* 6. Footer Frame: ╰─ [↑/↓] Scroll · [Esc/q/Enter] Close ──╯ */
    printf("\033[%d;%dH", start_row + box_height - 1, start_col);
    printf("\033[1;38;5;51m╰─ \033[1;38;5;75m[↑/k] Up \033[38;5;244m·\033[0m \033[1;38;5;75m[↓/j] Down \033[38;5;244m·\033[0m \033[1;38;5;196m[Esc/q] Close\033[1;38;5;51m ");
    int footer_vis_len = 39;
    for (int i = footer_vis_len; i < inner_w; i++) {
        fputs("─", stdout);
    }
    printf("╯\033[0m");

    fflush(stdout);
}

bool zbash_show_hover_help(const char *line, size_t len, size_t cursor_pos) {
    if (!line || len == 0) return false;
    if (cursor_pos > len) cursor_pos = len;

    /* 1. Extract command under or before cursor */
    size_t stmt_start = 0;
    for (size_t i = cursor_pos; i > 0; i--) {
        char c = line[i - 1];
        if (c == '|' || c == ';' || c == '&') {
            stmt_start = i;
            break;
        }
    }

    /* Skip leading spaces */
    while (stmt_start < cursor_pos && isspace((unsigned char)line[stmt_start])) stmt_start++;
    if (stmt_start >= cursor_pos) return false;

    /* Extract command name */
    char cmd_name[64] = {0};
    size_t cpos = 0;
    while (stmt_start < cursor_pos && !isspace((unsigned char)line[stmt_start]) && cpos < sizeof(cmd_name) - 1) {
        cmd_name[cpos++] = line[stmt_start++];
    }
    cmd_name[cpos] = '\0';
    if (cpos == 0) return false;

    /* 2. Lookup command descriptor in DB or JIT Man-Page */
    const zbash_cmd_desc_t *desc = zbash_find_cmd_desc(cmd_name);
    if (!desc) {
        desc = zbash_jit_man_lookup(cmd_name);
    }
    if (!desc) return false;

    /* 3. Determine Terminal Dimensions */
    struct winsize ws;
    int rows = 24;
    int cols = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_row > 10) rows = ws.ws_row;
        if (ws.ws_col > 30) cols = ws.ws_col;
    }

    /* 4. Switch to Alternate Screen Buffer with hidden cursor */
    printf("\033[?1049h\033[?25l");
    fflush(stdout);

    size_t total_flags = count_flags(desc);
    int flag_rows_visible = (rows - 6 > 18 ? 18 : rows - 6) - 6;
    if (flag_rows_visible < 1) flag_rows_visible = 1;

    size_t scroll_offset = 0;
    draw_box(desc, rows, cols, scroll_offset);

    /* 5. Modal Event Loop for Navigation & Dismiss */
    while (1) {
        char key = 0;
        if (read(STDIN_FILENO, &key, 1) <= 0) break;

        /* Exit keys: q, Esc (standalone), Enter, Ctrl-C, Space */
        if (key == 'q' || key == 'Q' || key == '\r' || key == '\n' || key == 3 || key == ' ') {
            break;
        }

        /* Escape sequence parsing */
        if (key == 27) {
            char seq[4] = {0};
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) break; /* Standalone ESC -> close */
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) break;

            if (seq[0] == '[') {
                if (seq[1] == 'A') { /* Up arrow */
                    if (scroll_offset > 0) {
                        scroll_offset--;
                        draw_box(desc, rows, cols, scroll_offset);
                    }
                    continue;
                }
                if (seq[1] == 'B') { /* Down arrow */
                    if (scroll_offset + flag_rows_visible < total_flags) {
                        scroll_offset++;
                        draw_box(desc, rows, cols, scroll_offset);
                    }
                    continue;
                }
            }
            break;
        }

        /* Vim-style navigation: k (Up), j (Down) */
        if (key == 'k' && scroll_offset > 0) {
            scroll_offset--;
            draw_box(desc, rows, cols, scroll_offset);
            continue;
        }
        if (key == 'j' && scroll_offset + flag_rows_visible < total_flags) {
            scroll_offset++;
            draw_box(desc, rows, cols, scroll_offset);
            continue;
        }
    }

    /* 6. Return to Normal Screen Buffer and restore cursor */
    printf("\033[?1049l\033[?25h");
    fflush(stdout);

    return true;
}
