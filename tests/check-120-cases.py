#!/usr/bin/env python3
"""Checks one run of the 120-degree test cases against the expected results.

Usage: check-120-cases.py <cases file> <run directory>

For each case, finds the candidate whose sites are the triangle's three points and
checks it against the expectation in the cases file (see the comments there).
Exits with 0 if no case FAILs. A case whose output has no such candidate is reported
as NOT EXERCISED (the OODC produced no face with those three sites), not as a failure.
Written for Python 3.5 (Ubuntu 16.04): no f-strings.
"""
import json
import os
import sys

REL_TOL = 1e-12   # for comparing MST lengths printed with max_digits10


def read_cases(path):
    cases = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            name, desc, points, expected = line.split('|')
            cases.append((name, desc, json.loads(points), expected.split()))
    return cases


def index_of(points, x, y):
    for i, p in enumerate(points):
        if float(p['x']) == float(x) and float(p['y']) == float(y):
            return i
    return None


def orient(a, b, c):
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])


def strictly_inside(p, tri):
    signs = [orient(tri[i], tri[(i + 1) % 3], p) for i in range(3)]
    return all(s > 0 for s in signs) or all(s < 0 for s in signs)


def close(a, b):
    return abs(a - b) <= REL_TOL * max(1.0, abs(a), abs(b))


def check_case(name, points, expected, run_dir):
    """Returns (status, message); status is PASS, FAIL or NOT EXERCISED."""
    try:
        with open(os.path.join(run_dir, name + '.exit')) as f:
            code = f.read().strip()
    except IOError:
        return 'FAIL', 'no output for this case (was it run?)'
    if code != '0':
        with open(os.path.join(run_dir, name + '.err')) as f:
            err = f.read().strip().replace('\n', ' | ')
        return 'FAIL', 'exit code {}: {}'.format(code, err)
    try:
        with open(os.path.join(run_dir, name + '.json')) as f:
            result = json.load(f)['compResult']
    except (IOError, ValueError, KeyError) as e:
        return 'FAIL', 'could not read the JSON output: {}'.format(e)

    inputs = result['inputPoints']
    tri_idx = [index_of(inputs, p[0], p[1]) for p in points[:3]]
    if None in tri_idx:
        return 'FAIL', 'a triangle point is missing from inputPoints'
    mst_len = float(result['minSpanningTree']['mstLength'])

    cands = []
    for item in result['candidateStPtDataList']:
        data = item['candidateStPtData']
        sites = sorted(int(s['index']) for s in data['steinerPoint']['geomMedNeighbourIndices'])
        if sites == sorted(tri_idx):
            cands.append(data)
    if not cands:
        return 'NOT EXERCISED', 'no candidate with sites {}'.format(sorted(tri_idx))
    if len(cands) > 1:
        return 'FAIL', '{} candidates with the same three sites'.format(len(cands))

    st = cands[0]['steinerPoint']
    coincides = st['geomMedCoincidesWithInputPt'] == 'true'
    coinc_idx = int(st['geomMedCoincidentInputPtIndex']['index'])
    med = (float(st['geomMedPoint']['x']), float(st['geomMedPoint']['y']))
    cand_len = float(cands[0]['minSpanningTree']['mstLength'])

    if expected[0] == 'vertex':
        want = index_of(inputs, expected[1], expected[2])
        if not coincides:
            return 'FAIL', 'expected the input point at ({}, {}), got a new point {}'.format(
                expected[1], expected[2], med)
        if coinc_idx != want:
            return 'FAIL', 'coincides with input index {}, expected {} ({}, {})'.format(
                coinc_idx, want, expected[1], expected[2])
        if not close(cand_len, mst_len):
            return 'FAIL', 'tree length {!r} differs from the MST length {!r}'.format(cand_len, mst_len)
        return 'PASS', 'coincides with input index {} ({}, {})'.format(want, expected[1], expected[2])

    # interior
    if coincides:
        return 'FAIL', 'expected a new interior point, got input index {}'.format(coinc_idx)
    tri = [(float(inputs[i]['x']), float(inputs[i]['y'])) for i in tri_idx]
    if not strictly_inside(med, tri):
        return 'FAIL', 'median {} is not strictly inside the triangle'.format(med)
    if len(inputs) == 3 and not cand_len < mst_len:
        return 'FAIL', 'tree length {!r} is not shorter than the MST length {!r}'.format(cand_len, mst_len)
    return 'PASS', 'interior point ({:.6g}, {:.6g}); tree {:.6g} < MST {:.6g}'.format(
        med[0], med[1], cand_len, mst_len)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    cases_file, run_dir = sys.argv[1], sys.argv[2]
    counts = {'PASS': 0, 'FAIL': 0, 'NOT EXERCISED': 0}
    for name, desc, points, expected in read_cases(cases_file):
        status, msg = check_case(name, points, expected, run_dir)
        counts[status] += 1
        print('{:<2} {:<13} {}   [{}]'.format(name, status, msg, desc))
    print('{} passed, {} failed, {} not exercised.'.format(
        counts['PASS'], counts['FAIL'], counts['NOT EXERCISED']))
    return 1 if counts['FAIL'] else 0


if __name__ == '__main__':
    sys.exit(main())
