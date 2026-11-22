# Performance

How version 1.5 of this program became version 2.0: what was measured, what
each change did, and how to reproduce the numbers. A summary is in the
[README](../README.md#performance).

## Method

- **Machine:** a VirtualBox virtual machine running Ubuntu 16.04, with 4 CPUs,
  3.8 GiB of RAM and about 32 GiB of swap. The program is single-threaded.
- **Build:** `make release`: g++ 5.4 with `-O2`, CGAL 4.11.2, statically linked.
  This is the toolchain the original was written with, so the speed-ups come
  from changes to the code alone.
- **Timing:** wall-clock time and peak resident memory from GNU `time`
  (`/usr/bin/time -f "%e %M"`), recorded for each test case by
  `tests/run-tests.sh`, with `vmstat` logging memory activity alongside (by
  `tests/run-all.sh`). Peak resident memory understates the real footprint when
  a run swaps; so for runs that swapped, the swap use from `vmstat` is given as
  well.
- **Conditions:** the reference runs were made from a text console, with the
  desktop logged out. Runs in a terminal inside the desktop (VS Code) were about
  5 to 15% slower on small inputs.
- **Correctness:** every change was checked against the test suites. Outputs are
  compared after putting them in a canonical form (`tests/compare-outputs.py`),
  so an equivalent output in a different order is not reported as a change.
- **Versions:** each step is a git tag, so each column of the results can be
  rebuilt: `v1.5`, `perf/sort-fix`, `perf/overlay`, `perf/rational-oodc`, and
  `v2.0`.

## Results

Wall-clock time and, where recorded, peak resident memory, for test cases from
`tests/cases-*.txt`:

| Case | v1.5 | `perf/sort-fix` | `perf/overlay` | `perf/rational-oodc` | v2.0 | Speed-up, v1.5 → v2.0 |
|---|---|---|---|---|---|---|
| `n6-s1`: 6 random sites | 102 s, 484 MB | 101 s | 74 s, 518 MB | 1.1 s, 3 MB | 1.13 s, 3 MB | 90× |
| `n7-s1`: 7 random sites | 453 s, 1838 MB | 311 s, 1526 MB | 274 s, 2051 MB | 1.6 s, 4 MB | 1.54 s, 3 MB | 290× |
| `square4`: unit square | 204 s, 352 MB | 1.6 s | 1.4 s | 0.4 s | 0.40 s | 510× |
| `plus5`: centre and 4 axis points | 581 s, 1025 MB | 9.4 s | 9.3 s | 0.6 s | 0.61 s | 950× |
| `grid6`: 6-site grid | 22,779 s (6 h 20 min), see below | 5.6 s, 26 MB | 4.6 s | 0.96 s | 0.94 s | 24,000× |
| `grid6-rot37`: the grid rotated by 37° | over 30 min (stopped) | 61 s | 44 s | 1.0 s | 1.02 s | over 1,700× |
| `n8-s1`: 8 random sites | 3,764 s (1 h 2 min 44 s), see below | | | | 2.02 s, 4 MB | 1,900× |

Empty cells were not measured. Two of the v1.5 runs needed more memory than the
virtual machine has, so their peak resident memory (3.6 GB, the limit) understates
their footprint:

- `grid6` filled the RAM after about 30 minutes, and then used swap steadily, up to
  16.4 GB: about 20 GB in all. The processor stayed busy and waited on disk only
  1.5% of the time, so the 6 h 20 min is mostly computation, not swapping.
- `n8-s1` used 3.6 GB of RAM and about 4.9 GB of swap, about 8.5 GB in all.

## Step 1: the candidate sort (`perf/sort-fix`)

**Symptom.** Degenerate inputs were far slower than random inputs of the same
size: the unit square (4 sites) took 204 s, while random 4-site inputs took 4 to
13 s, and the 6-site grid took over 6 hours.

**Finding the cause.** On a random input, `perf` showed about 78% of the time in
GMP's big-integer multiplication routines, the arithmetic behind the exact
number types, but it could not show which code called them: it fails to unwind
the call stack inside GMP's hand-written assembly. So the call stack was sampled
with gdb instead (`profiling/sample-backtraces.sh`). For the unit square, every
complete sample (out of 15) was in the same place: `std::stable_sort` of the
candidates, inside an exact comparison of two tree lengths, with CORE
recomputing square roots at ever higher precision, on numbers of 100,000 bits
and more.

**Cause.** A tree length is a sum of square roots. Symmetric inputs produce many
trees of exactly equal length, and every candidate that coincides with a site
has exactly the sites' MST length. Comparing two equal lengths exactly means
proving that a sum of square roots is exactly zero, which can require enormous
precision (the "sum of square roots" problem, for which no polynomial-time
algorithm is known), and CORE kept computing until it could prove it.

**Fix.** Sort by the midpoint of each length's interval approximation, which
`Lazy_exact_nt` maintains anyway as a double. Since the sort is stable, candidates
whose approximations are equal keep their order. The order is correct up to
double rounding: trees whose lengths agree to about 15 significant digits may be
listed in either order. (`CGAL::to_double` would not do: it computes the exact
value when the interval is too wide.)

**Effect.** Unit square 204 s → 1.6 s, 5 sites in a plus shape 581 s → 9.4 s, the
6-site grid 6 h 20 min → 5.6 s. Random inputs barely changed (6 sites: 102 s →
101 s), because their time was going somewhere else: into the overlays (step 2).

## Step 2: overlays per direction (`perf/overlay`)

**Before.** On random inputs, the gdb samples put about 80% of the time in
overlays. Each of the 6n cells was overlaid in turn onto one growing
arrangement, which can reach O(n²) size, so each overlay re-swept the whole
arrangement built so far: O(n³ log n) for all of them.

**Change.** The cells of one cone direction have disjoint interiors and together
form that direction's oriented Voronoi diagram, of size O(n). So each direction
now gets its own running arrangement, and the six are combined with 5 overlays
at the end: O(n² log n) in all (see the README's
[Complexity](../README.md#complexity) section).

**Effect.** The outputs were unchanged, but the gain was much smaller than the
3 to 4 times expected: 6 random sites 101 s → 74 s, 7 sites 311 s → 274 s, and
between 12% and 52% on other inputs. Peak memory also rose for some inputs
(7 sites: 1.5 GB → 2.1 GB).

**Why so little.** Memory checkpoints (`make checkpoints`) for 6 random sites
showed the cells and per-direction overlays taking 27% of the time, and the 5
combining overlays 73%. Yet the final arrangement had only 58 vertices, 106 edges
and 50 faces, and single overlays of such small arrangements took 5 to 25
seconds.
So the cost was not the size of the arrangements but the arithmetic. Cells of
different directions meet exactly at shared sites. At such places, the cheap
interval filter cannot decide the geometric tests. CORE then has to evaluate
deep expressions at high precision.
The asymptotic improvement is real, but at these sizes constant factors
dominate. What could make a real difference was changing the number type, not
the algorithm (step 3).

## Step 3: rational arithmetic for the OODC (`perf/rational-oodc`)

**Change.** The OODC needs no square roots. Its inputs and cone directions are
doubles, which are rational numbers, and it only intersects lines, which takes
only `+ − × ÷`. So every coordinate in it is rational. It now uses exact
rationals, `Lazy_exact_nt<Gmpq>`, instead of `CORE::Expr`, with its own kernel
and arrangement type. The input points and the convex hull are converted to the
rational kernel once, exactly; the geometric medians and MSTs still use
`CORE::Expr`, which they need for square roots. See the README's
[Number types](../README.md#number-types) section for the trade-offs.

**Effect.** On random inputs, 70 to 170 times faster: 6 random sites 74 s →
1.1 s, 7 sites 274 s → 1.6 s. The degenerate inputs, already much faster after
step 1, gained 3.5 to 45 times (the 6-site grid 4.6 s → 0.96 s).
Memory dropped from 2.1 GB to
4 MB for 7 sites. For 6 random sites, the 5 combining overlays that had taken
56 s now took 0.03 s, and memory stayed at about 3.5 MB throughout.

**Correctness.** Most outputs were identical, but the comparison flagged a few
differences in arrangement faces. They turned out to come from an existing bug
in the output, present since v1.5, not from the new arithmetic (see "Sliver
faces" below).

## Correctness fixes found along the way

Measuring and comparing outputs carefully also turned up three bugs in the
output, all present in v1.5. None of them changed which Steiner point was found.

**Sliver faces.** After step 3, a few test outputs showed different arrangement
face lists. The cause: the cone directions are not exactly 60° (`sin(π)` as a
double is 1.22 × 10⁻¹⁶, not 0), so some cone boundaries are tilted by about
10⁻¹⁶, creating sliver faces about 10⁻¹⁶ wide. Exact arithmetic correctly kept
their vertices distinct, but the output looked up each vertex's index by
coordinates within 10⁻⁶, which gave two distinct vertices the same index. Step 3
had only changed the order in which such near-identical vertices were listed,
which made the existing bug visible. A new check, `tests/check-face-indices.py`,
then found bad faces in 14 of the 16 cases of the `120` suite, some collapsed
entirely to a single repeated vertex. Indexing the vertices exactly fixed it:
the check now finds no bad faces in any suite.

**Silent truncation.** While exploring how large an input the program can
handle, a 120-site run printed exactly 512 MiB of output (2²⁹ bytes) and exited
with 0. The whole JSON was being built in memory before printing, and under the
run's memory limit the buffer could not grow. C++ streams do not throw on such a
failure; they set an error flag, which the program never checked, so a truncated
result looked like a success. The output is now written directly to standard
output as it is produced, and a failed write makes the program exit with 1.
This also removed the memory spent on the output, which had been about 4.5 times
its size.

**Nondeterminism.** The test for the output modes showed that repeated runs of
the same 10-site input occasionally gave different output: one run in about 10.
The difference was only the order of the two endpoints of some MST edges. CGAL
reports each triangulation edge from one of its two faces, choosing by comparing
the faces' memory addresses, and those addresses change from run to run because
of address-space randomisation. The program now lists each edge's endpoints in a
fixed order (sites by index, the Steiner point last). `profiling/check-determinism.sh`
confirmed it: 50 runs gave identical output, as did 50 runs with
address-space randomisation turned off.

## Scaling

**Small inputs** (v2.0, `scaling` and `regression` suites, mean of the 
random inputs of each size): 3 sites 0.27 s, 4 sites 0.49 s, 5 sites 0.77 s, 6 sites 1.1 s,
7 sites 1.5 s, 8 sites 2.0 s, 9 sites 2.6 s, 10 sites 3.2 s, with peak memory
under 10 MB.

**Larger inputs**, with `-b` (so the output stays small; see the README's
[Usage](../README.md#usage)):

| Sites | Time | Peak memory | Source |
|---|---|---|---|
| 12 | 4.7 s | 8 MB | `explore-size.sh` |
| 24 | 19.9 s | 30 MB | `explore-size.sh` |
| 30 | 31.6 s | 46 MB | `large` suite, mean of 3 |
| 48 | 83.7 s | 135 MB | `explore-size.sh` |
| 60 | 128 s | 225 MB | `large` suite, mean of 3 |
| 90 | 294 s | about 500 MB | `large` suite, mean of 2 |
| 96 | 346 s | 578 MB | `explore-size.sh` |
| 100 | 365 s | 679 MB | `large` suite |
| 120 | 541 s | 1.03 GB | `explore-size.sh` |
| 132 | over 10 min (stopped) | | `explore-size.sh` |

Time grows about like n²: doubling the number of sites multiplies the time by
about 4.2, where exactly n² would give 2² = 4. A least-squares fit over all the
sizes from 12 to 120 gives n^2.05 for time and n^2.1 for memory; from 30 to 100
sites alone, n^2.03 and n^2.2.
The limit of 100 sites keeps runs to about
6 minutes and under 1 GB on this machine; it is a constant in the code
(`MAX_NUM_INPUT_POINTS`), so it can be raised by rebuilding.

**The full output** grows faster than the computation, because it lists every
candidate's whole tree: 1.5 MB for 12 sites, 12 MB for 24, 78 MB for 48, and
381 MB for 96. That is why `-b`, `-f` and `-o` exist.

## Where the time and memory go now

Memory checkpoints (`make checkpoints`) for 96 random sites with `-b`, which
took about 355 s in all:

| Phase | Time | Memory at the end | Notes |
|---|---|---|---|
| Cells and per-direction overlays | 334 s (94%) | about 8 MB | about 3.4 s per site, nearly constant through the run; each direction's arrangement ends with about 300 vertices |
| Combining the six arrangements | 0.7 s | 19 MB | final arrangement: 3,064 vertices, 5,795 edges, 2,733 faces |
| Sites' MST, and generating the candidates (geometric medians) | 0.2 s | 55 MB | 15,278 candidates |
| Evaluating the candidates (an MST for each, then sorting) | 21 s (6%) | 576 MB | about 34 KB per candidate |

**Time.** Each site takes about 3.4 s: building its 6 cells, at 95 subtractions
each, and adding them to their directions' arrangements. That is at most about
6 ms per subtraction (the checkpoints do not separate the overlays from the
subtractions). The time per site does not grow during the run, because on
random inputs the cells stay small. That is why the whole phase grows like n²
(the README's [Complexity](../README.md#complexity) section has the worst-case
bounds).

**Memory.** Almost all of it goes to the candidates: of the 576 MB at the end
(36 MB was added while generating them and 521 MB while evaluating them), about
34 KB for each of the 15,278 candidates. Each candidate keeps its tree and its exact length
(a `CORE::Expr` with one square root per edge), and these exact lengths are
probably most of it (this has not been measured directly).

Options for both are in the README's
[Limitations and future work](../README.md#limitations-and-future-work).

## Sanitizers and static analysis

**Sanitizers.** `make asan` builds the program with AddressSanitizer (which also
runs LeakSanitizer) and UndefinedBehaviorSanitizer. Under it, the `120`,
`regression` and `slow` suites ran with no memory errors, no leaks, and no
undefined behaviour in this program's code; every case exited with 0, and the
outputs matched the release build's. The sanitizer build runs 15 to 20 times
slower than the release build (6 random sites: 18 s instead of 1.2 s), and uses
about 0.5 GB even for 3 sites, for the sanitizers' own bookkeeping.

Getting there took some sorting out of reports from CGAL itself:

- **UBSan's `vptr` check.** In C++, an object of a class with virtual functions
  carries a hidden pointer, the *vptr*, to a table describing its actual class.
  UBSan's `vptr` check uses it to verify, at each cast or member call, that the
  object really is of the class the code treats it as. CGAL's arrangement code
  stores its vertices, edges and faces as basic records, and casts them to
  derived classes (its `Vertex`, `Face` and so on) that add functions but no
  data. Strictly, casting an object to a class it isn't is undefined behaviour,
  so the check reported it: hundreds of times per run, all inside CGAL. In
  practice it works with every compiler CGAL supports, since the derived classes have exactly
  the same layout, and it is how CGAL has long been designed, so the check is
  turned off.
- **A false leak.** LeakSanitizer also reported 578 leaked allocations. They
  were made by `realloc` inside the C++ standard library, and the stack traces
  did not lead back into this program or CGAL. The number gave a clue. Inside a
  compiled program, C++ type names are stored in a compact encoded form
  ("mangled"). To print a readable name in a report, the sanitizer decodes it
  ("demangles" it) with a function from the standard library, which allocates
  memory for the result. Each `vptr` report printed two long type names, and
  there were about 290 reports: about 580 demangled names, against 578 "leaked"
  allocations. So the likely explanation was that the sanitizer never freed the
  names it demangled for its own reports. Turning the `vptr` check off confirmed
  it: with no more `vptr` reports, the leak reports disappeared too.
- **Two reports remain, both inside CGAL 4.11.2**, from ordinary calls by this
  program, and both harmless in practice:
  - `Arr_segment_traits_2.h:79`: a segment's default constructor leaves one of
    its `bool` flags uninitialised, and the overlay's sweep copies such a
    segment.
  - `Compact_container.h:1192`: inserting the first few points into the Delaunay
    triangulation creates faces whose neighbours are still null handles, and
    `set_neighbors` dereferences them without reading through them.

  Both are documented in the Makefile, and the checks allow exactly these two
  locations.

**Static analysis.** cppcheck 1.72, with its warning, style, performance and
portability checks, run on `src/` (without CGAL's headers), reported one issue:
a one-argument constructor not marked `explicit`, in the `Is_finite` filter
copied from CGAL's EMST example. With that fixed, it reports nothing.

## Reproducing

Each column of the results is a git tag: `v1.5`, `perf/sort-fix`,
`perf/overlay`, `perf/rational-oodc` and `v2.0`. All of them can be built with
the current Makefile: `v1.5` has no Makefile of its own, but its release build
used the same compiler flags and source files. The steps, for example for
`v1.5`, from the repository's root:

```bash
git worktree add /tmp/osp-v1.5 v1.5                          # check out the tag in a separate folder
make -C /tmp/osp-v1.5 -f "$PWD/Makefile" release             # build it with the current Makefile
cp /tmp/osp-v1.5/OneSteinerTreeProblem /tmp/osp-v1.5-bin     # keep the binary
rm -rf /tmp/osp-v1.5 && git worktree prune                   # tidy up
tests/run-all.sh /tmp/osp-v1.5-bin v1.5 regression:square4   # one case, about 3.5 minutes in v1.5
tests/run-all.sh ./OneSteinerTreeProblem mylabel regression:square4   # the same case, current version
tests/run-tests.sh compare regression v1.5 mylabel           # compare (other cases show as MISSING)
```

Things to know when comparing with versions before 2.0:

- Their outputs differ in some faces' vertex lists and in the order of MST edge
  endpoints. These are the correctness fixes described above, not changes in
  the results. `tests/compare-without-arrangement.py <suite> <label1> <label2>`
  compares everything except the arrangement.
  The face check also reports bad faces for these versions, for the same reason.
- They have no `-b`, `-f` or `-o`, and accept at most 10 sites, so the `large`
  suite and the `modes` suite do not apply to them.
- Whole suites take longer with old versions: in v1.5, `regression` takes about
  20 minutes and `slow` about 9. 
- Some inputs take hours in v1.5 (the `extreme` suite's `grid6` took 6 h 20 min).
  Setting `CASE_TIMEOUT`, e.g. `CASE_TIMEOUT=30m`, stops each case after that
  long and reports it as a timeout.
- For clean timings, run in a text console with the desktop logged out (see
  [Method](#method)).
  
