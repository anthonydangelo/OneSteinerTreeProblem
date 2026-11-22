#!/usr/bin/env bash
# Runs a test suite through the program, compares two runs, or checks a run
# against expected results.
#
# Usage (from the repo root):
#   tests/run-tests.sh run <suite> <binary> <label> [case ...]
#                                         e.g.  run regression ./OneSteinerTreeProblem before
#                                         or    run extreme ./OneSteinerTreeProblem before grid6-rot10
#   tests/run-tests.sh compare <suite> <label1> <label2>   e.g.  compare regression before after
#   tests/run-tests.sh check <suite> <label>          e.g.  check 120 after
#
# A suite is a cases file, tests/cases-<suite>.txt, with one case per line:
#   name|description|program arguments|expected result (optional)
# The arguments are split on whitespace, so -i point lists must not contain spaces.
#
# "run" runs every case in the suite, or only the named cases. Set CASE_TIMEOUT to stop
# any case that runs too long (a duration for timeout(1): e.g. CASE_TIMEOUT=2h); a
# stopped case gets exit code 124 and is reported as TIMEOUT.
# "run" never replaces an existing result for the same suite, label and case (so a
# baseline can't be overwritten by accident), except a TIMEOUT result. To replace
# results deliberately, set OVERWRITE=1.
# For each case, "run" writes <case>.json (stdout), <case>.err (stderr) and
# <case>.exit (exit code) into tests/out/<suite>/<label>/, and prints each case's
# wall-clock time. If GNU time is installed (/usr/bin/time; "sudo apt install time"),
# it also records the wall-clock time and peak memory (maximum resident set size) in
# <case>.time, and "compare" reports both. Peak memory is RAM only: if a run swaps,
# the real footprint is larger (watch vmstat's si/so columns for the large cases).
# "compare" compares two runs after putting both outputs in a canonical form
# (compare-outputs.py; see there), so a different but equivalent order of faces or
# tied candidates is not reported as a difference. Exits with 0 if all cases match.
# "check" checks one run against the expected results in the cases file, using the
# suite's checker check-<suite>-cases.py (only the 120 suite has one so far).
# The Python scripts need python3 (3.5 or later).

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_ROOT="$SCRIPT_DIR/out"
TIME_BIN="${TIME_BIN:-/usr/bin/time}"    # GNU time; override for testing

usage() {
    sed -n '2,/^[^#]/{/^#/p}' "$0" | sed 's/^# \{0,1\}//'
    exit 2
}

cases_file() {
    local f="$SCRIPT_DIR/cases-$1.txt"
    if [ ! -f "$f" ]; then
        echo "No such suite: $f" >&2
        exit 2
    fi
    echo "$f"
}

run_cases() {
    local suite="$1" bin="$2" label="$3"
    shift 3
    local -a only=("$@")
    local file dir name desc args expected code start end
    file="$(cases_file "$suite")" || exit 2
    dir="$OUT_ROOT/$suite/$label"
    if [ ! -x "$bin" ]; then
        echo "Not an executable: $bin" >&2
        exit 2
    fi
    mkdir -p "$dir"
    local have_time=0
    if [ -x "$TIME_BIN" ]; then
        have_time=1
    else
        echo "Note: $TIME_BIN not found; peak memory won't be recorded (sudo apt install time)." >&2
    fi
    local mem
    while IFS='|' read -r name desc args expected; do
        case "$name" in ''|'#'*) continue ;; esac
        if [ ${#only[@]} -gt 0 ]; then
            local wanted=0 c
            for c in "${only[@]}"; do [ "$c" = "$name" ] && wanted=1; done
            [ "$wanted" -eq 1 ] || continue
        fi
        if [ -f "$dir/$name.exit" ] && [ "${OVERWRITE:-0}" != "1" ] \
           && [ "$(cat "$dir/$name.exit")" != "124" ]; then
            printf '%-16s already done in %s: skipped (set OVERWRITE=1 to replace)\n' "$name" "$label"
            continue
        fi
        local -a argv
        read -r -a argv <<< "$args"
        local -a cmd=("$bin")
        if [ -n "${CASE_TIMEOUT:-}" ]; then
            cmd=(timeout "$CASE_TIMEOUT" "$bin")
        fi
        start=$(date +%s.%N)
        if [ "$have_time" -eq 1 ]; then
            # %e = elapsed seconds, %M = peak resident memory in KB (last line of the file)
            "$TIME_BIN" -f "%e %M" -o "$dir/$name.time" \
                "${cmd[@]}" "${argv[@]}" > "$dir/$name.json" 2> "$dir/$name.err" < /dev/null
        else
            rm -f "$dir/$name.time"
            "${cmd[@]}" "${argv[@]}" > "$dir/$name.json" 2> "$dir/$name.err" < /dev/null
        fi
        code=$?
        end=$(date +%s.%N)
        echo "$code" > "$dir/$name.exit"
        if [ -n "${CASE_TIMEOUT:-}" ] && [ "$code" -eq 124 ]; then
            desc="TIMEOUT after $CASE_TIMEOUT: $desc"
        fi
        mem=""
        if [ -s "$dir/$name.time" ]; then
            mem="$(tail -n 1 "$dir/$name.time" | awk '{ printf "peak %.0f MB", $2 / 1024 }')"
        fi
        printf '%-16s exit=%-3s %8ss  %-14s %s\n' "$name" "$code" \
            "$(awk -v s="$start" -v e="$end" 'BEGIN { printf "%.2f", e - s }')" "$mem" "$desc"
    done < "$file"
    echo "Outputs written to $dir"
}

case "${1:-}" in
    run)
        [ $# -ge 4 ] || usage
        shift
        run_cases "$@" ;;
    compare)
        [ $# -eq 4 ] || usage
        file="$(cases_file "$2")" || exit 2
        python3 "$SCRIPT_DIR/compare-outputs.py" "$file" "$OUT_ROOT/$2/$3" "$OUT_ROOT/$2/$4" ;;
    check)
        [ $# -eq 3 ] || usage
        file="$(cases_file "$2")" || exit 2
        checker="$SCRIPT_DIR/check-$2-cases.py"
        if [ ! -f "$checker" ]; then
            echo "Suite $2 has no expected results (no $checker)." >&2
            exit 2
        fi
        python3 "$checker" "$file" "$OUT_ROOT/$2/$3" ;;
    *)
        usage ;;
esac
