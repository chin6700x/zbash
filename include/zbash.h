/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZBASH_H
#define ZBASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__APPLE__)
#define zbash_check_exec(path) (faccessat(AT_FDCWD, (path), X_OK, AT_EACCESS) == 0)
#else
#define zbash_check_exec(path) (access((path), X_OK) == 0)
#endif

#define ZBASH_MAX_ARGS 64
#define ZBASH_MAX_PIPELINE 16
#define ZBASH_MAX_LIST 16
#define ZBASH_MAX_FUNCS 64
#define ZBASH_MAX_ARRAYS 64
#define ZBASH_MAX_SOURCE_DEPTH 16
#define ZBASH_MAX_ALIASES 128

typedef struct {
    char *name;
    char *val;
} zbash_alias_t;

#define ZBASH_MAX_VAR_ATTRS 256

#define ZBASH_ATTR_INTEGER  (1 << 0)
#define ZBASH_ATTR_READONLY (1 << 1)
#define ZBASH_ATTR_EXPORT   (1 << 2)
#define ZBASH_ATTR_TRACE    (1 << 3)

typedef struct {
    char *name;
    uint32_t flags;
} zbash_var_attr_t;

typedef struct {
    char *name;
    char *body;
    int start_lineno;
    bool is_readonly;
    bool is_trace;
} zbash_func_t;

typedef struct {
    char *name;
    char **elements;
    int count;
    int cap;
} zbash_array_t;

#define ZBASH_MAX_FRAMES 128

typedef struct {
    const char *func_name;
    const char *file_name;
    int call_line;
    int def_line;
} zbash_frame_t;

typedef struct {
    int last_status;     /* $? */
    pid_t shell_pid;     /* $$ */
    bool errexit;        /* -e */
    bool xtrace;         /* -x */
    bool noexec;         /* -n */
    bool verbose;        /* -v */
    bool expansion_error;
    bool should_exit;
    int exit_code;
    bool return_called;
    int return_code;
    bool loop_break;
    int loop_break_count;
    bool loop_continue;
    int loop_continue_count;
    int loop_depth;
    int argc;
    char **argv;

    /* Sourcing & BASH_SOURCE tracking */
    char *source_stack[ZBASH_MAX_SOURCE_DEPTH];
    int source_depth;

    /* User-defined shell functions */
    zbash_func_t funcs[ZBASH_MAX_FUNCS];
    int func_count;

    /* Shell arrays */
    zbash_array_t arrays[ZBASH_MAX_ARRAYS];
    int array_count;

    /* Function call depth & local variable scope stack */
    int func_depth;
    zbash_frame_t frames[ZBASH_MAX_FRAMES];
    int frame_count;
    struct {
        char *name;
        char *old_val;
        bool was_set;
    } local_vars[512];
    int local_var_count;

    /* Background jobs & signal/exit traps */
    pid_t last_bg_pid;
    char *exit_trap;
    char *sig_traps[32];
    bool sig_inherited_ign[32];
    bool had_subshell;
    int last_subshell_status;

    /* Aliases */
    zbash_alias_t aliases[ZBASH_MAX_ALIASES];
    int alias_count;
    bool expand_aliases;

    /* Execution tracking & variable attributes */
    int lineno;
    int stmt_start_lineno;
    zbash_var_attr_t var_attrs[ZBASH_MAX_VAR_ATTRS];
    int var_attr_count;
    bool posix;
    bool interactive;
    bool restricted;
    bool nounset;
    bool noclobber;
    int stdin_redir_depth;
    bool stdin_permanently_redirected;
    bool in_cmd_args;
    bool in_rhs;

    /* Shell options & builtin control */
    bool xpg_echo;
    bool sourcepath;
    bool extglob;
    char disabled_builtins[32][32];
    int disabled_builtin_count;
    bool positional_params_modified;
    bool in_cond_cmd;
    bool cmd_subst_occurred;
    const char *current_arith_expr;

    /* Debug & tracing */
    bool extdebug;
    bool functrace;
    char *debug_trap;
    char *return_trap;
    bool in_debug_trap;
    bool in_return_trap;
    bool suppress_debug_trap;
    /* Hashed commands */
    struct {
        char name[64];
        char path[256];
        int hits;
    } hash_table[128];
    int hash_count;
    int getopts_subindex;
    int getopts_last_optind;

    /* History & line editing */
    bool history_enabled;      /* set -o history */
    bool histexp_enabled;      /* set -H */
    char **history_lines;
    int history_count;
    int history_cap;
    int history_base;          /* start line index, default 1 */
    int history_last_written;  /* for history -a */
    bool history_file_loaded;
    char hist_subst_pat[128];  /* for :& repeating substitution */
    char hist_subst_rep[128];
    char hist_search_pat[128]; /* for !?string? and :% */
    char hist_search_word[128];
    int run_line_depth;

    /* Jobs & job control */
    bool monitor_mode;         /* set -m */
    struct {
        int job_id;            /* 1, 2, ... */
        pid_t pid;
        pid_t pgid;
        int status;            /* 0=Running, 1=Stopped, 2=Done */
        char cmd[256];
        bool notified;
        bool disowned;
        bool started_without_jc; /* if started when set +m */
    } jobs[64];
    int job_count;
    int current_job_idx;       /* index of current job %+ */
    int previous_job_idx;      /* index of previous job %- */
} zbash_state_t;

