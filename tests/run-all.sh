#!/usr/bin/env bash
# Runs several test suites with one binary and label, logging memory activity
# (vmstat, with timestamps) for the whole run.
#
# Usage (from the repo root):
#   tests/run-all.sh <binary> <label> [suite[:case,case...] ...]
#   default suites: 120 regression slow
# Examples:
#   tests/run-all.sh ./OneSteinerTreeProblem before
#   CASE_TIMEOUT=3h tests/run-all.sh ./OneSteinerTreeProblem before extreme:grid6-rot10,grid6-rot37
# "suite:case,case" runs only those cases of the suite. CASE_TIMEOUT (see run-tests.sh)
# limits every case's run time.
#
# Suites that have expected results (a checker, check-<suite>-cases.py) are checked
# right after they run. Everything printed is also saved in
# tests/out/run-all-<label>-<date-time>.log. The vmstat log goes to tests/out/vmstat-<label>-<date-time>.txt
# (a new file each time, so earlier logs are kept); vmstat is
# stopped when this script ends, even if a suite fails or the script is interrupted.
# Exit code: 0 if every suite ran and every check passed, 1 otherwise.

set -u

if [ $# -lt 2 ]; then
    sed -n '2,21p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
bin="$1"
label="$2"
shift 2
if [ $# -gt 0 ]; then
    suites=("$@")
else
    suites=(120 regression slow)
fi

mkdir -p "$SCRIPT_DIR/out"
stamp="$(date '+%Y%m%d-%H%M%S')"
run_log="$SCRIPT_DIR/out/run-all-$label-$stamp.log"
exec > >(tee -a "$run_log") 2>&1      # from here on, print to the screen and the log
vmstat_log="$SCRIPT_DIR/out/vmstat-$label-$stamp.txt"
vmstat -t 5 > "$vmstat_log" &
vmstat_pid=$!
trap 'kill "$vmstat_pid" 2> /dev/null' EXIT

if [ -n "${CASE_TIMEOUT:-}" ]; then
    echo "=== time limit per case: $CASE_TIMEOUT"
fi
status=0
for spec in "${suites[@]}"; do
    suite="${spec%%:*}"
    cases=()
    if [ "$spec" != "$suite" ]; then
        IFS=',' read -r -a cases <<< "${spec#*:}"
    fi
    # (written this way because bash 4.3, on Ubuntu 16.04, treats an empty array as
    # unset under "set -u")
    which=""
    if [ ${#cases[@]} -gt 0 ]; then
        which=" (cases: ${cases[*]})"
    fi
    echo "=== suite $suite$which, label $label: started $(date '+%Y-%m-%d %H:%M:%S')"
    "$SCRIPT_DIR/run-tests.sh" run "$suite" "$bin" "$label" ${cases[@]+"${cases[@]}"} || status=1
    if [ -f "$SCRIPT_DIR/check-$suite-cases.py" ]; then
        echo "=== checking suite $suite against its expected results"
        "$SCRIPT_DIR/run-tests.sh" check "$suite" "$label" || status=1
    fi
done
echo "=== all done: $(date '+%Y-%m-%d %H:%M:%S')"
echo "vmstat log: $vmstat_log"
echo "this log:   $run_log"
exit "$status"
