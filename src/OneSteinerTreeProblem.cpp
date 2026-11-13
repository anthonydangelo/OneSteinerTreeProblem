//============================================================================
// Name        : OneSteinerTreeProblem.cpp
// Author      : Anthony D'Angelo and the CGAL demos I re-worked. I tried to cite them in the code.
//               Also, random internet examples (also tried to cite those).
//              " CGAL, Computational Geometry Algorithms Library, https://www.cgal.org"
/*              https://doc.cgal.org/latest/Manual/how_to_cite_cgal.html
                Note: I used CGAL 4.11.2, but the manual for 4.12.1. They're pretty close.
                @book{ cgal:eb-18b
                , title        = "{CGAL} User and Reference Manual"
                , author      = "{The CGAL Project}"
                , publisher     = "{CGAL Editorial Board}"
                , edition     = "{4.12.1}"
                , year         = 2018
                , url =    "https://doc.cgal.org/4.12.1/Manual/packages.html"
                }
*/
// Version     :
/* 
******************************************************
** Read project's readme.md for a fuller elaboration.
******************************************************
** Description : Implementation of the Georgakopoulos–Papadimitriou 1-Steiner tree
**               algorithm (GPA):  (Georgakopoulos, Papadimitriou) 
**                                 The 1-Steiner tree algorithm (J. Algorithms 8(1): 122-130 (1987)). 
**
** Approach: Compute the Overlaid Oriented Dirichlet Cell (OODC) arrangement of the
**          input sites, then for each bounded face, enumerate all 3- and 4-point
**          combinations of the face's site indices, compute the geometric median
**          of each combination, and test its insertion into the EMST (via Delaunay
**          triangulation). The candidate with the shortest resulting EMST is a
**          1-Steiner point.
**
**          We do not implement the paper's O(n^2) MST-delta pre-processing to enable
**          constant-time MST-update queries. 
**          Instead, we compute each candidate's MST by inserting it into the Delaunay
**          triangulation, re-running Kruskal's on the updated triangulaion, 
**          and removing the point (in preparation for the next iteration). 
**          This was simpler to implement but makes each of the O(n^2) tests
**          for a candidate Steiner point take O(n log n) time 
**          (depending on how CGAL manages updating a Delaunay triangulation
**           and if their Kruskal's implementation is O(n log n) ).
**
**          I used c++11 and CGAL 4.11.2 (and its dependencies) heavily as a geometry library.
**
** Future work: Rather than implementing the preprocessing from the GPA paper that lets them
**              find MST deltas in constant time to test each candidate Steiner point,
**              implement the improvement described in the paper I co-authored (first described in other references):
**                  (Bose, P., D'Angelo, A., & Durocher, S.) 
**                      "On the Restricted k-Steiner Tree Problem.", 
**                      ( J. Comb. Optim. 44(4): 2893-2918 (2022))
**              See also:
**                  (Bose, Maheshwari, Narasimhan, Smid, and Zeh) 
**                      "Approximating geometric bottle-neck shortest paths.",
**                      (Comput. Geom., 29(3):233–249, 2004.)
**                  (Monma and Suri) 
**                      "Transitions in geometric minimum spanning trees.",
**                      (Discret. Comput. Geom., 8:265–293, 1992.)
**              That will let us find the solution without computing the MST every time. 
**              Then return the MST at the end with the solution.
**              If you for some reason want to continue inserting/removing a point at a time
**              from a Delaunay triangulation, consider using an algorithm for a dynamic data structure
**              (which offer some tradeoff to allow quicker insertions/deletions).
**
**              Also, with regards to testing Steiner points: whenever we compute a MST
**              (including when testing Steiner points), we then run a O(n^2) scan to 
**              map each endpoint of each edge from the MST (which has been extracted from the Delaunay triangulation) 
**              to an input point (testing for points being "close enough" because, for all I know, they may have 
**              been slightly changed by the Delaunay insertion). This is not efficient, but was fast to code. 
**              It should eventually be replaced by either using augmented vertices/edges/faces that keep track
**              of the necessary data (which may require writing a custom MST algorithm, but I wouldn't be surprised
**              if CGAL can use such augmented structures), or using a map with a custom "compare" function
**              that returns the index for the point that's "close enough".
**                    
**              Other future work would be to replace the OODC code with an O(n log n) 
**              algorithm for computing the oriented Voronoi / Dirichlet diagram.
**              For example:
**                  (Chang, Huang, and Tang)
**                      "An optimal algorithm for constructing oriented Voronoi diagrams 
**                       and geographic neighborhood graphs.", 
**                      (Inf. Process. Lett. 35(5):255–260, 1990)
*/
// Copyright   : This uses CGAL and the dependencies of CGAL, which have their own licences 
//               and copyrights and stuff that probably apply virally to this too.
//               From some random CGAL demos (they're probably all the same):
//                  Copyright (c) 2005-2009  INRIA Sophia-Antipolis (France).
//                  All rights reserved.
//
//                  This file is part of CGAL (www.cgal.org); you can redistribute it and/or
//                  modify it under the terms of the GNU Lesser General Public License as
//                  published by the Free Software Foundation; either version 3 of the License,
//                  or (at your option) any later version.
//
//                  Licensees holding a valid commercial license may use this file in
//                  accordance with the commercial license agreement provided with the software.
//
//                  This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
//                  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
//              This stuff goes for me too:
//              These files are provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
//              WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//              These files are provided as a kind of example, I guess... If you use stuff from it,
//              please give the others (CGAL, authors of the papers...) and I credit.
//              I hope I've done that properly myself...
//============================================================================

#include <getopt.h>

#include "1ST-Constants-Utilities.h"
#include "ComputationResult.hpp"

#include <ctime>
#include <chrono>  // for high_resolution_clock

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
