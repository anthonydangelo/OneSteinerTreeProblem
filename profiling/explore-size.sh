#!/usr/bin/env bash
# Finds roughly the largest number of random input points the program handles
# within a time limit (and a memory limit), for one seed.
#
# Usage (from the repo root):
#   profiling/explore-size.sh [binary]            default binary: ./OneSteinerTreeProblem
# Settings (environment variables, all optional):
#   TIME_LIMIT  per-run limit, as for timeout(1)        (default: 10m)
#   SEED        random seed (-s)                        (default: 1)
#   START       first n to try                          (default: 12)
#   MAX_N       never try more than this many points    (default: 2048)
#   MEM_LIMIT_KB  per-run virtual memory limit (ulimit -v), in kB
#               (default: 90% of MemAvailable when the script starts, so a run
#               fails instead of pushing the machine into swap)
#   TIME_BIN    GNU time                                (default: /usr/bin/time)
#   EXTRA_ARGS  extra program arguments, e.g. EXTRA_ARGS=-b (default: none)
#
# How it searches: n doubles (START, 2*START, ...) until a run fails (time limit,
# memory limit, or any other error); then it bisects between the last n that
# succeeded and the first that failed, until they are within 10% of each other.
# The binary's MAX_NUM_INPUT_POINTS must allow the n values tried; the script checks
# this first.
#
# Output goes to tests/out/explore-size-<date-time>.log (everything printed),
# a tab-separated results file next to it, and a vmstat log. Attach all three.
# The program's JSON output is counted (bytes) and discarded, not saved.

set -u

bin="${1:-./OneSteinerTreeProblem}"
TIME_LIMIT="${TIME_LIMIT:-10m}"
SEED="${SEED:-1}"
START="${START:-12}"
MAX_N="${MAX_N:-2048}"
TIME_BIN="${TIME_BIN:-/usr/bin/time}"
EXTRA_ARGS="${EXTRA_ARGS:-}"
if [ -z "${MEM_LIMIT_KB:-}" ]; then
    avail_kb=$(awk '/^MemAvailable:/ {print $2}' /proc/meminfo)
    MEM_LIMIT_KB=$(( avail_kb * 9 / 10 ))
fi

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
out_dir="$repo_root/tests/out"
mkdir -p "$out_dir"
stamp="$(date '+%Y%m%d-%H%M%S')"
log="$out_dir/explore-size-$stamp.log"
results="$out_dir/explore-size-$stamp.tsv"
vmstat_log="$out_dir/vmstat-explore-size-$stamp.txt"
exec > >(tee -a "$log") 2>&1
vmstat -t 5 > "$vmstat_log" &
vmstat_pid=$!
trap 'kill "$vmstat_pid" 2> /dev/null' EXIT

echo "=== explore-size started $(date '+%Y-%m-%d %H:%M:%S')"
echo "binary:      $bin ($(sha256sum "$bin" | cut -c1-16)...)"
echo "git commit:  $(cd "$repo_root" && git rev-parse --short HEAD 2> /dev/null)$(cd "$repo_root" && git diff --quiet 2> /dev/null || echo ' (with uncommitted changes)')"
echo "machine:     $(uname -srm), $(nproc) CPU(s)"
grep -E '^(MemTotal|MemAvailable|SwapTotal|SwapFree):' /proc/meminfo | sed 's/^/             /'
echo "settings:    TIME_LIMIT=$TIME_LIMIT SEED=$SEED START=$START MAX_N=$MAX_N MEM_LIMIT_KB=$MEM_LIMIT_KB EXTRA_ARGS=$EXTRA_ARGS"
echo

if [ ! -x "$TIME_BIN" ]; then
    echo "GNU time not found at $TIME_BIN (set TIME_BIN)."
    exit 2
fi

