/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "autocomplete.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(void) {
    printf("\033[1;36m=== Testing zbash Real-Time Autocomplete & Ghost Text Engine ===\033[0m\n\n");

    /* 1. Initialize Prefix Index */
    zbash_prefix_index_init();

    /* 2. Test Ghost Text for 'pw' -> 'pwd' */
    zbash_candidate_t ghost1;
    bool has_ghost1 = zbash_find_inline_ghost(NULL, "pw", 2, 2, &ghost1);
    TEST_ASSERT(has_ghost1 == true, "Find inline ghost text for 'pw'");
    TEST_ASSERT(strcmp(ghost1.full_text, "pwd") == 0, "Ghost candidate full_text is 'pwd'");
    TEST_ASSERT(strcmp(ghost1.display_suffix, "d") == 0, "Ghost display_suffix is 'd'");

    /* 3. Test Prefix Matches for 'who' (Ambiguity Case: who, whoami, whois) */
    zbash_candidate_list_t list_who;
    size_t count_who = zbash_get_completions(NULL, "who", 3, 3, &list_who);
    TEST_ASSERT(count_who >= 2, "Prefix 'who' yields >= 2 candidates (who, whoami, whois)");
    bool found_who = false, found_whoami = false;
    for (size_t i = 0; i < list_who.count; i++) {
        if (strcmp(list_who.items[i].full_text, "who") == 0) found_who = true;
        if (strcmp(list_who.items[i].full_text, "whoami") == 0) found_whoami = true;
    }
    TEST_ASSERT(found_who == true, "Candidates list includes 'who'");
    TEST_ASSERT(found_whoami == true, "Candidates list includes 'whoami'");
    zbash_candidate_list_free(&list_who);

    /* 4. Test Builtin 'read' and 'readonly' */
    zbash_candidate_list_t list_read;
    size_t count_read = zbash_get_completions(NULL, "read", 4, 4, &list_read);
    TEST_ASSERT(count_read >= 2, "Prefix 'read' yields 'read' and 'readonly'");
    zbash_candidate_list_free(&list_read);

    /* 5. Test Flag Autocomplete (from Phase 2 flag_db) */
    zbash_candidate_list_t list_flags;
    size_t count_flags = zbash_get_completions(NULL, "tar -c", 6, 6, &list_flags);
    TEST_ASSERT(count_flags >= 1, "Flag completion for 'tar -c' yields candidates");
    if (count_flags >= 1) {
        TEST_ASSERT(strstr(list_flags.items[0].full_text, "-c") != NULL, "Flag candidate text contains -c");
    }
    zbash_candidate_list_free(&list_flags);

    /* 6. Test File & Directory Path Completion */
    zbash_candidate_list_t list_path;
    size_t count_path = zbash_get_completions(NULL, "ls inc", 6, 6, &list_path);
    TEST_ASSERT(count_path >= 1, "Path completion for 'ls inc' matches 'include/'");
    if (count_path >= 1) {
        TEST_ASSERT(strstr(list_path.items[0].full_text, "include/") != NULL, "Directory match appends trailing slash '/'");
    }
    zbash_candidate_list_free(&list_path);

    /* 7. Test Nonexistent Prefix */
    zbash_candidate_list_t list_none;
    size_t count_none = zbash_get_completions(NULL, "xyznonexistentprefix999", 23, 23, &list_none);
    TEST_ASSERT(count_none == 0, "Nonexistent prefix returns 0 candidates");
    zbash_candidate_list_free(&list_none);

    /* 8. Test History Recency Ghost Priority */
    zbash_state_t mock_state;
    memset(&mock_state, 0, sizeof(mock_state));
    char *mock_hist[] = {"pwd", "whoami"};
    mock_state.history_lines = mock_hist;
    mock_state.history_count = 2;

    zbash_candidate_t ghost_hist;
    bool has_hist_ghost = zbash_find_inline_ghost(&mock_state, "who", 3, 3, &ghost_hist);
    TEST_ASSERT(has_hist_ghost == true, "History-driven ghost text for 'who'");
    TEST_ASSERT(strcmp(ghost_hist.full_text, "whoami") == 0, "History prioritizes recent 'whoami' over 'who'");
    TEST_ASSERT(strcmp(ghost_hist.display_suffix, "ami") == 0, "Ghost suffix for recent command is 'ami'");

    /* 9. Test Enter-Accept Paradigm (Simulation) */
    char enter_line[128] = "who";
    size_t enter_len = 3;
    size_t enter_cursor = 3;
    zbash_candidate_t enter_ghost;
    if (zbash_find_inline_ghost(NULL, enter_line, enter_len, enter_cursor, &enter_ghost)) {
        size_t slen = strlen(enter_ghost.display_suffix);
        memcpy(enter_line + enter_len, enter_ghost.display_suffix, slen);
        enter_len += slen;
        enter_line[enter_len] = '\0';
    }
    TEST_ASSERT(strcmp(enter_line, "whois") == 0 || strcmp(enter_line, "whoami") == 0, "Enter-Accept absorbs ghost text into executable buffer");
    TEST_ASSERT(enter_len > 3, "Buffer length updated after Enter-Accept absorption");

    /* 10. Test Circular Inline Tab Cycling */
    zbash_tab_cycle_reset();
    TEST_ASSERT(zbash_tab_cycle_is_active() == false, "Tab cycle initially inactive");

    char cycle_line[128] = "who";
    size_t cycle_len = 3;
    size_t cycle_cursor = 3;
    char ribbon_buf[512] = {0};

    /* 1st Tab -> First candidate */
    bool c1 = zbash_tab_cycle_next(NULL, cycle_line, &cycle_len, &cycle_cursor, ribbon_buf, sizeof(ribbon_buf));
    TEST_ASSERT(c1 == true, "1st Tab activates cycle and replaces token");
    TEST_ASSERT(zbash_tab_cycle_is_active() == true, "Tab cycle is active after 1st Tab");
    TEST_ASSERT(strstr(ribbon_buf, "[1/") != NULL, "Ribbon shows [1/N] on 1st Tab");
    TEST_ASSERT(strstr(ribbon_buf, "▸") != NULL, "Ribbon shows active marker ▸");
    char first_cand[64];
    strncpy(first_cand, cycle_line, sizeof(first_cand) - 1);

    /* 2nd Tab -> Second candidate */
    bool c2 = zbash_tab_cycle_next(NULL, cycle_line, &cycle_len, &cycle_cursor, ribbon_buf, sizeof(ribbon_buf));
    TEST_ASSERT(c2 == true, "2nd Tab advances to next candidate");
    TEST_ASSERT(strstr(ribbon_buf, "[2/") != NULL, "Ribbon shows [2/N] on 2nd Tab");

    /* 3rd Tab -> Third candidate (or wrap) */
    bool c3 = zbash_tab_cycle_next(NULL, cycle_line, &cycle_len, &cycle_cursor, ribbon_buf, sizeof(ribbon_buf));
    TEST_ASSERT(c3 == true, "3rd Tab advances candidate");

    /* Test Round-Robin Wrap-Around: keep tabbing until [1/N] appears again */
    for (int t = 0; t < 10; t++) {
        zbash_tab_cycle_next(NULL, cycle_line, &cycle_len, &cycle_cursor, ribbon_buf, sizeof(ribbon_buf));
        if (strstr(ribbon_buf, "[1/") != NULL) break;
    }
    TEST_ASSERT(strstr(ribbon_buf, "[1/") != NULL, "Tab cycling wraps around to candidate [1/N] (Round-Robin)");
    TEST_ASSERT(strcmp(cycle_line, first_cand) == 0, "Line buffer restored to 1st candidate on wrap-around");

    /* 11. Test Tab Cycle Cancel (Esc key) */
    bool cancelled = zbash_tab_cycle_cancel(cycle_line, &cycle_len, &cycle_cursor);
    TEST_ASSERT(cancelled == true, "zbash_tab_cycle_cancel returns true when cycle was active");
    TEST_ASSERT(strcmp(cycle_line, "who") == 0, "Esc restores original prefix 'who'");
    TEST_ASSERT(cycle_len == 3 && cycle_cursor == 3, "Length and cursor restored to original prefix");
    TEST_ASSERT(zbash_tab_cycle_is_active() == false, "Tab cycle reset after cancel");

    /* 12. Test History Quality Sanitizer (Rejecting Typo 127 commands) */
    zbash_state_t typo_state;
    memset(&typo_state, 0, sizeof(typo_state));
    /* whoiam is a typo (doesn't exist in PATH or builtins) */
    /* whoami is valid */
    char *typo_hist[] = {"whoiam", "whoami"};
    typo_state.history_lines = typo_hist;
    typo_state.history_count = 2;

    zbash_candidate_t ghost_clean;
    bool has_clean = zbash_find_inline_ghost(&typo_state, "who", 3, 3, &ghost_clean);
    TEST_ASSERT(has_clean == true, "Ghost text found with history containing typos");
    TEST_ASSERT(strcmp(ghost_clean.full_text, "whoami") == 0, "History Sanitizer picked valid 'whoami'");
    TEST_ASSERT(strcmp(ghost_clean.full_text, "whoiam") != 0, "History Sanitizer rejected typo 'whoiam'");

    /* Test history containing ONLY typos */
    char *only_typos[] = {"howami", "whoiam"};
    typo_state.history_lines = only_typos;
    typo_state.history_count = 2;

    zbash_candidate_t ghost_typo_only;
    /* "how" doesn't match any valid command in PATH/builtins */
    bool has_typo = zbash_find_inline_ghost(&typo_state, "how", 3, 3, &ghost_typo_only);
    TEST_ASSERT(has_typo == false, "History Sanitizer rejects history containing only invalid typo 'howami'");

    /* 13. Test Trailing Space Suppression */
    zbash_candidate_t ghost_space;
    bool has_space_ghost = zbash_find_inline_ghost(NULL, "who ", 4, 4, &ghost_space);
    TEST_ASSERT(has_space_ghost == false, "Trailing space suppresses inline ghost text");

    printf("\n\033[1;36m========================================================\033[0m\n");
    printf("Total Tests: %d | Passed: \033[1;32m%d\033[0m | Failed: \033[1;%sm%d\033[0m\n",
           s_pass + s_fail, s_pass, s_fail > 0 ? "31" : "32", s_fail);

    return (s_fail == 0) ? 0 : 1;
}
