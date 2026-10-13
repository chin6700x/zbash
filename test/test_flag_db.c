/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "flag_db.h"
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
    printf("\033[1;36m=== Testing zbash Flag Introspection & Ribbon Engine ===\033[0m\n\n");

    /* 1. Command Lookup Tests */
    const zbash_cmd_desc_t *desc_read = zbash_find_cmd_desc("read");
    TEST_ASSERT(desc_read != NULL, "Lookup builtin 'read'");
    TEST_ASSERT(desc_read && strstr(desc_read->synopsis, "-p prompt") != NULL, "'read' synopsis contains -p");

    const zbash_cmd_desc_t *desc_tar = zbash_find_cmd_desc("tar");
    TEST_ASSERT(desc_tar != NULL, "Lookup tool 'tar'");

    const zbash_cmd_desc_t *desc_path = zbash_find_cmd_desc("/bin/ls");
    TEST_ASSERT(desc_path != NULL, "Lookup with full path '/bin/ls' resolves to 'ls'");

    const zbash_cmd_desc_t *desc_nonexistent = zbash_find_cmd_desc("some_random_nonexistent_command_12345");
    TEST_ASSERT(desc_nonexistent == NULL, "Lookup nonexistent command returns NULL");

    /* 2. Flag Lookup Tests */
    const zbash_flag_entry_t *flag_p = zbash_find_flag(desc_read, 'p', NULL);
    TEST_ASSERT(flag_p != NULL, "Find flag -p on 'read'");
    TEST_ASSERT(flag_p && strcmp(flag_p->arg_desc, "<prompt>") == 0, "Flag -p has argument <prompt>");

    const zbash_flag_entry_t *flag_create = zbash_find_flag(desc_tar, 'c', "--create");
    TEST_ASSERT(flag_create != NULL, "Find flag 'c' / '--create' on 'tar'");
    TEST_ASSERT(flag_create && strstr(flag_create->description, "create") != NULL, "Flag --create description valid");

    /* 3. Ribbon Generation: Command Synopsis */
    char *ribbon1 = zbash_inspect_ribbon("tar", 3, 3);
    TEST_ASSERT(ribbon1 != NULL, "Ribbon for 'tar' (synopsis mode)");
    TEST_ASSERT(ribbon1 && strstr(ribbon1, "tape archives") != NULL, "Ribbon 'tar' contains summary");
    if (ribbon1) free(ribbon1);

    /* 4. Ribbon Generation: Single Flag */
    char *ribbon2 = zbash_inspect_ribbon("read -p", 7, 7);
    TEST_ASSERT(ribbon2 != NULL, "Ribbon for 'read -p'");
    TEST_ASSERT(ribbon2 && strstr(ribbon2, "<prompt>") != NULL, "Ribbon 'read -p' shows prompt arg");
    if (ribbon2) free(ribbon2);

    /* 5. Ribbon Generation: Trailing Space expecting Argument */
    char *ribbon3 = zbash_inspect_ribbon("read -p ", 8, 8);
    TEST_ASSERT(ribbon3 != NULL, "Ribbon for 'read -p ' (expecting argument)");
    TEST_ASSERT(ribbon3 && strstr(ribbon3, "<prompt>") != NULL, "Ribbon 'read -p ' retains argument hint");
    if (ribbon3) free(ribbon3);

    /* 6. Ribbon Generation: Clustered Short Flags */
    char *ribbon4 = zbash_inspect_ribbon("tar -czvf", 9, 9);
    TEST_ASSERT(ribbon4 != NULL, "Ribbon for 'tar -czvf' (cluster mode)");
    TEST_ASSERT(ribbon4 && strstr(ribbon4, "-c") && strstr(ribbon4, "-z") && strstr(ribbon4, "-v") && strstr(ribbon4, "-f"),
                "Ribbon cluster parses all 4 flags (-c, -z, -v, -f)");
    if (ribbon4) free(ribbon4);

    /* 7. Ribbon Generation: Long Flag */
    const char *cmd_long = "grep --extended-regexp";
    char *ribbon5 = zbash_inspect_ribbon(cmd_long, strlen(cmd_long), strlen(cmd_long));
    TEST_ASSERT(ribbon5 != NULL, "Ribbon for 'grep --extended-regexp'");
    TEST_ASSERT(ribbon5 && strstr(ribbon5, "extended regular expression") != NULL,
                "Ribbon long opt displays accurate description");
    if (ribbon5) free(ribbon5);

    /* 8. Ribbon Generation: Pipeline Boundary Recognition */
    char *ribbon6 = zbash_inspect_ribbon("cat file.txt | grep -i", 22, 22);
    TEST_ASSERT(ribbon6 != NULL, "Ribbon across pipeline 'cat file.txt | grep -i'");
    TEST_ASSERT(ribbon6 && strstr(ribbon6, "ignore case") != NULL,
                "Ribbon correctly isolates active command 'grep' after pipe");
    if (ribbon6) free(ribbon6);

    /* 9. Ribbon Generation: Unknown command / Empty */
    char *ribbon7 = zbash_inspect_ribbon("unknownxyz -abc", 15, 15);
    TEST_ASSERT(ribbon7 == NULL, "Ribbon for unknown command returns NULL (unobtrusive)");

    char *ribbon8 = zbash_inspect_ribbon("", 0, 0);
    TEST_ASSERT(ribbon8 == NULL, "Ribbon for empty line returns NULL");

    /* 10. Top Unix Tools Ribbon Tests */
    char *r_ls = zbash_inspect_ribbon("ls -la", 6, 6);
    TEST_ASSERT(r_ls != NULL && strstr(r_ls, "-l") && strstr(r_ls, "-a"), "Ribbon for 'ls -la'");
    if (r_ls) free(r_ls);

    char *r_git = zbash_inspect_ribbon("git commit -m", 13, 13);
    TEST_ASSERT(r_git != NULL && strstr(r_git, "<msg>"), "Ribbon for 'git commit -m'");
    if (r_git) free(r_git);

    char *r_curl = zbash_inspect_ribbon("curl -L", 7, 7);
    TEST_ASSERT(r_curl != NULL && strstr(r_curl, "redirects"), "Ribbon for 'curl -L'");
    if (r_curl) free(r_curl);

    printf("\n\033[1;36m========================================================\033[0m\n");
    printf("Total Tests: %d | Passed: \033[1;32m%d\033[0m | Failed: \033[1;%sm%d\033[0m\n",
           s_pass + s_fail, s_pass, s_fail > 0 ? "31" : "32", s_fail);

    return (s_fail == 0) ? 0 : 1;
}
