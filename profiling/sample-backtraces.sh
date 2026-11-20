#!/usr/bin/env bash
# "Poor man's profiler": runs a command and records its call stack with gdb every few
# seconds. Useful where perf can't unwind the stack (e.g. inside GMP's assembly).
#
# Usage (from the repo root):
#   profiling/sample-backtraces.sh <output file> <first sample after (s)> <interval (s)> <count> <command> [args...]
# Example:
#   profiling/sample-backtraces.sh /tmp/sq4-backtraces.txt 5 10 15 \
#       ./OneSteinerTreeProblem-perf -i '[[0,0],[1,0],[1,1],[0,1]]'
#
# Attaching gdb to a running program needs sudo on Ubuntu (kernel.yama.ptrace_scope = 1
# allows ptrace only on your own child processes); the script asks for the
# password once, at the start. The command's normal output is discarded; its stderr
# goes to <output file>.stderr. Each sample briefly pauses the program.


set -u

if [ $# -lt 5 ]; then
    sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
fi

out="$1"; first="$2"; interval="$3"; count="$4"
shift 4

SUDO="${SUDO-sudo}"                     # (tests can set SUDO= to run without sudo)
if [ -n "$SUDO" ]; then
    $SUDO -v || exit 1                  # ask for the password now, not mid-run
fi

"$@" > /dev/null 2> "$out.stderr" &
pid=$!
echo "started $* (pid $pid); sampling $count times, every $interval s, after $first s"
: > "$out"
sleep "$first"
for k in $(seq 1 "$count"); do
    if ! kill -0 "$pid" 2> /dev/null; then
        echo "program finished after $((k - 1)) samples"
        break
    fi
    {
        echo "=== sample $k at $(date '+%H:%M:%S')"
        $SUDO gdb -p "$pid" -batch -ex "set print frame-arguments none" -ex "bt 40" 2> /dev/null | grep '^#'
    } >> "$out"
    echo "sample $k done"
    sleep "$interval"
done
wait "$pid"
echo "program exit code: $?; backtraces in $out"
