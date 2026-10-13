/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

#include "flag_db.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* 1. Flag & Option Definitions (Tier 1 Builtins & Tier 2 Top CLI Tools)     */
/* ========================================================================= */

/* --- read --- */
static const zbash_flag_entry_t s_flags_read[] = {
    {'r', NULL, NULL, "do not allow backslashes to escape characters"},
    {'p', NULL, "<prompt>", "output string PROMPT without trailing newline"},
    {'t', NULL, "<timeout>", "time out after TIMEOUT seconds"},
    {'a', NULL, "<array>", "assign words to sequential indices of ARRAY"},
    {'n', NULL, "<nchars>", "read only NCHARS characters"},
    {'N', NULL, "<nchars>", "read NCHARS characters without honoring delimiter"},
    {'s', NULL, NULL, "silent mode (do not echo input)"},
    {'d', NULL, "<delim>", "read until first character of DELIM"},
    {'u', NULL, "<fd>", "read from file descriptor FD"},
    {0, NULL, NULL, NULL}
};

/* --- cd --- */
static const zbash_flag_entry_t s_flags_cd[] = {
    {'L', NULL, NULL, "force symbolic links to be followed"},
    {'P', NULL, NULL, "use physical directory structure without following symlinks"},
    {'e', NULL, NULL, "exit with non-zero status if target directory cannot be determined"},
    {'@', NULL, NULL, "display extended attribute keys (Darwin)"},
    {0, NULL, NULL, NULL}
};

/* --- pwd --- */
static const zbash_flag_entry_t s_flags_pwd[] = {
    {'L', NULL, NULL, "print value of $PWD, honoring symlinks"},
    {'P', NULL, NULL, "print physical directory, resolving all symlinks"},
    {0, NULL, NULL, NULL}
};

/* --- echo --- */
static const zbash_flag_entry_t s_flags_echo[] = {
    {'n', NULL, NULL, "do not output trailing newline"},
    {'e', NULL, NULL, "enable interpretation of backslash escapes (\\n, \\t, etc.)"},
    {'E', NULL, NULL, "disable interpretation of backslash escapes"},
    {0, NULL, NULL, NULL}
};

/* --- export --- */
static const zbash_flag_entry_t s_flags_export[] = {
    {'p', NULL, NULL, "display all exported variables in reuse format"},
    {'n', NULL, NULL, "remove export property from each NAME"},
    {'f', NULL, NULL, "treat names as functions rather than variables"},
    {0, NULL, NULL, NULL}
};

/* --- declare / typeset --- */
static const zbash_flag_entry_t s_flags_declare[] = {
    {'a', NULL, NULL, "declare indexed array variable"},
    {'A', NULL, NULL, "declare associative array variable"},
    {'i', NULL, NULL, "treat variable as integer"},
    {'l', NULL, NULL, "convert value to lowercase on assignment"},
    {'u', NULL, NULL, "convert value to uppercase on assignment"},
    {'r', NULL, NULL, "mark variable as read-only"},
    {'x', NULL, NULL, "mark variable for export to environment"},
    {'p', NULL, NULL, "display attributes and values of variables"},
    {'f', NULL, NULL, "restrict display to function definitions"},
    {'F', NULL, NULL, "display function names without definitions"},
    {0, NULL, NULL, NULL}
};

/* --- readonly --- */
static const zbash_flag_entry_t s_flags_readonly[] = {
    {'p', NULL, NULL, "display all read-only variables"},
    {'a', NULL, NULL, "refer to indexed array variables"},
    {'A', NULL, NULL, "refer to associative array variables"},
    {'f', NULL, NULL, "refer to function names"},
    {0, NULL, NULL, NULL}
};

/* --- unset --- */
static const zbash_flag_entry_t s_flags_unset[] = {
    {'v', NULL, NULL, "treat each NAME as a shell variable"},
    {'f', NULL, NULL, "treat each NAME as a shell function"},
    {0, NULL, NULL, NULL}
};

/* --- history --- */
static const zbash_flag_entry_t s_flags_history[] = {
    {'c', NULL, NULL, "clear history list"},
    {'d', NULL, "<offset>", "delete history entry at OFFSET"},
    {'a', NULL, "[file]", "append new history lines to history file"},
    {'w', NULL, "[file]", "write current history to history file"},
    {'r', NULL, "[file]", "read history file and append to current history"},
    {0, NULL, NULL, NULL}
};

/* --- set --- */
static const zbash_flag_entry_t s_flags_set[] = {
    {'e', NULL, NULL, "exit immediately if a command exits with non-zero status"},
    {'u', NULL, NULL, "treat unset variables as an error when substituting"},
    {'x', NULL, NULL, "print commands and their arguments as they are executed"},
    {'v', NULL, NULL, "print shell input lines as they are read"},
    {'f', NULL, NULL, "disable pathname expansion (globbing)"},
    {'o', NULL, "<option>", "set named option (errexit, pipefail, nounset, etc.)"},
    {0, NULL, NULL, NULL}
};