#define ZBASH_MAX_REDIRS 16

typedef enum {
    REDIR_NONE = 0,
    REDIR_IN,           /* < file */
    REDIR_OUT,          /* > file */
    REDIR_APPEND,       /* >> file */
    REDIR_CLOBBER,      /* >| file */
    REDIR_RDWR,         /* <> file */
    REDIR_DUP_IN,       /* <& target_fd */
    REDIR_DUP_OUT,      /* >& target_fd */
    REDIR_CLOSE_IN,     /* <&- */
    REDIR_CLOSE_OUT,    /* >&- */
    REDIR_HERESTR,      /* <<< string */
    REDIR_AMBIGUOUS     /* ambiguous redirect */
} zbash_redir_type_t;

typedef struct {
    zbash_redir_type_t type;
    int src_fd;
    int target_fd;
    char *filename;
} zbash_redir_t;

typedef struct {
    char *argv[ZBASH_MAX_ARGS];
    int argc;
    int prefix_assign_count;
    char *in_file;
    char *out_file;
    bool append_out;
    char *err_file;
    bool append_err;
    bool err_to_out;     /* 2>&1 */
    bool out_to_err;     /* >&2, 1>&2 */
    zbash_redir_t redirs[ZBASH_MAX_REDIRS];
    int redir_count;
} zbash_cmd_t;

typedef enum {
    CONN_NONE,
    CONN_AND,     /* && */
    CONN_OR,      /* || */
    CONN_SEMI     /* ; */
} zbash_conn_t;

typedef struct {
    zbash_cmd_t cmds[ZBASH_MAX_PIPELINE];
    int cmd_count;
    zbash_conn_t next_conn;
} zbash_pipeline_t;

/* Shell initialization and lifecycle */
void zbash_init(zbash_state_t *state, int argc, char **argv);
int zbash_run_line(zbash_state_t *state, const char *line);
int zbash_run_file(zbash_state_t *state, const char *filepath);
int zbash_run_stream(zbash_state_t *state, void *stream);
int zbash_run_interactive(zbash_state_t *state);
int zbash_source_file(zbash_state_t *state, const char *filepath);
int zbash_source_file_args(zbash_state_t *state, const char *filepath, int argc, char **argv);

/* Builtin handlers */
bool zbash_is_builtin(const char *name);
bool zbash_is_builtin_enabled(zbash_state_t *state, const char *name);
int zbash_exec_builtin(zbash_state_t *state, int argc, char **argv);

