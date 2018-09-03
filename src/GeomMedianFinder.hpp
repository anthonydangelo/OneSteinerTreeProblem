#ifndef GMFINDER_HPP
#define GMFINDER_HPP

#include "1ST-Constants-Utilities.h"


//Interesting read about typedef'ing structs in c++, though apparently unnecessary in most situations:
//https://stackoverflow.com/questions/612328/difference-between-struct-and-typedef-struct-in-c/612476#612476
typedef struct GeomMedianData
{
    //compiler didn't like the const vector, wanted me to make some kind of copy op, but that's not doable with const casts... w/e.
    MyPoint_2 medPoint;
    vector<size_t> inputPtIndices;
    bool coincidesWithInputPt = false;
    size_t coincidentInputPtIndex = 0;
    GeomMedianData(const vector<size_t> &inputSet) : inputPtIndices(inputSet) {}
} GeomMedianData;

namespace GeomMedianFinder
{
    GeomMedianData computeGeomMedian(const vector< MyPoint_2 >& myPts, const vector<size_t>& originalInputPtIndices);
    GeomMedianData computeGeomMedian_3Pts(const vector< MyPoint_2 >& myPts, const vector<size_t>& originalInputPtIndices);
    GeomMedianData computeGeomMedian_4Pts(const vector< MyPoint_2 >& myPts, const vector<size_t>& originalInputPtIndices);
}

#endif