#!/bin/bash
# Runs test suites with tests/run-all.sh, then compares each suite against a baseline
# label, saving the comparison next to run-all's own log.
#
# Usage (from the repo root):
#   tests/run-compare.sh <label> [suite[:case,case]...]      (default suites: slow extreme)
# Examples:
#   tests/run-compare.sh after-sort-fix
#   tests/run-compare.sh after-overlay 120 regression
#   BASELINE=after-sort-fix tests/run-compare.sh after-overlay extreme:grid6
# Environment (all optional):
#   BASELINE      label to compare against           (default: before)
#   BINARY        program to test                    (default: ./OneSteinerTreeProblem)
#   CASE_TIMEOUT  per-case limit, passed to run-all  (default: 30m)

set -u

if [ $# -lt 1 ]; then
    echo "usage: $0 <label> [suite[:case,case]...]" >&2
    exit 2
fi

label=$1
shift
if [ $# -eq 0 ]; then
    set -- slow extreme
fi

baseline=${BASELINE:-before}
binary=${BINARY:-./OneSteinerTreeProblem}
export CASE_TIMEOUT=${CASE_TIMEOUT:-30m}

mkdir -p tests/out
compareFile="tests/out/compare-$label-vs-$baseline-$(date +%Y%m%d-%H%M%S).txt"

tests/run-all.sh "$binary" "$label" "$@"
runStatus=$?

# Compare each suite once, even if it was named twice (e.g. extreme:a extreme:b).
seen=" "
{
    for spec in "$@"; do
        suite=${spec%%:*}
        if [ "$suite" = "modes" ]; then
            continue
        fi        
        case "$seen" in
            *" $suite "*) continue ;;
        esac
        seen="$seen$suite "
        echo "=== compare $suite: $baseline -> $label"
        tests/run-tests.sh compare "$suite" "$baseline" "$label"
    done
} 2>&1 | tee "$compareFile"

echo "Comparison saved to $compareFile"
exit $runStatus
