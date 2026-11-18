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

#include "GeomMedianFinder.hpp"

//See OneSteinerTreeProblem.cpp for project notes

using namespace One_St_Pt_GPA;

//No one needs to see these file-level scoped functions and enums
namespace
{
    enum class DegeneratePointIndex {FIRST, SECOND, THIRD};

    MyPoint_2 findEqTriIntPoint(const MyPoint_2 & pointA, const MyPoint_2 & pointB, const MyPoint_2 & avoidMe);

    bool testForDegenerateGeomMedian3Pts(const MyPoint_2 & pointA, const MyPoint_2 & pointB, const MyPoint_2 & pointC,
                                     DegeneratePointIndex & degenPt);
    void findCollinearMedian_3Pts(const MyPoint_2 & pointA, const MyPoint_2 & pointB, const MyPoint_2 & pointC,
                              const vector< size_t > & originalInputPtIndices, GeomMedianData & fillMe); 
    // The unique point where a and b meet (see the definition below).
    template< class Obj1, class Obj2 >
    MyPoint_2 uniqueIntersectionPoint(const Obj1 & a, const Obj2 & b);

    bool angleAtLeast120(const MyPoint_2 & u, const MyPoint_2 & v, const MyPoint_2 & w);

} // anon namespace



namespace One_St_Pt_GPA
{
    GeomMedianData GeomMedianFinder::computeGeomMedian(const vector< MyPoint_2 > & myPts,
                                                    const vector< size_t > & originalInputPtIndices)
    {
        assert((myPts.size() == 3) || (myPts.size() == 4));
        if (myPts.size() == 3)
        {
            return GeomMedianFinder::computeGeomMedian_3Pts(myPts, originalInputPtIndices);
        }
        else
        {
            return GeomMedianFinder::computeGeomMedian_4Pts(myPts, originalInputPtIndices);
        }
    }

    GeomMedianData GeomMedianFinder::computeGeomMedian_3Pts(const vector< MyPoint_2 > & myPts, const vector< size_t > & originalInputPtIndices)
    {
        assert(3 == myPts.size());

        GeomMedianData result(originalInputPtIndices);
        
        //assume no input points are the same
        if (collinear(myPts[0], myPts[1], myPts[2]))
        {
            //The middle is the geom median
            findCollinearMedian_3Pts(myPts[0], myPts[1], myPts[2], originalInputPtIndices, result);
        }
        else
        {
            DegeneratePointIndex degenPt;
            bool detectedDegen = false;
            // Test for an angle of 120 degrees or more (then the median is that vertex).
            // The test is exact (see angleAtLeast120), but the Fermat-point construction
            // below uses 60-degree rotations that are only accurate to double precision
            // (see computeConeRays). So the pointsAreTooClose check after this block
            // treats a median within the (absolute) tolerance of a vertex as that vertex:
            // that keeps DelaunayTriEMST::findEdgePointIndices, 
            // which matches points by coordinates within the same tolerance, 
            // from mistaking the candidate Steiner point for an input point.    
            if (testForDegenerateGeomMedian3Pts(myPts[0], myPts[1], myPts[2], degenPt))
            {
                detectedDegen = true;
                result.coincidesWithInputPt = true;
                switch (degenPt)
                {
                case DegeneratePointIndex::FIRST:
                    result.medPoint = myPts[0];
                    result.coincidentInputPtIndex = originalInputPtIndices[0];
                    break;
                case DegeneratePointIndex::SECOND:
                    result.medPoint = myPts[1];
                    result.coincidentInputPtIndex = originalInputPtIndices[1];
                    break;
                case DegeneratePointIndex::THIRD:
                    result.medPoint = myPts[2];
                    result.coincidentInputPtIndex = originalInputPtIndices[2];
                    break;
                }
            }
            else
            {
                //If no angle of the triangle is >= 120 degrees, we can find the intersection of two "simpson lines"
                const MyPoint_2 firstIntPoint = findEqTriIntPoint(myPts[0], myPts[1], myPts[2]);
                const MySegment_2 firstSimpsonLine = MySegment_2(firstIntPoint, myPts[2]);
                const MyPoint_2 secondIntPoint = findEqTriIntPoint(myPts[0], myPts[2], myPts[1]);
                const MySegment_2 secondSimpsonLine = MySegment_2(secondIntPoint, myPts[1]);
                //https://doc.cgal.org/latest/Kernel_23/group__intersection__linear__grp.html && Kernel_23/intersection_get.cpp
                // All angles are < 120 degrees, so the Simpson lines meet at the Fermat point.
                result.medPoint = uniqueIntersectionPoint(firstSimpsonLine, secondSimpsonLine);                
            }
            //check if it's too close to an input pt
            if (!detectedDegen)
            {
                if (Utility_Functions::pointsAreTooClose(result.medPoint, myPts[0]))
                {
                    result.coincidesWithInputPt = true;
                    result.medPoint = myPts[0];
                    result.coincidentInputPtIndex = originalInputPtIndices[0];
                }
                else if (Utility_Functions::pointsAreTooClose(result.medPoint, myPts[1]))
                {
                    result.coincidesWithInputPt = true;
                    result.medPoint = myPts[1];
                    result.coincidentInputPtIndex = originalInputPtIndices[1];
                }
                else if (Utility_Functions::pointsAreTooClose(result.medPoint, myPts[2]))
                {
                    result.coincidesWithInputPt = true;
                    result.medPoint = myPts[2];
                    result.coincidentInputPtIndex = originalInputPtIndices[2];
                }
            }
        }

#   if (MY_VERBOSE)
        cout << "geom med 3 pts: " << result.medPoint << " coincides? " << result.coincidesWithInputPt << " index: " << result.coincidentInputPtIndex << endl;
#   endif

        return result;
    }


