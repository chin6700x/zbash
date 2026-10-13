/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "core_internal.h"

#if defined(__EMSCRIPTEN__)
#define fpurge(fp) ((void)(fp))
#elif defined(__linux__)
#include <stdio_ext.h>
#define fpurge(fp) __fpurge(fp)
#endif

static const char *get_script_name(zbash_state_t *state) {
    if (state && state->source_depth > 0 && state->source_stack[state->source_depth - 1]) {
        return state->source_stack[state->source_depth - 1];
    }
    if (state && state->argv && state->argv[0]) return state->argv[0];
    return "zbash";
}

/* ========================================================================= */
/* Subshell Command Substitution                                             */
/* ========================================================================= */

char *zbash_subshell_exec(zbash_state_t *state, const char *cmd) {
    if (!cmd || !*cmd) return strdup("");
#if defined(__EMSCRIPTEN__)
    int saved_out = dup(STDOUT_FILENO);
    FILE *tmp_fp = fopen("/tmp/zbash_sub.tmp", "w+");
    if (!tmp_fp) return strdup("");
    int tmp_fd = fileno(tmp_fp);
    dup2(tmp_fd, STDOUT_FILENO);

    zbash_state_t sub = *state;
    zbash_run_line(&sub, cmd);
    fflush(stdout);

    dup2(saved_out, STDOUT_FILENO);
    close(saved_out);

    fseek(tmp_fp, 0, SEEK_SET);
    size_t cap = 2048;
    size_t len = 0;
    char *buf = malloc(cap);
    char read_buf[1024];
    size_t nr;
    while ((nr = fread(read_buf, 1, sizeof(read_buf), tmp_fp)) > 0) {
        if (len + nr + 1 >= cap) {
            cap = (cap + nr + 1) * 2;
            buf = realloc(buf, cap);
        }
        memcpy(buf + len, read_buf, nr);
        len += nr;
    }
    buf[len] = '\0';
    fclose(tmp_fp);
    unlink("/tmp/zbash_sub.tmp");
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
    return buf;
#else
    int pfd[2];
    if (pipe(pfd) < 0) return strdup("");

    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if (pid == 0) {
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);
        zbash_state_t sub = *state;
        if (!sub.functrace) {
            sub.debug_trap = NULL;
            sub.return_trap = NULL;
        }
        if (sub.lineno > 0) sub.lineno--;
        FILE *mem_fp = fmemopen((char *)cmd, strlen(cmd), "r");
        int rc = 0;
        if (mem_fp) {
            rc = zbash_run_stream(&sub, mem_fp);
            fclose(mem_fp);
        } else {
            if (sub.lineno >= 0) sub.lineno++;
            rc = zbash_run_line(&sub, cmd);
        }
        fflush(stdout);
        fflush(stderr);
        _exit(rc);
    } else if (pid < 0) {
        close(pfd[0]);
        close(pfd[1]);
        return strdup("");
    }

    close(pfd[1]);
    size_t cap = 2048;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        close(pfd[0]);
        waitpid(pid, NULL, 0);
        return strdup("");
    }

    char read_buf[1024];
    ssize_t n;
    while ((n = read(pfd[0], read_buf, sizeof(read_buf))) > 0) {
        if (len + n + 1 >= cap) {
            cap = (cap + n + 1) * 2;
            char *new_b = realloc(buf, cap);
            if (!new_b) break;
            buf = new_b;
        }
        memcpy(buf + len, read_buf, n);
        len += n;
    }
    close(pfd[0]);

    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFEXITED(st)) state->last_status = WEXITSTATUS(st);

    /* Discard embedded null bytes per Bash command substitution behavior */
    size_t clean_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != '\0') {
            buf[clean_len++] = buf[i];
        }
    }
    len = clean_len;

    /* Strip trailing newlines */
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        len--;
    }
    buf[len] = '\0';
    return buf;
#endif
}

/* ========================================================================= */
/* Command Execution & Pipeline                                              */
/* ========================================================================= */

int apply_redirections(zbash_state_t *state, zbash_cmd_t *cmd) {
    return zbash_apply_redirections_saved(state, cmd, NULL, NULL);
}

