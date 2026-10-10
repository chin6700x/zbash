/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "zbash.h"
#include "zephyr_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_usage(void) {
    fprintf(stderr, "usage: zbash [-exnv] [-c string | script_file [args...]]\n");
}

int main(int argc, char *argv[]) {
    if (zephyr_handle_meta(argc, argv, "zbash", "[-exnv] [-c string | script_file [args...]]")) {
        return 0;
    }

    zbash_state_t state;
    zbash_init(&state, argc, argv);

    const char *cmd_string = NULL;
    int opt_idx = 1;

    while (opt_idx < argc) {
        if (strcmp(argv[opt_idx], "--") == 0) {
            opt_idx++;
            break;
        }

        if (argv[opt_idx][0] == '-' && argv[opt_idx][1] != '\0') {
            const char *opt = argv[opt_idx] + 1;
            bool stop_parsing = false;

            while (*opt) {
                switch (*opt) {
                    case 'c':
                        if (opt_idx + 1 < argc) {
                            cmd_string = argv[++opt_idx];
                            stop_parsing = true;
                        } else {
                            fprintf(stderr, "zbash: -c: option requires an argument\n");
                            return 2;
                        }
                        break;
                    case 'e': state.errexit = true; break;
                    case 'x': state.xtrace = true; break;
                    case 'n': state.noexec = true; break;
                    case 'v': state.verbose = true; break;
                    case 'r':
                        state.restricted = true;
                        zbash_set_var_attr(&state, "PATH", ZBASH_ATTR_READONLY);
                        zbash_set_var_attr(&state, "SHELL", ZBASH_ATTR_READONLY);
                        zbash_set_var_attr(&state, "ENV", ZBASH_ATTR_READONLY);
                        zbash_set_var_attr(&state, "BASH_ENV", ZBASH_ATTR_READONLY);
                        break;
                    case 's': /* Read stdin */ break;
                    default:
                        fprintf(stderr, "zbash: illegal option -- %c\n", *opt);
                        print_usage();
                        return 2;
                }
                if (stop_parsing) break;
                opt++;
            }
            opt_idx++;
            if (stop_parsing) break;
        } else {
            break;
        }
    }

    if (cmd_string) {
        if (opt_idx < argc) {
            state.argc = argc - opt_idx;
            state.argv = argv + opt_idx;
        } else {
            state.argc = 1;
            state.argv = argv;
        }
        FILE *mem_fp = fmemopen((char *)cmd_string, strlen(cmd_string), "r");
        if (mem_fp) {
            zbash_run_stream(&state, mem_fp);
            fclose(mem_fp);
        } else {
            zbash_run_line(&state, cmd_string);
        }
        int final_code = state.should_exit ? state.exit_code : state.last_status;
        if (state.exit_trap) {
            char *t = state.exit_trap;
            state.exit_trap = NULL;
            state.should_exit = false;
            zbash_run_line(&state, t);
            free(t);
            if (state.should_exit) final_code = state.exit_code;
        }
        return final_code;
    }

    if (opt_idx < argc) {
        const char *script_file = argv[opt_idx];
        state.argc = argc - opt_idx;
        state.argv = argv + opt_idx;
        int rc = zbash_run_file(&state, script_file);
        int final_code = state.should_exit ? state.exit_code : rc;
        if (state.exit_trap) {
            char *t = state.exit_trap;
            state.exit_trap = NULL;
            state.should_exit = false;
            zbash_run_line(&state, t);
            free(t);
            if (state.should_exit) final_code = state.exit_code;
        }
        return final_code;
    }

    state.argc = 1;
    state.argv = argv;
    int rc = zbash_run_interactive(&state);
    int final_code = state.should_exit ? state.exit_code : rc;
    if (state.exit_trap) {
        char *t = state.exit_trap;
        state.exit_trap = NULL;
        state.should_exit = false;
        zbash_run_line(&state, t);
        free(t);
        if (state.should_exit) final_code = state.exit_code;
    }
    return final_code;
}