/* --- shopt --- */
static const zbash_flag_entry_t s_flags_shopt[] = {
    {'s', NULL, "<opt>", "enable (set) each OPTNAME"},
    {'u', NULL, "<opt>", "disable (unset) each OPTNAME"},
    {'p', NULL, NULL, "list settable options in reuse format"},
    {'q', NULL, NULL, "suppress normal output; return exit status 0 if set"},
    {0, NULL, NULL, NULL}
};

/* --- jobs --- */
static const zbash_flag_entry_t s_flags_jobs[] = {
    {'l', NULL, NULL, "list process IDs in addition to normal job info"},
    {'p', NULL, NULL, "list only process ID of job's process group leader"},
    {'n', NULL, NULL, "display only jobs whose status changed since last notification"},
    {'r', NULL, NULL, "restrict output to running jobs only"},
    {'s', NULL, NULL, "restrict output to stopped jobs only"},
    {0, NULL, NULL, NULL}
};

/* --- kill --- */
static const zbash_flag_entry_t s_flags_kill[] = {
    {'l', NULL, NULL, "list signal names"},
    {'s', NULL, "<signal>", "specify signal to send by name (e.g. TERM, KILL)"},
    {'n', NULL, "<signum>", "specify signal to send by number"},
    {0, NULL, NULL, NULL}
};

/* --- trap --- */
static const zbash_flag_entry_t s_flags_trap[] = {
    {'l', NULL, NULL, "print list of signal names and numbers"},
    {'p', NULL, NULL, "display trap commands associated with each signal"},
    {0, NULL, NULL, NULL}
};

/* --- fc --- */
static const zbash_flag_entry_t s_flags_fc[] = {
    {'l', NULL, NULL, "list lines in history instead of editing"},
    {'n', NULL, NULL, "suppress line numbers when listing"},
    {'r', NULL, NULL, "reverse the order of the lines"},
    {'s', NULL, "<pat=rep>", "re-execute command without invoking an editor"},
    {0, NULL, NULL, NULL}
};

/* --- dirs --- */
static const zbash_flag_entry_t s_flags_dirs[] = {
    {'c', NULL, NULL, "clear directory stack"},
    {'l', NULL, NULL, "produce longer listing without using ~ prefix"},
    {'p', NULL, NULL, "print directory stack with one entry per line"},
    {'v', NULL, NULL, "produce vertical listing with index prefix"},
    {0, NULL, NULL, NULL}
};

/* --- pushd / popd --- */
static const zbash_flag_entry_t s_flags_pushd[] = {
    {'n', NULL, NULL, "suppress normal directory change when manipulating stack"},
    {0, NULL, NULL, NULL}
};

/* --- type --- */
static const zbash_flag_entry_t s_flags_type[] = {
    {'a', NULL, NULL, "display all locations containing executable named NAME"},
    {'t', NULL, NULL, "print word describing type (alias, builtin, file, function)"},
    {'p', NULL, NULL, "print disk file path that would be executed"},
    {'P', NULL, NULL, "force PATH search, ignoring builtins and functions"},
    {0, NULL, NULL, NULL}
};

/* --- which --- */
static const zbash_flag_entry_t s_flags_which[] = {
    {'a', NULL, NULL, "print all matching pathnames of each argument"},
    {'s', NULL, NULL, "silent mode; return 0 if found, 1 if not"},
    {0, NULL, NULL, NULL}
};

/* --- help --- */
static const zbash_flag_entry_t s_flags_help[] = {
    {'d', NULL, NULL, "output short description for each topic"},
    {'m', NULL, NULL, "display usage in pseudo-manpage format"},
    {'s', NULL, NULL, "output only a short usage synopsis"},
    {0, NULL, NULL, NULL}
};

/* ========================================================================= */
/* Tier 2: Top Core CLI Tools                                                */
/* ========================================================================= */

/* --- tar --- */
static const zbash_flag_entry_t s_flags_tar[] = {
    {'c', "--create", NULL, "create a new archive"},
    {'x', "--extract", NULL, "extract files from an archive"},
    {'t', "--list", NULL, "list contents of an archive"},
    {'z', "--gzip", NULL, "compress/decompress archive using gzip"},
    {'j', "--bzip2", NULL, "compress/decompress archive using bzip2"},
    {'J', "--xz", NULL, "compress/decompress archive using xz"},
    {'v', "--verbose", NULL, "verbosely list files processed"},
    {'f', "--file", "<archive>", "use archive file or device ARCHIVE"},
    {'C', "--directory", "<dir>", "change to directory DIR before operation"},
    {'p', "--preserve-permissions", NULL, "preserve file permissions and mode"},
    {0, NULL, NULL, NULL}
};