int zbash_apply_redirections_saved(zbash_state_t *state, zbash_cmd_t *cmd, int saved_fds[][2], int *saved_count) {
    fflush(stdout);
    fflush(stderr);

    const char *src = (state && state->source_depth > 0) ? state->source_stack[state->source_depth - 1] :
                      (state && state->argv && state->argv[0] ? state->argv[0] : "zbash");
    int lineno = state ? (state->stmt_start_lineno > 0 ? state->stmt_start_lineno : (state->lineno > 0 ? state->lineno : 1)) : 1;

    if (cmd->redir_count > 0) {
        for (int i = 0; i < cmd->redir_count; i++) {
            zbash_redir_t *r = &cmd->redirs[i];
            int sfd = r->src_fd;

            if (r->type == REDIR_AMBIGUOUS) {
                fprintf(stderr, "%s: line %d: %s: ambiguous redirect\n", src, lineno, r->filename ? r->filename : "");
                return -1;
            }

            if (saved_fds && saved_count) {
                bool already = false;
                for (int s = 0; s < *saved_count; s++) {
                    if (saved_fds[s][0] == sfd) { already = true; break; }
                }
                if (!already && *saved_count < ZBASH_MAX_REDIRS) {
                    int c = dup(sfd);
                    if (c >= 0) {
                        fcntl(c, F_SETFD, FD_CLOEXEC);
                        saved_fds[*saved_count][0] = sfd;
                        saved_fds[*saved_count][1] = c;
                        (*saved_count)++;
                        if (sfd == STDIN_FILENO && state) {
                            state->stdin_redir_depth++;
                        }
                    }
                }
            } else {
                if (sfd == STDIN_FILENO && state) {
                    state->stdin_permanently_redirected = true;
                }
            }

            switch (r->type) {
                case REDIR_IN: {
                    int fd = open(r->filename, O_RDONLY);
                    if (fd < 0) {
                        fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, r->filename, strerror(errno));
                        return -1;
                    }
                    dup2(fd, sfd);
                    if (fd != sfd) close(fd);
                    if (sfd == 0) fpurge(stdin);
                    break;
                }
                case REDIR_OUT: {
                    if (state && state->restricted) {
                        fprintf(stderr, "%s: line %d: %s: restricted: cannot redirect output\n", src, lineno, r->filename ? r->filename : "");
                        return -1;
                    }
                    if (state && state->noclobber) {
                        struct stat st;
                        if (stat(r->filename, &st) == 0 && S_ISREG(st.st_mode)) {
                            fprintf(stderr, "%s: line %d: %s: cannot overwrite existing file\n", src, lineno, r->filename);
                            return -1;
                        }
                    }
                    int fd = open(r->filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (fd < 0) {
                        fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, r->filename, strerror(errno));
                        return -1;
                    }
                    dup2(fd, sfd);
                    if (fd != sfd) close(fd);
                    break;
                }
                case REDIR_CLOBBER: {
                    if (state && state->restricted) {
                        fprintf(stderr, "%s: line %d: %s: restricted: cannot redirect output\n", src, lineno, r->filename ? r->filename : "");
                        return -1;
                    }
                    int fd = open(r->filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (fd < 0) {
                        fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, r->filename, strerror(errno));
                        return -1;
                    }
                    dup2(fd, sfd);
                    if (fd != sfd) close(fd);
                    break;
                }
                case REDIR_APPEND: {
                    if (state && state->restricted) {
                        fprintf(stderr, "%s: line %d: %s: restricted: cannot redirect output\n", src, lineno, r->filename ? r->filename : "");
                        return -1;
                    }
                    int fd = open(r->filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
                    if (fd < 0) {
                        fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, r->filename, strerror(errno));
                        return -1;
                    }
                    dup2(fd, sfd);
                    if (fd != sfd) close(fd);
                    break;
                }
                case REDIR_RDWR: {
                    if (state && state->restricted) {
                        fprintf(stderr, "%s: line %d: %s: restricted: cannot redirect output\n", src, lineno, r->filename ? r->filename : "");
                        return -1;
                    }
                    int fd = open(r->filename, O_RDWR | O_CREAT, 0666);
                    if (fd < 0) {
                        fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, r->filename, strerror(errno));
                        return -1;
                    }
                    dup2(fd, sfd);
                    if (fd != sfd) close(fd);
                    if (sfd == 0) fpurge(stdin);
                    break;
                }
                case REDIR_DUP_IN:
                case REDIR_DUP_OUT: {
                    if (dup2(r->target_fd, sfd) < 0) {
                        fprintf(stderr, "%s: line %d: %d: Bad file descriptor\n", src, lineno, r->target_fd);
                        return -1;
                    }
                    if (sfd == 0) fpurge(stdin);
                    break;
                }
                case REDIR_CLOSE_IN:
                case REDIR_CLOSE_OUT: {
                    close(sfd);
                    break;
                }
                default:
                    break;
            }
        }
        return 0;
    }

    /* Fallback to legacy fields */
    if (cmd->redir_count == 0) {
        if (cmd->in_file) {
            int fd = open(cmd->in_file, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, cmd->in_file, strerror(errno));
                return -1;
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            fpurge(stdin);
        }
        if (cmd->out_file) {
            if (state && state->restricted) {
                fprintf(stderr, "%s: line %d: %s: restricted: cannot redirect output\n", src, lineno, cmd->out_file);
                return -1;
            }
            int flags = O_WRONLY | O_CREAT | (cmd->append_out ? O_APPEND : O_TRUNC);
            int fd = open(cmd->out_file, flags, 0666);
            if (fd < 0) {
                fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, cmd->out_file, strerror(errno));
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (cmd->err_file) {
            int flags = O_WRONLY | O_CREAT | (cmd->append_err ? O_APPEND : O_TRUNC);
            int fd = open(cmd->err_file, flags, 0666);
            if (fd < 0) {
                fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, cmd->err_file, strerror(errno));
                return -1;
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        if (cmd->err_to_out) {
            dup2(STDOUT_FILENO, STDERR_FILENO);
        }
        if (cmd->out_to_err) {
            dup2(STDERR_FILENO, STDOUT_FILENO);
        }
    }
    return 0;
}

void zbash_restore_redirections(zbash_state_t *state, int saved_fds[][2], int saved_count) {
    for (int i = saved_count - 1; i >= 0; i--) {
        if (state && saved_fds[i][0] == STDIN_FILENO && state->stdin_redir_depth > 0) {
            state->stdin_redir_depth--;
        }
        dup2(saved_fds[i][1], saved_fds[i][0]);
        close(saved_fds[i][1]);
    }
}

void free_cmd_strings(zbash_cmd_t *cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        if (cmd->argv[i]) {
            free(cmd->argv[i]);
            cmd->argv[i] = NULL;
        }
    }
    for (int r = 0; r < cmd->redir_count; r++) {
        if (cmd->redirs[r].filename) {
            if (strncmp(cmd->redirs[r].filename, "/tmp/zbash_hs_", 14) == 0) {
                unlink(cmd->redirs[r].filename);
            }
            free(cmd->redirs[r].filename);
            cmd->redirs[r].filename = NULL;
        }
    }
    if (cmd->in_file) {
        if (strncmp(cmd->in_file, "/tmp/zbash_hs_", 14) == 0) {
            unlink(cmd->in_file);
        }
        free(cmd->in_file);
        cmd->in_file = NULL;
    }
    if (cmd->out_file) { free(cmd->out_file); cmd->out_file = NULL; }
    if (cmd->err_file) { free(cmd->err_file); cmd->err_file = NULL; }
}

