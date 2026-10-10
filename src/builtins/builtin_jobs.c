/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "builtins_internal.h"

/* ========================================================================= */
/* Job Management Subsystem                                                  */
/* ========================================================================= */

int zbash_jobs_add(zbash_state_t *state, pid_t pid, const char *cmd, bool started_without_jc) {
    if (!state || pid <= 0) return -1;
    zbash_jobs_update_status(state);

    int slot = -1;
    for (int i = 0; i < 64; i++) {
        if (state->jobs[i].pid == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return -1;

    int max_id = 0;
    for (int i = 0; i < 64; i++) {
        if (state->jobs[i].pid > 0 && state->jobs[i].job_id > max_id) {
            max_id = state->jobs[i].job_id;
        }
    }

    state->jobs[slot].job_id = max_id + 1;
    state->jobs[slot].pid = pid;
    state->jobs[slot].pgid = pid;
    state->jobs[slot].status = 0; /* Running */
    state->jobs[slot].notified = false;
    state->jobs[slot].disowned = false;
    state->jobs[slot].started_without_jc = started_without_jc;

    if (cmd) {
        strncpy(state->jobs[slot].cmd, cmd, sizeof(state->jobs[slot].cmd) - 1);
        state->jobs[slot].cmd[sizeof(state->jobs[slot].cmd) - 1] = '\0';
    } else {
        snprintf(state->jobs[slot].cmd, sizeof(state->jobs[slot].cmd), "process %d", pid);
    }

    state->previous_job_idx = state->current_job_idx;
    state->current_job_idx = slot;
    state->job_count++;
    return slot;
}

void zbash_jobs_update_status(zbash_state_t *state) {
    if (!state) return;
    for (int i = 0; i < 64; i++) {
        if (state->jobs[i].pid > 0) {
            int status = 0;
            pid_t r = waitpid(state->jobs[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
            if (r > 0) {
                if (WIFSTOPPED(status)) {
                    state->jobs[i].status = 1; /* Stopped */
                } else if (WIFCONTINUED(status)) {
                    state->jobs[i].status = 0; /* Running */
                } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    state->jobs[i].status = 2; /* Done */
                }
            }
        }
    }
}

int zbash_parse_jobspec(zbash_state_t *state, const char *spec, pid_t *out_pid, int *out_job_idx) {
    if (!state || !spec) return -2;
    zbash_jobs_update_status(state);

    if (spec[0] == '%') {
        const char *s = spec + 1;
        if (*s == '\0' || strcmp(s, "%") == 0 || strcmp(s, "+") == 0) {
            /* %+ or %%: current job */
            int cur = state->current_job_idx;
            if (cur >= 0 && cur < 64 && state->jobs[cur].pid > 0 && !state->jobs[cur].disowned) {
                if (out_pid) *out_pid = state->jobs[cur].pid;
                if (out_job_idx) *out_job_idx = cur;
                return 0;
            }
            return -1;
        }
        if (strcmp(s, "-") == 0) {
            /* %-: previous job */
            int prev = state->previous_job_idx;
            if (prev >= 0 && prev < 64 && state->jobs[prev].pid > 0 && !state->jobs[prev].disowned) {
                if (out_pid) *out_pid = state->jobs[prev].pid;
                if (out_job_idx) *out_job_idx = prev;
                return 0;
            }
            return -1;
        }
        if (isdigit((unsigned char)*s)) {
            int target_id = atoi(s);
            for (int i = 0; i < 64; i++) {
                if (state->jobs[i].pid > 0 && state->jobs[i].job_id == target_id && !state->jobs[i].disowned) {
                    if (out_pid) *out_pid = state->jobs[i].pid;
                    if (out_job_idx) *out_job_idx = i;
                    return 0;
                }
            }
            return -1;
        }
        if (*s == '?') {
            const char *pat = s + 1;
            for (int i = 63; i >= 0; i--) {
                if (state->jobs[i].pid > 0 && !state->jobs[i].disowned && strstr(state->jobs[i].cmd, pat)) {
                    if (out_pid) *out_pid = state->jobs[i].pid;
                    if (out_job_idx) *out_job_idx = i;
                    return 0;
                }
            }
            return -1;
        }
        /* %string */
        for (int i = 63; i >= 0; i--) {
            if (state->jobs[i].pid > 0 && !state->jobs[i].disowned && strncmp(state->jobs[i].cmd, s, strlen(s)) == 0) {
                if (out_pid) *out_pid = state->jobs[i].pid;
                if (out_job_idx) *out_job_idx = i;
                return 0;
            }
        }
        return -1;
    }

    /* Raw number / PID check */
    bool all_digits = true;
    for (int k = 0; spec[k]; k++) {
        if (!isdigit((unsigned char)spec[k])) {
            all_digits = false;
            break;
        }
    }
    if (all_digits && spec[0] != '\0') {
        pid_t p = (pid_t)atoi(spec);
        if (out_pid) *out_pid = p;
        for (int i = 0; i < 64; i++) {
            if (state->jobs[i].pid == p) {
                if (out_job_idx) *out_job_idx = i;
                return 0;
            }
        }
        return 1; /* Valid PID, but not a job slot */
    }

    return -2; /* Invalid syntax */
}

/* ========================================================================= */
/* Builtin: jobs                                                             */
/* ========================================================================= */

int builtin_jobs(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state->lineno > 0 ? state->lineno : 1;

    bool opt_p = false, opt_r = false, opt_s = false;
    int first_spec = argc;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            for (int c = 1; argv[i][c]; c++) {
                char ch = argv[i][c];
                if (ch == 'p') opt_p = true;
                else if (ch == 'r') opt_r = true;
                else if (ch == 's') opt_s = true;
                else {
                    fprintf(stderr, "%s: line %d: jobs: -%c: invalid option\njobs: usage: jobs [-lnprs] [jobspec ...] or jobs -x cmd [args]\n", src, lineno, ch);
                    return 2;
                }
            }
        } else {
            first_spec = i;
            break;
        }
    }

    zbash_jobs_update_status(state);

    if (first_spec < argc) {
        /* Specific jobs requested */
        int rc = 0;
        for (int a = first_spec; a < argc; a++) {
            pid_t p = 0;
            int idx = -1;
            int ret = zbash_parse_jobspec(state, argv[a], &p, &idx);
            if (ret != 0 || idx < 0 || state->jobs[idx].disowned) {
                fprintf(stderr, "%s: line %d: jobs: %s: no such job\n", src, lineno, argv[a]);
                rc = 1;
                continue;
            }
            if (opt_p) {
                printf("%d\n", state->jobs[idx].pid);
            } else {
                char cur_char = ' ';
                if (idx == state->current_job_idx) cur_char = '+';
                else if (idx == state->previous_job_idx) cur_char = '-';
                const char *st = (state->jobs[idx].status == 1) ? "Stopped" : "Running";
                printf("[%d]%c  %-23s %s\n", state->jobs[idx].job_id, cur_char, st, state->jobs[idx].cmd);
            }
        }
        return rc;
    }

    /* Print all jobs in increasing order of job_id */
    for (int target_id = 1; target_id <= 64; target_id++) {
        for (int i = 0; i < 64; i++) {
            if (state->jobs[i].pid > 0 && state->jobs[i].job_id == target_id && !state->jobs[i].disowned) {
                if (opt_r && state->jobs[i].status != 0) continue;
                if (opt_s && state->jobs[i].status != 1) continue;

                if (opt_p) {
                    printf("%d\n", state->jobs[i].pid);
                } else {
                    char cur_char = ' ';
                    if (i == state->current_job_idx) cur_char = '+';
                    else if (i == state->previous_job_idx) cur_char = '-';
                    const char *st = (state->jobs[i].status == 1) ? "Stopped" : "Running";
                    printf("[%d]%c  %-23s %s\n", state->jobs[i].job_id, cur_char, st, state->jobs[i].cmd);
                }
            }
        }
    }
    return 0;
}

/* ========================================================================= */
/* Builtin: fg                                                               */
/* ========================================================================= */

int builtin_fg(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state->lineno > 0 ? state->lineno : 1;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "%s: line %d: fg: %s: invalid option\nfg: usage: fg [job_spec]\n", src, lineno, argv[i]);
            return 2;
        }
    }

    const char *spec = (argc > 1) ? argv[1] : "%+";
    pid_t p = 0;
    int idx = -1;
    int ret = zbash_parse_jobspec(state, spec, &p, &idx);

    if (ret != 0 || idx < 0 || state->jobs[idx].disowned) {
        fprintf(stderr, "%s: line %d: fg: %s: no such job\n", src, lineno, spec);
        return 1;
    }

    if (!state->monitor_mode) {
        fprintf(stderr, "%s: line %d: fg: no job control\n", src, lineno);
        return 1;
    }

    if (state->jobs[idx].started_without_jc) {
        fprintf(stderr, "%s: line %d: fg: job %d started without job control\n", src, lineno, state->jobs[idx].job_id);
        return 1;
    }

    /* Print command without trailing & */
    char clean_cmd[256];
    strncpy(clean_cmd, state->jobs[idx].cmd, sizeof(clean_cmd) - 1);
    clean_cmd[sizeof(clean_cmd) - 1] = '\0';
    size_t clen = strlen(clean_cmd);
    while (clen > 0 && (isspace((unsigned char)clean_cmd[clen - 1]) || clean_cmd[clen - 1] == '&')) {
        clean_cmd[--clen] = '\0';
    }
    printf("%s\n", clean_cmd);

    if (state->jobs[idx].status == 1) {
        kill(state->jobs[idx].pid, SIGCONT);
        state->jobs[idx].status = 0;
    }

    int status = 0;
    waitpid(state->jobs[idx].pid, &status, WUNTRACED);

    if (WIFSTOPPED(status)) {
        state->jobs[idx].status = 1;
        state->current_job_idx = idx;
    } else {
        state->jobs[idx].pid = 0;
        state->jobs[idx].job_id = 0;
        state->job_count--;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 0;
}

