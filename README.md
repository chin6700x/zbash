# zbash

A high-performance command language interpreter and interactive shell engineered from first principles in ISO C11 and native ARM64 NEON assembly.

---

## 1. Design Philosophy and Purpose

zbash was created to rethink the command-line execution layer from the silicon up, focusing on speed, minimal memory footprint, and modern developer ergonomics without carrying legacy overhead.

### Core Objectives

- **Sub-Millisecond Cold-Spawn Latency:** Initializes and executes in under 0.5 ms, making it ideal for high-throughput scripting, process automation, and instant terminal launches.
- **Hardware-Vectorized Lexing:** Features an ARM64 NEON assembly engine (`simd_lexer.s`) that scans tokens and skips delimiters 16 bytes per clock cycle using 128-bit vector registers (`q0-q5`, `v16-v23`), sustaining 35+ GB/s bus throughput.
- **Ultra-Compact Footprint:** Compiled to a self-contained ~70 KB native binary designed to fit entirely within CPU L2 cache, eliminating cache thrashing. Initial runtime resident memory is under 0.5 MB.
- **Zero External Dependencies:** Built purely on ISO C11 and standard system primitives, eliminating reliance on external terminal libraries, heavy runtimes, or vintage compatibility layers.
- **Zero-Copy Pipeline Streaming:** Implements a direct descriptor handoff mechanism that reduces kernel context switching overhead across multi-stage execution pipelines.
- **First-Class Interactive Ergonomics:** Integrates native syntax highlighting, parameter hover ribbons, real-time repository inspection, and an embedded modal visual editor (`zedit`) directly into the core binary.
- **Dual-Target Silicon Architecture:** Compiles natively to Apple Silicon Darwin Mach-O as well as WebAssembly (`zbash.wasm`), enabling in-browser terminal execution and cloud micro-sandboxes.

---

## 2. Building and Installation

### Prerequisites

- macOS on Apple Silicon (ARM64) or Linux POSIX environment
- C Compiler supporting ISO C11 (Clang or GCC)
- GNU Make

### Build from Source

```sh
git clone https://github.com/chin6700x/zbash.git
cd zbash
make
```

The compiled binary will be placed at `bin/zbash`.

### Running Tests

Execute the comprehensive test suite (including syntax verification, UX tests, editor checks, and parser validations):

```sh
make test
```

### Installation

To install `zbash` system-wide into `/usr/local/bin`:

```sh
sudo cp bin/zbash /usr/local/bin/zbash
sudo chmod 755 /usr/local/bin/zbash
```

---

## 3. Invocation and Command-Line Options

zbash accepts standard invocation flags for both interactive sessions and automated scripting:

```text
zbash [-exnvi] [-c string | script_file [arguments...]]
```

| Option | Description |
|---|---|
| `-c string` | Read and execute commands directly from the specified string. Remaining arguments are assigned to positional parameters (`$1`, `$2`, ...). |
| `-i` | Force interactive shell mode, enabling line editing, prompt rendering, and history. |
| `-s` | Read commands sequentially from standard input. |
| `-e` | Exit immediately if a command or pipeline exits with a non-zero status (`errexit`). |
| `-x` | Print commands and their expanded arguments to stderr before execution (`xtrace`). |
| `-n` | Read commands and check for syntax errors without executing them (`noexec`). |
| `-v` | Print shell input lines as they are read (`verbose`). |
| `-r` | Run in restricted shell mode (locks down `$PATH`, `$SHELL`, and directory traversal). |
| `--norc` | Do not load startup configuration files (`~/.zbashrc`). |
| `--noprofile` | Do not load login profile scripts (`/etc/zprofile`, `~/.zprofile`). |
| `--help` | Display command-line option summary and exit. |
| `--version` | Display version, target architecture, and license information. |

---

## 4. Usage Guide

### 4.1 Running Scripts

#### Direct Script Execution
Pass the path to any script file as the first argument:

```sh
zbash deploy.sh production
```

#### Shebang Support
Set `zbash` in your script's shebang header:

