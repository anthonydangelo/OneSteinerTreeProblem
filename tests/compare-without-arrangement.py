#!/usr/bin/env python3
# Compares two runs of a suite like "run-tests.sh compare", but ignores the
# arrangement (odcArrangement), to check that inputs, MSTs and every candidate agree.
# Usage:
#   python3 tests/compare-without-arrangement.py <suite> <label1> <label2>
import importlib.util, json, os, sys

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('co', os.path.join(TESTS_DIR, 'compare-outputs.py'))
co = importlib.util.module_from_spec(spec)
spec.loader.exec_module(co)

suite, a, b = sys.argv[1:4]
cases = [c[0] for c in co.read_cases(os.path.join(TESTS_DIR, 'cases-{}.txt'.format(suite)))]
for name in cases:
    pa = os.path.join(TESTS_DIR, 'out', suite, a, name + '.json')
    pb = os.path.join(TESTS_DIR, 'out', suite, b, name + '.json')
    try:
        ca, _ = co.canonical(json.loads(open(pa).read()))
        cb, _ = co.canonical(json.loads(open(pb).read()))
    except (IOError, ValueError, KeyError) as e:
        print('{:16} SKIPPED ({})'.format(name, e.__class__.__name__))
        continue
    ca.pop('odcArrangement', None)
    cb.pop('odcArrangement', None)
    if ca == cb:
        status = 'SAME'
    elif co.numbers_close(ca, cb):
        status = 'CLOSE     ' + co.first_difference(ca, cb)
    else:
        status = 'DIFFERENT ' + co.first_difference(ca, cb)
    print('{:16} {}'.format(name, status))
