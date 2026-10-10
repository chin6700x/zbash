/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "prompt.h"
#include "core_internal.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static char s_prompt_buf[2048];

bool zbash_get_git_info(char *branch_out, size_t maxlen, bool *is_dirty) {
    if (!branch_out || maxlen == 0) return false;
    branch_out[0] = '\0';
    if (is_dirty) *is_dirty = false;

    char curr_path[1024];
    if (!getcwd(curr_path, sizeof(curr_path))) {
        const char *pwd = getenv("PWD");
        if (pwd) {
            strncpy(curr_path, pwd, sizeof(curr_path) - 1);
            curr_path[sizeof(curr_path) - 1] = '\0';
        } else {
            return false;
        }
    }

    char git_dir[1024];
    bool found_git = false;
    char check_path[1024];
    strncpy(check_path, curr_path, sizeof(check_path) - 1);
    check_path[sizeof(check_path) - 1] = '\0';

    for (int depth = 0; depth < 16; depth++) {
        snprintf(git_dir, sizeof(git_dir), "%s/.git", check_path);
        struct stat st;
        if (stat(git_dir, &st) == 0) {
            found_git = true;
            if (S_ISREG(st.st_mode)) {
                /* Git submodule or worktree: .git is a text file with "gitdir: <path>" */
                FILE *fp = fopen(git_dir, "r");
                if (fp) {
                    char line[1024];
                    if (fgets(line, sizeof(line), fp)) {
                        char *p = line;
                        while (*p && isspace((unsigned char)*p)) p++;
                        if (strncmp(p, "gitdir:", 7) == 0) {
                            p += 7;
                            while (*p && isspace((unsigned char)*p)) p++;
                            char *eol = p + strlen(p);
                            while (eol > p && (eol[-1] == '\r' || eol[-1] == '\n' || isspace((unsigned char)eol[-1]))) {
                                *--eol = '\0';
                            }
                            if (*p == '/') {
                                strncpy(git_dir, p, sizeof(git_dir) - 1);
                            } else {
                                snprintf(git_dir, sizeof(git_dir), "%s/%s", check_path, p);
                            }
                        }
                    }
                    fclose(fp);
                }
            }
            break;
        }

        /* Walk up to parent directory */
        char *last_slash = strrchr(check_path, '/');
        if (!last_slash || last_slash == check_path) {
            break;
        }
        *last_slash = '\0';
    }

    if (!found_git) return false;

    /* Read HEAD file */
    char head_file[1024];
    snprintf(head_file, sizeof(head_file), "%s/HEAD", git_dir);
    FILE *hfp = fopen(head_file, "r");
    if (!hfp) return false;

    char head_buf[256];
    if (fgets(head_buf, sizeof(head_buf), hfp)) {
        char *eol = head_buf + strlen(head_buf);
        while (eol > head_buf && (eol[-1] == '\r' || eol[-1] == '\n' || isspace((unsigned char)eol[-1]))) {
            *--eol = '\0';
        }
        if (strncmp(head_buf, "ref: refs/heads/", 16) == 0) {
            strncpy(branch_out, head_buf + 16, maxlen - 1);
            branch_out[maxlen - 1] = '\0';
        } else if (strncmp(head_buf, "ref: ", 5) == 0) {
            const char *slash = strrchr(head_buf, '/');
            const char *bname = slash ? slash + 1 : head_buf + 5;
            strncpy(branch_out, bname, maxlen - 1);
            branch_out[maxlen - 1] = '\0';
        } else {
            /* Detached HEAD - 7 character commit hash */
            size_t hlen = strlen(head_buf);
            if (hlen > 7) head_buf[7] = '\0';
            strncpy(branch_out, head_buf, maxlen - 1);
            branch_out[maxlen - 1] = '\0';
        }
    }
    fclose(hfp);

    /* Fast dirty status check */
    if (is_dirty) {
        char index_file[1024];
        snprintf(index_file, sizeof(index_file), "%s/index", git_dir);
        struct stat ist, cst;
        if (stat(index_file, &ist) == 0 && stat(curr_path, &cst) == 0) {
            if (cst.st_mtime > ist.st_mtime) {
                *is_dirty = true;
            }
        }
    }

    return (branch_out[0] != '\0');
}