```sh
#!/usr/local/bin/zbash
set -e

echo "Starting automated task..."
for item in alpha beta gamma; do
    echo "Processing: ${item}"
done
```

Make the script executable and run it directly:

```sh
chmod +x deploy.sh
./deploy.sh
```

#### Inline Commands
Execute command strings without creating a file:

```sh
zbash -c 'for i in 1 2 3; do echo "Count: $i"; done'
```

---

### 4.2 Interactive Environment

When launched without arguments in a terminal, `zbash` opens in interactive mode:

```sh
zbash
```

#### Two-Line Structured Prompt
The prompt organizes context and input into a clean visual hierarchy:

```text
╭─ zbash in ~/projects/engine on git:(main)*
╰─❯ 
```

- **Top Line (Context Bar):** Displays the active shell, working directory (abbreviated with `~`), and current source repository branch status.
- **Bottom Line (Input Line):** Highlights command input with real-time syntax coloring, displaying the exit code indicator when a previous command fails.

#### Interactive Features
- **Real-Time Syntax Highlighting:** Valid commands appear highlighted, while unknown commands or syntax errors are indicated before execution.
- **Context-Aware Completion:** Press `Tab` to complete filenames, paths, built-in commands, and environment variables.
- **Flag Documentation Ribbons:** When typing common command-line flags (e.g., `-l`, `-v`), contextual help ribbons display flag definitions above the cursor.
- **Persistent History:** Commands are recorded in `~/.zbash_history` and synchronized across sessions.
- **Embedded Editor (`zedit`):** Launch a lightweight, built-in visual modal editor directly from the command line without leaving the shell:

```sh
zedit config.txt
```

---

### 4.3 Scripting Language Reference

#### Variables and Parameter Expansion
```sh
# Variable assignment
NAME="Orion"
PORT=8080

# Basic expansion
echo "Service ${NAME} listening on port ${PORT}"

# Positional and special variables
echo "Script name: $0"
echo "First argument: $1"
echo "All arguments: $@"
echo "Argument count: $#"
echo "Process PID: $$"
echo "Last exit status: $?"

# Default value assignment
echo "${TIMEOUT:-30}"
```

#### Arrays and Slicing
```sh
# Array declaration
modules=(core parser engine codegen)

# Accessing elements
echo "First module: ${modules[0]}"
echo "All modules: ${modules[@]}"
echo "Element count: ${#modules[@]}"

# Slicing (offset:length)
echo "Slice: ${modules[@]:1:2}"
```

#### Pipelines and Redirection
```sh
# Multi-stage pipelines
cat access.log | grep "ERROR" | cut -d' ' -f5 | sort | uniq -c

# Output redirection (overwrite and append)
echo "Init" > run.log
echo "Step 1" >> run.log

# Error redirection
command_that_might_fail 2> error.log
command_with_combined_output > all.log 2>&1

# Input redirection
while read line; do
    echo "Line: ${line}"
done < input.txt
```

#### Control Flow Structures

**Conditionals (`if` / `elif` / `else`):**
```sh
if [ -f "/etc/config.json" ]; then
    echo "Configuration exists"
elif [ -d "/etc/config.d" ]; then
    echo "Configuration directory exists"
else
    echo "No configuration found"
fi
```

**Loops (`for`, `while`, `until`):**
```sh
# Iterate over word lists
for target in arm64 x86_64 wasm; do
    echo "Compiling for ${target}"
done

# While loop with condition
COUNT=5
while [ $COUNT -gt 0 ]; do
    echo "Countdown: $COUNT"
    COUNT=$((COUNT - 1))
done

# Infinite loop with break
while true; do
    read -p "Enter command (q to quit): " user_input
    if [ "$user_input" = "q" ]; then
        break
    fi
done
```

**Branch Logic:**
```sh
make build && echo "Build succeeded" || echo "Build failed"
```

#### Arithmetic Expressions
```sh
# In-place arithmetic evaluation
(( count = 10 ))
(( count += 5 ))

# Inline arithmetic expansion
TOTAL=$(( 100 * 4 + 25 ))
echo "Result: $TOTAL"
```

