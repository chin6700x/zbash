#!/usr/bin/env bash
#
# zbash Arena: High-Precision Performance Benchmark vs macOS /bin/bash (GNU Bash 3.2)
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#

export LC_ALL=C

C_RESET="\033[0m"
C_BOLD="\033[1m"
C_GREEN="\033[38;2;46;213;115m"
C_AMBER="\033[38;2;255;165;2m"
C_RED="\033[38;2;255;71;87m"
C_BLUE="\033[38;2;77;142;255m"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
X_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ZBASH_BIN="$X_DIR/bin/zbash"
ORIG_BASH="/bin/bash"

if [ ! -x "$ZBASH_BIN" ]; then
    echo "Error: $ZBASH_BIN not found. Run 'make -C zephyr-x-series zbash' first."
    exit 1
fi

measure_best_ms() {
    local cmd="$1"
    local warmup=2
    local runs=5

    perl -MTime::HiRes=time -e '
    my $cmd = $ARGV[0];
    my $warmup = int($ARGV[1]);
    my $runs = int($ARGV[2]);

    for (1..$warmup) {
        system("$cmd >/dev/null 2>&1");
    }

    my $best = 1e9;
    for (1..$runs) {
        my $t0 = time();
        system("$cmd >/dev/null 2>&1");
        my $dt = (time() - $t0) * 1000.0;
        $best = $dt if $dt < $best;
    }
    printf "%.2f", $best;
    ' "$cmd" "$warmup" "$runs"
}

run_bench() {
    local label="$1"
    local cmd_orig="$2"
    local cmd_zephyr="$3"

    local ms_o=$(measure_best_ms "$cmd_orig")
    local ms_z=$(measure_best_ms "$cmd_zephyr")
    local ratio=$(perl -e "my \$z=$ms_z; my \$o=$ms_o; printf '%.2f', (\$z > 0 ? \$o/\$z : 1.0)")

    local badge
    if (( $(echo "$ratio >= 1.05" | bc -l) )); then
        badge="${C_GREEN}${ratio}x FASTER ⚡${C_RESET}"
    elif (( $(echo "$ratio >= 0.95" | bc -l) )); then
        badge="${C_AMBER}${ratio}x (Baseline Match)${C_RESET}"
    else
        badge="${C_RED}${ratio}x${C_RESET}"
    fi

    printf "  %-36s | BASH: %6.2f ms | ZBASH: %6.2f ms | %b\n" "$label" "$ms_o" "$ms_z" "$badge"
}

echo -e "\n${C_BOLD}${C_BLUE}==========================================================================================================${C_RESET}"
echo -e "${C_BOLD}  ZEPHYR ARENA: zbash (Bare-Metal POSIX Shell Engine) vs macOS /bin/bash (GNU Bash 3.2)${C_RESET}"
echo -e "${C_BOLD}${C_BLUE}==========================================================================================================${C_RESET}"

run_bench "Sub-Millisecond Launch (50x)" \
    "for i in {1..50}; do \"$ORIG_BASH\" -c 'echo test'; done" \
    "for i in {1..50}; do \"$ZBASH_BIN\" -c 'echo test'; done"

run_bench "Pipeline Chaining (30x)" \
    "for i in {1..30}; do \"$ORIG_BASH\" -c 'echo \"apple banana cherry\" | tr \" \" \"\\n\" | sort'; done" \
    "for i in {1..30}; do \"$ZBASH_BIN\" -c 'echo \"apple banana cherry\" | tr \" \" \"\\n\" | sort'; done"

run_bench "Conditionals & Logic (30x)" \
    "for i in {1..30}; do \"$ORIG_BASH\" -c 'true && echo ok || echo fail ; test -d /tmp && echo yes'; done" \
    "for i in {1..30}; do \"$ZBASH_BIN\" -c 'true && echo ok || echo fail ; test -d /tmp && echo yes'; done"

echo -e "${C_BOLD}${C_BLUE}==========================================================================================================${C_RESET}\n"
