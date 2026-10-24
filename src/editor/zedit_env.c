/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "zedit.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <spawn.h>

extern char **environ;

zedit_env_t zedit_detect_environment(void) {
#if defined(__APPLE__)
    const char *ssh_client = getenv("SSH_CLIENT");
    const char *ssh_tty = getenv("SSH_TTY");

    /* If running via SSH without X11/GUI display, treat as headless */
    if ((ssh_client || ssh_tty) && !getenv("DISPLAY")) {
        return ZEDIT_ENV_HEADLESS;
    }

    /* On macOS with WindowServer active, GUI is available */
    return ZEDIT_ENV_MACOS_GUI;

#else
    /* 1. Check Wayland Compositor */
    const char *wayland_disp = getenv("WAYLAND_DISPLAY");
    if (wayland_disp && wayland_disp[0]) {
        const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
        if (runtime_dir) {
            char wayland_sock[512];
            snprintf(wayland_sock, sizeof(wayland_sock), "%s/%s", runtime_dir, wayland_disp);
            if (access(wayland_sock, R_OK | W_OK) == 0) {
                return ZEDIT_ENV_LINUX_GUI;
            }
        }
    }

    /* 2. Check X11 Server (DISPLAY is empty if startx hasn't been run) */
    const char *x11_disp = getenv("DISPLAY");
    if (x11_disp && x11_disp[0]) {
        const char *colon = strchr(x11_disp, ':');
        if (colon) {
            int disp_num = atoi(colon + 1);
            char x11_sock[256];
            snprintf(x11_sock, sizeof(x11_sock), "/tmp/.X11-unix/X%d", disp_num);
            if (access(x11_sock, R_OK | W_OK) == 0) {
                return ZEDIT_ENV_LINUX_GUI;
            }
        }
    }

    /* 3. Check Session Type */
    const char *session_type = getenv("XDG_SESSION_TYPE");
    if (session_type && strcmp(session_type, "tty") == 0) {
        return ZEDIT_ENV_HEADLESS;
    }

    /* 4. Check Virtual Console (/dev/tty1 - /dev/tty6) */
    if (isatty(STDIN_FILENO)) {
        char *tty = ttyname(STDIN_FILENO);
        if (tty && strncmp(tty, "/dev/tty", 8) == 0 && tty[8] >= '0' && tty[8] <= '9') {
            return ZEDIT_ENV_HEADLESS;
        }
    }

    return ZEDIT_ENV_HEADLESS;
#endif
}

bool zedit_spawn_gui(const char *filepath) {
    if (!filepath || !*filepath) return false;

    char gui_bin[1024] = {0};
    bool found = false;

    /* 1. Explicit override via environment variable */
    const char *custom_path = getenv("ZEDIT_GUI_BIN");
    if (custom_path && access(custom_path, X_OK) == 0) {
        strncpy(gui_bin, custom_path, sizeof(gui_bin) - 1);
        found = true;
    }

    /* 2. Check ~/.local/bin/zedit-ui */
    if (!found) {
        const char *home = getenv("HOME");
        if (home) {
            snprintf(gui_bin, sizeof(gui_bin), "%s/.local/bin/zedit-ui", home);
            if (access(gui_bin, X_OK) == 0) found = true;
        }
    }

    /* 3. Check /Volumes/Mac Backup/zero-tools/zeditor/bin/zedit-ui */
    if (!found) {
        const char *zdir = "/Volumes/Mac Backup/zero-tools/zeditor/bin/zedit-ui";
        if (access(zdir, X_OK) == 0) {
            strncpy(gui_bin, zdir, sizeof(gui_bin) - 1);
            found = true;
        }
    }

    /* 4. Check system PATH */
    if (!found && access("zedit-ui", X_OK) == 0) {
        strncpy(gui_bin, "zedit-ui", sizeof(gui_bin) - 1);
        found = true;
    }

    if (!found) return false;

    /* Spawn zedit-ui as a background process */
    pid_t pid = 0;
    char *argv[] = { gui_bin, (char *)filepath, NULL };

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

    if (posix_spawnp(&pid, gui_bin, &actions, NULL, argv, environ) == 0) {
        posix_spawn_file_actions_destroy(&actions);
        printf("\033[1;36m⚡ [zbash]\033[0m Launched zedit-ui (PID %d) for '%s'\n", (int)pid, filepath);
        fflush(stdout);
        return true;
    }

    posix_spawn_file_actions_destroy(&actions);
    return false;
}
