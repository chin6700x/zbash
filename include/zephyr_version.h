/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZEPHYR_VERSION_H
#define ZEPHYR_VERSION_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define ZEPHYR_BANNER "V.0.0.1 © 2026 Wirot Chookeaw  https://github.com/chin6700x"

static inline void zephyr_show_version(void) {
    puts(ZEPHYR_BANNER);
}

static inline void zephyr_show_help(const char *tool_name, const char *usage_str) {
    puts(ZEPHYR_BANNER);
    printf("usage: %s %s\n", tool_name, usage_str);
}

static inline bool zephyr_handle_meta(int argc, char **argv, const char *tool_name, const char *usage_str) {
    if (argc < 2) return false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) {
            break;
        }
        if (strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "--version") == 0) {
            zephyr_show_version();
            return true;
        }
        if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
            zephyr_show_help(tool_name, usage_str);
            return true;
        }
    }
    return false;
}

#endif /* ZEPHYR_VERSION_H */
