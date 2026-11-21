#!/usr/bin/env bash
# Checks whether the program gives the same output every time for the same input.
# Runs it RUNS times as usual, RUNS times with address-space randomisation turned off
# (setarch -R), and RUNS times with -b; keeps one copy of each distinct output
# (elapsedTime line removed) and counts how often each occurred. If an output varies,
# it shows a diff of the first two variants. It also compares the -b output's MST and
# best candidate with those of every distinct full output.
#
# Usage (from the repo root):
#   profiling/check-determinism.sh [binary] [program arguments...]
#   default: ./OneSteinerTreeProblem -n 10 -s 2
# Settings: RUNS (default 50).
# Everything printed is also saved in tests/out/determinism-<date-time>.log.

set -u

bin="${1:-./OneSteinerTreeProblem}"
if [ $# -gt 1 ]; then
    shift
    args=("$@")
else
    args=(-n 10 -s 2)
fi
RUNS="${RUNS:-50}"

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
stamp="$(date '+%Y%m%d-%H%M%S')"
work="$(mktemp -d)"
log="$repo_root/tests/out/determinism-$stamp.log"
mkdir -p "$repo_root/tests/out"
exec > >(tee -a "$log") 2>&1

echo "=== check-determinism started $(date '+%Y-%m-%d %H:%M:%S')"
echo "binary: $bin  arguments: ${args[*]}  runs per variant: $RUNS"
echo "git commit: $(cd "$repo_root" && git rev-parse --short HEAD 2> /dev/null)"
echo

# Runs the program RUNS times with the given prefix command and extra options;
# saves each distinct output in $work/<variant>/<hash>.json and prints the counts.
run_variant() {
    local variant="$1"
    shift
    local dir="$work/$variant"
    mkdir -p "$dir"
    local i h
    for i in $(seq 1 "$RUNS"); do
        "$@" | grep -v '"s":' > "$dir/current"
        h=$(md5sum < "$dir/current" | cut -c1-32)
        [ -f "$dir/$h.json" ] || cp "$dir/current" "$dir/$h.json"
        echo "$h"
    done | sort | uniq -c | sed 's/^/    /'
    rm -f "$dir/current"
    local n
    n=$(ls "$dir"/*.json | wc -l)
    echo "    -> $n distinct output(s)"
    if [ "$n" -gt 1 ]; then
        local files
        files=( "$dir"/*.json )
        echo "    diff of the first two (first 60 lines; $(diff "${files[0]}" "${files[1]}" | wc -l) lines in all):"
        diff "${files[0]}" "${files[1]}" | head -n 60 | sed 's/^/      /'
    fi
    echo
}

echo "=== full output, $RUNS runs"
run_variant full "$bin" "${args[@]}"

if command -v setarch > /dev/null; then
    echo "=== full output with address randomisation off (setarch -R), $RUNS runs"
    run_variant norand setarch "$(uname -m)" -R "$bin" "${args[@]}"
else
    echo "=== setarch not found: skipping the run without address randomisation"
    echo
fi

echo "=== -b output, $RUNS runs"
run_variant best "$bin" -b "${args[@]}"

echo "=== -b's MST and best candidate vs each distinct full output"
python3 - "$work" <<'EOF'
import glob, json, os, sys
work = sys.argv[1]
bests = sorted(glob.glob(os.path.join(work, 'best', '*.json')))
fulls = sorted(glob.glob(os.path.join(work, 'full', '*.json')))
for b in bests:
    rb = json.load(open(b))['compResult']
    for f in fulls:
        rf = json.load(open(f))['compResult']
        mst = 'same' if rb['minSpanningTree'] == rf['minSpanningTree'] else 'DIFFERENT'
        cand = 'same' if rb['candidateStPtDataList'] == rf['candidateStPtDataList'][:1] else 'DIFFERENT'
        print('    -b {} vs full {}: MST {}, best candidate {}'.format(
            os.path.basename(b)[:8], os.path.basename(f)[:8], mst, cand))
EOF
echo
echo "=== all done: $(date '+%Y-%m-%d %H:%M:%S')"
echo "distinct outputs kept in: $work"
echo "this log: $log"