/* --- ls --- */
static const zbash_flag_entry_t s_flags_ls[] = {
    {'l', NULL, NULL, "use long listing format (permissions, owner, size, date)"},
    {'a', "--all", NULL, "do not ignore entries starting with . (hidden files)"},
    {'A', "--almost-all", NULL, "list hidden files except . and .."},
    {'h', "--human-readable", NULL, "print sizes in human readable format (1K, 234M, 2G)"},
    {'t', NULL, NULL, "sort by modification time, newest first"},
    {'r', "--reverse", NULL, "reverse order while sorting"},
    {'S', NULL, NULL, "sort by file size, largest first"},
    {'R', "--recursive", NULL, "list subdirectories recursively"},
    {'1', NULL, NULL, "list one file per line"},
    {'G', "--color", NULL, "enable colorized output"},
    {0, NULL, NULL, NULL}
};

/* --- grep --- */
static const zbash_flag_entry_t s_flags_grep[] = {
    {'i', "--ignore-case", NULL, "ignore case distinctions in patterns and input"},
    {'v', "--invert-match", NULL, "select non-matching lines"},
    {'r', "--recursive", NULL, "search directories recursively"},
    {'R', "--dereference-recursive", NULL, "recursively search directories following symlinks"},
    {'n', "--line-number", NULL, "prefix each line of output with its 1-based line number"},
    {'c', "--count", NULL, "print only a count of selected lines per file"},
    {'l', "--files-with-matches", NULL, "print only names of FILEs with matching lines"},
    {'L', "--files-without-match", NULL, "print only names of FILEs containing no match"},
    {'E', "--extended-regexp", NULL, "interpret pattern as extended regular expression (ERE)"},
    {'F', "--fixed-strings", NULL, "interpret pattern as fixed strings, not regex"},
    {'w', "--word-regexp", NULL, "match whole words only"},
    {'x', "--line-regexp", NULL, "match whole lines only"},
    {'o', "--only-matching", NULL, "show only the non-empty parts of lines matching"},
    {'q', "--quiet", NULL, "quiet mode; exit immediately with 0 if match found"},
    {0, NULL, NULL, NULL}
};

/* --- find --- */
static const zbash_flag_entry_t s_flags_find[] = {
    {0, "-name", "<pattern>", "match filename against shell pattern"},
    {0, "-iname", "<pattern>", "case-insensitive filename match"},
    {0, "-type", "<f|d|l>", "filter by file type: f (file), d (dir), l (symlink)"},
    {0, "-maxdepth", "<levels>", "descend at most LEVELS directory levels"},
    {0, "-mindepth", "<levels>", "do not apply tests at levels less than LEVELS"},
    {0, "-mtime", "<days>", "file data was last modified DAYS*24 hours ago"},
    {0, "-size", "<size>", "file uses SIZE units of space (c, k, M, G)"},
    {0, "-exec", "<cmd> ;", "execute command on each matched file"},
    {0, "-delete", NULL, "delete matched files"},
    {0, "-print", NULL, "print full filename followed by newline"},
    {0, "-print0", NULL, "print full filename followed by null character"},
    {0, NULL, NULL, NULL}
};

/* --- cp --- */
static const zbash_flag_entry_t s_flags_cp[] = {
    {'r', "--recursive", NULL, "copy directories recursively"},
    {'R', NULL, NULL, "copy directory trees recursively"},
    {'p', "--preserve", NULL, "preserve file mode, ownership, and timestamps"},
    {'a', "--archive", NULL, "archive mode (same as -dpR)"},
    {'f', "--force", NULL, "force copy by unlinking existing destination files"},
    {'i', "--interactive", NULL, "prompt before overwriting existing files"},
    {'v', "--verbose", NULL, "explain what is being done"},
    {0, NULL, NULL, NULL}
};

/* --- mv --- */
static const zbash_flag_entry_t s_flags_mv[] = {
    {'f', "--force", NULL, "do not prompt before overwriting"},
    {'i', "--interactive", NULL, "prompt before overwriting destination"},
    {'n', "--no-clobber", NULL, "do not overwrite an existing file"},
    {'v', "--verbose", NULL, "explain what is being done"},
    {0, NULL, NULL, NULL}
};

/* --- rm --- */
static const zbash_flag_entry_t s_flags_rm[] = {
    {'r', "--recursive", NULL, "remove directories and their contents recursively"},
    {'R', NULL, NULL, "equivalent to -r (remove recursively)"},
    {'f', "--force", NULL, "ignore nonexistent files and arguments, never prompt"},
    {'i', "--interactive", NULL, "prompt before every removal"},
    {'v', "--verbose", NULL, "explain what is being done"},
    {0, NULL, NULL, NULL}
};

