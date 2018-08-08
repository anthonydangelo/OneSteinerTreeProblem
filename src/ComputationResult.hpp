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
    bool onlyPoints;
    string outFilePrefix;

    Random rand;
    set<MyPoint_2> pointSet;
    Point_generator randPointGen;
    vector<MyPoint_2> convexHullList;

  public:
    ComputationResult(int numInputPoints = DEFAULT_NUM_INPUT_POINTS,
                      int randomSeed = RAND_SEED_DEFAULT,
                      int inputGridLength = GRID_LENGTH_DEFAULT,
                      bool onlyProducePoints = ONLY_POINTS_DEFAULT,
                      string outFilePrefix = "");
    
    string outputResultToJSONString() const;

  protected:
    void computeConvexHull(const set<MyPoint_2> &pointSet, vector<MyPoint_2> &result);
//    template <typename Container>
//    string outputCollectionToJSONFile(string name, const Container &myColl) const;
    string outputCollectionToJSONString(string name, const set<MyPoint_2> &myColl, int tabLevel=0) const;    
};

#endif