    GeomMedianData GeomMedianFinder::computeGeomMedian_4Pts(const vector< MyPoint_2 > & myPts, const vector< size_t > & originalInputPtIndices)
    {
        assert(4 == myPts.size());

        GeomMedianData result(originalInputPtIndices);    


        //assume no input points are the same
        if (collinear(myPts[0], myPts[1], myPts[2]) && collinear(myPts[3], myPts[1], myPts[2]))
        {
            //The middle (or anywhere on the middle segment) is the geom median
            //Thus, this still works for 4 pts.
            findCollinearMedian_3Pts(myPts[0], myPts[1], myPts[2], originalInputPtIndices, result);
        }
        else
        {
            vector< MyPoint_2 >   convexHullList;

            Utility_Functions::computeConvexHull(myPts, convexHullList);

            if (4 != convexHullList.size())
            {
                //The one not on the hull is the geom med.
                //Even if it's collinear with two others, if we take the int. of the diagonals, it'll be that fourth pt.
                //TODO Better way to test this?
                for (size_t ptInd = 0; ptInd < myPts.size(); ++ptInd)
                {
                    bool foundPt = false;
                    for (size_t chInd = 0; chInd < convexHullList.size(); ++chInd)
                    {
                        if (Utility_Functions::pointsAreTooClose(myPts[ptInd], convexHullList[chInd]))
                        {
                            foundPt = true;
                            break;
                        }
                    }
                    if (!foundPt)
                    {
                        //this is the pt missing from the ch list
                        result.medPoint = myPts[ptInd];
                        result.coincidentInputPtIndex = originalInputPtIndices[ptInd];
                        result.coincidesWithInputPt = true;
                        break;
                    }
                }
            }
            else
            {
                const MySegment_2 firstSeg = MySegment_2(convexHullList[0], convexHullList[2]);
                const MySegment_2 secondSeg = MySegment_2(convexHullList[1], convexHullList[3]);
                // The diagonals of a convex quadrilateral cross inside it.
                result.medPoint = uniqueIntersectionPoint(firstSeg, secondSeg);                
            }
        }
#   if (MY_VERBOSE)
        cout << "geom med 4 pts: " << result.medPoint << endl;
#   endif

        return result;
    }
} // namespace One_St_Pt_GPA