/* --- mkdir --- */
static const zbash_flag_entry_t s_flags_mkdir[] = {
    {'p', "--parents", NULL, "create parent directories as needed, no error if existing"},
    {'m', "--mode", "<mode>", "set file permissions mode (as in chmod)"},
    {'v', "--verbose", NULL, "print a message for each created directory"},
    {0, NULL, NULL, NULL}
};

/* --- chmod --- */
static const zbash_flag_entry_t s_flags_chmod[] = {
    {'R', "--recursive", NULL, "change files and directories recursively"},
    {'v', "--verbose", NULL, "output a diagnostic for every file processed"},
    {'c', "--changes", NULL, "like verbose but report only when a change is made"},
    {0, NULL, NULL, NULL}
};

/* --- chown --- */
static const zbash_flag_entry_t s_flags_chown[] = {
    {'R', "--recursive", NULL, "operate on files and directories recursively"},
    {'v', "--verbose", NULL, "output a diagnostic for every file processed"},
    {'h', "--no-dereference", NULL, "affect symlinks instead of referenced files"},
    {0, NULL, NULL, NULL}
};

/* --- cat --- */
static const zbash_flag_entry_t s_flags_cat[] = {
    {'n', "--number", NULL, "number all output lines starting from 1"},
    {'b', "--number-nonblank", NULL, "number non-empty output lines only"},
    {'s', "--squeeze-blank", NULL, "suppress repeated empty output lines"},
    {'v', "--show-nonprinting", NULL, "display non-printing characters using ^ and M-"},
    {'e', NULL, NULL, "display $ at end of each line (BSD: -e)"},
    {'t', NULL, NULL, "display TAB characters as ^I"},
    {0, NULL, NULL, NULL}
};

/* --- wc --- */
static const zbash_flag_entry_t s_flags_wc[] = {
    {'l', "--lines", NULL, "print newline counts"},
    {'w', "--words", NULL, "print word counts"},
    {'c', "--bytes", NULL, "print byte counts"},
    {'m', "--chars", NULL, "print character counts"},
    {'L', "--max-line-length", NULL, "print length of longest line"},
    {0, NULL, NULL, NULL}
};

/* --- head --- */
static const zbash_flag_entry_t s_flags_head[] = {
    {'n', "--lines", "<num>", "print first NUM lines instead of default 10"},
    {'c', "--bytes", "<num>", "print first NUM bytes of each file"},
    {'q', "--quiet", NULL, "never print headers giving file names"},
    {'v', "--verbose", NULL, "always print headers giving file names"},
    {0, NULL, NULL, NULL}
};

/* --- tail --- */
static const zbash_flag_entry_t s_flags_tail[] = {
    {'n', "--lines", "<num>", "output last NUM lines instead of default 10"},
    {'f', "--follow", NULL, "output appended data as the file grows"},
    {'F', NULL, NULL, "follow by filename with retry if file recreated"},
    {'c', "--bytes", "<num>", "output last NUM bytes"},
    {'q', "--quiet", NULL, "never output headers giving file names"},
    {0, NULL, NULL, NULL}
};

/* --- sort --- */
static const zbash_flag_entry_t s_flags_sort[] = {
    {'n', "--numeric-sort", NULL, "compare according to string numerical value"},
    {'r', "--reverse", NULL, "reverse the result of comparisons"},
    {'u', "--unique", NULL, "output only the first of an equal run"},
    {'k', "--key", "<pos>", "sort by field at key POS"},
    {'t', "--field-separator", "<sep>", "use SEP instead of whitespace as separator"},
    {'h', "--human-numeric-sort", NULL, "compare human readable numbers (2K, 1G)"},
    {'f', "--ignore-case", NULL, "fold lower case to upper case characters"},
    {0, NULL, NULL, NULL}
};

/* --- uniq --- */
static const zbash_flag_entry_t s_flags_uniq[] = {
    {'c', "--count", NULL, "prefix lines by number of occurrences"},
    {'d', "--repeated", NULL, "only print duplicate lines, one for each group"},
    {'u', "--unique", NULL, "only print unique lines"},
    {'i', "--ignore-case", NULL, "ignore differences in case when comparing"},
    {0, NULL, NULL, NULL}
};

/* --- cut --- */
static const zbash_flag_entry_t s_flags_cut[] = {
    {'d', "--delimiter", "<delim>", "use DELIM instead of TAB for field delimiter"},
    {'f', "--fields", "<list>", "select only these fields (e.g. 1,3 or 1-5)"},
    {'c', "--characters", "<list>", "select only these characters"},
    {'b', "--bytes", "<list>", "select only these bytes"},
    {0, NULL, NULL, NULL}
};

