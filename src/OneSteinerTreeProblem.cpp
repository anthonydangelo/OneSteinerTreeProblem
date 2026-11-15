//============================================================================
// Name        : OneSteinerTreeProblem.cpp
// Author      : Anthony D'Angelo
//               Portions adapted from CGAL examples and other sources,
//               cited inline where used.
// Uses        : CGAL 4.11.2 (https://www.cgal.org), C++11
//
// Copyright (c) 2018 Anthony D'Angelo
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program uses CGAL packages (Nef_2, Arrangement_on_surface_2,
// Triangulation_2, Convex_hull_2) licensed under the GPL v3 or later.
// See the LICENSE file for details.
//============================================================================
/****************************************************************************
** See README.md for a fuller discussion, including complexity analysis.
*****************************************************************************
** Description:
**   Implementation of the Georgakopoulos-Papadimitriou algorithm (GPA) for
**   the 1-Steiner tree problem:
**     Georgakopoulos and Papadimitriou, "The 1-Steiner Tree Problem,"
**     J. Algorithms 8(1):122-130, 1987.
**   Given n points in the plane, find one additional (Steiner) point that
**   minimizes the length of the minimum spanning tree (MST) of the points
**   plus the new point.
**
** Approach:
**   1. Compute the overlaid oriented Dirichlet cell (OODC) arrangement of the
**      input sites, clipped to their convex hull. This loses nothing: an
**      optimal Steiner point is a geometric median of its MST neighbours
**      (Georgakopoulos and Papadimitriou, Sec. 3, p. 127), and a geometric
**      median lies in the convex hull of its points (Morrison, "The FedEx
**      Problem," College Math. J. 41(3):222-232, 2010, Thm. 1; cf. Gilbert
**      and Pollak, "Steiner minimal trees," SIAM J. Appl. Math.
**      16(1):1-29, 1968, Sec. 3.5).
**   2. Each bounded face is labelled with at most 6 sites (one per 60-degree
**      cone direction), so it yields a constant number of candidates: the
**      geometric median of every 3- and 4-site subset of its label.
**   3. Evaluate each candidate by inserting it into the Delaunay
**      triangulation, computing the MST with Kruskal's algorithm, and
**      removing it again. A candidate with a shortest MST is a
**      1-Steiner point.
**
** Differences from the paper:
**   - GPA evaluates every proper subset (up to 5 sites) of each face's
**     label; this implementation uses only 3- and 4-site subsets, since
**     an optimal Steiner point has degree 3 or 4. Degree > 2 follows from
**     the triangle inequality, and degree 5 is ruled out by:
**       Rubinstein, Thomas, and Weng, "Degree-five Steiner points cannot
**       reduce network costs for planar sets," Networks 22(6):531-537, 1992.
**   - GPA uses O(n^2) preprocessing to evaluate each candidate in O(1)
**     time. This implementation recomputes the MST per candidate, which
**     costs O(n^2) per candidate (dominated by mapping MST endpoints back
**     to input indices with a linear scan), for O(n^4) overall.
**   - GPA builds the OODC partition in O(n^2) time. This implementation
**     builds each cell with CGAL Nef_2 Boolean operations and overlays
**     the cells one at a time, for O(n^3 log n) total time.
**
** Future work (see README.md for details and tradeoffs):
**   - OODC overlays (highest practical priority): even with the current
**     cell construction, overlaying each cell into a running arrangement
**     for its own cone direction (each stays O(n), since same-direction
**     cells only share boundaries) and then combining the six partitions
**     with 5 overlays reduces the overlay phase from O(n^3 log n) to
**     O(n^2 log n). Profiling (n = 6) showed the overlays take roughly 80%
**     of the running time, so this should give a large speed-up. The
**     Nef_2 cell construction (O(n^3 log n)) would then be the next
**     bottleneck; building each direction's partition directly with
**     Chang, Huang, and Tang's algorithm (below) would replace it.
**   - Candidate evaluation, from most to least improvement:
**     * Constant-time MST updates per candidate, using either GPA's O(n^2)
**       preprocessing or the O(n log n) preprocessing described in:
**         Bose, D'Angelo, and Durocher, "On the Restricted k-Steiner Tree
**         Problem," J. Comb. Optim. 44(4):2893-2918, 2022.
**       See also:
**         Bose, Maheshwari, Narasimhan, Smid, and Zeh, "Approximating
**         geometric bottleneck shortest paths," Comput. Geom.
**         29(3):233-249, 2004.
**         Monma and Suri, "Transitions in geometric minimum spanning
**         trees," Discrete Comput. Geom. 8:265-293, 1992.
**     * Without preprocessing: compute the MST of the sites once, then for
**       each candidate x run Kruskal's on the MST's n-1 edges plus edges
**       from x to its closest site in each of the six cones (at most 6),
**       which suffices by Brazil, Ras, Swanepoel, and Thomas, "Generalised
**       k-Steiner Tree Problems in Normed Planes," Algorithmica
**       71:66-86, 2015, Lemma 5 and Thm. 10 (the latter from Korte and
**       Vygen, "Combinatorial Optimization," Springer, 2008, pp. 120-121).
**       With the MST edges sorted once, this is nearly linear per
**       candidate, needs no Delaunay updates, and gives each edge's
**       endpoint indices directly.
**     * Keeping the Delaunay approach: store each vertex's index and kind
**       (input or Steiner) in the vertex
**       (CGAL::Triangulation_vertex_base_with_info_2) instead of searching
**       by coordinates. This is O(n log n) per candidate in practice, but
**       removing a vertex of degree d costs O(d^2) in the worst case, so
**       O(n^2) per candidate remains possible. Copying the triangulation
**       per candidate instead of removing guarantees O(n log n), but is
**       usually slower. Point location for insertions currently walks
**       from a fixed face (O(n) worst case); starting the walk at a face
**       next to one of the candidate's median sites usually makes it
**       short, and CGAL::Triangulation_hierarchy_2 would give expected
**       O(log n).
**   - JSON output: look up each arrangement vertex's index in O(1) (a hash
**     map keyed by vertex handle, or index data stored in the arrangement's
**     vertices) instead of a linear scan; this reduces writing the
**     arrangement from O(n^4) to O(n^2) without changing the output.
**   - OODC construction: build the six oriented Voronoi diagrams directly
**     (O(n log n) each), e.g. using:
**       Chang, Huang, and Tang, "An optimal algorithm for constructing
**       oriented Voronoi diagrams and geographic neighborhood graphs,"
**       Inf. Process. Lett. 35(5):255-260, 1990,
**     and combine them with 5 overlays instead of overlaying the 6n cells
**     one at a time. With CGAL's sweep-line overlay this would cost
**     O(n^2 log n). An O(n^2) method would make this phase O(n^2): GPA's
**     overlay (Theorem 1); Finke and Hinrichs, "Overlaying simply
**     connected planar subdivisions in linear time," Proc. 11th ACM SoCG,
**     119-126, 1995; or computing all intersections at once with
**     Chazelle and Edelsbrunner, "An optimal algorithm for intersecting
**     line segments in the plane," J. ACM 39(1):1-54, 1992.
**   - The JSON output lists every candidate's MST, so its size is
**     Theta(n^3) in the worst case. An O(n^2) implementation would need a
**     smaller output format (e.g., only the best tree), which would require
**     changes to the companion web page; a runtime flag could switch
**     between the full and compact output.
**===========================================================================*/

#include <getopt.h>

#include "1ST-Constants-Utilities.h"
#include "ComputationResult.hpp"

#include <ctime>
#include <chrono>  // for high_resolution_clock
#include <cstring> // for strerror

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>


using namespace One_St_Pt_GPA;

namespace
{
    using std::stoi;
    
    struct OneStPtResourceMutex {
        //FLOCK mutex
        int mutexFD = -1;
        OneStPtResourceMutex() = default;
        ~OneStPtResourceMutex() 
        { 
            if (mutexFD >= 0) 
            {
                close(mutexFD); 
            }
        }
        OneStPtResourceMutex(const OneStPtResourceMutex &) = delete;
        OneStPtResourceMutex & operator=(const OneStPtResourceMutex &) = delete;
        OneStPtResourceMutex(OneStPtResourceMutex &&) = delete;
        OneStPtResourceMutex & operator=(OneStPtResourceMutex &&) = delete;        
    };

    struct OneStPtConfig
    {
        vector< MyPoint_2 > pointsVec;
        int numPoints   = DEFAULT_NUM_INPUT_POINTS;
        int randSeed    = RAND_SEED_DEFAULT;
        int gridLength  = GRID_LENGTH_DEFAULT;
        OneStPtResourceMutex flockMutex;
        bool useMutex   = true;
        bool trueRandom = TRUE_RANDOM_DEFAULT;
        bool onlyPoints = ONLY_POINTS_DEFAULT;
    };

    void PrintHelp()
    {
        ostringstream msg;
        msg <<  "\nOneSteinerTreeProblem / OneSteinerTreeProblem_d : Help\n"
                "====================================================================================================================\n"
                "--onlyPoints (-p):         Only create new random points and return\n"
                "--numPoints  (-n) <int>:   Number of random points to generate (between "
                            << MIN_NUM_INPUT_POINTS << " and " << MAX_NUM_INPUT_POINTS << ", default is "
                            << DEFAULT_NUM_INPUT_POINTS << ")\n"
                "--trueRandom (-r):         Seed the rand. num. gen. with the current time rather than a fixed seed (default is "
                            << TRUE_RANDOM_DEFAULT << ")\n"
                "--randSeed   (-s) <int>:   Use the following fixed seed for the rand. num. gen. (default is "
                            << RAND_SEED_DEFAULT << ")\n"
                "--gridLength (-g) <int>:   Choose points from a square grid with this length (default is "
                            << GRID_LENGTH_DEFAULT << " x " << GRID_LENGTH_DEFAULT << ")\n"
                "--no-mutex:                Do not use a resource mutex (currently a file lock with a blocking lock call).\n"
                "                           This option is useful, e.g., if you want to manage the mutex in an invoking bash script.\n"
                "--inputList  (-i) <string s = \"[[x,y],[u,v],...]\">:   Use these points (comes out of numPoints' budget). Needs quotes around list.\n"
                "--help       (-h):         Show help\n"
                "\ne.g., -i \"[[1,0], [2,0], [3,0]]\"\n"
                "All arguments are optional.\n";

        cout << msg.str();
        return;
    }
    

    //https://codeyarns.com/2015/01/30/how-to-parse-program-options-in-c-using-getopt_long/
    bool ProcessArgs(int argc, char **argv, OneStPtConfig & config)
    {
        const char *const short_opts = "pn:rs:g:i:h";
        const option long_opts[] = {
            {"onlyPoints", no_argument, nullptr, 'p'},
            {"numPoints", required_argument, nullptr, 'n'},
            {"trueRandom", no_argument, nullptr, 'r'},
            {"randSeed", required_argument, nullptr, 's'},
            {"gridLength", required_argument, nullptr, 'g'},
            {"no-mutex", no_argument, nullptr, 256},    // "Any value that won't collide with the short option chars. 256 is a common choice since it's outside the ASCII range."
            {"inputList", required_argument, nullptr, 'i'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, no_argument, nullptr, 0}};
        string inputListString = "";

        //note: string(optarg) is to take in and make a string
        while (true)
        {
            const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

            if (-1 == opt)
            {
                break;
            }

            switch (opt)
            {
            case 'p':
                config.onlyPoints = true;
                break;

            case 'n':
                if (optarg)
                {
                    try
                    {
                        int temp = stoi(optarg);
                        if (Utility_Functions::intInRangeInclusive(temp, MIN_NUM_INPUT_POINTS, MAX_NUM_INPUT_POINTS))
                        {
                            config.numPoints = temp;
                        } else 
                        {
                            cerr << "ERROR: Invalid number of points. Using default of " << DEFAULT_NUM_INPUT_POINTS << endl;
                        }
                    }
                    catch (...)
                    {
                        config.numPoints = DEFAULT_NUM_INPUT_POINTS;
                        cerr << endl << "Exception caught" << endl;
                        cerr << "ERROR: Invalid number of points. Using default of " << DEFAULT_NUM_INPUT_POINTS << endl;
                    }
                }
                break;

            case 'r':
                config.trueRandom = true;
                break;

            case 's':
                if (optarg)
                {
                    try
                    {
                        //note: also stof exists
                        config.randSeed = stoi(optarg);
                    }
                    catch (...)
                    {
                        config.randSeed = RAND_SEED_DEFAULT;
                        cerr << endl << "Exception caught" << endl;
                        cerr << "ERROR: Invalid value for random seed. Using default of " << RAND_SEED_DEFAULT << endl;
                    }
                }
                break;

            case 'g':
                if (optarg)
                {
                    try
                    {
                        config.gridLength = stoi(optarg);
                    }
                    catch (...)
                    {
                        config.gridLength = GRID_LENGTH_DEFAULT;
                        cerr << endl << "Exception caught" << endl;
                        cerr << "ERROR: Invalid value for grid length. Using default of " << GRID_LENGTH_DEFAULT << endl;
                    }
                }
                break;

            case 256:
                // request to not to use a resource mutex in the program.
                // this is useful if you want to manage a mutex in a bash script instead
                config.useMutex = false;
                break;                

            case 'i':
                if(optarg)
                {
                    inputListString = string(optarg);
                    if ( !Utility_Functions::extractPointsFromJSON2DArrayString(inputListString, config.pointsVec))
                    {
                        // malformed input point list. bounce back to user.
                        // error message printed in function
                        // pointsVec is returned empty
                        return false;
                    }                                
                }
                break;

            case 'h': // -h or --help
            case '?': // Unrecognized option
            default:
                return false;
            }
        }
        return true;
    }

    void initializeOrDie(int argc, char **argv, OneStPtConfig & config)
    {
        if ( !ProcessArgs(argc, argv, config))
        {
            cout << "\nThere was an error starting up the program. Review the following and ensure the inputs are legal." << endl;
            PrintHelp();
            if( config.onlyPoints)
            {
                cout << "\nYour request is that we only generate and return up to " << config.numPoints << " random point(s) "
                        "from a square with side length " << config.gridLength <<
                        " using the following value to seed the random number generator: ";
                if(config.trueRandom)
                {
                    cout << "the current time\n";
                } else
                {
                    cout << config.randSeed << endl;
                }

                if( !config.pointsVec.empty())
                {
                    cout << "You have requested up to " << config.numPoints << " of the following points be part of the output:\n";
                    cout << "\"";
                    for(size_t ind = 0; ind < config.pointsVec.size(); ++ind)
                    {
                        MyPoint_2 p = config.pointsVec[ind];
                        cout << "( " << p.x() << ", " << p.y() << ") ";
                        if(ind < config.pointsVec.size() - 1)
                        {
                            cout << ", ";
                        }
                    }
                    cout << "\"" << endl;                 
                }
            } else
            {
                int ptsRemain = config.numPoints - config.pointsVec.size();

                cout << "\nYour request is to compute the 1-Steiner tree on ";

                if( !config.pointsVec.empty())
                {
                    cout << "the following points:\n";
                    cout << "\"";
                    for(size_t ind = 0; ind < config.pointsVec.size(); ++ind)
                    {
                        MyPoint_2 p = config.pointsVec[ind];
                        cout << "( " << p.x() << ", " << p.y() << ") ";
                        if(ind < config.pointsVec.size() - 1)
                        {
                            cout << ", ";
                        }
                    }
                    cout << "\"" << endl; 
                    
                    if( ptsRemain > 0 )
                    {
                        cout << "with an additional " << ptsRemain;
                    }               
                } else
                {
                    cout << config.numPoints;
                }

                if( ptsRemain > 0 )
                {
                    cout << " random points generated with a random number generator using the following seed: \n";
                    if(config.trueRandom)
                    {
                        cout << "the current time\n";
                    } else
                    {
                        cout << config.randSeed << endl;
                    }    
                    cout << "with the points chosen from a square of side length " << config.gridLength << endl; 
                }                                  
            }
            exit(1);
        }

        //http://www.cplusplus.com/reference/cstdlib/srand/
        if(config.trueRandom) {
            config.randSeed = time(nullptr);
        }

        if(config.useMutex)
        {
            config.flockMutex.mutexFD = open(FLOCK_PATH, O_CREAT | O_RDWR, 0600);
            if (config.flockMutex.mutexFD == -1) 
            {
                int err = errno;
                cerr << "Failed to open the lock file \'" << FLOCK_PATH << "\' for the one steiner tree problem (used as a resource mutex): "
                        << strerror(err) << " (" << err << ")\n";
                exit(1);
            }

            // N.B.: the non-blocking version returns -1 if in use -- flock(mutexFD, LOCK_EX | LOCK_NB) 
            flock(config.flockMutex.mutexFD, LOCK_EX);
            // Lock is released automatically:
            // when you close(fd), when the process exits (even SIGKILL, even segfault)
            // for us, the destructor of the struct is called when it goes out of scope and calls close
        }        

        return;
    }
} // anon namespace

int main(int argc, char **argv)
{
    OneStPtConfig config;

    initializeOrDie(argc, argv, config);

    //https://www.pluralsight.com/blog/software-development/how-to-measure-execution-time-intervals-in-c--
    // Record start time
    auto start = std::chrono::high_resolution_clock::now();
    ostringstream sStream;

    ComputationResult myCompResult(config.numPoints, config.randSeed, config.gridLength, config.onlyPoints, config.pointsVec);
    string compResult = myCompResult.outputResultToJSONString();
    sStream << "{\n\"" << COMP_RESULT_NAME_STRING << "\": ";
    sStream << compResult << "\n";

    sStream << ", \"" << ELAPSED_TIME_NAME_STRING << "\" :{";
    // Record end time
    auto finish = std::chrono::high_resolution_clock::now();  

    std::chrono::duration< double > elapsed = finish - start;
    double timeInSecs = elapsed.count();
    long timeNumb = static_cast< long >(floor(timeInSecs));

    sStream << "\"s\":\"" << (timeNumb % 60) << "\", \"m\":\"";
    timeNumb /= 60;
    sStream << (timeNumb % 60) << "\", \"h\":\""; 
    timeNumb /= 60;
    sStream << timeNumb << "\" }\n }" << endl;
    cout << sStream.str();

    return 0;
}