bool is_assignment(const char *arg) {
    if (!arg) return false;
    const char *eq = strchr(arg, '=');
    if (!eq || eq == arg) return false;
    if (!isalpha((unsigned char)arg[0]) && arg[0] != '_') return false;
    const char *name_end = eq;
    if (eq > arg && *(eq - 1) == '+') {
        name_end = eq - 1;
    }
    const char *bracket = strchr(arg, '[');
    if (bracket && bracket < name_end) {
        const char *close_b = strchr(bracket, ']');
        if (close_b && close_b < name_end) {
            for (const char *p = arg; p < bracket; p++) {
                if (!isalnum((unsigned char)*p) && *p != '_') return false;
            }
            return true;
        }
    }
    for (const char *p = arg; p < name_end; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_') return false;
    }
    return true;
}

int execute_single_cmd(zbash_state_t *state, zbash_cmd_t *cmd) {
    if (cmd->argc < 1 || !cmd->argv[0]) {
        if (cmd->redir_count > 0 || cmd->out_file || cmd->in_file || cmd->err_file) {
            int saved_fds[ZBASH_MAX_REDIRS][2];
            int saved_count = 0;
            if (zbash_apply_redirections_saved(state, cmd, saved_fds, &saved_count) != 0) {
                zbash_restore_redirections(state, saved_fds, saved_count);
                state->last_status = 1;
                char sbuf[16]; snprintf(sbuf, sizeof(sbuf), "1");
                zbash_set_array_elem(state, "PIPESTATUS", 0, sbuf);
                return 1;
            }
            fflush(stdout);
            fflush(stderr);
            zbash_restore_redirections(state, saved_fds, saved_count);
            state->last_status = 0;
            char sbuf[16]; snprintf(sbuf, sizeof(sbuf), "0");
            zbash_set_array_elem(state, "PIPESTATUS", 0, sbuf);
            return 0;
        }
        return 0;
    }

    /* Handle compound brace { ... } */
    if (strcmp(cmd->argv[0], "{") == 0) {
        if (cmd->argc > 1 && cmd->argv[1]) {
            if (cmd->redir_count > 0 || cmd->out_file || cmd->in_file || cmd->err_file || cmd->err_to_out || cmd->out_to_err) {
                int saved_fds[ZBASH_MAX_REDIRS][2];
                int saved_count = 0;
                if (zbash_apply_redirections_saved(state, cmd, saved_fds, &saved_count) != 0) {
                    zbash_restore_redirections(state, saved_fds, saved_count);
                    return 1;
                }
                int rc = zbash_run_line(state, cmd->argv[1]);
                fflush(stdout);
                fflush(stderr);
                zbash_restore_redirections(state, saved_fds, saved_count);
                return rc;
            }
            return zbash_run_line(state, cmd->argv[1]);
        }
        return 0;
    }

    /* Handle compound subshell ( ... ) */
    if (strcmp(cmd->argv[0], "(") == 0) {
        if (cmd->argc > 1 && cmd->argv[1]) {
            fflush(stdout);
            fflush(stderr);
            pid_t pid = fork();
            if (pid == 0) {
                if (state->exit_trap) { free(state->exit_trap); state->exit_trap = NULL; }
                if (apply_redirections(state, cmd) != 0) _exit(1);
                int rc = zbash_run_line(state, cmd->argv[1]);
                int final_code = state->should_exit ? state->exit_code : rc;
                if (state->exit_trap) {
                    char *t = state->exit_trap;
                    state->exit_trap = NULL;
                    state->should_exit = false;
                    zbash_run_line(state, t);
                    free(t);
                    if (state->should_exit) final_code = state->exit_code;
                }
                _exit(final_code);
            } else if (pid > 0) {
                int status = 0;
                waitpid(pid, &status, 0);
                return WIFEXITED(status) ? WEXITSTATUS(status) : (128 + WTERMSIG(status));
            }
        }
        return 0;
    }

    /* Handle arithmetic command: (( expr )) */
    if (strncmp(cmd->argv[0], "((", 2) == 0) {
        char full[1024] = {0};
        for (int a = 0; a < cmd->argc; a++) {
            strcat(full, cmd->argv[a]);
            if (a < cmd->argc - 1) strcat(full, " ");
        }
        char *start = strstr(full, "((");
        char *end = strstr(start + 2, "))");
        if (start && end) {
            *end = '\0';
            long long val = zbash_eval_arithmetic(state, start + 2);
            return (val != 0) ? 0 : 1;
        }
        return 0;
    }

    /* Handle function call */
    zbash_func_t *fn = zbash_find_func(state, cmd->argv[0]);
    if (fn) {
        if (cmd->redir_count > 0 || cmd->out_file || cmd->in_file || cmd->err_file || cmd->err_to_out || cmd->out_to_err) {
            int saved_fds[ZBASH_MAX_REDIRS][2];
            int saved_count = 0;
            if (zbash_apply_redirections_saved(state, cmd, saved_fds, &saved_count) != 0) {
                zbash_restore_redirections(state, saved_fds, saved_count);
                return 1;
            }
            int rc = zbash_exec_func(state, fn, cmd->argc, cmd->argv);
            fflush(stdout);
            fflush(stderr);
            zbash_restore_redirections(state, saved_fds, saved_count);
            return rc;
        }
        return zbash_exec_func(state, fn, cmd->argc, cmd->argv);
    }

    /* Handle variable assignments: VAR=val or VAR=val cmd ... */
    int cmd_offset = cmd->prefix_assign_count;

    if (cmd_offset == cmd->argc) {
        /* Pure variable assignments */
        for (int k = 0; k < cmd_offset; k++) {
            char *eq = strchr(cmd->argv[k], '=');
            bool is_append = (eq > cmd->argv[k] && *(eq - 1) == '+');
            if (is_append) *(eq - 1) = '\0';
            else *eq = '\0';
            char *bracket = strchr(cmd->argv[k], '[');
            if (bracket) {
                *bracket = '\0';
                char *close_b = strrchr(bracket + 1, ']');
                if (close_b) {
                    *close_b = '\0';
                    const char *sub_expr = bracket + 1;
                    const char *arr_name = cmd->argv[k];
                    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "./array.tests";
                    int lineno = state->lineno > 0 ? state->lineno : 1;

                    if (*sub_expr == '\0' || strcmp(sub_expr, "*") == 0 || strcmp(sub_expr, "@") == 0) {
                        fprintf(stderr, "%s: line %d: %s[%s]: bad array subscript\n", src, lineno, arr_name, sub_expr);
                        state->last_status = 1;
                        if (is_append) *(eq - 1) = '+';
                        *eq = '=';
                        return 1;
                    }
                    if (eq[1] == '(') {
                        fprintf(stderr, "%s: line %d: %s[%s]: cannot assign list to array member\n", src, lineno, arr_name, sub_expr);
                        state->last_status = 1;
                        if (is_append) *(eq - 1) = '+';
                        *eq = '=';
                        return 1;
                    }

                    int idx = (int)zbash_eval_arithmetic(state, sub_expr);
                    if (idx < 0) {
                        fprintf(stderr, "%s: line %d: %s[%s]: bad array subscript\n", src, lineno, arr_name, sub_expr);
                        state->last_status = 1;
                        if (is_append) *(eq - 1) = '+';
                        *eq = '=';
                        return 1;
                    }
                    if (state->last_status != 0 || state->expansion_error) {
                        state->last_status = 1;
                        if (is_append) *(eq - 1) = '+';
                        *eq = '=';
                        return 1;
                    }
                    if (zbash_is_readonly(state, arr_name)) {
                        const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
                        if (state->lineno > 0) {
                            fprintf(stderr, "%s: line %d: %s: readonly variable\n", src, state->lineno, arr_name);
                        } else {
                            fprintf(stderr, "%s: %s: readonly variable\n", src, arr_name);
                        }
                        state->last_status = 1;
                        if (is_append) *(eq - 1) = '+';
                        *eq = '=';
                        return 1;
                    }
                    if (is_append) {
                        const char *old = zbash_get_array_elem(state, arr_name, idx);
                        if (zbash_is_integer(state, arr_name)) {
                            long long old_num = (old && *old) ? zbash_eval_arithmetic(state, old) : 0;
                            long long add_num = (eq[1]) ? zbash_eval_arithmetic(state, eq + 1) : 0;
                            char nbuf[32];
                            snprintf(nbuf, sizeof(nbuf), "%lld", old_num + add_num);
                            zbash_set_array_elem(state, arr_name, idx, nbuf);
                        } else {
                            char new_val[8192];
                            snprintf(new_val, sizeof(new_val), "%s%s", old ? old : "", eq + 1);
                            zbash_set_array_elem(state, arr_name, idx, new_val);
                        }
                    } else {
                        if (zbash_is_integer(state, arr_name)) {
                            long long num = (eq[1]) ? zbash_eval_arithmetic(state, eq + 1) : 0;
                            char nbuf[32];
                            snprintf(nbuf, sizeof(nbuf), "%lld", num);
                            zbash_set_array_elem(state, arr_name, idx, nbuf);
                        } else {
                            zbash_set_array_elem(state, arr_name, idx, eq + 1);
                        }
                    }
                }
            } else {
                if (zbash_assign_var(state, cmd->argv[k], eq + 1, is_append) != 0) {
                    if (is_append) *(eq - 1) = '+';
                    *eq = '=';
                    return 1;
                }
            }
            if (is_append) *(eq - 1) = '+';
            *eq = '=';
        }
        if (state && state->cmd_subst_occurred) {
            state->cmd_subst_occurred = false;
            return state->last_status;
        }
        return 0;
    }

    typedef struct {
        char *name;
        char *old_val;
        uint32_t old_attr;
        bool was_set;
    } saved_env_t;
    saved_env_t saved_envs[ZBASH_MAX_ARGS];
    int saved_count = 0;

    if (cmd_offset > 0) {
        /* Prefix assignments preceding a command (e.g. VAR=val cmd) */
        for (int k = 0; k < cmd_offset; k++) {
            char *eq = strchr(cmd->argv[k], '=');
            bool is_append = (eq > cmd->argv[k] && *(eq - 1) == '+');
            if (is_append) *(eq - 1) = '\0';
            else *eq = '\0';
            const char *key = cmd->argv[k];

            saved_envs[saved_count].name = strdup(key);
            saved_envs[saved_count].old_attr = zbash_get_var_attr(state, key);
            const char *cur = getenv(key);
            if (cur) {
                saved_envs[saved_count].old_val = strdup(cur);
                saved_envs[saved_count].was_set = true;
            } else {
                saved_envs[saved_count].old_val = NULL;
                saved_envs[saved_count].was_set = false;
            }
            saved_count++;

            setenv(key, eq + 1, 1);
            zbash_set_var_attr(state, key, ZBASH_ATTR_EXPORT);

            if (is_append) *(eq - 1) = '+';
            *eq = '=';
        }
    }

    char **effective_argv = cmd->argv + cmd_offset;
    int effective_argc = cmd->argc - cmd_offset;
    int cmd_ret = 0;

    /* Handle function call */
    fn = zbash_find_func(state, effective_argv[0]);
    if (fn) {
        if (cmd->redir_count == 0 && !cmd->in_file && !cmd->out_file && !cmd->err_file && !cmd->err_to_out && !cmd->out_to_err) {
            cmd_ret = zbash_exec_func(state, fn, effective_argc, effective_argv);
        } else {
            int saved_fds[ZBASH_MAX_REDIRS][2];
            int saved_count = 0;
            if (zbash_apply_redirections_saved(state, cmd, saved_fds, &saved_count) != 0) {
                zbash_restore_redirections(state, saved_fds, saved_count);
                cmd_ret = 1;
            } else {
                cmd_ret = zbash_exec_func(state, fn, effective_argc, effective_argv);
                fflush(stdout);
                fflush(stderr);
                zbash_restore_redirections(state, saved_fds, saved_count);
            }
        }
    } else if (zbash_is_builtin_enabled(state, effective_argv[0])) {
        if (strcmp(effective_argv[0], "exec") == 0 && effective_argc == 1) {
            /* exec with only redirections: apply permanently */
            if (apply_redirections(state, cmd) != 0) {
                cmd_ret = 1;
            } else {
                cmd_ret = 0;
            }
        } else if (cmd->redir_count == 0 && !cmd->in_file && !cmd->out_file && !cmd->err_file && !cmd->err_to_out && !cmd->out_to_err) {
            cmd_ret = zbash_exec_builtin(state, effective_argc, effective_argv);
        } else {
            int saved_fds[ZBASH_MAX_REDIRS][2];
            int saved_count = 0;
            if (zbash_apply_redirections_saved(state, cmd, saved_fds, &saved_count) != 0) {
                zbash_restore_redirections(state, saved_fds, saved_count);
                cmd_ret = 1;
            } else {
                cmd_ret = zbash_exec_builtin(state, effective_argc, effective_argv);
                fflush(stdout);
                fflush(stderr);
                zbash_restore_redirections(state, saved_fds, saved_count);
            }
        }
    } else {
        /* Auto-CD in interactive mode: typing directory path changes cwd */
        struct stat st_cd;
        if (state && state->interactive && effective_argc >= 1 &&
            stat(effective_argv[0], &st_cd) == 0 && S_ISDIR(st_cd.st_mode)) {
            char *cd_args[3];
            cd_args[0] = "cd";
            cd_args[1] = effective_argv[0];
            cd_args[2] = NULL;
            cmd_ret = builtin_cd(state, 2, cd_args);
        } else if (state && state->restricted && strchr(effective_argv[0], '/')) {
            const char *src = get_script_name(state);
            int lineno = state->lineno > 0 ? state->lineno : 1;
            fprintf(stderr, "%s: line %d: %s: restricted: cannot specify `/' in command names\n", src, lineno, effective_argv[0]);
            cmd_ret = 1;
        } else {
#if defined(__EMSCRIPTEN__)
        extern int zbash_wasm_dispatch_external(const char *cmd_name, int argc, char **argv);
        cmd_ret = zbash_wasm_dispatch_external(effective_argv[0], effective_argc, effective_argv);
#else
        fflush(stdout);
        fflush(stderr);
        pid_t pid = fork();
        if (pid == 0) {
            if (apply_redirections(state, cmd) != 0) _exit(1);
            execvp(effective_argv[0], effective_argv);
            const char *src = get_script_name(state);
            int lineno = state->lineno > 0 ? state->lineno : 1;
            if (errno == ENOENT && !strchr(effective_argv[0], '/')) {
                fprintf(stderr, "%s: line %d: %s: command not found\n", src, lineno, effective_argv[0]);
            } else {
                fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, effective_argv[0], strerror(errno));
            }
            _exit((errno == ENOENT) ? 127 : 126);
        } else if (pid < 0) {
            perror("zbash: fork");
            cmd_ret = 1;
        } else {
            int status = 0;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) cmd_ret = WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) cmd_ret = 128 + WTERMSIG(status);
            else cmd_ret = 1;
        }
