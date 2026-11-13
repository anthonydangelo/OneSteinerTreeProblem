#ifndef GMFINDER_HPP
#define GMFINDER_HPP

/*
** Geometric median (Fermat–Weber point) for 3 or 4 points.
**
** Do not call this unless your input is exactly 3 or 4 points.
**
** For 3 collinear points, it's the middle point.
** For 3 non-collinear points: if all angles < 120 degrees, the median is the
** intersection of two "Simpson lines" (lines from each vertex through the
** apex of an equilateral triangle on the opposite side). If any angle >= 120 degrees,
** the median is that vertex (degenerate case).
**
** For 4 points: if all collinear, the median is any point on the middle 
** segment defined by the 4 points. 
** If not collinear, if one point is inside the triangle of the other three, 
** it is the median.
** Otherwise, the median is the intersection of the diagonals of the
** convex quadrilateral.
*/
//See OneSteinerTreeProblem.cpp for project notes

#include "1ST-Constants-Utilities.h"

namespace One_St_Pt_GPA
{
    //Interesting read about typedef'ing structs in c++, though apparently unnecessary in most situations:
    //https://stackoverflow.com/questions/612328/difference-between-struct-and-typedef-struct-in-c/612476#612476
    struct GeomMedianData
    {
        MyPoint_2 medPoint;
        vector<size_t> inputPtIndices;
        bool coincidesWithInputPt = false;
        size_t coincidentInputPtIndex = 0;
        GeomMedianData(const vector< size_t > & inputSet) : inputPtIndices(inputSet) {}
    };

    namespace GeomMedianFinder
    {
        GeomMedianData computeGeomMedian(const vector< MyPoint_2 > & myPts, const vector< size_t > & originalInputPtIndices);
        GeomMedianData computeGeomMedian_3Pts(const vector< MyPoint_2 > & myPts, const vector< size_t > & originalInputPtIndices);
        GeomMedianData computeGeomMedian_4Pts(const vector< MyPoint_2 > & myPts, const vector< size_t > & originalInputPtIndices);
    }
} // namespace One_St_Pt_GPA
#endif