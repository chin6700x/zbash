/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "zedit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CLR_RESET      "\033[0m"
#define CLR_KEYWORD    "\033[1;36m"            /* Neon Cyan */
#define CLR_PREPROC    "\033[1;35m"            /* Neon Magenta */
#define CLR_STRING     "\033[38;2;255;170;80m" /* Warm Orange */
#define CLR_COMMENT    "\033[38;2;120;135;155m"/* Titanium Gray */
#define CLR_NUMBER     "\033[38;2;140;220;140m"/* Mint Green */
#define CLR_VAR        "\033[38;2;100;190;255m"/* Sky Blue */

static bool is_word_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static bool match_keyword(const char *p, const char *kw, size_t kw_len) {
    if (strncmp(p, kw, kw_len) == 0) {
        if (!is_word_char(p[kw_len])) {
            return true;
        }
    }
    return false;
}

static const char *C_KEYWORDS[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "int", "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
    "bool", "true", "false", "NULL", "size_t", "ssize_t", "uint8_t", "uint16_t",
    "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t", NULL
};

static const char *PY_KEYWORDS[] = {
    "and", "as", "assert", "async", "await", "break", "class", "continue",
    "def", "del", "elif", "else", "except", "finally", "for", "from",
    "global", "if", "import", "in", "is", "lambda", "nonlocal", "not",
    "or", "pass", "raise", "return", "try", "while", "with", "yield",
    "True", "False", "None", "self", NULL
};

static const char *SH_KEYWORDS[] = {
    "if", "then", "else", "elif", "fi", "for", "in", "do", "done",
    "case", "esac", "while", "until", "function", "select", "time",
    "return", "local", "export", "readonly", "declare", "typeset", NULL
};

static const char *RS_KEYWORDS[] = {
    "fn", "let", "mut", "impl", "struct", "enum", "match", "pub", "use",
    "mod", "trait", "where", "type", "unsafe", "return", "if", "else",
    "loop", "while", "for", "in", "move", "ref", "static", "const", NULL
};

static const char *GO_KEYWORDS[] = {
    "func", "package", "import", "var", "type", "struct", "interface",
    "defer", "go", "chan", "return", "if", "else", "for", "range",
    "switch", "case", "fallthrough", "default", "map", "select", NULL
};

static const char **get_keywords_for_ext(const char *ext) {
    if (!ext) return NULL;
    if (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0 || strcmp(ext, "cpp") == 0 || strcmp(ext, "cc") == 0) {
        return C_KEYWORDS;
    }
    if (strcmp(ext, "py") == 0) {
        return PY_KEYWORDS;
    }
    if (strcmp(ext, "sh") == 0 || strcmp(ext, "bash") == 0 || strcmp(ext, "zsh") == 0) {
        return SH_KEYWORDS;
    }
    if (strcmp(ext, "rs") == 0) {
        return RS_KEYWORDS;
    }
    if (strcmp(ext, "go") == 0) {
        return GO_KEYWORDS;
    }
    return NULL;
}

void zedit_syntax_highlight_line(const char *raw_line, const char *ext, char *out_ansi, size_t max_len) {
    if (!raw_line || !out_ansi || max_len == 0) return;
    out_ansi[0] = '\0';

    const char **keywords = get_keywords_for_ext(ext);
    size_t in_len = strlen(raw_line);
    size_t out_idx = 0;

#define APPEND_STR(s) do { \
    const char *_src = (s); \
    while (*_src && out_idx + 1 < max_len) { \
        out_ansi[out_idx++] = *_src++; \
    } \
} while(0)

#define APPEND_CHAR(c) do { \
    if (out_idx + 1 < max_len) { \
        out_ansi[out_idx++] = (c); \
    } \
} while(0)

    size_t i = 0;
    while (i < in_len) {
        char ch = raw_line[i];

        /* 1. Comments */
        /* C/C++/Go/Rust double slash: // */
        if ((ch == '/' && raw_line[i + 1] == '/') && 
            (ext && (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0 || strcmp(ext, "cpp") == 0 ||
                     strcmp(ext, "go") == 0 || strcmp(ext, "rs") == 0))) {
            APPEND_STR(CLR_COMMENT);
            while (i < in_len) APPEND_CHAR(raw_line[i++]);
            APPEND_STR(CLR_RESET);
            break;
        }

        /* Shell / Python hash comment: # */
        if (ch == '#' && (ext && (strcmp(ext, "sh") == 0 || strcmp(ext, "py") == 0 || strcmp(ext, "bash") == 0))) {
            APPEND_STR(CLR_COMMENT);
            while (i < in_len) APPEND_CHAR(raw_line[i++]);
            APPEND_STR(CLR_RESET);
            break;
        }

        /* C Preprocessor: #include, #define etc. */
        if (ch == '#' && (ext && (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0 || strcmp(ext, "cpp") == 0))) {
            APPEND_STR(CLR_PREPROC);
            while (i < in_len && !isspace((unsigned char)raw_line[i])) {
                APPEND_CHAR(raw_line[i++]);
            }
            APPEND_STR(CLR_RESET);
            continue;
        }

        /* 2. String literals */
        if (ch == '"' || ch == '\'') {
            char quote = ch;
            APPEND_STR(CLR_STRING);
            APPEND_CHAR(raw_line[i++]);
            while (i < in_len) {
                char sc = raw_line[i++];
                APPEND_CHAR(sc);
                if (sc == quote && (i == 1 || raw_line[i - 2] != '\\')) {
                    break;
                }
            }
            APPEND_STR(CLR_RESET);
            continue;
        }

        /* 3. Variables ($VAR, ${VAR} in shell) */
        if (ch == '$' && (ext && (strcmp(ext, "sh") == 0 || strcmp(ext, "bash") == 0))) {
            APPEND_STR(CLR_VAR);
            APPEND_CHAR(raw_line[i++]);
            if (i < in_len && raw_line[i] == '{') {
                while (i < in_len && raw_line[i] != '}') APPEND_CHAR(raw_line[i++]);
                if (i < in_len) APPEND_CHAR(raw_line[i++]);
            } else {
                while (i < in_len && (isalnum((unsigned char)raw_line[i]) || raw_line[i] == '_')) {
                    APPEND_CHAR(raw_line[i++]);
                }
            }
            APPEND_STR(CLR_RESET);
            continue;
        }

        /* 4. Numbers */
        if (isdigit((unsigned char)ch) && (i == 0 || !is_word_char(raw_line[i - 1]))) {
            APPEND_STR(CLR_NUMBER);
            while (i < in_len && (isalnum((unsigned char)raw_line[i]) || raw_line[i] == '.' || raw_line[i] == 'x' || raw_line[i] == 'X')) {
                APPEND_CHAR(raw_line[i++]);
            }
            APPEND_STR(CLR_RESET);
            continue;
        }

        /* 5. Keywords */
        if (is_word_char(ch) && (i == 0 || !is_word_char(raw_line[i - 1]))) {
            bool matched = false;
            if (keywords) {
                for (int k = 0; keywords[k] != NULL; k++) {
                    size_t kw_len = strlen(keywords[k]);
                    if (match_keyword(raw_line + i, keywords[k], kw_len)) {
                        APPEND_STR(CLR_KEYWORD);
                        APPEND_STR(keywords[k]);
                        APPEND_STR(CLR_RESET);
                        i += kw_len;
                        matched = true;
                        break;
                    }
                }
            }
            if (matched) continue;
        }

        /* Default character */
        APPEND_CHAR(raw_line[i++]);
    }

    out_ansi[out_idx] = '\0';
}
