/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "hover_box.h"
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
    printf("\033[1;36m=== Testing zbash Phase 3 Hover Box & Phase 4 Dynamic JIT Engine ===\033[0m\n\n");

    /* 1. Dynamic JIT Man-Page Lookup */
    const zbash_cmd_desc_t *jit_ls = zbash_jit_man_lookup("ls");
    TEST_ASSERT(jit_ls != NULL, "Dynamic JIT lookup for 'ls' from manpage");
    TEST_ASSERT(jit_ls && strstr(jit_ls->synopsis, "ls") != NULL, "JIT synopsis extracted correctly");

    /* 2. JIT Cache Deduplication */
    const zbash_cmd_desc_t *jit_ls2 = zbash_jit_man_lookup("ls");
    TEST_ASSERT(jit_ls == jit_ls2, "Second JIT lookup returns cached pointer (O(1))");

    /* 3. Nonexistent command JIT */
    const zbash_cmd_desc_t *jit_none = zbash_jit_man_lookup("nonexistent_utility_12345");
    TEST_ASSERT(jit_none == NULL, "JIT lookup for nonexistent utility returns NULL");

    /* 4. Empty line hover check */
    bool hover_empty = zbash_show_hover_help("", 0, 0);
    TEST_ASSERT(hover_empty == false, "Hover help on empty line returns false");

    /* 5. Unknown command hover check */
    bool hover_unknown = zbash_show_hover_help("xyzrandom123", 12, 12);
    TEST_ASSERT(hover_unknown == false, "Hover help on unknown command returns false");

    printf("\n\033[1;36m========================================================\033[0m\n");
    printf("Total Tests: %d | Passed: \033[1;32m%d\033[0m | Failed: \033[1;%sm%d\033[0m\n",
           s_pass + s_fail, s_pass, s_fail > 0 ? "31" : "32", s_fail);

    return (s_fail == 0) ? 0 : 1;
}
