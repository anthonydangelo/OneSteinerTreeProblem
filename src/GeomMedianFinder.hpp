#ifndef GMFINDER_HPP
#define GMFINDER_HPP

#include "1ST-Constants-Utilities.h"


typedef struct GeomMedianData{
    MyPoint_2 medPoint;
    const vector<size_t>& inputPtIndices;
    bool coincidesWithInputPt = false;
    size_t coincidentInputPtIndex = 0;
    GeomMedianData(const vector<size_t>& inputSet) :inputPtIndices(inputSet){}
} GeomMedianData;

class GeomMedianFinder
{
  protected:
    enum class DegeneratePointIndex {FIRST, SECOND, THIRD};

  public:
    GeomMedianFinder();
    GeomMedianData computeGeomMedian_3Pts(const vector<MyPoint_2>& myPts, const vector<size_t>& originalInputPtIndices) const;
    GeomMedianData computeGeomMedian_4Pts(const vector<MyPoint_2>& myPts, const vector<size_t>& originalInputPtIndices) const;

  protected:
    MyPoint_2 findEqTriIntPoint(const MyPoint_2 &pointA, const MyPoint_2 &pointB, const MyPoint_2 &avoidMe) const;
    MyPoint_2 findEqTriIntPoint(const MyPoint_2 &pointA, const MyPoint_2 &pointB, const MyPoint_2 &avoidMe,
                                const MyLine_2 &baseLine, const vector<MyDirection_2> &coneRays) const;
    bool testForDegenerateGeomMedian3Pts(const MyPoint_2 &pointA, const MyPoint_2 &pointB, const MyPoint_2 &pointC,
                                         MyLine_2 &baseLineToFill, vector<MyDirection_2> &coneRaysToFill,
                                         DegeneratePointIndex &degenPt) const;
    void findCollinearMedian_3Pts(const MyPoint_2 &pointA, const MyPoint_2 &pointB, const MyPoint_2 &pointC,
                                  const vector<size_t>& originalInputPtIndices, GeomMedianData &fillMe) const;
};

#endif