/* --- tr --- */
static const zbash_flag_entry_t s_flags_tr[] = {
    {'d', "--delete", NULL, "delete characters in SET1, do not translate"},
    {'s', "--squeeze-repeats", NULL, "replace repeated characters with single char"},
    {'c', "--complement", NULL, "use the complement of SET1"},
    {0, NULL, NULL, NULL}
};

/* --- git --- */
static const zbash_flag_entry_t s_flags_git[] = {
    {'m', "--message", "<msg>", "commit message"},
    {'b', NULL, "<branch>", "create and switch to new branch"},
    {'a', "--all", NULL, "stage all modified and deleted files"},
    {'p', "--patch", NULL, "interactively choose hunks of patch"},
    {'v', "--verbose", NULL, "be verbose in output"},
    {0, "--version", NULL, "print git suite version"},
    {0, "--help", NULL, "print git help summary"},
    {0, NULL, NULL, NULL}
};

/* --- curl --- */
static const zbash_flag_entry_t s_flags_curl[] = {
    {'s', "--silent", NULL, "silent mode (don't show progress or errors)"},
    {'S', "--show-error", NULL, "when used with -s, show error if it fails"},
    {'o', "--output", "<file>", "write output to FILE instead of stdout"},
    {'O', "--remote-name", NULL, "write output to local file named like remote"},
    {'L', "--location", NULL, "follow HTTP redirects (3xx responses)"},
    {'I', "--head", NULL, "fetch HTTP response headers only (HEAD request)"},
    {'X', "--request", "<method>", "specify HTTP method (e.g. POST, PUT, DELETE)"},
    {'H', "--header", "<header>", "pass custom HTTP header (e.g. 'Auth: token')"},
    {'d', "--data", "<data>", "send specified data in HTTP POST request"},
    {'u', "--user", "<usr:pwd>", "specify user and password for authentication"},
    {'k', "--insecure", NULL, "allow insecure SSL connections without cert validation"},
    {'v', "--verbose", NULL, "make curl verbose, printing request/response"},
    {0, NULL, NULL, NULL}
};

/* --- ssh --- */
static const zbash_flag_entry_t s_flags_ssh[] = {
    {'p', NULL, "<port>", "port to connect to on remote host"},
    {'i', NULL, "<identity>", "identity (private key) file for authentication"},
    {'v', NULL, NULL, "verbose mode (debugging messages)"},
    {'N', NULL, NULL, "do not execute remote command (port forwarding only)"},
    {'L', NULL, "<spec>", "local port forwarding: [bind_addr:]port:host:hostport"},
    {'R', NULL, "<spec>", "remote port forwarding: [bind_addr:]port:host:hostport"},
    {'D', NULL, "<port>", "dynamic application-level port forwarding (SOCKS)"},
    {'C', NULL, NULL, "request compression of all data"},
    {'X', NULL, NULL, "enable X11 forwarding"},
    {0, NULL, NULL, NULL}
};

/* --- make --- */
static const zbash_flag_entry_t s_flags_make[] = {
    {'j', "--jobs", "<n>", "number of jobs to run simultaneously"},
    {'f', "--file", "<file>", "read FILE as a makefile"},
    {'B', "--always-make", NULL, "unconditionally make all targets"},
    {'C', "--directory", "<dir>", "change to DIR before reading makefiles"},
    {'k', "--keep-going", NULL, "keep going when some targets fail"},
    {'n', "--just-print", NULL, "print commands without executing them"},
    {'s', "--silent", NULL, "don't echo recipe commands"},
    {0, NULL, NULL, NULL}
};

/* --- docker --- */
static const zbash_flag_entry_t s_flags_docker[] = {
    {'d', "--detach", NULL, "run container in background and print container ID"},
    {'p', "--publish", "<host:cnt>", "publish container port(s) to host"},
    {'v', "--volume", "<src:dst>", "bind mount a volume"},
    {'e', "--env", "<k=v>", "set environment variable"},
    {'i', "--interactive", NULL, "keep STDIN open even if not attached"},
    {'t', "--tty", NULL, "allocate a pseudo-TTY"},
    {0, "--name", "<name>", "assign a name to the container"},
    {0, "--rm", NULL, "automatically remove container when it exits"},
    {0, NULL, NULL, NULL}
};

/* ========================================================================= */
/* 2. Master Command Registry Table                                          */
/* ========================================================================= */