#endif
        }
    }

    bool persist_prefix = false;
    bool export_attr = false;
    if (state && effective_argc > 0) {
        const char *c0 = effective_argv[0];
        if (strcmp(c0, "export") == 0) {
            persist_prefix = true;
            export_attr = true;
        } else if (strcmp(c0, "declare") == 0 || strcmp(c0, "typeset") == 0) {
            for (int a = 1; a < effective_argc; a++) {
                if (strstr(effective_argv[a], "-x")) {
                    persist_prefix = true;
                    export_attr = true;
                    break;
                }
            }
        } else if (state->posix && (
            strcmp(c0, ".") == 0 || strcmp(c0, ":") == 0 ||
            strcmp(c0, "break") == 0 || strcmp(c0, "continue") == 0 ||
            strcmp(c0, "eval") == 0 || strcmp(c0, "exec") == 0 ||
            strcmp(c0, "exit") == 0 || strcmp(c0, "export") == 0 ||
            strcmp(c0, "readonly") == 0 ||
            strcmp(c0, "return") == 0 || strcmp(c0, "set") == 0 ||
            strcmp(c0, "shift") == 0 || strcmp(c0, "source") == 0 ||
            strcmp(c0, "times") == 0 || strcmp(c0, "trap") == 0 ||
            strcmp(c0, "unset") == 0)) {
            persist_prefix = true;
        }
    }

    if (saved_count > 0) {
        for (int s = 0; s < saved_count; s++) {
            if (!persist_prefix) {
                if (saved_envs[s].was_set) {
                    setenv(saved_envs[s].name, saved_envs[s].old_val, 1);
                    free(saved_envs[s].old_val);
                    zbash_set_var_attr(state, saved_envs[s].name, saved_envs[s].old_attr);
                } else {
                    unsetenv(saved_envs[s].name);
                    zbash_set_var_attr(state, saved_envs[s].name, 0);
                }
            } else {
                const char *cur = getenv(saved_envs[s].name);
                if (cur) {
                    zbash_assign_var(state, saved_envs[s].name, cur, false);
                    if (export_attr) {
                        zbash_set_var_attr(state, saved_envs[s].name, ZBASH_ATTR_EXPORT);
                    }
                }
                if (saved_envs[s].old_val) free(saved_envs[s].old_val);
            }
            free(saved_envs[s].name);
        }
    }
    state->last_status = cmd_ret;
    char sbuf[16];
    snprintf(sbuf, sizeof(sbuf), "%d", cmd_ret);
    zbash_set_array_elem(state, "PIPESTATUS", 0, sbuf);
    return cmd_ret;
}

