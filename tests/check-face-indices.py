#!/usr/bin/env python3
# Checks the arrangement output of a test run. In each face's corner list:
#  - consecutive corners are the two ends of an edge, so they must be different
#    vertices (an arrangement has no zero-length edges), including last -> first;
#  - indices must refer to listed points;
#  - a face needs at least 3 corners.
# A vertex repeated non-consecutively is possible (a face whose boundary touches
# itself at a vertex), so it is reported as a note, not an error.
# Prints only cases with problems or notes, then a summary.
# Exit code: 0 if no face has an error, 1 otherwise.
# Usage:
#   python3 tests/check-face-indices.py <suite> <label>
import glob, json, os, sys

OUT_ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')

if len(sys.argv) != 3:
    sys.exit('usage: python3 tests/check-face-indices.py <suite> <label>')

suite, label = sys.argv[1:3]
checked = 0
bad = 0
for path in sorted(glob.glob(os.path.join(OUT_ROOT, suite, label, '*.json'))):
    name = os.path.splitext(os.path.basename(path))[0]
    try:
        arr = json.loads(open(path).read())['compResult'].get('odcArrangement')
    except (ValueError, KeyError):
        print('{:16} ERROR (unreadable JSON)'.format(name))
        bad += 1
        continue
    if arr is None:
        continue
    checked += 1
    numPoints = len(arr['arrangementPoints'])
    errors = []
    notes = []
    for f in arr['arrangementFaces']:
        face = f['arrFace']
        idx = [int(v['index']) for v in face['arrangementFaceCCWVIndices']]
        if len(idx) < 3:
            errors.append('{}: fewer than 3 corners {}'.format(face['name'], idx))
        if any(i < 0 or i >= numPoints for i in idx):
            errors.append('{}: index out of range {}'.format(face['name'], idx))
        if any(idx[k] == idx[(k + 1) % len(idx)] for k in range(len(idx))):
            errors.append('{}: same vertex twice in a row {}'.format(face['name'], idx))
        elif len(set(idx)) != len(idx):
            notes.append('{}: vertex repeated (pinch point?) {}'.format(face['name'], idx))
    if errors:
        bad += 1
        print('{:16} {} bad face(s), e.g. {}'.format(name, len(errors), errors[0]))
    for n in notes:
        print('{:16} note: {}'.format(name, n))
print('{} case(s) checked, {} with bad faces.'.format(checked, bad))
sys.exit(1 if bad else 0)