static const zbash_cmd_desc_t s_cmd_table[] = {
    /* Builtins */
    {"read",     "read [-r] [-p prompt] [-t timeout] [-a array] [name ...]", "read line from standard input into shell variables", s_flags_read},
    {"cd",       "cd [-L|-P] [dir]", "change current working directory", s_flags_cd},
    {"pwd",      "pwd [-L|-P]", "print name of current/working directory", s_flags_pwd},
    {"echo",     "echo [-n|-e|-E] [arg ...]", "write arguments to standard output", s_flags_echo},
    {"export",   "export [-n] [-f] [-p] [name[=val] ...]", "set export attribute for shell variables", s_flags_export},
    {"declare",  "declare [-a|-A|-i|-l|-u|-r|-x] [name[=val] ...]", "set variable values and attributes", s_flags_declare},
    {"typeset",  "typeset [-a|-A|-i|-l|-u|-r|-x] [name[=val] ...]", "set variable values and attributes (alias for declare)", s_flags_declare},
    {"readonly", "readonly [-a|-A|-f|-p] [name[=val] ...]", "mark variables as unchangeable and read-only", s_flags_readonly},
    {"unset",    "unset [-v|-f] [name ...]", "unset values and attributes of variables and functions", s_flags_unset},
    {"history",  "history [-c] [-d offset] [-a|-w|-r file]", "display or manipulate shell command history list", s_flags_history},
    {"set",      "set [-e|-u|-x|-v|-f] [-o option] [arg ...]", "set or unset shell options and positional parameters", s_flags_set},
    {"shopt",    "shopt [-s|-u|-p|-q] [optname ...]", "set and unset shell options", s_flags_shopt},
    {"jobs",     "jobs [-l|-p|-n|-r|-s] [jobspec ...]", "display status of jobs in current active session", s_flags_jobs},
    {"kill",     "kill [-s signal|-n signum|-l] pid | jobspec", "send signal to processes or jobs", s_flags_kill},
    {"trap",     "trap [-l|-p] [[action] signal_spec ...]", "trap system signals and execution events", s_flags_trap},
    {"fc",       "fc [-l|-n|-r|-s] [first] [last]", "display or re-execute commands from history list", s_flags_fc},
    {"dirs",     "dirs [-c|-l|-p|-v]", "display directory stack list", s_flags_dirs},
    {"pushd",    "pushd [-n] [dir | +N | -N]", "push directory onto directory stack and cd", s_flags_pushd},
    {"popd",     "popd [-n] [+N | -N]", "pop directory from directory stack and cd", s_flags_pushd},
    {"type",     "type [-a|-t|-p|-P] [name ...]", "display information about command type and location", s_flags_type},
    {"which",    "which [-a|-s] [command ...]", "locate executable file in PATH", s_flags_which},
    {"help",     "help [-d|-m|-s] [pattern ...]", "display information about builtin commands", s_flags_help},

    /* Top Unix & Developer CLI tools */
    {"tar",      "tar [-c|-x|-t] [options] [archive] [files...]", "manipulate tape archives and tarball files", s_flags_tar},
    {"ls",       "ls [-l|-a|-h|-t|-r|-S|-R] [path ...]", "list directory contents and file attributes", s_flags_ls},
    {"grep",     "grep [-i|-v|-r|-n|-c|-l|-E|-F] pattern [files...]", "search text using regular expressions or patterns", s_flags_grep},
    {"find",     "find [path ...] [expression]", "search for files in directory hierarchy", s_flags_find},
    {"cp",       "cp [-r|-p|-a|-f|-i|-v] source ... target", "copy files and directories", s_flags_cp},
    {"mv",       "mv [-f|-i|-n|-v] source ... target", "move or rename files and directories", s_flags_mv},
    {"rm",       "rm [-r|-f|-i|-v] file ...", "remove files or directories", s_flags_rm},
    {"mkdir",    "mkdir [-p|-m mode|-v] directory ...", "make directories", s_flags_mkdir},
    {"chmod",    "chmod [-R|-v|-c] mode file ...", "change file mode bits (permissions)", s_flags_chmod},
    {"chown",    "chown [-R|-v|-h] owner[:group] file ...", "change file owner and group", s_flags_chown},
    {"cat",      "cat [-n|-b|-s|-v] [file ...]", "concatenate and print files to standard output", s_flags_cat},
    {"wc",       "wc [-l|-w|-c|-m] [file ...]", "print newline, word, and byte counts", s_flags_wc},
    {"head",     "head [-n lines|-c bytes] [file ...]", "output first part of files", s_flags_head},
    {"tail",     "tail [-n lines|-f|-c bytes] [file ...]", "output last part of files", s_flags_tail},
    {"sort",     "sort [-n|-r|-u|-k pos|-t sep] [file ...]", "sort lines of text files", s_flags_sort},
    {"uniq",     "uniq [-c|-d|-u|-i] [input [output]]", "report or filter out repeated lines", s_flags_uniq},
    {"cut",      "cut [-d delim -f fields|-c chars] [file ...]", "remove sections from each line of files", s_flags_cut},
    {"tr",       "tr [-d|-s|-c] string1 [string2]", "translate or delete characters", s_flags_tr},
    {"git",      "git [command] [options] [args...]", "fast, scalable, distributed revision control system", s_flags_git},
    {"curl",     "curl [-s|-o file|-L|-I|-X method|-H hdr|-d data] [url]", "transfer data from or to a server", s_flags_curl},
    {"ssh",      "ssh [-p port|-i id_file|-v|-N|-L spec] [user@]host", "OpenSSH remote login and port forwarding client", s_flags_ssh},
    {"make",     "make [-j jobs|-f file|-B|-C dir|-n] [target ...]", "GNU/POSIX Silicon DAG build engine", s_flags_make},
    {"docker",   "docker [run|build|ps|images|stop|rm|exec] [options]", "container application platform engine", s_flags_docker},
    {NULL, NULL, NULL, NULL}
};

