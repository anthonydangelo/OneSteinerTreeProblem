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
# Exit code: 0 if every check passes, 1 otherwise.
import json, subprocess, sys


def run(binary, extra, args):
    out = subprocess.check_output([binary] + extra + args)
    return json.loads(out.decode('utf-8'))['compResult']


def indices(items):
    return [int(i['index']) for i in items]


def face_key(face, points):
    """A face as (sites, corner coordinates starting at the smallest rotation)."""
    f = face['arrFace']
    corners = [json.dumps(points[i], sort_keys=True) for i in indices(f['arrangementFaceCCWVIndices'])]
    rot = min(corners[k:] + corners[:k] for k in range(len(corners))) if corners else []
    return (tuple(sorted(indices(f['arrangementOODCSiteIndices']))), tuple(rot))


def check_input(binary, args):
    """Returns (list of failure messages, info string) for one input."""
    failures = []
    info = ''

    def check(condition, message):
        if not condition:
            failures.append(message)

    full = run(binary, [], args)
    best = run(binary, ['-b'], args)
    faces = run(binary, ['-f'], args)
    oodc = run(binary, ['-o'], args)
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
    try:
        failures, info = check_input(binary, args)
    except (OSError, subprocess.CalledProcessError, ValueError, KeyError) as e:
        failures, info = ['could not run or read the output: {}'.format(e)], ''
    if failures:
        bad += 1
        print('{:16} FAIL  {}'.format(name, failures[0]))
        for f in failures[1:]:
            print('{:16}       {}'.format('', f))
    else:
        print('{:16} OK    {}'.format(name, info))
print('{} input(s) checked, {} failed.'.format(len(cases), bad))
sys.exit(1 if bad else 0)
