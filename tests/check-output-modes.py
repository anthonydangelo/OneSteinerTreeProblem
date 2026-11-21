#!/usr/bin/env python3
# Checks that the output modes agree with the full output. For each input it runs the
# program four times (full, -b, -f, -o) and checks:
#   -b: no arrangement; only the best candidate (the full output's first); same
#       inputs, hull and MST;
#   -o: the full arrangement, unchanged; only the best candidate;
#   -f: exactly the faces of the full arrangement whose site set contains the best
#       candidate's sites (compared by corner coordinates and sites), and as points
#       exactly those faces' corners, in the full output's order.
# Usage (from the repo root):
#   python3 tests/check-output-modes.py ./OneSteinerTreeProblem -n 6 -s 1      (one input)
#   python3 tests/check-output-modes.py --cases tests/cases-modes.txt ./OneSteinerTreeProblem
# A cases file has the format of the other suites (name|description|arguments).
# When an input fails, its four outputs are saved for inspection, as
# tests/out/modes-fail/<case>-full.json, -b.json, -f.json and -o.json.
# Exit code: 0 if every check passes, 1 otherwise.
import json, os, re, subprocess, sys

FAIL_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out', 'modes-fail')
MODES = (('full', []), ('-b', ['-b']), ('-f', ['-f']), ('-o', ['-o']))


def run_all_modes(binary, args):
    """Runs the program once per output mode; returns {mode: raw output text}."""
    return dict((name, subprocess.check_output([binary] + extra + args).decode('utf-8'))
                for name, extra in MODES)


def save_outputs(case, raw):
    """Saves a failing input's outputs; returns the folder they went to."""
    if not os.path.isdir(FAIL_DIR):
        os.makedirs(FAIL_DIR)
    safe = re.sub(r'[^A-Za-z0-9._-]+', '_', case).strip('_-') or 'input'
    for name, text in raw.items():
        with open(os.path.join(FAIL_DIR, '{}-{}.json'.format(safe, name.lstrip('-'))), 'w') as f:
            f.write(text)
    return FAIL_DIR


def indices(items):
    return [int(i['index']) for i in items]


def face_key(face, points):
    """A face as (sites, corner coordinates starting at the smallest rotation)."""
    f = face['arrFace']
    corners = [json.dumps(points[i], sort_keys=True) for i in indices(f['arrangementFaceCCWVIndices'])]
    rot = min(corners[k:] + corners[:k] for k in range(len(corners))) if corners else []
    return (tuple(sorted(indices(f['arrangementOODCSiteIndices']))), tuple(rot))


def check_input(raw):
    """Returns (list of failure messages, info string) for one input's outputs."""
    failures = []
    info = ''

    def check(condition, message):
        if not condition:
            failures.append(message)

    full, best, faces, oodc = (json.loads(raw[name])['compResult'] for name in ('full', '-b', '-f', '-o'))
    cands = full['candidateStPtDataList']

    for name, r in (('-b', best), ('-f', faces), ('-o', oodc)):
        for key in ('inputPoints', 'convexHullPoints', 'minSpanningTree'):
            check(r.get(key) == full.get(key), '{}: {} differs from the full output'.format(name, key))
        check(r['candidateStPtDataList'] == cands[:1],
              '{}: candidate list is not exactly the full output\'s best candidate'.format(name))

    check('odcArrangement' not in best, '-b: has an arrangement')
    check(oodc.get('odcArrangement') == full.get('odcArrangement'), '-o: arrangement differs from the full output')

    if not cands:
        check('odcArrangement' not in faces, '-f: has an arrangement although there are no candidates')
        info = 'no candidates'
    else:
        sites = set(indices(cands[0]['candidateStPtData']['steinerPoint']['geomMedNeighbourIndices']))
        fullArr = full['odcArrangement']
        fullPts = fullArr['arrangementPoints']
        expectedFaces = [f for f in fullArr['arrangementFaces']
                         if sites <= set(indices(f['arrFace']['arrangementOODCSiteIndices']))]
        arr = faces.get('odcArrangement')
        check(arr is not None, '-f: no arrangement')
        if arr is not None:
            pts = arr['arrangementPoints']
            got = sorted(face_key(f, pts) for f in arr['arrangementFaces'])
            want = sorted(face_key(f, fullPts) for f in expectedFaces)
            check(got == want, '-f: faces differ from the full output\'s faces containing sites {}'.format(sorted(sites)))
            used = set()
            for f in expectedFaces:
                used.update(indices(f['arrFace']['arrangementFaceCCWVIndices']))
            check(pts == [p for i, p in enumerate(fullPts) if i in used],
                  '-f: points are not exactly the corners of those faces, in order')
            info = '-f wrote {} of {} faces, {} of {} points'.format(
                len(arr['arrangementFaces']), len(fullArr['arrangementFaces']), len(pts), len(fullPts))
    return failures, info


def read_cases(path):
    cases = []
    for line in open(path):
        line = line.strip()
        if line and not line.startswith('#'):
            fields = line.split('|')
            cases.append((fields[0], fields[2].split() if len(fields) > 2 else []))
    return cases


if len(sys.argv) >= 3 and sys.argv[1] == '--cases':
    cases, binary = read_cases(sys.argv[2]), sys.argv[3]
else:
    binary = sys.argv[1]
    cases = [(' '.join(sys.argv[2:]), sys.argv[2:])]

bad = 0
for name, args in cases:
    raw = {}
    try:
        raw = run_all_modes(binary, args)
        failures, info = check_input(raw)
    except (OSError, subprocess.CalledProcessError, ValueError, KeyError) as e:
        failures, info = ['could not run or read the output: {}'.format(e)], ''
    if failures:
        bad += 1
        print('{:16} FAIL  {}'.format(name, failures[0]))
        for f in failures[1:]:
            print('{:16}       {}'.format('', f))
        if raw:
            print('{:16}       outputs saved in {}'.format('', save_outputs(name, raw)))
    else:
        print('{:16} OK    {}'.format(name, info))
print('{} input(s) checked, {} failed.'.format(len(cases), bad))
sys.exit(1 if bad else 0)