/* Function table */
int zbash_register_func(zbash_state_t *state, const char *name, const char *body, int start_lineno);
zbash_func_t *zbash_find_func(zbash_state_t *state, const char *name);
int zbash_remove_func(zbash_state_t *state, const char *name);
int zbash_exec_func(zbash_state_t *state, zbash_func_t *fn, int argc, char **argv);
int zbash_eval_cond(zbash_state_t *state, const char *expr);

/* Array table */
void zbash_set_array(zbash_state_t *state, const char *name, int count, char **elements);
zbash_array_t *zbash_find_array(zbash_state_t *state, const char *name);
const char *zbash_get_array_elem(zbash_state_t *state, const char *name, int index);
void zbash_set_array_elem(zbash_state_t *state, const char *name, int index, const char *val);
void zbash_unset_array(zbash_state_t *state, const char *name);
void zbash_unset_array_elem(zbash_state_t *state, const char *name, int index);
bool check_array_def(zbash_state_t *state, const char *p);
bool check_array_def_ext(zbash_state_t *state, const char *p, bool expand);

/* Variable attributes */
uint32_t zbash_get_var_attr(zbash_state_t *state, const char *name);
void zbash_set_var_attr(zbash_state_t *state, const char *name, uint32_t flag);
void zbash_clear_var_attr(zbash_state_t *state, const char *name, uint32_t flag);
void zbash_remove_var_attr(zbash_state_t *state, const char *name);
bool zbash_is_readonly(zbash_state_t *state, const char *name);
bool zbash_is_integer(zbash_state_t *state, const char *name);
int zbash_assign_var(zbash_state_t *state, const char *name, const char *val, bool is_append);

/* Subshell and expansion */
char *zbash_expand_vars(zbash_state_t *state, const char *input);
char *zbash_expand_heredoc(zbash_state_t *state, const char *input);
bool zbash_get_param_value(zbash_state_t *state, const char *vname, char *buf, size_t buf_sz, const char **out_val, char **out_dynamic);
char *zbash_subshell_exec(zbash_state_t *state, const char *cmd);
long long zbash_eval_arithmetic(zbash_state_t *state, const char *expr);
void zbash_parse_simple_cmd(zbash_state_t *state, const char *cmd_str, zbash_cmd_t *cmd);
char **zbash_expand_braces(const char *word);
void zbash_free_string_list(char **list);
void zbash_sync_call_stack_arrays(zbash_state_t *state);
int zbash_trigger_debug_trap(zbash_state_t *state, int line_num);

/* ARM64 NEON Vector Accelerators */
#if defined(__arm64__) || defined(__aarch64__)
size_t zbash_simd_skip_spaces(const char *str, size_t len);
size_t zbash_simd_scan_token(const char *str, size_t len);
#endif

/* Pattern matching (src/parser/pattern.c) */
#define ZBASH_FNM_EXTMATCH 1
#define ZBASH_FNM_PATHNAME 2
#define ZBASH_FNM_PERIOD   4
#define ZBASH_FNM_NOESCAPE 8
#define ZBASH_FNM_CASEFOLD 16
int zbash_fnmatch(const char *pattern, const char *string, int flags);
bool zbash_has_glob(zbash_state_t *state, const char *word);
int zbash_expand_path_glob(zbash_state_t *state, const char *pattern, char ***out_matches, int *out_count);

/* History management & expansion */
void zbash_history_init(zbash_state_t *state);
void zbash_history_free(zbash_state_t *state);
void zbash_history_add(zbash_state_t *state, const char *line);
int zbash_history_load(zbash_state_t *state, const char *filename);
int zbash_histexp_expand(zbash_state_t *state, const char *line, char **out_expanded);

/* Job management */
int zbash_jobs_add(zbash_state_t *state, pid_t pid, const char *cmd, bool started_without_jc);
void zbash_jobs_update_status(zbash_state_t *state);
int zbash_parse_jobspec(zbash_state_t *state, const char *spec, pid_t *out_pid, int *out_job_idx);

#endif /* ZBASH_H */
