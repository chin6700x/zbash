/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include <stdio.h>
#include <stdlib.h>

static void strprint(const char *str) {
    if (!str) return;
    for (const unsigned char *s = (const unsigned char *)str; *s; s++) {
        if (*s < ' ') {
            putchar('^');
            putchar(*s + 64);
        } else if (*s == 127) {
            putchar('^');
            putchar('?');
        } else {
            putchar(*s);
        }
    }
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printf("argv[%d] = <", i);
        strprint(argv[i]);
        printf(">\n");
    }
    return 0;
}