void zbash_format_short_path(const char *pwd, char *out, size_t maxlen) {
    if (!out || maxlen == 0) return;
    if (!pwd || !*pwd) {
        snprintf(out, maxlen, ".");
        return;
    }

    const char *home = getenv("HOME");
    char path_buf[1024];

    if (home && *home && strncmp(pwd, home, strlen(home)) == 0) {
        const char *rel = pwd + strlen(home);
        if (*rel == '\0') {
            snprintf(out, maxlen, "~");
            return;
        } else if (*rel == '/') {
            snprintf(path_buf, sizeof(path_buf), "~%s", rel);
        } else {
            snprintf(path_buf, sizeof(path_buf), "%s", pwd);
        }
    } else if (strncmp(pwd, "/Volumes/", 9) == 0) {
        const char *after_vol = strchr(pwd + 9, '/');
        if (after_vol) {
            snprintf(path_buf, sizeof(path_buf), "…%s", after_vol);
        } else {
            snprintf(path_buf, sizeof(path_buf), "%s", pwd);
        }
    } else {
        snprintf(path_buf, sizeof(path_buf), "%s", pwd);
    }

    size_t plen = strlen(path_buf);
    if (plen <= 26) {
        strncpy(out, path_buf, maxlen - 1);
        out[maxlen - 1] = '\0';
        return;
    }

    /* Count directory slashes */
    int slashes = 0;
    for (size_t i = 0; i < plen; i++) {
        if (path_buf[i] == '/') slashes++;
    }

    if (slashes <= 2) {
        strncpy(out, path_buf, maxlen - 1);
        out[maxlen - 1] = '\0';
        return;
    }

    /* Retain the last 2-3 components preceded by …/ */
    int target_slash = slashes - 2;
    int seen = 0;
    const char *keep_ptr = path_buf;
    for (size_t i = 0; i < plen; i++) {
        if (path_buf[i] == '/') {
            seen++;
            if (seen == target_slash) {
                keep_ptr = path_buf + i;
                break;
            }
        }
    }

    if (path_buf[0] == '~') {
        snprintf(out, maxlen, "~…%s", keep_ptr);
    } else {
        snprintf(out, maxlen, "…%s", keep_ptr);
    }
}

static char *zbash_format_custom_ps1(zbash_state_t *state, const char *ps1, bool is_libedit) {
    char buf[1024];
    size_t bi = 0;
    const char *p = ps1;

    while (*p && bi + 1 < sizeof(buf)) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            switch (*p) {
                case 'u': {
                    const char *user = getenv("USER");
                    if (!user) user = "user";
                    size_t ulen = strlen(user);
                    if (bi + ulen < sizeof(buf)) {
                        memcpy(buf + bi, user, ulen);
                        bi += ulen;
                    }
                    break;
                }
                case 'h': {
                    char host[128];
                    if (gethostname(host, sizeof(host)) == 0) {
                        char *dot = strchr(host, '.');
                        if (dot) *dot = '\0';
                        size_t hlen = strlen(host);
                        if (bi + hlen < sizeof(buf)) {
                            memcpy(buf + bi, host, hlen);
                            bi += hlen;
                        }
                    }
                    break;
                }
                case 'w': {
                    const char *pwd = getenv("PWD");
                    const char *home = getenv("HOME");
                    if (pwd) {
                        if (home && strncmp(pwd, home, strlen(home)) == 0) {
                            buf[bi++] = '~';
                            const char *rem = pwd + strlen(home);
                            size_t rlen = strlen(rem);
                            if (bi + rlen < sizeof(buf)) {
                                memcpy(buf + bi, rem, rlen);
                                bi += rlen;
                            }
                        } else {
                            size_t plen = strlen(pwd);
                            if (bi + plen < sizeof(buf)) {
                                memcpy(buf + bi, pwd, plen);
                                bi += plen;
                            }
                        }
                    }
                    break;
                }
                case 'W': {
                    const char *pwd = getenv("PWD");
                    if (pwd) {
                        const char *slash = strrchr(pwd, '/');
                        const char *base = slash ? (slash[1] ? slash + 1 : "/") : pwd;
                        size_t blen = strlen(base);
                        if (bi + blen < sizeof(buf)) {
                            memcpy(buf + bi, base, blen);
                            bi += blen;
                        }
                    }
                    break;
                }
                case 'g': {
                    /* Git branch token */
                    char branch[128];
                    bool dirty = false;
                    if (zbash_get_git_info(branch, sizeof(branch), &dirty)) {
                        char gbuf[160];
                        snprintf(gbuf, sizeof(gbuf), "git:(%s%s)", branch, dirty ? "*" : "");
                        size_t glen = strlen(gbuf);
                        if (bi + glen < sizeof(buf)) {
                            memcpy(buf + bi, gbuf, glen);
                            bi += glen;
                        }
                    }
                    break;
                }
                case '$':
                    buf[bi++] = (geteuid() == 0) ? '#' : '$';
                    break;
                case 'n':
                    buf[bi++] = '\n';
                    break;
                case '\\':
                    buf[bi++] = '\\';
                    break;
                case 'e':
                    buf[bi++] = '\033';
                    break;
                case '[':
                    if (is_libedit) buf[bi++] = '\1';
                    break;
                case ']':
                    if (is_libedit) buf[bi++] = '\1';
                    break;
                default:
                    buf[bi++] = *p;
                    break;
            }
            p++;
        } else {
            buf[bi++] = *p++;
        }
    }
    buf[bi] = '\0';
    snprintf(s_prompt_buf, sizeof(s_prompt_buf), "%s", buf);
    (void)state;
    return s_prompt_buf;
}