# Check that the binary accepts large n: with -p it only generates the points.
check_n="$MAX_N"
count=$("$bin" -p -n "$check_n" -s "$SEED" 2> /dev/null | grep -o '"x"' | wc -l)
if [ "$count" -ne "$check_n" ]; then
    echo "The binary did not generate $check_n points with -p -n $check_n (got $count)."
    echo "Raise MAX_NUM_INPUT_POINTS and rebuild, or lower MAX_N."
    exit 2
fi

printf 'n\tseed\tstatus\texit\tseconds\tpeak_kB\toutput_bytes\n' > "$results"
tmp_time="$(mktemp)"
tmp_err="$(mktemp)"
tmp_bytes="$(mktemp)"
trap 'kill "$vmstat_pid" 2> /dev/null; rm -f "$tmp_time" "$tmp_err" "$tmp_bytes"' EXIT

# Runs the program once for n points; prints a result line; returns 0 on success.
probe() {
    local n="$1"
    local bytes code secs kb status
    : > "$tmp_time"
    # Not inside $( ): PIPESTATUS must be read in this shell. The program's output is
    # only counted (wc -c), never stored.
    ( ulimit -v "$MEM_LIMIT_KB"
      exec "$TIME_BIN" -o "$tmp_time" -f '%e %M' \
           timeout "$TIME_LIMIT" "$bin" $EXTRA_ARGS -n "$n" -s "$SEED" 2> "$tmp_err" ) | wc -c > "$tmp_bytes"
    code=${PIPESTATUS[0]}
    bytes=$(cat "$tmp_bytes")
    secs=""
    kb=""
    read -r secs kb < <(tail -n 1 "$tmp_time")
    if [ "$code" -eq 0 ]; then
        status=ok
    elif [ "$code" -eq 124 ]; then
        status=timeout
    elif grep -qiE 'alloc|memory' "$tmp_err"; then
        status=memory
    elif grep -qi 'assert' "$tmp_err"; then
        status=assert
    else
        status=error
    fi
    printf '%s  n=%-5s %-8s exit=%-4s %9s s  peak %8s kB  output %12s bytes\n' \
           "$(date '+%H:%M:%S')" "$n" "$status" "$code" "$secs" "$kb" "$bytes"
    if [ "$status" != ok ] && [ -s "$tmp_err" ]; then
        echo "          last stderr lines:"
        tail -n 5 "$tmp_err" | sed 's/^/            /'
    fi
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$n" "$SEED" "$status" "$code" "$secs" "$kb" "$bytes" >> "$results"
    [ "$status" = ok ]
}

echo "=== doubling"
lo=0
hi=0
n="$START"
while true; do
    if probe "$n"; then
        lo="$n"
        if [ "$n" -ge "$MAX_N" ]; then
            echo "=== reached MAX_N=$MAX_N without a failure"
            break
        fi
        n=$(( n * 2 > MAX_N ? MAX_N : n * 2 ))
    else
        hi="$n"
        break
    fi
done

if [ "$hi" -eq 0 ]; then
    :   # no failure: nothing to bisect
elif [ "$lo" -eq 0 ]; then
    echo "=== the first run (n=$START) already failed; try a smaller START"
else
    echo "=== bisecting between n=$lo (ok) and n=$hi (failed)"
    while [ $(( hi - lo )) -gt $(( lo / 10 > 1 ? lo / 10 : 1 )) ]; do
        mid=$(( (lo + hi) / 2 ))
        if probe "$mid"; then
            lo="$mid"
        else
            hi="$mid"
        fi
    done
    echo "=== largest n that succeeded: $lo; smallest that failed: $hi"
    echo
    echo "Suggested sparse scaling cases (edit before use):"
    for frac in 25 50 75 100; do
        m=$(( lo * frac / 100 ))
        [ "$m" -lt 11 ] && continue
        for s in 1 2 3; do
            echo "n$m-s$s|random, $m points, seed $s|-n $m -s $s"
        done
    done
fi
echo
echo "=== all done: $(date '+%Y-%m-%d %H:%M:%S')"
echo "this log:     $log"
echo "results:      $results"
echo "vmstat log:   $vmstat_log"
