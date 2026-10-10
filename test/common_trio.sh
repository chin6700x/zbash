#!/usr/bin/env bash
# ==============================================================================
# ⚡ ZEPHYR ECOSYSTEM: TRI-PILLAR DIFFERENTIAL TEST HARNESS (common_trio.sh)
# ==============================================================================
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# Description: Shared high-precision microsecond timer, sandbox environment,
#              bit-for-bit diff comparator, and terminal reporting suite.
# ==============================================================================

# Terminal ANSI Colors
BOLD='\033[1m'
CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
MAGENTA='\033[0;35m'
BLUE='\033[0;34m'
RED='\033[0;31m'
WHITE='\033[1;37m'
NC='\033[0m'

# Binary Targets
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ZBASH_ROOT="$(cd "$TEST_DIR/.." && pwd)"
ZBASH_BIN="${ZBASH_BIN:-$ZBASH_ROOT/bin/zbash}"
BASH_BIN="${BASH_BIN:-/bin/bash}"

# Check targets exist
if [ ! -x "$ZBASH_BIN" ]; then
    echo -e "${RED}Error: zbash binary not found or not executable at: $ZBASH_BIN${NC}"
    exit 1
fi

if [ ! -x "$BASH_BIN" ]; then
    echo -e "${RED}Error: Golden baseline /bin/bash not found at: $BASH_BIN${NC}"
    exit 1
fi

# High-resolution microsecond timer (returns floating seconds)
get_hires_time() {
    perl -MTime::HiRes=time -e 'printf "%.6f\n", time' 2>/dev/null || date +%s
}

# Format milliseconds from two timestamps
calc_ms() {
    local t_start="$1"
    local t_end="$2"
    awk -v s="$t_start" -v e="$t_end" 'BEGIN { printf "%.2f", (e - s) * 1000 }'
}

# Format speedup factor
calc_speedup() {
    local t_bash="$1"
    local t_zbash="$2"
    awk -v b="$t_bash" -v z="$t_zbash" 'BEGIN {
        if (z <= 0.000001) printf "%.2fx", (b > 0 ? 99.9 : 1.0);
        else printf "%.2fx", b / z;
    }'
}

# Create temporary sandbox
create_trio_sandbox() {
    local sb
    sb=$(mktemp -d /tmp/zbash_trio_XXXXXX)
    echo "$sb"
}

# Run a command with hard timeout (seconds) to prevent hanging on FIFOs/interactive loops
run_with_timeout() {
    local to="${1:-5}"
    shift
    perl -e '
        my $to = shift;
        my $pid = fork();
        if (!defined $pid) { exit 1; }
        if ($pid == 0) {
            exec @ARGV;
            exit 127;
        }
        eval {
            local $SIG{ALRM} = sub { die "alarm\n" };
            alarm $to;
            waitpid($pid, 0);
            alarm 0;
        };
        if ($@) {
            kill 9, $pid;
            waitpid($pid, 0);
            exit 124;
        }
        exit ($? >> 8);
    ' "$to" "$@"
}

# Pretty-print test card header
print_test_banner() {
    local suite_title="$1"
    local total_count="$2"
    echo -e "${BOLD}${CYAN}================================================================================${NC}"
    echo -e "${BOLD}⚡ ZEPHYR DIFFERENTIAL TEST HARNESS: ${WHITE}${suite_title}${NC}"
    echo -e "   Target Engine  : ${CYAN}${ZBASH_BIN}${NC}"
    echo -e "   Golden Baseline: ${YELLOW}${BASH_BIN}${NC} ($(bash --version | head -n 1))"
    [ -n "$total_count" ] && echo -e "   Total Tests    : ${GREEN}${total_count}${NC}"
    echo -e "${BOLD}${CYAN}================================================================================${NC}"
}
