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
// Description : Implementation of (most of) Georgakopolous and Papadimitriou's 
//               O(n^2) 1 Steiner tree algorithm from Journal of Algorithms, 1987.
//               Use CGAL for everything, e.g., OODC arrangement creation.
//               We don't do their n^2 pre-processing to figure out MST length changes.
//               Instead, we compute the new MSTs with the test Steiner points
//               by inserting them into a Delaunay tri., taking the MST, then removing
//               the candidate St point so we can re-use the tri. This might make this
//               implementation (n^2)log(n) (depends on how CGAL does this)...
//               Future work: implement the monma-suri approach to help find the MST deltas.
//                      (see Transitions in geometric minimum spanning trees by 
//                          Monma and Suri, 1992, Journal of Discrete and Computational Geometry,
//                          Lemma 4.1 on page "277" for the idea)
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
//              If any papers come out of this, you'll probably have to cite the individual chapters of the CGAL manual too.
//============================================================================

#include <getopt.h>

#include "1ST-Constants-Utilities.h"
#include "ComputationResult.hpp"

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
    
    struct OneStPtConfig
    {
        int numPoints = DEFAULT_NUM_INPUT_POINTS;
        int randSeed = RAND_SEED_DEFAULT;
        int gridLength = GRID_LENGTH_DEFAULT;
        bool trueRandom = TRUE_RANDOM_DEFAULT;
        bool onlyPoints = ONLY_POINTS_DEFAULT;
        bool useMutex = true;
        vector<MyPoint_2> pointsVec;
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
    bool ProcessArgs(int argc, char **argv, OneStPtConfig &config)
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

    void initializeOrDie(int argc, char **argv, OneStPtConfig &config)
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

        return;
    }
} // anon namespace

int main(int argc, char **argv)
{

    int mutexFD = -1;
    OneStPtConfig config;

    initializeOrDie(argc, argv, config);

    if(config.useMutex)
    {
        mutexFD = open(FLOCK_PATH, O_CREAT | O_RDWR, 0600);
        if (mutexFD == -1) 
        {
            int err = errno;
            cerr << "Failed to open the lock file \'" << FLOCK_PATH << "\' for the one steiner tree problem (used as a resource mutex): "
                    << strerror(err) << " (" << err << ")\n";
            return 1;
        }

        // N.B.: the non-blocking version returns -1 if in use -- flock(mutexFD, LOCK_EX | LOCK_NB) 
        flock(mutexFD, LOCK_EX);
        // Lock is released automatically:
        // when you close(fd), when the process exits (even SIGKILL, even segfault)
    }


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

    if(config.useMutex)
    {
        // Note: No unlock needed — flock is released automatically when fd is closed (end of scope) or the process dies.        
        close(mutexFD);
    }    

    std::chrono::duration<double> elapsed = finish - start;
    double timeInSecs = elapsed.count();
    long timeNumb = static_cast<long>(floor(timeInSecs));

    sStream << "\"s\":\"" << (timeNumb % 60) << "\", \"m\":\"";
    timeNumb /= 60;
    sStream << (timeNumb % 60) << "\", \"h\":\""; 
    timeNumb /= 60;
    sStream << timeNumb << "\" }\n }" << endl;
    cout << sStream.str();

    return 0;
}