/* ========================================================================= */
/* 3. Lookup Functions                                                       */
/* ========================================================================= */

const zbash_cmd_desc_t *zbash_find_cmd_desc(const char *cmd_name) {
    if (!cmd_name || !*cmd_name) return NULL;

    /* Skip leading path if any (e.g. /bin/ls -> ls) */
    const char *slash = strrchr(cmd_name, '/');
    if (slash) cmd_name = slash + 1;

    for (size_t i = 0; s_cmd_table[i].cmd_name != NULL; i++) {
        if (strcmp(s_cmd_table[i].cmd_name, cmd_name) == 0) {
            return &s_cmd_table[i];
        }
    }
    return NULL;
}

const zbash_flag_entry_t *zbash_find_flag(const zbash_cmd_desc_t *desc, char short_opt, const char *long_opt) {
    if (!desc || !desc->flags) return NULL;

    for (size_t i = 0; desc->flags[i].description != NULL; i++) {
        const zbash_flag_entry_t *f = &desc->flags[i];
        if (short_opt != 0 && f->short_opt == short_opt) {
            return f;
        }
        if (long_opt != NULL && f->long_opt != NULL && strcmp(f->long_opt, long_opt) == 0) {
            return f;
        }
    }
    return NULL;
}

/* ========================================================================= */
/* 4. Monaco-Grade Parameter Hints & Flag Ribbon Generator                   */
/* ========================================================================= */

/*
 * Token structure for parsing up to cursor
 */
typedef struct {
    char text[128];
    size_t start;
    size_t end;
} token_span_t;

/*
 * Parse tokens from line segment [start, end)
 */
static size_t tokenize_segment(const char *line, size_t start, size_t end, token_span_t *tokens, size_t max_tokens) {
    size_t count = 0;
    size_t i = start;

    while (i < end && count < max_tokens) {
        /* Skip whitespace */
        while (i < end && isspace((unsigned char)line[i])) i++;
        if (i >= end) break;

        size_t tok_start = i;
        char in_quote = 0;

        while (i < end) {
            char c = line[i];
            if (in_quote) {
                if (c == in_quote) in_quote = 0;
            } else {
                if (c == '\'' || c == '"') {
                    in_quote = c;
                } else if (isspace((unsigned char)c) || c == '|' || c == '&' || c == ';') {
                    break;
                }
            }
            i++;
        }

        size_t tok_len = i - tok_start;
        if (tok_len > 0) {
            if (tok_len >= sizeof(tokens[count].text)) tok_len = sizeof(tokens[count].text) - 1;
            memcpy(tokens[count].text, line + tok_start, tok_len);
            tokens[count].text[tok_len] = '\0';
            tokens[count].start = tok_start;
            tokens[count].end = i;
            count++;
        }
    }
    return count;
}

