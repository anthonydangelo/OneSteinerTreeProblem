#!/usr/bin/env python3
"""Compares two runs of a test suite, ignoring differences that don't matter.

Usage: compare-outputs.py <cases file> <run directory A> <run directory B>
(normally called by run-tests.sh compare <suite> <label A> <label B>)

Both outputs are first put in a canonical form, so that equivalent results compare
equal even if the program lists things in a different order:
  - elapsedTime is dropped (run time is reported separately: from <case>.time, written
    by run-tests.sh when GNU time is installed, which also gives peak memory; otherwise
    from elapsedTime, in whole seconds);
  - arrangement faces: the generated names (arrFace__k) are dropped, each face's CCW
    vertex cycle is rotated to start at its smallest vertex index, and the faces are
    sorted;
  - candidate Steiner points are keyed by their sorted site indices and sorted by that
    key (equal-length candidates may be listed in a different order); in each
    candidate's tree, the Steiner endpoint's index (its position in the candidate
    list) is replaced by "S";
  - MST edge lists are sorted, each edge as a sorted pair.
Input points and arrangement points need no remapping: the program already lists
both sorted by (x, y), so their indices don't depend on the computation order.

All values are compared as printed (exact text). If two outputs differ only by
numbers within a relative 1e-9 of each other, the case is reported as CLOSE.
Exit code: 0 if every case is SAME, 1 otherwise.
Set IGNORE_STDERR=1 to skip comparing stderr (e.g. against a sanitizer build, whose
stderr has the sanitizer's reports); exit codes and outputs are still compared.
Written for Python 3.5 (Ubuntu 16.04): no f-strings.
"""
import json
import os
import sys

REL_TOL = 1e-9


def read_cases(path):
    cases = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            fields = line.split('|')
            cases.append((fields[0], fields[1]))
    return cases


def indices(lst):
    return [int(item['index']) for item in lst]


def rotate_to_min(cycle):
    if not cycle:
        return cycle
    return min(cycle[k:] + cycle[:k] for k in range(len(cycle)))


def canon_edges(edge_list):
    edges = []
    for item in edge_list:
        e = item['mstEdge']
        ends = indices(e['mstEdgeEndpointIndices'])
        steiner = [e['firstEndPointIsSteiner'] == 'true', e['secondEndPointIsSteiner'] == 'true']
        pair = ['S' if steiner[i] else str(ends[i]) for i in range(2)]
        edges.append(tuple(sorted(pair)))
    return sorted(edges)


def canon_mst(mst):
    return {'mstLength': mst['mstLength'], 'edges': canon_edges(mst.get('mstEdgeList', []))}


def canon_face(item):
    f = item['arrFace']
    return {'sites': indices(f['arrangementOODCSiteIndices']),
            'cycle': rotate_to_min(indices(f['arrangementFaceCCWVIndices']))}


def canon_candidate(item):
    c = item['candidateStPtData']
    st = c['steinerPoint']
    return {'sites': sorted(indices(st['geomMedNeighbourIndices'])),
            'point': st['geomMedPoint'],
            'coincides': st['geomMedCoincidesWithInputPt'],
            'coincidentIndex': st['geomMedCoincidentInputPtIndex']['index'],
            'tree': canon_mst(c['minSpanningTree'])}


def canonical(result):
    """Returns (canonical dict, elapsed seconds or None)."""
    elapsed = None
    if 'elapsedTime' in result:
        t = result['elapsedTime']
        try:
            elapsed = float(t.get('h', 0)) * 3600 + float(t.get('m', 0)) * 60 + float(t.get('s', 0))
        except (ValueError, AttributeError):
            elapsed = None
    r = result['compResult']
    out = {}
    for key in r:
        if key == 'odcArrangement':
            arr = r[key]
            faces = [canon_face(f) for f in arr['arrangementFaces']]
            out[key] = {'arrangementPoints': arr['arrangementPoints'],
                        'arrangementFaces': sorted(faces, key=lambda f: json.dumps(f, sort_keys=True))}
        elif key == 'minSpanningTree':
            out[key] = canon_mst(r[key])
        elif key == 'candidateStPtDataList':
            cands = [canon_candidate(c) for c in r[key]]
            out[key] = sorted(cands, key=lambda c: c['sites'])
        else:
            out[key] = r[key]
    return out, elapsed


def best(cands):
    """(best tree length, list of site sets achieving it)."""
    if not cands:
        return None, []
    lengths = [float(c['tree']['mstLength']) for c in cands]
    m = min(lengths)
    return m, [c['sites'] for c, l in zip(cands, lengths) if l == m]


def numbers_close(a, b):
    """True if a and b have the same structure and differ only in numbers within REL_TOL."""
    if isinstance(a, dict) and isinstance(b, dict):
        return a.keys() == b.keys() and all(numbers_close(a[k], b[k]) for k in a)
    if isinstance(a, (list, tuple)) and isinstance(b, (list, tuple)):
        return len(a) == len(b) and all(numbers_close(x, y) for x, y in zip(a, b))
    if a == b:
        return True
    try:
        x, y = float(a), float(b)
    except (TypeError, ValueError):
        return False
    return abs(x - y) <= REL_TOL * max(1.0, abs(x), abs(y))


