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
    if (collinear(myPts[0], myPts[1], myPts[2]))
    {
        //The middle is the geom median
        result.coincidesWithInputPt = true;
        if (collinear_are_strictly_ordered_along_line(myPts[0], myPts[1], myPts[2]))
        {
            result.medPoint = myPts[1];
            result.coincidentInputPtIndex = originalInputPtIndices[1];
        }
        else if (collinear_are_strictly_ordered_along_line(myPts[0], myPts[2], myPts[1]))
        {
            result.medPoint = myPts[2];
            result.coincidentInputPtIndex = originalInputPtIndices[2];
        }
        else if (collinear_are_strictly_ordered_along_line(myPts[1], myPts[0], myPts[2]))
        {
            result.medPoint = myPts[0];
            result.coincidentInputPtIndex = originalInputPtIndices[0];
        }
    }
    else
    {
        vector<MyDirection_2> coneRays;
        MyLine_2 baseLine;
        DegeneratePointIndex degenPt;
        if (testForDegenerateGeomMedian3Pts(myPts[0], myPts[1], myPts[2], baseLine, coneRays, degenPt))
        {
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
            MyPoint_2 firstIntPoint = findEqTriIntPoint(myPts[0], myPts[1], myPts[2]);
            MySegment_2 firstSimpsonLine = MySegment_2(firstIntPoint, myPts[2]);
            MyPoint_2 secondIntPoint = findEqTriIntPoint(myPts[0], myPts[2], myPts[1]);
            MySegment_2 secondSimpsonLine = MySegment_2(secondIntPoint, myPts[1]);
            //https://doc.cgal.org/latest/Kernel_23/group__intersection__linear__grp.html && Kernel_23/intersection_get.cpp
            CGAL::cpp11::result_of<MyIntersect_2(MySegment_2, MySegment_2)>::type intRes = intersection(firstSimpsonLine, secondSimpsonLine);
            if (intRes)
            { //these two non-par segments will intersect... do i really have to test for result?
                result.medPoint = boost::get<MyPoint_2>(*intRes);
            } //TODO Else throw error or something...
        }
    }

    return result;
}

MyPoint_2 GeomMedianFinder::findEqTriIntPoint(const MyPoint_2& pointA, const MyPoint_2& pointB, const MyPoint_2& avoidMe) const
{
    MyPoint_2 intersectionPoint;
    vector<MyDirection_2> coneRays;
    MyLine_2 baseLine = MyLine_2(pointA, pointB);
    MyDirection_2 initialDirection = MyDirection_2(baseLine);
    computeConeRays(initialDirection, coneRays);
    
    return findEqTriIntPoint(pointA, pointB, avoidMe, baseLine, coneRays);
}

MyPoint_2 GeomMedianFinder::findEqTriIntPoint(const MyPoint_2& pointA, const MyPoint_2& pointB, const MyPoint_2& avoidMe, 
                                                const MyLine_2& baseLine, const vector<MyDirection_2>& coneRays) const
{
    MyPoint_2 intersectionPoint;
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
    CGAL::cpp11::result_of<MyIntersect_2(MyLine_2, MyLine_2)>::type intRes = intersection(firstEqTriSide, secondEqTriSide);
    if (intRes)
    { //these two non-par lines will intersect... do i really have to test for result?
        intersectionPoint = boost::get<MyPoint_2>(*intRes);
    } //TODO Else throw error or something...

    return intersectionPoint;

}

bool GeomMedianFinder::testForDegenerateGeomMedian3Pts(const MyPoint_2 &pointA, const MyPoint_2 &pointB, const MyPoint_2 &pointC,
                                                       MyLine_2 &baseLineToFill, vector<MyDirection_2> &coneRaysToFill, 
                                                       DegeneratePointIndex& degenPt) const
{
    //Assume non-collinear points a,b,c
    baseLineToFill = MyLine_2(pointA, pointB);
    MyDirection_2 initialDirection = MyDirection_2(baseLineToFill);
    computeConeRays(initialDirection, coneRaysToFill);
    MyDirection_2 bttmRightDir = coneRaysToFill[5];
    MyDirection_2 bttmLeftDir = coneRaysToFill[4];
    MyPoint_2 leftPoint = pointA;
    MyPoint_2 rightPoint = pointB;
    bool switchSides = false;
    
    MyLine_2 effectiveBaseLine = baseLineToFill;
    if ( ! effectiveBaseLine.has_on_negative_side(pointC))
    {
        //Flip everything around
        //Now we know c is on the 'negative' side, i.e. the right-hand side
        effectiveBaseLine = effectiveBaseLine.opposite();
        bttmRightDir = coneRaysToFill[2];
        bttmLeftDir = coneRaysToFill[1];
        leftPoint = pointB;
        rightPoint = pointA;
        switchSides = true;
    }    

    //test angles abc and bac for 120 degrees
    MyLine_2 firstTestLine;
    firstTestLine = MyLine_2(rightPoint, bttmRightDir);
    if ( ! firstTestLine.has_on_negative_side(pointC))
    {
        //right point is the degenerate point
        if(switchSides)
        {
            degenPt = DegeneratePointIndex::FIRST;
        } else 
        {
            degenPt = DegeneratePointIndex::SECOND;
        }
        return true;
    }
    MyLine_2 secondTestLine;
    secondTestLine = MyLine_2(leftPoint, bttmLeftDir);
    if ( ! secondTestLine.has_on_positive_side(pointC))
    {
        //left point is the degenerate point
        if(switchSides)
        {
            degenPt = DegeneratePointIndex::SECOND;
        } else 
        {
            degenPt = DegeneratePointIndex::FIRST;
        }
        return true;
    }

    MyLine_2 secondBaseline = MyLine_2(leftPoint, pointC);
    MyDirection_2 secondInitDirection = MyDirection_2(secondBaseline);
    vector<MyDirection_2> secondConeRays;
    computeConeRays(secondInitDirection, secondConeRays);    
    MyLine_2 thirdTestLine;
    thirdTestLine = MyLine_2(pointC, secondConeRays[1]);
    if ( ! thirdTestLine.has_on_positive_side(rightPoint))
    {
        //point C is the degenerate point
        degenPt = DegeneratePointIndex::THIRD;        
        return true;
    }

    return false;
}