char *zbash_inspect_ribbon(const char *line, size_t len, size_t cursor_pos) {
    if (!line || len == 0) return NULL;
    if (cursor_pos > len) cursor_pos = len;

    /* 1. Find start of current statement before cursor (look back for |, &, ;) */
    size_t stmt_start = 0;
    for (size_t i = cursor_pos; i > 0; i--) {
        char c = line[i - 1];
        if (c == '|' || c == ';' || c == '&') {
            stmt_start = i;
            break;
        }
    }

    /* 2. Tokenize the current statement up to cursor_pos */
    token_span_t tokens[32];
    size_t num_tokens = tokenize_segment(line, stmt_start, cursor_pos, tokens, 32);
    if (num_tokens == 0) return NULL;

    const char *cmd_name = tokens[0].text;
    const zbash_cmd_desc_t *cmd_desc = zbash_find_cmd_desc(cmd_name);
    if (!cmd_desc) return NULL;

    /* Allocate buffer for Ribbon (up to 512 chars) */
    char *ribbon = malloc(512);
    if (!ribbon) return NULL;
    ribbon[0] = '\0';

    /* 3. Determine token currently under cursor or right before cursor */
    const token_span_t *last_tok = &tokens[num_tokens - 1];
    bool cursor_in_last_token = (cursor_pos >= last_tok->start && cursor_pos <= last_tok->end);
    bool trailing_space = (cursor_pos > last_tok->end);

    /* CASE 1: Cursor is in or immediately at a flag (starts with '-') */
    if (cursor_in_last_token && last_tok->text[0] == '-') {
        const char *flag_text = last_tok->text;

        /* Subcase 1A: Long Option e.g. "--verbose" or "--create" */
        if (flag_text[1] == '-') {
            const zbash_flag_entry_t *fe = zbash_find_flag(cmd_desc, 0, flag_text);
            if (fe) {
                if (fe->arg_desc) {
                    snprintf(ribbon, 512,
                        "\033[38;5;220m💡\033[0m \033[1;38;5;51m%s\033[0m \033[1;38;5;221m%s\033[0m \033[38;5;141m%s\033[0m: \033[38;5;252m%s\033[0m",
                        cmd_desc->cmd_name, fe->long_opt, fe->arg_desc, fe->description);
                } else {
                    snprintf(ribbon, 512,
                        "\033[38;5;220m💡\033[0m \033[1;38;5;51m%s\033[0m \033[1;38;5;221m%s\033[0m: \033[38;5;252m%s\033[0m",
                        cmd_desc->cmd_name, fe->long_opt, fe->description);
                }
                return ribbon;
            }
        }
        /* Subcase 1B: Short Option or Cluster e.g. "-l", "-czvf", "-p" */
        else if (flag_text[1] != '\0') {
            size_t flen = strlen(flag_text);
            if (flen == 2) {
                /* Single short option: "-p" */
                char opt = flag_text[1];
                const zbash_flag_entry_t *fe = zbash_find_flag(cmd_desc, opt, NULL);
                if (fe) {
                    if (fe->arg_desc) {
                        snprintf(ribbon, 512,
                            "\033[38;5;220m💡\033[0m \033[1;38;5;51m%s\033[0m \033[1;38;5;221m-%c\033[0m \033[38;5;141m%s\033[0m: \033[38;5;252m%s\033[0m",
                            cmd_desc->cmd_name, opt, fe->arg_desc, fe->description);
                    } else {
                        snprintf(ribbon, 512,
                            "\033[38;5;220m💡\033[0m \033[1;38;5;51m%s\033[0m \033[1;38;5;221m-%c\033[0m: \033[38;5;252m%s\033[0m",
                            cmd_desc->cmd_name, opt, fe->description);
                    }
                    return ribbon;
                }
            } else {
                /* Cluster: e.g. "-czvf" */
                size_t rpos = 0;
                rpos += snprintf(ribbon + rpos, 512 - rpos,
                    "\033[38;5;220m💡\033[0m \033[1;38;5;51m%s\033[0m ", cmd_desc->cmd_name);

                int matched = 0;
                for (size_t c = 1; c < flen && rpos < 480; c++) {
                    char opt = flag_text[c];
                    const zbash_flag_entry_t *fe = zbash_find_flag(cmd_desc, opt, NULL);
                    if (fe) {
                        if (matched > 0) {
                            rpos += snprintf(ribbon + rpos, 512 - rpos, " \033[38;5;240m│\033[0m ");
                        }
                        rpos += snprintf(ribbon + rpos, 512 - rpos,
                            "\033[1;38;5;221m-%c\033[0m: \033[38;5;252m%s\033[0m",
                            opt, fe->description);
                        matched++;
                    }
                }
                if (matched > 0) return ribbon;
            }
        }
    }

    /* CASE 2: Trailing space after a flag that requires an argument (e.g. "read -p ") */
    if (trailing_space && last_tok->text[0] == '-') {
        const char *flag_text = last_tok->text;
        const zbash_flag_entry_t *fe = NULL;
        if (flag_text[1] == '-') {
            fe = zbash_find_flag(cmd_desc, 0, flag_text);
        } else if (flag_text[1] != '\0' && flag_text[2] == '\0') {
            fe = zbash_find_flag(cmd_desc, flag_text[1], NULL);
        }

        if (fe && fe->arg_desc) {
            snprintf(ribbon, 512,
                "\033[38;5;220m💡\033[0m \033[1;38;5;51m%s\033[0m \033[1;38;5;221m%s\033[0m \033[1;38;5;141m%s\033[0m: \033[38;5;252m%s\033[0m",
                cmd_desc->cmd_name, flag_text, fe->arg_desc, fe->description);
            return ribbon;
        }
    }

    /* CASE 3: Command Synopsis & Summary (when typing command or arguments without specific flag) */
    snprintf(ribbon, 512,
        "\033[38;5;220m💡\033[0m \033[1;38;5;51m%s\033[0m \033[38;5;244m[%s]\033[0m \033[38;5;240m──\033[0m \033[38;5;252m%s\033[0m",
        cmd_desc->cmd_name, cmd_desc->synopsis, cmd_desc->summary);

    return ribbon;
}