int execute_pipeline(zbash_state_t *state, zbash_pipeline_t *pipeline) {
    int n = pipeline->cmd_count;
    if (n == 0) return 0;
    if (n == 1) return execute_single_cmd(state, &pipeline->cmds[0]);

#if defined(__EMSCRIPTEN__)
    char prev_tmp[64] = {0};
    int last_st = 0;

    for (int i = 0; i < n; i++) {
        zbash_cmd_t *cmd = &pipeline->cmds[i];
        char next_tmp[64] = {0};

        if (i > 0) {
            cmd->in_file = strdup(prev_tmp);
        }
        if (i < n - 1) {
            snprintf(next_tmp, sizeof(next_tmp), "/tmp/wasm_pipe_%d.tmp", i);
            cmd->out_file = strdup(next_tmp);
        }

        last_st = execute_single_cmd(state, cmd);

        if (prev_tmp[0]) {
            unlink(prev_tmp);
            prev_tmp[0] = '\0';
        }
        if (next_tmp[0]) {
            strcpy(prev_tmp, next_tmp);
        }
    }
    if (prev_tmp[0]) unlink(prev_tmp);
    return last_st;
#else
    int pipe_fds[64][2];
    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipe_fds[i]) < 0) {
            perror("zbash: pipe");
            return 1;
        }
    }

    pid_t pids[64];
    for (int i = 0; i < n; i++) {
        zbash_cmd_t *cmd = &pipeline->cmds[i];
        fflush(stdout);
        fflush(stderr);
        pids[i] = fork();

        if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipe_fds[i - 1][0], STDIN_FILENO);
            }
            if (i < n - 1) {
                dup2(pipe_fds[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < n - 1; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }

            if (apply_redirections(state, cmd) != 0) {
                _exit(1);
            }

            if (cmd->argc == 0 || !cmd->argv[0]) {
                _exit(0);
            }

            if (strcmp(cmd->argv[0], "{") == 0) {
                int rc = (cmd->argc > 1 && cmd->argv[1]) ? zbash_run_line(state, cmd->argv[1]) : 0;
                fflush(stdout);
                fflush(stderr);
                _exit(state->should_exit ? state->exit_code : rc);
            }

            if (strcmp(cmd->argv[0], "(") == 0) {
                int rc = (cmd->argc > 1 && cmd->argv[1]) ? zbash_run_line(state, cmd->argv[1]) : 0;
                fflush(stdout);
                fflush(stderr);
                _exit(state->should_exit ? state->exit_code : rc);
            }

            /* Handle function in pipeline child */
            zbash_func_t *fn = zbash_find_func(state, cmd->argv[0]);
            if (fn) {
                int rc = zbash_exec_func(state, fn, cmd->argc, cmd->argv);
                fflush(stdout);
                fflush(stderr);
                _exit(state->should_exit ? state->exit_code : rc);
            }

            if (zbash_is_builtin(cmd->argv[0])) {
                int rc = zbash_exec_builtin(state, cmd->argc, cmd->argv);
                fflush(stdout);
                fflush(stderr);
                _exit(state->should_exit ? state->exit_code : rc);
            }

            execvp(cmd->argv[0], cmd->argv);
            const char *src = get_script_name(state);
            int lineno = state->lineno > 0 ? state->lineno : 1;
            if (errno == ENOENT && !strchr(cmd->argv[0], '/')) {
                fprintf(stderr, "%s: line %d: %s: command not found\n", src, lineno, cmd->argv[0]);
            } else {
                fprintf(stderr, "%s: line %d: %s: %s\n", src, lineno, cmd->argv[0], strerror(errno));
            }
            _exit((errno == ENOENT) ? 127 : 126);
        } else if (pids[i] < 0) {
            perror("zbash: fork");
            return 1;
        }
    }

    for (int j = 0; j < n - 1; j++) {
        close(pipe_fds[j][0]);
        close(pipe_fds[j][1]);
    }

    int last_status = 0;
    for (int i = 0; i < n; i++) {
        int status = 0;
        waitpid(pids[i], &status, 0);
        int cur_st = 0;
        if (WIFEXITED(status)) cur_st = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) cur_st = 128 + WTERMSIG(status);
        char sbuf[16];
        snprintf(sbuf, sizeof(sbuf), "%d", cur_st);
        zbash_set_array_elem(state, "PIPESTATUS", i, sbuf);
        if (i == n - 1) {
            last_status = cur_st;
        }
    }
    state->last_status = last_status;
    return last_status;
#endif
}
