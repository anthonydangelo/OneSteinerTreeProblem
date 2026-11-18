#!/usr/bin/env bash
# Runs the 120-degree test triangles through the program, and compares two runs.
#
# Usage (from the repo root):
#   tests/run-120-cases.sh run <binary> <label>      e.g.  run ./OneSteinerTreeProblem before
#   tests/run-120-cases.sh compare <label1> <label2>  e.g.  compare before after
#   tests/run-120-cases.sh check <label>              e.g.  check after
#
# "run" writes, for each case, <case>.json (stdout), <case>.err (stderr) and
# <case>.exit (exit code) into tests/out/<label>/.
# "compare" diffs two labels (ignoring elapsedTime), prints a one-line summary per
# case, writes the full diffs to tests/out/compare-<label1>-vs-<label2>.txt, and
# exits with 0 if every case is identical, 1 otherwise.
# "check" checks one run against the expected results in cases-120.txt (using
# check-120-cases.py; needs python3) and exits with 0 if no case fails.
#
# The cases and their expected results are in cases-120.txt (one line per case).

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_ROOT="$SCRIPT_DIR/out"
CASES_FILE="$SCRIPT_DIR/cases-120.txt"

# Read the cases (skipping comments and blank lines) into "name|description|points".
CASES=()
while IFS='|' read -r name desc pts expected; do
    case "$name" in ''|'#'*) continue ;; esac
    CASES+=("$name|$desc|$pts")
done < "$CASES_FILE"

usage() {
    sed -n '2,15p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
}

run_cases() {
    local bin="$1" label="$2"
    local dir="$OUT_ROOT/$label"
    if [ ! -x "$bin" ]; then
        echo "Not an executable: $bin" >&2
        exit 2
    fi
    mkdir -p "$dir"
    local entry name desc pts code
    for entry in "${CASES[@]}"; do
        IFS='|' read -r name desc pts <<< "$entry"
        "$bin" -i "$pts" > "$dir/$name.json" 2> "$dir/$name.err"
        code=$?
        echo "$code" > "$dir/$name.exit"
        printf '%-2s exit=%-3s %s\n' "$name" "$code" "$desc"
    done
    echo "Outputs written to $dir"
}

# Succeeds if case $1 is identical in directories $2 and $3 (ignoring elapsedTime).
same_case() {
    local name="$1" a="$2" b="$3"
    diff -q "$a/$name.exit" "$b/$name.exit" > /dev/null 2>&1 &&
    diff -q "$a/$name.err"  "$b/$name.err"  > /dev/null 2>&1 &&
    diff -q <(grep -v elapsedTime "$a/$name.json" 2>&1) \
            <(grep -v elapsedTime "$b/$name.json" 2>&1) > /dev/null
}

compare_runs() {
    local labelA="$1" labelB="$2"
    local a="$OUT_ROOT/$labelA" b="$OUT_ROOT/$labelB"
    local report="$OUT_ROOT/compare-$labelA-vs-$labelB.txt"
    local entry name desc pts status nSame=0 nDiff=0
    for d in "$a" "$b"; do
        if [ ! -d "$d" ]; then
            echo "No run found: $d" >&2
            exit 2
        fi
    done
    : > "$report"
    for entry in "${CASES[@]}"; do
        IFS='|' read -r name desc pts <<< "$entry"
        if same_case "$name" "$a" "$b"; then
            status="same"
            nSame=$((nSame + 1))
        else
            status="DIFFERENT"
            nDiff=$((nDiff + 1))
            {
                echo "=== $name: $desc"
                echo "points: $pts"
                echo "--- exit code ($labelA vs $labelB)"
                diff "$a/$name.exit" "$b/$name.exit"
                echo "--- stderr"
                diff "$a/$name.err" "$b/$name.err"
                echo "--- stdout (elapsedTime ignored)"
                diff <(grep -v elapsedTime "$a/$name.json") <(grep -v elapsedTime "$b/$name.json")
                echo
            } >> "$report" 2>&1
        fi
        printf '%-2s %-9s exit %s -> %s   %s\n' "$name" "$status" \
            "$(cat "$a/$name.exit" 2>/dev/null)" "$(cat "$b/$name.exit" 2>/dev/null)" "$desc"
    done
    echo "$nSame same, $nDiff different."
    if [ "$nDiff" -gt 0 ]; then
        echo "Details: $report"
        return 1
    fi
    rm -f "$report"
    return 0
}

case "${1:-}" in
    run)     [ $# -eq 3 ] || usage; run_cases "$2" "$3" ;;
    compare) [ $# -eq 3 ] || usage; compare_runs "$2" "$3" ;;
    check)   [ $# -eq 2 ] || usage; python3 "$SCRIPT_DIR/check-120-cases.py" "$CASES_FILE" "$OUT_ROOT/$2" ;;
    *)       usage ;;
esac