char *zbash_build_prompt(zbash_state_t *state, bool is_libedit) {
    const char *ps1 = getenv("PS1");
    if (ps1 && *ps1) {
        return zbash_format_custom_ps1(state, ps1, is_libedit);
    }

    char branch[128];
    bool is_dirty = false;
    bool has_git = zbash_get_git_info(branch, sizeof(branch), &is_dirty);

    char cwd_buf[1024];
    const char *pwd = getcwd(cwd_buf, sizeof(cwd_buf));
    if (!pwd || !*pwd) {
        pwd = getenv("PWD");
        if (!pwd || !*pwd) pwd = ".";
    }
    char short_path[256];
    zbash_format_short_path(pwd, short_path, sizeof(short_path));

    const char *term = getenv("TERM");
    bool is_dumb = (term && strcmp(term, "dumb") == 0);

    /* Plain text fallback for non-color terminals */
    if (is_dumb) {
        if (has_git) {
            snprintf(s_prompt_buf, sizeof(s_prompt_buf),
                     "+- [zbash] %s on git:%s%s\n+-> ",
                     short_path, branch, is_dirty ? "*" : "");
        } else {
            snprintf(s_prompt_buf, sizeof(s_prompt_buf),
                     "+- [zbash] %s\n+-> ",
                     short_path);
        }
        return s_prompt_buf;
    }

    bool nerd_fonts = (getenv("ZBASH_NERD_FONTS") != NULL && strcmp(getenv("ZBASH_NERD_FONTS"), "0") != 0);

    /* EditLine delimiters to prevent cursor wrapping errors */
    const char *esc_s = is_libedit ? "\1" : "";
    const char *esc_e = is_libedit ? "\1" : "";

    char c_reset[32], c_gray[32], c_yellow[32], c_cyan[32], c_dim[32], c_blue[32], c_magenta[32], c_green[32], c_red[32];
    snprintf(c_reset, sizeof(c_reset), "%s\033[0m%s", esc_s, esc_e);
    snprintf(c_gray, sizeof(c_gray), "%s\033[38;5;242m%s", esc_s, esc_e);
    snprintf(c_yellow, sizeof(c_yellow), "%s\033[1;33m%s", esc_s, esc_e);
    snprintf(c_cyan, sizeof(c_cyan), "%s\033[1;36m%s", esc_s, esc_e);
    snprintf(c_dim, sizeof(c_dim), "%s\033[2m%s", esc_s, esc_e);
    snprintf(c_blue, sizeof(c_blue), "%s\033[1;34m%s", esc_s, esc_e);
    snprintf(c_magenta, sizeof(c_magenta), "%s\033[1;35m%s", esc_s, esc_e);
    snprintf(c_green, sizeof(c_green), "%s\033[1;32m%s", esc_s, esc_e);
    snprintf(c_red, sizeof(c_red), "%s\033[1;31m%s", esc_s, esc_e);

    char line1[1024];
    char git_part[256] = "";
    if (has_git) {
        if (nerd_fonts) {
            snprintf(git_part, sizeof(git_part), " %son %s\xee\x82\xa0 %s%s%s%s",
                     c_dim, c_magenta, branch, is_dirty ? c_yellow : "", is_dirty ? "*" : "", c_reset);
        } else {
            snprintf(git_part, sizeof(git_part), " %son %sgit:(%s)%s%s%s",
                     c_dim, c_magenta, branch, is_dirty ? c_yellow : "", is_dirty ? "*" : "", c_reset);
        }
    }

    char dur_part[64] = "";
    if (state && state->last_cmd_duration_ms > 1500.0) {
        snprintf(dur_part, sizeof(dur_part), " %s[took %.2fs]%s",
                 c_dim, state->last_cmd_duration_ms / 1000.0, c_reset);
    }

    char err_part[64] = "";
    int status = state ? state->last_status : 0;
    if (status != 0) {
        snprintf(err_part, sizeof(err_part), " %s[%d]%s", c_red, status, c_reset);
    }

    snprintf(line1, sizeof(line1),
             "%s╭─%s %s⚡%s %szbash%s %sin%s %s%s%s%s%s%s",
             c_gray, c_reset,
             c_yellow, c_reset,
             c_cyan, c_reset,
             c_dim, c_reset,
             c_blue, short_path, c_reset,
             git_part, dur_part, err_part);

    const char *arrow_color = (status == 0) ? c_green : c_red;
    char line2[256];
    snprintf(line2, sizeof(line2), "%s╰─%s%s❯%s ", c_gray, c_reset, arrow_color, c_reset);

    snprintf(s_prompt_buf, sizeof(s_prompt_buf), "%s\n%s", line1, line2);
    return s_prompt_buf;
}

void zbash_load_rc(zbash_state_t *state) {
    if (!state || state->norc) return;

    if (access("/etc/zbashrc", R_OK) == 0) {
        zbash_source_file(state, "/etc/zbashrc");
    }

    const char *home = getenv("HOME");
    if (home && *home) {
        char rc_path[1024];
        snprintf(rc_path, sizeof(rc_path), "%s/.zbashrc", home);
        if (access(rc_path, R_OK) == 0) {
            zbash_source_file(state, rc_path);
        }
    }
}
