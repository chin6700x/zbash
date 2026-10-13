/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "highlight.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int pass_count = 0;
static int total_count = 0;

static void assert_contains(const char *test_name, const char *haystack, const char *needle) {
    total_count++;
    if (haystack && strstr(haystack, needle)) {
        printf("  [PASS] %s\n", test_name);
        pass_count++;
    } else {
        printf("  [FAIL] %s: Expected substring '%s' not found in: '%s'\n", test_name, needle, haystack ? haystack : "(null)");
    }
}

int main(void) {
    printf("=== Testing Monaco-Grade Real-Time Highlighting Engine ===\n");

    zbash_state_t state;
    memset(&state, 0, sizeof(state));
    zbash_path_cache_init();

    /* Test 1: Valid builtin command (Cyan) */
    char *h1 = zbash_highlight_line(&state, "cd /tmp", 7, 0);
    assert_contains("Builtin command highlighted in Cyan", h1, "\033[1;38;5;51mcd");
    free(h1);

    /* Test 2: Valid PATH command (Green) */
    char *h2 = zbash_highlight_line(&state, "ls -la", 6, 0);
    assert_contains("Valid PATH binary highlighted in Green", h2, "\033[1;38;5;48mls");
    assert_contains("Flag highlighted in Amber Yellow", h2, "\033[38;5;221m-la");
    free(h2);

    /* Test 3: Invalid / Typo command (Red Underline) */
    char *h3 = zbash_highlight_line(&state, "nonexistent_cmd_xyz123 arg", 26, 0);
    assert_contains("Invalid command highlighted in Red Underline", h3, "\033[4;38;5;196mnonexistent_cmd_xyz123");
    free(h3);

    /* Test 4: Closed quotes vs Unclosed quotes */
    char *h4_closed = zbash_highlight_line(&state, "echo 'hello world'", 18, 0);
    assert_contains("Closed single quote in Warm Orange", h4_closed, "\033[38;5;214m'hello world'");
    free(h4_closed);

    char *h4_unclosed = zbash_highlight_line(&state, "echo 'hello world", 17, 0);
    assert_contains("Unclosed single quote in Alert Gray Underline", h4_unclosed, "\033[4;38;5;244m'hello world");
    free(h4_unclosed);

    /* Test 5: Operators (| and &&) in Neon Magenta */
    char *h5 = zbash_highlight_line(&state, "cat file | grep 'pattern' && echo done", 38, 0);
    assert_contains("Pipe operator in Neon Magenta", h5, "\033[1;38;5;201m|");
    assert_contains("AND operator in Neon Magenta", h5, "\033[1;38;5;201m&&");
    /* After pipe, 'grep' is at command position -> Valid green */
    assert_contains("Command after pipe is Green", h5, "\033[1;38;5;48mgrep");
    free(h5);

    /* Test 6: Variables ($VAR) in Sky Blue */
    char *h6 = zbash_highlight_line(&state, "echo $USER ${HOME}", 18, 0);
    assert_contains("Variable $USER in Sky Blue", h6, "\033[1;38;5;75m$USER");
    assert_contains("Variable ${HOME} in Sky Blue", h6, "\033[1;38;5;75m${HOME}");
    free(h6);

    /* Test 7: Comment (# ...) in Titanium Gray */
    char *h7 = zbash_highlight_line(&state, "echo test # this is a comment", 29, 0);
    assert_contains("Comment in Titanium Gray", h7, "\033[38;5;242m# this is a comment");
    free(h7);

    zbash_path_cache_free();

    printf("\nHighlighting Engine Results: %d / %d PASS (%.1f%%)\n",
           pass_count, total_count, (double)pass_count / total_count * 100.0);

    return (pass_count == total_count) ? 0 : 1;
}
