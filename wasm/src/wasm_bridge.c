/*
 * zbash WebAssembly Bridge
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * Description: Emscripten/WASI Export Interface connecting zbash execution engine
 *              directly to modern Web Browser JavaScript, OPFS, and Web Workers.
 */

#include "zbash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <emscripten/emscripten.h>

static zbash_state_t g_wasm_state;
static bool g_initialized = false;

#define WASM_OUT_CAP (256 * 1024)
#define WASM_ERR_CAP (64 * 1024)

static char g_stdout_buf[WASM_OUT_CAP];
static size_t g_stdout_len = 0;

static char g_stderr_buf[WASM_ERR_CAP];
static size_t g_stderr_len = 0;

/* JavaScript callback for streaming output */
EM_JS(void, js_on_terminal_output, (const char* str, int is_err), {
    if (typeof window !== 'undefined' && window.zbashOnOutput) {
        window.zbashOnOutput(UTF8ToString(str), is_err === 1);
    }
});

/* JavaScript callback for external command dispatch */
EM_JS(int, js_on_external_command, (const char* cmd_name, const char* full_args), {
    if (typeof window !== 'undefined' && window.zbashOnExternal) {
        return window.zbashOnExternal(UTF8ToString(cmd_name), UTF8ToString(full_args));
    }
    return -1;
});

EMSCRIPTEN_KEEPALIVE
int zbash_wasm_init(void) {
    if (!g_initialized) {
        char *dummy_argv[] = { "zbash", NULL };
        zbash_init(&g_wasm_state, 1, dummy_argv);
        setenv("SHELL", "/bin/zbash", 1);
        setenv("USER", "zephyr", 1);
        setenv("HOME", "/home/zephyr", 1);
        setenv("TERM", "xterm-256color", 1);
        setenv("PATH", "/bin:/usr/bin:/local/bin", 1);
        g_initialized = true;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
void zbash_wasm_clear_output(void) {
    g_stdout_buf[0] = '\0';
    g_stdout_len = 0;
    g_stderr_buf[0] = '\0';
    g_stderr_len = 0;
}

EMSCRIPTEN_KEEPALIVE
const char *zbash_wasm_get_stdout(void) {
    return g_stdout_buf;
}

EMSCRIPTEN_KEEPALIVE
const char *zbash_wasm_get_stderr(void) {
    return g_stderr_buf;
}

/*
 * External command dispatcher for WebAssembly
 */
int zbash_wasm_dispatch_external(const char *cmd_name, int argc, char **argv) {
    if (!cmd_name) return 127;

    /* Combine arguments */
    char full_args[2048] = {0};
    for (int i = 0; i < argc; i++) {
        strcat(full_args, argv[i]);
        if (i < argc - 1) strcat(full_args, " ");
    }

    int js_rc = js_on_external_command(cmd_name, full_args);
    if (js_rc >= 0) {
        return js_rc;
    }

    /* Fallback default tools built-in simulator */
    if (strcmp(cmd_name, "uname") == 0) {
        printf("Darwin Zephyr-Wasm 26.0.0 AppleSilicon/Wasm32\n");
        return 0;
    }
    if (strcmp(cmd_name, "whoami") == 0) {
        printf("zephyr\n");
        return 0;
    }
    if (strcmp(cmd_name, "clear") == 0) {
        printf("\033[2J\033[H");
        return 0;
    }
    if (strcmp(cmd_name, "date") == 0) {
        printf("Thu Oct  8 18:00:00 UTC 2026\n");
        return 0;
    }

    fprintf(stderr, "zbash: %s: command not found\n", cmd_name);
    return 127;
}

EMSCRIPTEN_KEEPALIVE
int zbash_wasm_eval(const char *cmd) {
    if (!cmd) return 0;
    if (!g_initialized) zbash_wasm_init();

    zbash_wasm_clear_output();

    /* Redirect stdout and stderr to in-memory temp file */
    int saved_out = dup(STDOUT_FILENO);
    int saved_err = dup(STDERR_FILENO);

    FILE *out_fp = fopen("/tmp/wasm_out.tmp", "w+");
    FILE *err_fp = fopen("/tmp/wasm_err.tmp", "w+");

    if (out_fp) dup2(fileno(out_fp), STDOUT_FILENO);
    if (err_fp) dup2(fileno(err_fp), STDERR_FILENO);

    int rc = 0;
    FILE *mem_fp = fmemopen((char *)cmd, strlen(cmd), "r");
    if (mem_fp) {
        rc = zbash_run_stream(&g_wasm_state, mem_fp);
        fclose(mem_fp);
    } else {
        rc = zbash_run_line(&g_wasm_state, cmd);
    }

    fflush(stdout);
    fflush(stderr);

    if (saved_out >= 0) { dup2(saved_out, STDOUT_FILENO); close(saved_out); }
    if (saved_err >= 0) { dup2(saved_err, STDERR_FILENO); close(saved_err); }

    /* Read captured output */
    if (out_fp) {
        fseek(out_fp, 0, SEEK_SET);
        size_t nr = fread(g_stdout_buf, 1, sizeof(g_stdout_buf) - 1, out_fp);
        g_stdout_buf[nr] = '\0';
        g_stdout_len = nr;
        fclose(out_fp);
        unlink("/tmp/wasm_out.tmp");
    }

    if (err_fp) {
        fseek(err_fp, 0, SEEK_SET);
        size_t nr = fread(g_stderr_buf, 1, sizeof(g_stderr_buf) - 1, err_fp);
        g_stderr_buf[nr] = '\0';
        g_stderr_len = nr;
        fclose(err_fp);
        unlink("/tmp/wasm_err.tmp");
    }

    /* Stream to JS if output exists */
    if (g_stdout_len > 0) {
        js_on_terminal_output(g_stdout_buf, 0);
    }
    if (g_stderr_len > 0) {
        js_on_terminal_output(g_stderr_buf, 1);
    }

    return rc;
}

int main(int argc, char **argv) {
    zbash_wasm_init();
    if (argc > 1) {
        return zbash_wasm_eval(argv[1]);
    }
    return 0;
}
