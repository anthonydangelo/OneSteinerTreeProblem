//============================================================================
// Name        : OneSteinerTreeProblem.cpp
// Author      : Anthony D'Angelo
// Version     :
// Copyright   : Your copyright notice
// Description : Implementation of Georgakopolous and Papadimitriou's O(n^2) 1 Steiner tree algorithm
//============================================================================

#include <getopt.h>
#include <iostream>
#include <sstream> // for ostringstream
#include <string>

#include <iostream>
#include <unistd.h>
#include <cstdlib>

#include "1ST-Constants-Utilities.h"

using namespace std;

void PrintHelp()
{
	ostringstream msg;
	msg <<
			"--onlyPoints:        Only create new random points and return\n"
            "--numPoints <int>:   Number of random points to generate (between "
					<< MIN_NUM_INPUT_POINTS << " and " << MAX_NUM_INPUT_POINTS << ", default is "
					<< MIN_NUM_INPUT_POINTS << ")\n"
            "--trueRandom:        Seed the rand. num. gen. with the current time rather than a fixed seed (default is "
					<< TRUE_RANDOM_DEFAULT << ")\n"
            "--randSeed <int>:    Use the following fixed seed for the rand. num. gen. (default is " << RAND_SEED << ")\n"
            "--help:              Show help\n";

	cout << msg.str();
    exit(1);
}

int numPoints = MIN_NUM_INPUT_POINTS;
int randSeed = RAND_SEED;
bool trueRandom = TRUE_RANDOM_DEFAULT;
bool onlyPoints = ONLY_POINTS_DEFAULT;

//https://codeyarns.com/2015/01/30/how-to-parse-program-options-in-c-using-getopt_long/
void ProcessArgs(int argc, char** argv)
{
    const char* const short_opts = "pn:rs:h";
    const option long_opts[] = {
            {"onlyPoints", no_argument, nullptr, 'p'},
            {"numPoints", required_argument, nullptr, 'n'},
            {"trueRandom", no_argument, nullptr, 'r'},
            {"randSeed", required_argument, nullptr, 's'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, no_argument, nullptr, 0}
    };

    //note: string(optarg) is to take in and make a string
    while (true)
    {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

        if (-1 == opt)
            break;

        switch (opt)
        {
        case 'p':
        	onlyPoints = true;
        	break;

        case 'n':
        	if(optarg){
        		numPoints = stoi(optarg);
        	}
            break;

        case 'r':
        	trueRandom = true;
            break;

        case 's':
        	if(optarg){
        		//note: also stof exists
        		randSeed = stoi(optarg);
        	}
            break;

        case 'h': // -h or --help
        case '?': // Unrecognized option
        default:
            PrintHelp();
            break;
        }
    }
    return;
}

int main(int argc, char** argv) {

	 ProcessArgs(argc, argv);

	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	return 0;

}
