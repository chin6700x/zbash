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

#if defined(__aarch64__) || defined(__arm64__)
#define ZBASH_ARCH "ARM64"
#elif defined(__x86_64__)
#define ZBASH_ARCH "x86_64"
#elif defined(__EMSCRIPTEN__)
#define ZBASH_ARCH "WASM"
#else
#define ZBASH_ARCH "POSIX"
#endif

#define ZEPHYR_BANNER "V.0.0.1 (" ZBASH_ARCH ") © 2026 Wirot Chookeaw  https://github.com/chin6700x"

static inline void zephyr_show_version(void) {
    puts(ZEPHYR_BANNER);
}

static inline void zephyr_show_help(const char *tool_name, const char *usage_str) {
    printf("╭────────────────────────────────────────────────────────────────────────────╮\n");
    printf("│  ZEPHYR %s Shell Engine — V.0.0.1 (%s)                               │\n", tool_name, ZBASH_ARCH);
    printf("│  Bare-Metal Silicon Velocity · POSIX Standard · Native ISO C11             │\n");
    printf("│  © 2026 Wirot Chookeaw (chin6700x)                                         │\n");
    printf("╰────────────────────────────────────────────────────────────────────────────╯\n");
    printf("usage: %s %s\n\n", tool_name, usage_str);
    printf("Options:\n");
    printf("  -c <string>         Execute commands from string\n");
    printf("  -e                  Exit immediately if a command exits with non-zero status\n");
    printf("  -x                  Print commands and arguments as they are executed\n");
    printf("  -n                  Read commands but do not execute them (syntax check)\n");
    printf("  -v                  Print shell input lines as they are read\n");
    printf("  -i                  Force interactive shell mode\n");
    printf("  --norc              Do not load ~/.zbashrc startup file\n");
    printf("  -help, --help       Display this help information and exit\n");
    printf("  -version, --version Display version and architecture information\n");
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