#### Functions and Scoping
```sh
calculate_checksum() {
    local filepath="$1"
    if [ ! -f "$filepath" ]; then
        echo "File not found" >&2
        return 1
    fi
    shasum -a 256 "$filepath"
}

calculate_checksum "bin/zbash"
```

#### Signal Traps and Cleanup
```sh
cleanup() {
    echo "Cleaning temporary files..."
    rm -f /tmp/work_*.tmp
}

trap cleanup EXIT INT TERM
```

---

## 5. Built-in Commands Reference

zbash provides high-speed, in-process implementations of essential system built-ins:

| Builtin | Syntax | Description |
|---|---|---|
| `cd` | `cd [dir]` | Change current working directory. Supports `cd -` for previous directory. |
| `pwd` | `pwd [-L \| -P]` | Print the absolute path of the current working directory. |
| `echo` | `echo [-n] [args...]` | Print arguments separated by spaces, followed by a newline. |
| `printf` | `printf format [args...]` | Format and print data according to format string. |
| `export` | `export [name[=val]...]` | Mark variables for export to child environments. |
| `unset` | `unset [-v] [name...]` | Remove variable or function definitions. |
| `declare` | `declare [-r \| -i \| -x] [name=val]` | Declare variables with specific attributes (read-only, integer, export). |
| `test` / `[` | `[ expr ]` | Evaluate conditional expression (file tests, string comparisons, integers). |
| `read` | `read [-r] [-p prompt] [var...]` | Read a line of input from standard input into variables. |
| `exit` | `exit [code]` | Terminate the shell process with the specified exit code. |
| `exec` | `exec command [args...]` | Replace the shell process image with the target command. |
| `eval` | `eval [args...]` | Concatenate arguments and execute the result as shell commands. |
| `source` / `.` | `source file [args...]` | Execute commands from a file within the current shell environment. |
| `pushd` | `pushd [dir]` | Push directory onto directory stack and change directory. |
| `popd` | `popd` | Pop directory from directory stack and change directory. |
| `dirs` | `dirs [-v]` | Display directory stack. |
| `history` | `history [-c]` | Display command history or clear history entries. |
| `jobs` | `jobs` | List active background jobs and process IDs. |
| `fg` | `fg [job_id]` | Bring background job to foreground. |
| `bg` | `bg [job_id]` | Resume suspended job in background. |
| `trap` | `trap [action] [signal...]` | Register command handler for system signals and lifecycle events. |
| `caller` | `caller [depth]` | Inspect function call frame and stack trace. |
| `zedit` | `zedit [file]` | Launch the built-in modal terminal editor. |

---

## 6. Startup Files and Configuration

During initialization, `zbash` searches for configuration files in the following sequence:

1. `/etc/zprofile` - System-wide configuration evaluated for login sessions.
2. `~/.zprofile` - User configuration evaluated for login sessions.
3. `~/.zbashrc` - User configuration evaluated for all interactive sessions. Recommended for custom environment variables, aliases, and functions.
4. `~/.zbash_history` - Stores interactive command history.

Example `~/.zbashrc`:

```sh
# Environment paths
export PATH="$HOME/.local/bin:/usr/local/bin:$PATH"
export EDITOR="zedit"

# Aliases
alias ll="ls -la"
alias g="git"

# Custom utility function
mkcd() {
    mkdir -p "$1" && cd "$1"
}
```

---

## 7. WebAssembly Target

zbash can be compiled to WebAssembly for execution in browser environments, Node.js, and serverless WASM runtimes:

```sh
# Compile to WebAssembly core
emcc -O3 -DZBASH_WASM -Iinclude src/core/*.c src/parser/*.c src/builtins/*.c -o bin/zbash.wasm
```

The compiled WASM module provides an in-memory command parser and execution core that operates sandbox-isolated.

---

## 8. License

zbash is released under the **Apache License 2.0**. See the [LICENSE](LICENSE) file for complete licensing terms.
