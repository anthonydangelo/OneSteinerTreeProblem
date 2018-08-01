#ifndef COMPUTATION_RESULT_HPP
#define COMPUTATION_RESULT_HPP

#include "1ST-Constants-Utilities.h"

class ComputationResult
{

  protected:
    //https://stackoverflow.com/questions/11594846/default-member-values-best-practice
    int numPoints;
    int randSeed;
    int gridLength;
    bool trueRandom;
    bool onlyPoints;

    Random rand;
    set<MyPoint_2> pointSet;
    Point_generator randPointGen;
    vector<MyPoint_2> convexHullList;

  public:
    ComputationResult(int numInputPoints = DEFAULT_NUM_INPUT_POINTS,
                      int randomSeed = RAND_SEED_DEFAULT,
                      int inputGridLength = GRID_LENGTH_DEFAULT,
                      bool trueRandomSeed = TRUE_RANDOM_DEFAULT,
                      bool onlyProducePoints = ONLY_POINTS_DEFAULT);
    
    void outputResultToJSONFile();
    template <typename Container>
    string outputCollectionToJSONFile(string name, const Container &myColl);

  protected:
    void computeConvexHull(const set<MyPoint_2> &pointSet, vector<MyPoint_2> &result);
};

#endif