/* ========================================================================= */
/* Builtin: bg                                                               */
/* ========================================================================= */

int builtin_bg(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state->lineno > 0 ? state->lineno : 1;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "%s: line %d: bg: %s: invalid option\nbg: usage: bg [job_spec ...]\n", src, lineno, argv[i]);
            return 2;
        }
    }

    int rc = 0;
    int start_arg = (argc > 1) ? 1 : 0;
    int end_arg = (argc > 1) ? argc : 1;

    for (int i = start_arg; i < end_arg; i++) {
        const char *spec = (argc > 1) ? argv[i] : "%+";
        pid_t p = 0;
        int idx = -1;
        int ret = zbash_parse_jobspec(state, spec, &p, &idx);

        if (ret != 0 || idx < 0 || state->jobs[idx].disowned) {
            fprintf(stderr, "%s: line %d: bg: %s: no such job\n", src, lineno, spec);
            rc = 1;
            continue;
        }

        if (!state->monitor_mode) {
            fprintf(stderr, "%s: line %d: bg: no job control\n", src, lineno);
            return 1;
        }

        if (state->jobs[idx].status == 0) {
            fprintf(stderr, "%s: line %d: bg: job %d already in background\n", src, lineno, state->jobs[idx].job_id);
            rc = 1;
            continue;
        }

        /* Send SIGCONT and background */
        kill(state->jobs[idx].pid, SIGCONT);
        state->jobs[idx].status = 0;

        char clean_cmd[256];
        strncpy(clean_cmd, state->jobs[idx].cmd, sizeof(clean_cmd) - 1);
        clean_cmd[sizeof(clean_cmd) - 1] = '\0';
        size_t clen = strlen(clean_cmd);
        while (clen > 0 && (isspace((unsigned char)clean_cmd[clen - 1]) || clean_cmd[clen - 1] == '&')) {
            clean_cmd[--clen] = '\0';
        }

        printf("[%d]+ %s &\n", state->jobs[idx].job_id, clean_cmd);
    }
    return rc;
}

