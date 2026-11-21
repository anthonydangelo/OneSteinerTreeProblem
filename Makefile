# Makefile for OneSteinerTreeProblem (GNU make).
#
#   make            same as "make release"
#   make release    optimized build:              ./OneSteinerTreeProblem
#   make perf       release flags + profiling:    ./OneSteinerTreeProblem-perf
#   make debug      debug build:                  ./OneSteinerTreeProblem_d
#   make checkpoints release flags + memory checkpoints on stderr:
#                                                 ./OneSteinerTreeProblem-mem
#   make asan       AddressSanitizer + UndefinedBehaviorSanitizer build (not static):
#                                                 ./OneSteinerTreeProblem-asan
#   make clean      delete the binaries
#
# CGAL_DIR must point to the CGAL 4.11.2 build directory (the one containing lib/),
# e.g. "export CGAL_DIR=$HOME/CGAL-4.11.2" in ~/.profile; or pass it on the command
# line: make CGAL_DIR=/path/to/CGAL-4.11.2
# Extra compiler flags can be added with EXTRA_FLAGS, e.g. make release EXTRA_FLAGS=-DNDEBUG
#
# All builds except asan are statically linked, so the binaries run on other x86-64
# Linux systems without CGAL, Boost, GMP or MPFR installed. The sanitizer runtime
# can't be linked statically, so the asan build links CGAL, Boost, GMP and MPFR
# statically but the C and C++ libraries (and the sanitizer runtime) dynamically.
# It is for testing only: measured on the regression suite, it runs about 15-20 times
# slower than the release build (n6-s1: 18 s vs 1.2 s), with a peak of about 0.5 GB
# even for 3 points (sanitizer bookkeeping).
# Sanitizer reports go to stderr. After a test run, list the UBSan reports by location
# and count, and the files with an ASan/LSan report:
#   grep -h 'runtime error:' tests/out/*/<label>/*.err | cut -d: -f1-3 | sort | uniq -c
#   grep -l -E 'ERROR: (Address|Leak)Sanitizer' tests/out/*/<label>/*.err
# Two known UBSan reports come from inside CGAL 4.11.2 and are harmless in practice:
#   CGAL/Arr_segment_traits_2.h:79:9  "load of value N, which is not a valid value for
#     type 'bool'": the overlay's sweep copies a default-constructed segment, whose
#     default constructor leaves the is_pt_max flag uninitialised.
#   CGAL/Compact_container.h:1192:51  "reference binding to null pointer": inserting
#     into the Delaunay triangulation creates faces whose neighbours are still null
#     handles, and set_neighbors dereferences them.
# Any other location needs looking at.
# (ASan reserves a very large amount of virtual memory, so don't run it under ulimit -v.)

SRCS := src/OneSteinerTreeProblem.cpp src/ComputationResult.cpp src/OrientedDirichletCell.cpp \
        src/DelaunayTriEMST.cpp src/GeomMedianFinder.cpp src/PrettyPrint.cpp src/UtilityFunctions.cpp
HDRS := $(wildcard src/*.hpp)

# Flags shared by all builds. -frounding-math is required by CGAL with g++.
COMMON_FLAGS := -frounding-math -std=c++11 -pthread
WARN_FLAGS   := -Wall -Wextra

RELEASE_FLAGS := -O2 $(WARN_FLAGS)
PERF_FLAGS    := -O2 -g -fno-omit-frame-pointer $(WARN_FLAGS)
DEBUG_FLAGS   := -g
MEM_FLAGS     := $(RELEASE_FLAGS) -DMEMORY_CHECKPOINTS=1
# -O1 keeps the stack traces readable; asserts stay on (no NDEBUG).
# -fno-sanitize=vptr: CGAL's arrangement casts its DCEL records (e.g. Arr_vertex) to the
# arrangement's derived handle classes (Vertex, Face), which add no data members. UBSan's
# vptr check reports every such cast as a downcast to the wrong type, hundreds per run,
# all inside CGAL's headers.
ASAN_FLAGS    := -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize=vptr $(WARN_FLAGS)

LDFLAGS      := -static -L$(CGAL_DIR)/lib
LIBS         := -lCGAL_Core -lCGAL -lmpfr -lgmp -lboost_thread -lboost_system -lpthread
DEBUG_LIBS   := -lCGAL_Core_d -lCGAL_d -lmpfr -lgmp -lboost_thread -lboost_system -lpthread
# For the asan build: the third-party libraries static (-Bstatic), then back to dynamic
# (-Bdynamic) for pthread and the libraries g++ adds itself (libc, libstdc++, libasan,
# libubsan).
ASAN_LDFLAGS  := -L$(CGAL_DIR)/lib
ASAN_LIBS     := -Wl,-Bstatic -lCGAL_Core -lCGAL -lmpfr -lgmp -lboost_thread -lboost_system \
                 -Wl,-Bdynamic -lpthread

.PHONY: all release perf debug checkpoints asan clean check-cgal

all: release

release: OneSteinerTreeProblem
perf: OneSteinerTreeProblem-perf
debug: OneSteinerTreeProblem_d
checkpoints: OneSteinerTreeProblem-mem
asan: OneSteinerTreeProblem-asan

# Stop with a clear message if CGAL_DIR is not set (instead of a confusing link error).
check-cgal:
	@if [ -z "$(CGAL_DIR)" ]; then \
	    echo "CGAL_DIR is not set: see the comment at the top of the Makefile." >&2; exit 1; \
	fi

# Each binary is rebuilt when any source file or header changes. ($@ is the target's
# name; the "|" makes check-cgal run first without counting as a dependency for
# rebuild decisions.)
OneSteinerTreeProblem: $(SRCS) $(HDRS) | check-cgal
	$(CXX) $(RELEASE_FLAGS) $(COMMON_FLAGS) $(EXTRA_FLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

OneSteinerTreeProblem-perf: $(SRCS) $(HDRS) | check-cgal
	$(CXX) $(PERF_FLAGS) $(COMMON_FLAGS) $(EXTRA_FLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

OneSteinerTreeProblem_d: $(SRCS) $(HDRS) | check-cgal
	$(CXX) $(DEBUG_FLAGS) $(COMMON_FLAGS) $(EXTRA_FLAGS) $(SRCS) -o $@ $(LDFLAGS) $(DEBUG_LIBS)

OneSteinerTreeProblem-mem: $(SRCS) $(HDRS) | check-cgal
	$(CXX) $(MEM_FLAGS) $(COMMON_FLAGS) $(EXTRA_FLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

OneSteinerTreeProblem-asan: $(SRCS) $(HDRS) | check-cgal
	$(CXX) $(ASAN_FLAGS) $(COMMON_FLAGS) $(EXTRA_FLAGS) $(SRCS) -o $@ $(ASAN_LDFLAGS) $(ASAN_LIBS)

clean:
	rm -f OneSteinerTreeProblem OneSteinerTreeProblem-perf OneSteinerTreeProblem_d OneSteinerTreeProblem-mem OneSteinerTreeProblem-asan
