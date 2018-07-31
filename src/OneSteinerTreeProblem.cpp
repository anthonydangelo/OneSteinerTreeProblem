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
    msg << "--onlyPoints (-p):        Only create new random points and return\n"
           "--numPoints (-n) <int>:   Number of random points to generate (between "
                        << MIN_NUM_INPUT_POINTS << " and " << MAX_NUM_INPUT_POINTS << ", default is "
                        << DEFAULT_NUM_INPUT_POINTS << ")\n"
           "--trueRandom (-r):        Seed the rand. num. gen. with the current time rather than a fixed seed (default is "
                        << TRUE_RANDOM_DEFAULT << ")\n"
           "--randSeed (-s) <int>:    Use the following fixed seed for the rand. num. gen. (default is "
                        << RAND_SEED_DEFAULT << ")\n"
           "--gridLength (-g) <int>:    Choose points from a square grid with this length (default is "
                        << GRID_LENGTH_DEFAULT << " x " << GRID_LENGTH_DEFAULT << ")\n"
           "--help (-h):              Show help\n";

    cout << msg.str();
    exit(1);
}

int numPoints = DEFAULT_NUM_INPUT_POINTS;
int randSeed = RAND_SEED_DEFAULT;
int gridLength = GRID_LENGTH_DEFAULT;
bool trueRandom = TRUE_RANDOM_DEFAULT;
bool onlyPoints = ONLY_POINTS_DEFAULT;

//https://codeyarns.com/2015/01/30/how-to-parse-program-options-in-c-using-getopt_long/
void ProcessArgs(int argc, char **argv)
{
    const char *const short_opts = "pn:rs:g:h";
    const option long_opts[] = {
        {"onlyPoints", no_argument, nullptr, 'p'},
        {"numPoints", required_argument, nullptr, 'n'},
        {"trueRandom", no_argument, nullptr, 'r'},
        {"randSeed", required_argument, nullptr, 's'},
        {"gridLength", required_argument, nullptr, 'g'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, no_argument, nullptr, 0}};

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
            if (optarg)
            {
                try
                {
                    int temp = stoi(optarg);
                    if (intInRangeInclusive(temp, MIN_NUM_INPUT_POINTS, MAX_NUM_INPUT_POINTS))
                    {
                        numPoints = temp;
                    }
                }
                catch (...)
                {
                    numPoints = DEFAULT_NUM_INPUT_POINTS;
                    cerr << "Invalid number of points. Using default of " << DEFAULT_NUM_INPUT_POINTS << endl;
                }
            }
            break;

        case 'r':
            trueRandom = true;
            break;

        case 's':
            if (optarg)
            {
                try
                {
                    //note: also stof exists
                    randSeed = stoi(optarg);
                }
                catch (...)
                {
                    randSeed = RAND_SEED_DEFAULT;
                    cerr << "Invalid value for random seed. Using default of " << RAND_SEED_DEFAULT << endl;
                }
            }
            break;

        case 'g':
            if (optarg)
            {
                try
                {
                    gridLength = stoi(optarg);
                }
                catch (...)
                {
                    gridLength = GRID_LENGTH_DEFAULT;
                    cerr << "Invalid value for grid length. Using default of " << GRID_LENGTH_DEFAULT << endl;
                }
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

int main(int argc, char **argv)
{

    ProcessArgs(argc, argv);

    cout << "!!!Hello World!!!\n"
         << trueRandom << onlyPoints << numPoints << randSeed << gridLength << endl; // prints !!!Hello World!!!
    return 0;
}