/* ========================================================================= */
/* Builtin: disown                                                           */
/* ========================================================================= */

int builtin_disown(zbash_state_t *state, int argc, char **argv) {
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : ((state->argc > 0 && state->argv) ? state->argv[0] : "zbash");
    int lineno = state->lineno > 0 ? state->lineno : 1;

    int first_arg = argc;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            for (int c = 1; argv[i][c]; c++) {
                char ch = argv[i][c];
                if (ch == 'h' || ch == 'a' || ch == 'r') {
                    /* valid disown flags */
                } else {
                    fprintf(stderr, "%s: line %d: disown: -%c: invalid option\ndisown: usage: disown [-h] [-ar] [jobspec ...]\n", src, lineno, ch);
                    return 2;
                }
            }
        } else {
            first_arg = i;
            break;
        }
    }

    if (first_arg >= argc) {
        /* Disown current job */
        int cur = state->current_job_idx;
        if (cur >= 0 && cur < 64 && state->jobs[cur].pid > 0) {
            state->jobs[cur].disowned = true;
        }
        return 0;
    }

    int rc = 0;
    for (int i = first_arg; i < argc; i++) {
        pid_t p = 0;
        int idx = -1;
        int ret = zbash_parse_jobspec(state, argv[i], &p, &idx);
        if (ret != 0 || idx < 0 || state->jobs[idx].disowned) {
            fprintf(stderr, "%s: line %d: disown: %s: no such job\n", src, lineno, argv[i]);
            rc = 1;
            continue;
        }
        state->jobs[idx].disowned = true;
    }
    return rc;
}
