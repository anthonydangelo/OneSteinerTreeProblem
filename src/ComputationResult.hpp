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
  const GeomMedianData steinerPt;
  const MyEMSTData mstData;

  bool operator<(const CandidateSteinerPointData& other) const
  {
    return mstData < other.mstData; //compares lengths
  }

  //need ctor because GeomMedData needs one for its reference
  CandidateSteinerPointData(const GeomMedianData& myStPt, const MyEMSTData& myMstData) : steinerPt(myStPt), mstData(myMstData) {}

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
    Point_generator     randPointGen;
    vector<MyPoint_2>   convexHullList;

    vector<MyDirection_2> coneRays;
    vector< const MyPoint_2 > inputPtVector;
    MyArrangement_2       resultODCArrangement;

    MyEMSTData origMST;

    vector<CandidateSteinerPointData> steinerPoints;

  public:
    ComputationResult(int numInputPoints = DEFAULT_NUM_INPUT_POINTS,
                      int randomSeed = RAND_SEED_DEFAULT,
                      int inputGridLength = GRID_LENGTH_DEFAULT,
                      bool onlyProducePoints = ONLY_POINTS_DEFAULT,
                      string outFilePrefix = "",
                      const vector< const MyPoint_2 >& userPointList = vector<const MyPoint_2>());
    
    string outputResultToJSONString() const;

  protected:
//    template <typename Container>
//    string outputCollectionToJSONFile(string name, const Container &myColl) const;
    void preparePointSet(const vector< const MyPoint_2 >& userPointList);
    void computeOODC();
    void computeMSTAndStPts();
    string pointSetToJSONString(string name, const set<MyPoint_2> &myColl, int tabLevel=0) const;
    string pointVectorToJSONString(string name, const vector< const MyPoint_2 >& myColl, int tabLevel=0) const;
    string vertexIndicesToJSONString(string name, const vector<MyPoint_2> &myColl,
                                     const vector<const MyPoint_2> &myPtSet, int tabLevel = 0) const;
    void insertArrangementPointsIntoPointSet(vector< const MyPoint_2 >& arrPointsVec) const;
    string arrangementFaceToJSONString(string faceName, const MyArrangement_2::Face_const_iterator fit, 
                                        const vector<const MyPoint_2>& myPtSet, int tabLevel) const;
    string arrangementToJSONString(int tabLevel=0) const;   
    string mstDataToJSONString(const MyEMSTData& mst, int tabLevel=0) const;
    string mstEdgeToJSONString(const pair< const pair<size_t, size_t>, const pair<bool, bool> >& edgeData, int tabLevel=0) const;
    string geomMedDataToJSONString(const GeomMedianData& stPtData, int tabLevel=0) const;
    string candidateSteinerPtDataToJSONString(const CandidateSteinerPointData& stPtData, int tabLevel=0) const;
    string steinerPointsToJSONString(int tabLevel=0) const;
    void computePotentialStPts(const vector<size_t>& siteIndices, const int numComboPts, 
                                vector<const GeomMedianData>& results, set< vector<size_t> >& seenList) const; 
    vector<const GeomMedianData> computeStPtsForOODC() const;
};

#endif