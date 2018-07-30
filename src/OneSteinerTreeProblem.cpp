//============================================================================
// Name        : OneSteinerTreeProblem.cpp
// Author      : Anthony D'Angelo
// Version     :
// Copyright   : Your copyright notice
// Description : Implementation of Georgakopolous and Papadimitriou's O(n^2) 1 Steiner tree algorithm
//============================================================================

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/optional/optional.hpp>
#include <iostream>
#include <sstream> // for ostringstream
#include <string>

#include "1ST-Constants-Utilities.h"

using namespace std;
namespace po = boost::program_options;

int main(int argc, char** argv) {

	int numPoints = MIN_NUM_INPUT_POINTS;
	int randSeed = RAND_SEED;
	bool trueRandom = TRUE_RANDOM_DEFAULT;
	boost::optional<int> nPts;
	boost::optional<int> rSeed;
	boost::optional<bool> trueRndm;

	ostringstream numPointsMsg, randSeedMsg, trueRandomMsg;
	numPointsMsg << "number of random points to use (from " << MIN_NUM_INPUT_POINTS << " to " << MAX_NUM_INPUT_POINTS << ")";
	randSeedMsg << "random num. gen. seed to use (default is " << RAND_SEED << ")";
	trueRandomMsg << "false if we should use a fixed seed for the random num. gen. (default is " << TRUE_RANDOM_DEFAULT << ")";

	po::options_description desc("Options");

	desc.add_options()
				("help, h", "produce help message")
				("trueRandom, r", po::value<bool>(&trueRndm)->default_value(TRUE_RANDOM_DEFAULT), trueRandomMsg.str())
	            ("numPoints, n", po::value<int>(&nPts)->default_value(MIN_NUM_INPUT_POINTS), numPointsMsg.str())
	            ("randSeed, s", po::value<int>(&rSeed)->default_value(MIN_NUM_INPUT_POINTS), randSeedMsg.str())
	            ("filename", po::value<std::string>()->default_value(""));


	if(nPts){
		numPoints = *nPts;
	}
	if(rSeed){
		randSeed = *rSeed;
	}
	if(trueRndm){
		trueRandom = *trueRndm;
	}


    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv,desc),vm);
    po::notify(vm);

	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	return 0;

}