namespace
{
    MyPoint_2 findEqTriIntPoint(const MyPoint_2 & pointA, const MyPoint_2 & pointB, const MyPoint_2 & avoidMe)
    {
        MyPoint_2 intersectionPoint;
        vector< MyDirection_2 > coneRays(6);
        const MyLine_2 baseLine = MyLine_2(pointA, pointB);
        const MyDirection_2 initialDirection = MyDirection_2(baseLine);
        Utility_Functions::computeConeRays(initialDirection, coneRays);
        
        MyLine_2 firstEqTriSide, secondEqTriSide;
        if (baseLine.has_on_negative_side(avoidMe))
        {
            firstEqTriSide = MyLine_2(pointA, coneRays[1]);
            secondEqTriSide = MyLine_2(pointB, coneRays[2]);
        }
        else
        {
            //not collinear, therefore must be on positive side
            firstEqTriSide = MyLine_2(pointA, coneRays[5]);
            secondEqTriSide = MyLine_2(pointB, coneRays[4]);
        }
        //https://doc.cgal.org/latest/Kernel_23/group__intersection__linear__grp.html && Kernel_23/intersection_get.cpp
        // The two sides leave the base's endpoints at +/-60 degrees, so they meet at the apex.
        intersectionPoint = uniqueIntersectionPoint(firstEqTriSide, secondEqTriSide);    

        return intersectionPoint;

    } 

    bool testForDegenerateGeomMedian3Pts(const MyPoint_2 & pointA, const MyPoint_2 & pointB, const MyPoint_2 & pointC,
                                         DegeneratePointIndex & degenPt)
    {
        // Assumes a, b, c are not collinear, so at most one angle can be >= 120 degrees.
        if (angleAtLeast120(pointB, pointA, pointC))
        {
            degenPt = DegeneratePointIndex::FIRST;
            return true;
        }
        if (angleAtLeast120(pointA, pointB, pointC))
        {
            degenPt = DegeneratePointIndex::SECOND;
            return true;
        }
        if (angleAtLeast120(pointA, pointC, pointB))
        {
            degenPt = DegeneratePointIndex::THIRD;
            return true;
        }
        return false;
    }

    void findCollinearMedian_3Pts(const MyPoint_2 & pointA, const MyPoint_2 & pointB, const MyPoint_2 & pointC, 
                                                    const vector< size_t > & originalInputPtIndices, GeomMedianData & fillMe)
    {
        fillMe.coincidesWithInputPt = true;
        if (collinear_are_strictly_ordered_along_line(pointA, pointB, pointC))
        {
            fillMe.medPoint = pointB;
            fillMe.coincidentInputPtIndex = originalInputPtIndices[1];
        }
        else if (collinear_are_strictly_ordered_along_line(pointA, pointC, pointB))
        {
            fillMe.medPoint = pointC;
            fillMe.coincidentInputPtIndex = originalInputPtIndices[2];
        }
        else if (collinear_are_strictly_ordered_along_line(pointB, pointA, pointC))
        {
            fillMe.medPoint = pointA;
            fillMe.coincidentInputPtIndex = originalInputPtIndices[0];
        }
        return;
    }

    // The unique point where a and b meet. CGAL's intersection() returns an empty
    // optional if they don't meet, or a variant holding a point or (for overlapping
    // collinear inputs) a segment; callers guarantee a single point, so anything else
    // is a logic error.
    template< class Obj1, class Obj2 >
    MyPoint_2 uniqueIntersectionPoint(const Obj1 & a, const Obj2 & b)
    {
        auto intRes = intersection(a, b);
        const MyPoint_2 * p = intRes ? boost::get< MyPoint_2 >(intRes.get_ptr()) : nullptr;
        assert(p != nullptr);
        if (nullptr == p)
        {
            throw std::logic_error("uniqueIntersectionPoint: the objects do not meet in a single point");
        }
        return *p;
    }    

    // True if the angle at vertex v of triangle (u, v, w) is at least 120 degrees
    // (when its cosine is at most -1/2.).
    // With a = u - v and b = w - v: cos(angle) = a.b / (|a| |b|) <= -1/2 iff
    // a.b <= 0 and 4 (a.b)^2 >= |a|^2 |b|^2. Only +, -, * are used, so the test is
    // exact in MyKernel aka CGAL::Simple_cartesian< Lazy_FT > (no square roots, no rotated directions).
    bool angleAtLeast120(const MyPoint_2 & u, const MyPoint_2 & v, const MyPoint_2 & w)
    {
        const MyKernel::Vector_2 a = u - v;
        const MyKernel::Vector_2 b = w - v;
        const My_Number_type dot = a * b;    // CGAL: Vector_2 * Vector_2 is the dot product
        return (CGAL::sign(dot) != CGAL::POSITIVE)
                    && (My_Number_type(4) * dot * dot >= a.squared_length() * b.squared_length());
    }    

}  // anon namespace
