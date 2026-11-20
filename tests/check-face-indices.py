#!/usr/bin/env python3
# Checks the arrangement output of a test run. In each face's corner list:
#  - consecutive corners are the two ends of an edge, so they must be different
#    vertices (an arrangement has no zero-length edges), including last -> first;
#  - indices must refer to listed points;
#  - a face needs at least 3 corners.
# A vertex repeated non-consecutively is possible (a face whose boundary touches
# itself at a vertex), so it is reported as a note, not an error.
# Usage (from the repo root):
#   python3 tests/check-face-indices.py <suite> <label>
import glob, json, os, sys

suite, label = sys.argv[1:3]
bad = 0
for path in sorted(glob.glob(os.path.join('tests', 'out', suite, label, '*.json'))):
    name = os.path.splitext(os.path.basename(path))[0]
    try:
        arr = json.loads(open(path).read())['compResult'].get('odcArrangement')
    except (ValueError, KeyError):
        print('{:16} SKIPPED (unreadable JSON)'.format(name))
        continue
    if arr is None:
        continue
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
    else:
        print('{:16} OK ({} points, {} faces)'.format(name, numPoints, len(arr['arrangementFaces'])))
    for n in notes:
        print('{:16}   note: {}'.format('', n))
print('{} case(s) with bad faces.'.format(bad))
