/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#ifndef ZBASH_BUILTINS_INTERNAL_H
#define ZBASH_BUILTINS_INTERNAL_H

#include "zbash.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* Builtin function prototypes */
int builtin_cd(zbash_state_t *state, int argc, char **argv);
int builtin_pwd(int argc, char **argv);
int builtin_echo(zbash_state_t *state, int argc, char **argv);
int builtin_read(zbash_state_t *state, int argc, char **argv);
int builtin_test(zbash_state_t *state, int argc, char **argv);
int builtin_let(zbash_state_t *state, int argc, char **argv);
int builtin_declare(zbash_state_t *state, int argc, char **argv);
int builtin_kill(zbash_state_t *state, int argc, char **argv);
int builtin_trap(zbash_state_t *state, int argc, char **argv);
int builtin_export(zbash_state_t *state, int argc, char **argv);
int builtin_unset(zbash_state_t *state, int argc, char **argv);
int builtin_set(zbash_state_t *state, int argc, char **argv);
int builtin_shift(zbash_state_t *state, int argc, char **argv);
int builtin_local(zbash_state_t *state, int argc, char **argv);
int builtin_return(zbash_state_t *state, int argc, char **argv);
int builtin_eval(zbash_state_t *state, int argc, char **argv);
int builtin_builtin(zbash_state_t *state, int argc, char **argv);
int builtin_command(zbash_state_t *state, int argc, char **argv);
int builtin_type(zbash_state_t *state, int argc, char **argv);
int builtin_umask(int argc, char **argv);
int builtin_enable(zbash_state_t *state, int argc, char **argv);
int builtin_hash(zbash_state_t *state, int argc, char **argv);
int builtin_times(void);
int builtin_shopt(zbash_state_t *state, int argc, char **argv);
int builtin_alias(zbash_state_t *state, int argc, char **argv);
int builtin_unalias(zbash_state_t *state, int argc, char **argv);
int builtin_exec(zbash_state_t *state, int argc, char **argv);
int builtin_exit(zbash_state_t *state, int argc, char **argv);
int builtin_wait(zbash_state_t *state, int argc, char **argv);
int builtin_printf(zbash_state_t *state, int argc, char **argv);
int builtin_dirs(zbash_state_t *state, int argc, char **argv);
int builtin_pushd(zbash_state_t *state, int argc, char **argv);
int builtin_popd(zbash_state_t *state, int argc, char **argv);
int builtin_caller(zbash_state_t *state, int argc, char **argv);
int builtin_getopts(zbash_state_t *state, int argc, char **argv);
int builtin_help(int argc, char **argv);
int builtin_history(zbash_state_t *state, int argc, char **argv);
int builtin_fc(zbash_state_t *state, int argc, char **argv);
int builtin_jobs(zbash_state_t *state, int argc, char **argv);
int builtin_fg(zbash_state_t *state, int argc, char **argv);
int builtin_bg(zbash_state_t *state, int argc, char **argv);
int builtin_disown(zbash_state_t *state, int argc, char **argv);
int builtin_zedit(zbash_state_t *state, int argc, char **argv);

void zbash_print_func_def(const char *name, const char *raw_body);
zbash_func_t *zbash_find_func(zbash_state_t *state, const char *name);
void print_set_o_table(zbash_state_t *state, bool reusable);
bool set_set_o_option_by_name(zbash_state_t *state, const char *name, bool turn_on);
void zbash_trigger_hook(zbash_state_t *state, const char *hook_name);

#endif /* ZBASH_BUILTINS_INTERNAL_H */
