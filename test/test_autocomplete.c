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

    printf("\n\033[1;36m========================================================\033[0m\n");
    printf("Total Tests: %d | Passed: \033[1;32m%d\033[0m | Failed: \033[1;%sm%d\033[0m\n",
           s_pass + s_fail, s_pass, s_fail > 0 ? "31" : "32", s_fail);

    return (s_fail == 0) ? 0 : 1;
}