def first_difference(a, b, path=''):
    """A short description of where a and b first differ."""
    if isinstance(a, dict) and isinstance(b, dict):
        for k in sorted(set(a) | set(b)):
            if k not in a or k not in b:
                return '{}/{}: present in only one run'.format(path, k)
            if a[k] != b[k]:
                return first_difference(a[k], b[k], path + '/' + k)
    if isinstance(a, (list, tuple)) and isinstance(b, (list, tuple)):
        if len(a) != len(b):
            return '{}: {} vs {} items'.format(path, len(a), len(b))
        for i, (x, y) in enumerate(zip(a, b)):
            if x != y:
                return first_difference(x, y, '{}[{}]'.format(path, i))
    return '{}: {!r} vs {!r}'.format(path, a, b)


def read_text(path):
    try:
        with open(path) as f:
            return f.read()
    except IOError:
        return None


def compare_case(name, dir_a, dir_b):
    """Returns (status, detail, time A, time B)."""
    exit_a = read_text(os.path.join(dir_a, name + '.exit'))
    exit_b = read_text(os.path.join(dir_b, name + '.exit'))
    if exit_a is None or exit_b is None:
        return 'MISSING', 'no output in {}'.format(dir_a if exit_a is None else dir_b), None, None
    if exit_a.strip() != exit_b.strip():
        return 'DIFFERENT', 'exit code {} vs {}'.format(exit_a.strip(), exit_b.strip()), None, None
    if os.environ.get('IGNORE_STDERR') != '1' and \
            read_text(os.path.join(dir_a, name + '.err')) != read_text(os.path.join(dir_b, name + '.err')):
        return 'DIFFERENT', 'stderr differs', None, None
    if exit_a.strip() != '0':
        return 'SAME', 'both exited with {} (same stderr)'.format(exit_a.strip()), None, None
    try:
        ca, ta = canonical(json.loads(read_text(os.path.join(dir_a, name + '.json'))))
        cb, tb = canonical(json.loads(read_text(os.path.join(dir_b, name + '.json'))))
    except (TypeError, ValueError, KeyError) as e:
        return 'ERROR', 'could not read the JSON output: {!r}'.format(e), None, None
    ba = best(ca.get('candidateStPtDataList', []))
    summary = 'best tree {:.10g} from sites {}'.format(ba[0], ba[1]) if ba[0] is not None else 'no candidates'
    if ca == cb:
        return 'SAME', summary, ta, tb
    bb = best(cb.get('candidateStPtDataList', []))
    best_note = '' if ba == bb else '; BEST DIFFERS: {} vs {}'.format(ba, bb)
    if numbers_close(ca, cb):
        return 'CLOSE', 'numbers differ within {}: {}{}'.format(REL_TOL, first_difference(ca, cb), best_note), ta, tb
    return 'DIFFERENT', first_difference(ca, cb) + best_note, ta, tb


def read_time_file(path):
    """(elapsed seconds, peak KB) from a GNU time "%e %M" file, or (None, None)."""
    text = read_text(path)
    if not text or not text.strip():
        return None, None
    try:
        elapsed, peak = text.strip().splitlines()[-1].split()
        return float(elapsed), int(peak)
    except ValueError:
        return None, None


def fmt_time(t):
    if t is None:
        return '?'
    return '{:.1f}s'.format(t) if t < 10 else '{:.0f}s'.format(t)


def fmt_mem(kb):
    return '?' if kb is None else '{:.0f} MB'.format(kb / 1024.0)


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    cases_file, dir_a, dir_b = sys.argv[1:]
    counts = {}
    for name, desc in read_cases(cases_file):
        status, detail, ta, tb = compare_case(name, dir_a, dir_b)
        counts[status] = counts.get(status, 0) + 1
        ea, ma = read_time_file(os.path.join(dir_a, name + '.time'))
        eb, mb = read_time_file(os.path.join(dir_b, name + '.time'))
        if ea is not None or eb is not None:
            ta = ea if ea is not None else ta
            tb = eb if eb is not None else tb
        timing = ''
        if ta is not None or tb is not None:
            timing = '  time {} -> {}'.format(fmt_time(ta), fmt_time(tb))
        if ma is not None or mb is not None:
            timing += ', peak {} -> {}'.format(fmt_mem(ma), fmt_mem(mb))
        print('{:<16} {:<9} {}{}   [{}]'.format(name, status, detail, timing, desc))
    print(', '.join('{} {}'.format(counts[k], k.lower()) for k in sorted(counts)) + '.')
    return 0 if set(counts) <= {'SAME'} else 1


if __name__ == '__main__':
    sys.exit(main())
