#include "GeomMedianFinder.hpp"


GeomMedianFinder::GeomMedianFinder()
{
    return;
}


GeomMedianData GeomMedianFinder::computeGeomMedian3Pts(const vector<MyPoint_2>& myPts, const vector<size_t>& originalInputPtIndices) const
{
    assert(3 == myPts.size());

    GeomMedianData result(originalInputPtIndices);

    //assume no input points are the same
    if(collinear(myPts[0], myPts[1], myPts[2]))
    {
        //The middle is the geom median
        result.coincidesWithInputPt = true;
        if(collinear_are_strictly_ordered_along_line(myPts[0], myPts[1], myPts[2]))
        {
            result.medPoint = myPts[1];
            result.coincidentInputPtIndex = originalInputPtIndices[1];
        } else if(collinear_are_strictly_ordered_along_line(myPts[0], myPts[2], myPts[1]))
        {
            result.medPoint = myPts[2];
            result.coincidentInputPtIndex = originalInputPtIndices[2];            
        } else if(collinear_are_strictly_ordered_along_line(myPts[1], myPts[0], myPts[2]))
        {
            result.medPoint = myPts[0];
            result.coincidentInputPtIndex = originalInputPtIndices[0];
        }
    } else 
    {
        //convert to circle-stuff
        vector<CircularPoint_2> cPts;
        cPts.push_back(CircularPoint_2(myPts[0].x(), myPts[0].y()));
        cPts.push_back(CircularPoint_2(myPts[1].x(), myPts[1].y()));
        cPts.push_back(CircularPoint_2(myPts[2].x(), myPts[2].y()));
        
        My_Number_type firstDist = squared_distance(cPts[0], cPts[1]);
        My_Number_type secondDist = squared_distance(cPts[0], cPts[2]);
#if (DEBUG_CIRCLES)        
        My_Number_type firstDbgDist = squared_distance(myPts[0], myPts[1]);
        My_Number_type secondDbgDist = squared_distance(myPts[0], myPts[2]);
        assert (fabs(firstDist.exact() - firstDbgDist.exact()) < DOUBLE_EPSILON);
        assert (fabs(secondDist.exact() - secondDbgDist.exact()) < DOUBLE_EPSILON);
#endif
        MyCircle_2 firstCircle(cPts[0], firstDist);
        MyCircle_2 secondCircle(cPts[1], firstDist);

        vector<CircleIntersection_result>  intersectionResults;
        intersection(firstCircle, secondCircle, back_inserter(intersectionResults));
#if (DEBUG_CIRCLES)
        assert(intersectionResults.size() == 2);
#endif
        //There will be two point intersections
        CircleIntersection_result firstIntRes = intersectionResults[0];
        CircleIntersection_result secondIntRes = intersectionResults[1];
#if (DEBUG_CIRCLES)        
        assert(firstIntRes);
        assert(secondIntRes);        
#endif


    }

    return result;
}
