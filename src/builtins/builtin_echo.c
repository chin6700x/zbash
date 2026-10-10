/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

int builtin_echo(zbash_state_t *state, int argc, char **argv) {
    bool no_newline = false;
    bool interpret_escapes = (state && state->xpg_echo);
    int idx = 1;

    while (idx < argc && argv[idx][0] == '-' && argv[idx][1] != '\0') {
        bool valid = true;
        for (int k = 1; argv[idx][k] != '\0'; k++) {
            if (argv[idx][k] != 'n' && argv[idx][k] != 'e' && argv[idx][k] != 'E') {
                valid = false;
                break;
            }
        }
        if (!valid) break;

        for (int k = 1; argv[idx][k] != '\0'; k++) {
            if (argv[idx][k] == 'n') no_newline = true;
            else if (argv[idx][k] == 'e') interpret_escapes = true;
            else if (argv[idx][k] == 'E') interpret_escapes = false;
        }
        idx++;
    }

    for (int i = idx; i < argc; i++) {
        const char *s = argv[i];
        if (interpret_escapes) {
            bool halt = false;
            for (size_t j = 0; s[j] != '\0' && !halt; j++) {
                if (s[j] == '\\' && s[j + 1] != '\0') {
                    j++;
                    char next = s[j];
                    if (next == 'c') {
                        return 0; /* \c halts all output, suppresses newline */
                    } else if (next == 'n') putchar('\n');
                    else if (next == 't') putchar('\t');
                    else if (next == 'r') putchar('\r');
                    else if (next == '\\') putchar('\\');
                    else if (next == 'a') putchar('\a');
                    else if (next == 'b') putchar('\b');
                    else if (next == 'e' || next == 'E') putchar(0x1B);
                    else if (next == 'f') putchar('\f');
                    else if (next == 'v') putchar('\v');
                    else if (next == '0') {
                        int val = 0;
                        int digits = 0;
                        while (digits < 3 && s[j + 1] >= '0' && s[j + 1] <= '7') {
                            val = (val << 3) + (s[++j] - '0');
                            digits++;
                        }
                        putchar((char)(unsigned char)val);
                    } else if (next >= '1' && next <= '7') {
                        int val = next - '0';
                        int digits = 1;
                        while (digits < 3 && s[j + 1] >= '0' && s[j + 1] <= '7') {
                            val = (val << 3) + (s[++j] - '0');
                            digits++;
                        }
                        putchar((char)(unsigned char)val);
                    } else if (next == 'x') {
                        int val = 0;
                        int digits = 0;
                        while (digits < 2 && isxdigit((unsigned char)s[j + 1])) {
                            char h = s[++j];
                            int hex_val = isdigit((unsigned char)h) ? (h - '0') : (tolower((unsigned char)h) - 'a' + 10);
                            val = (val << 4) + hex_val;
                            digits++;
                        }
                        if (digits == 0) {
                            putchar('\\');
                            putchar('x');
                        } else {
                            putchar((char)(unsigned char)val);
                        }
                    } else {
                        putchar('\\');
                        putchar(next);
                    }
                } else {
                    putchar(s[j]);
                }
            }
        } else {
            fputs(s, stdout);
        }
        if (i < argc - 1) {
            putchar(' ');
        }
    }

    if (!no_newline) {
        putchar('\n');
    }
    fflush(stdout);
    return 0;
}
