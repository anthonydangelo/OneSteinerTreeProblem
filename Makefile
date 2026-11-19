# Makefile for OneSteinerTreeProblem (GNU make).
#
#   make            same as "make release"
#   make release    optimized build:              ./OneSteinerTreeProblem
#   make perf       release flags + profiling:    ./OneSteinerTreeProblem-perf
#   make debug      debug build:                  ./OneSteinerTreeProblem_d
#   make clean      delete the three binaries
#
# CGAL_DIR must point to the CGAL 4.11.2 build directory (the one containing lib/),
# e.g. "export CGAL_DIR=$HOME/CGAL-4.11.2" in ~/.profile; or pass it on the command
# line: make CGAL_DIR=/path/to/CGAL-4.11.2
# Extra compiler flags can be added with EXTRA_FLAGS, e.g. make release EXTRA_FLAGS=-DNDEBUG
#
# All builds are statically linked, so the binaries run on other x86-64 Linux systems
# without CGAL, Boost, GMP or MPFR installed.

SRCS := src/OneSteinerTreeProblem.cpp src/ComputationResult.cpp src/OrientedDirichletCell.cpp \
        src/DelaunayTriEMST.cpp src/GeomMedianFinder.cpp src/PrettyPrint.cpp src/UtilityFunctions.cpp
HDRS := $(wildcard src/*.hpp)

# Flags shared by all builds. -frounding-math is required by CGAL with g++.
COMMON_FLAGS := -frounding-math -std=c++11 -pthread
WARN_FLAGS   := -Wall -Wextra

RELEASE_FLAGS := -O2 $(WARN_FLAGS)
PERF_FLAGS    := -O2 -g -fno-omit-frame-pointer $(WARN_FLAGS)
DEBUG_FLAGS   := -g

LDFLAGS      := -static -L$(CGAL_DIR)/lib
LIBS         := -lCGAL_Core -lCGAL -lmpfr -lgmp -lboost_thread -lboost_system -lpthread
DEBUG_LIBS   := -lCGAL_Core_d -lCGAL_d -lmpfr -lgmp -lboost_thread -lboost_system -lpthread

.PHONY: all release perf debug clean check-cgal

all: release

release: OneSteinerTreeProblem
perf: OneSteinerTreeProblem-perf
debug: OneSteinerTreeProblem_d

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

clean:
	rm -f OneSteinerTreeProblem OneSteinerTreeProblem-perf OneSteinerTreeProblem_d
