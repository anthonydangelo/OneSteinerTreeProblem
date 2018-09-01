#ifndef COMPUTATION_RESULT_HPP
#define COMPUTATION_RESULT_HPP

#include "1ST-Constants-Utilities.h"

#include <CGAL/point_generators_2.h>
#include <CGAL/Random.h>
#include "CGAL/Combination_enumerator.h"

#include "OrientedDirichletCell.hpp"
#include "DelaunayTriEMST.hpp"
#include "GeomMedianFinder.hpp"

//https://doc.cgal.org/latest/Generator/index.html
typedef Creator_uniform_2<double, MyPoint_2> Creator;
typedef Random_points_in_square_2<MyPoint_2, Creator> Point_generator;

typedef struct CandidateSteinerPointData
{
  GeomMedianData steinerPt;
  MyEMSTData mstData;

  bool operator<(const CandidateSteinerPointData &other)
  {
    return mstData < other.mstData; //compares lengths
  }

} CandidateSteinerPointData;

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
    set<MyPoint_2>      pointSet;
    Point_generator     randPointGen;
    vector<MyPoint_2>   convexHullList;

    vector<MyDirection_2> coneRays;
    //MyPolygon_set_2       convexHull;
    vector< reference_wrapper<const MyPoint_2> > inputPtVector;
    MyArrangement_2       resultODCArrangement;

    DelaunayTriEMST       myEMST;

  public:
    ComputationResult(int numInputPoints = DEFAULT_NUM_INPUT_POINTS,
                      int randomSeed = RAND_SEED_DEFAULT,
                      int inputGridLength = GRID_LENGTH_DEFAULT,
                      bool onlyProducePoints = ONLY_POINTS_DEFAULT,
                      string outFilePrefix = "",
                      vector<MyPoint_2> userPointList = vector<MyPoint_2>());
    
    string outputResultToJSONString() const;

  protected:
//    template <typename Container>
//    string outputCollectionToJSONFile(string name, const Container &myColl) const;
    string pointSetToJSONString(string name, const set<MyPoint_2> &myColl, int tabLevel=0) const;
    string pointVectorToJSONString(string name, const vector<MyPoint_2> &myColl, int tabLevel=0) const;
    string vertexIndicesToJSONString(string name, const vector<MyPoint_2> &myColl, const set<MyPoint_2> &myPtSet, int tabLevel=0) const;
    void insertArrangementPointsIntoPointSet(set<MyPoint_2> &arrPoints) const;
    string arrangementFaceToJSONString(string faceName, const MyArrangement_2::Face_const_iterator fit, 
                                        const set<MyPoint_2> &myPtSet, int tabLevel) const;
    string arrangementToJSONString(int tabLevel=0) const;    
    vector<GeomMedianData> computeStPtsForOODC() const;
};

#endif