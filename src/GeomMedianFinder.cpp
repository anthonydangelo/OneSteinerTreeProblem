#include "GeomMedianFinder.hpp"

//See OneSteinerTreeProblem.cpp for project notes

//No one needs to see these file-level scoped functions and enums
//My understanding is that the anon namespace makes this effectively a file-scoped enum
namespace
{
    enum class DegeneratePointIndex {FIRST, SECOND, THIRD};
}

//My understanding is that the anon namespace could make these effectively file-scoped, but so can the static keyword. 
//I like C... Plus, apparently the anon namespace is not exactly equivalent; e.g. it allows me to declare, e.g., int a both inside
//and outside of the namespace.
static MyPoint_2 findEqTriIntPoint(const MyPoint_2 &pointA, const MyPoint_2 &pointB, const MyPoint_2 &avoidMe);
static bool testForDegenerateGeomMedian3Pts(const MyPoint_2 &pointA, const MyPoint_2 &pointB, const MyPoint_2 &pointC,
                                     DegeneratePointIndex &degenPt);
static void findCollinearMedian_3Pts(const MyPoint_2 &pointA, const MyPoint_2 &pointB, const MyPoint_2 &pointC,
                              const vector<size_t> &originalInputPtIndices, GeomMedianData &fillMe);

GeomMedianData GeomMedianFinder::computeGeomMedian(const vector<MyPoint_2> &myPts,
                                                   const vector<size_t> &originalInputPtIndices)
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

GeomMedianData GeomMedianFinder::computeGeomMedian_3Pts(const vector< MyPoint_2 >& myPts, const vector<size_t>& originalInputPtIndices)
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
        //Test for an angle of 120 degrees or greater
        //This doesn't work like it should since we can't specify precision and we lose things to input noise...
        //We have to do an explicit check afterwards
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
//            CGAL::cpp11::result_of<MyIntersect_2(MySegment_2, MySegment_2)>::type intRes = intersection(firstSimpsonLine, secondSimpsonLine);
//compiler doesn't like the previous line declaration... wtf...! maybe it's cus of the consts...
            auto intRes = intersection(firstSimpsonLine, secondSimpsonLine);
            if (intRes)
            { //these two non-par segments will intersect... do i really have to test for result?
                result.medPoint = boost::get<MyPoint_2>(*intRes);
            } //TODO Else throw error or something...
        }
        //check if it's too close to an input pt
        if (!detectedDegen)
        {
            if (Vasco_Rossi::pointsAreTooClose(result.medPoint, myPts[0]))
            {
                result.coincidesWithInputPt = true;
                result.medPoint = myPts[0];
                result.coincidentInputPtIndex = originalInputPtIndices[0];
            }
            else if (Vasco_Rossi::pointsAreTooClose(result.medPoint, myPts[1]))
            {
                result.coincidesWithInputPt = true;
                result.medPoint = myPts[1];
                result.coincidentInputPtIndex = originalInputPtIndices[1];
            }
            else if (Vasco_Rossi::pointsAreTooClose(result.medPoint, myPts[2]))
            {
                result.coincidesWithInputPt = true;
                result.medPoint = myPts[2];
                result.coincidentInputPtIndex = originalInputPtIndices[2];
            }
        }
    }

#if (MY_VERBOSE)
cout << "geom med 3 pts: " << result.medPoint << " coincides? " << result.coincidesWithInputPt << " index: " << result.coincidentInputPtIndex << endl;
#endif

    return result;
}

static MyPoint_2 findEqTriIntPoint(const MyPoint_2& pointA, const MyPoint_2& pointB, const MyPoint_2& avoidMe)
{
    MyPoint_2 intersectionPoint;
    vector<MyDirection_2> coneRays(6);
    const MyLine_2 baseLine = MyLine_2(pointA, pointB);
    const MyDirection_2 initialDirection = MyDirection_2(baseLine);
    Vasco_Rossi::computeConeRays(initialDirection, coneRays);
    
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
//    CGAL::cpp11::result_of<MyIntersect_2(MyLine_2, MyLine_2)>::type intRes = intersection(firstEqTriSide, secondEqTriSide);
    auto intRes = intersection(firstEqTriSide, secondEqTriSide);
    if (intRes)
    { //these two non-par lines will intersect... do i really have to test for result?
        intersectionPoint = boost::get<MyPoint_2>(*intRes);
    } //TODO Else throw error or something...

    return intersectionPoint;

}

static bool testForDegenerateGeomMedian3Pts(const MyPoint_2 &pointA, const MyPoint_2 &pointB, const MyPoint_2 &pointC,
                                                       DegeneratePointIndex& degenPt)
{
    //Assume non-collinear points a,b,c
    //need to fill the vec w assignable elements. not sure if reserve would work...
    vector<MyDirection_2> coneRays(6);
    const MyLine_2 baseLine = MyLine_2(pointA, pointB);
    const MyDirection_2 initialDirection = MyDirection_2(baseLine);
    Vasco_Rossi::computeConeRays(initialDirection, coneRays);
    MyDirection_2 bttmRightDir = coneRays[5];
    MyDirection_2 bttmLeftDir = coneRays[4];
    MyPoint_2 leftPoint = pointA;
    MyPoint_2 rightPoint = pointB;
    bool switchSides = false;
#if (MY_VERBOSE)
cout << "degen test: pt a " << pointA <<", pt b " << pointB << endl;
cout << "result of 'has on': " << boolalpha << baseLine.has_on(pointC) << endl;
cout << "result of 'has neg': " << boolalpha << baseLine.has_on_negative_side(pointC) << endl;
cout << "result of 'has pos': " << boolalpha << baseLine.has_on_positive_side(pointC) << endl;
#endif
    if ( ! baseLine.has_on_negative_side(pointC))
    {
        //Flip everything around
        //Now we know c is on the 'negative' side, i.e. the right-hand side
        bttmRightDir = coneRays[2];
        bttmLeftDir = coneRays[1];
        leftPoint = pointB;
        rightPoint = pointA;
        switchSides = true;
    }    
#if (MY_VERBOSE)
cout << "degen test: left " << leftPoint <<", right " << rightPoint << endl;
cout << "testing against pt c " << pointC << endl;
cout << "bttm right dir: " << bttmRightDir << endl;
#endif    

    //test angles abc and bac for 120 degrees
    MyLine_2 firstTestLine;
    firstTestLine = MyLine_2(rightPoint, bttmRightDir);
#if (MY_VERBOSE)
cout << "result of 'has on': " << boolalpha << firstTestLine.has_on(pointC) << endl;
cout << "result of 'has neg': " << boolalpha << firstTestLine.has_on_negative_side(pointC) << endl;
cout << "result of 'has pos': " << boolalpha << firstTestLine.has_on_positive_side(pointC) << endl;
#endif
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

    const MyLine_2 secondBaseline = MyLine_2(leftPoint, pointC);
    const MyDirection_2 secondInitDirection = MyDirection_2(secondBaseline);
    vector<MyDirection_2> secondConeRays(6);
    Vasco_Rossi::computeConeRays(secondInitDirection, secondConeRays);    
    MyLine_2 thirdTestLine;
    thirdTestLine = MyLine_2(pointC, secondConeRays[1]);
#if (MY_VERBOSE)
cout << "degen test: second baseline " << leftPoint <<", " << pointC << endl;
#endif        
    if ( ! thirdTestLine.has_on_positive_side(rightPoint))
    {
        //point C is the degenerate point
        degenPt = DegeneratePointIndex::THIRD;        
        return true;
    }

    return false;
}


GeomMedianData GeomMedianFinder::computeGeomMedian_4Pts(const vector< MyPoint_2 >& myPts, const vector<size_t>& originalInputPtIndices)
{
    assert(4 == myPts.size());

    GeomMedianData result(originalInputPtIndices);    


    //assume no input points are the same
    if (collinear(myPts[0], myPts[1], myPts[2]) && collinear(myPts[3], myPts[1], myPts[2]))
    {
        //The middle (or anywhere on the middle segment) is the geom median
        //This still works for 4 pts.
        findCollinearMedian_3Pts(myPts[0], myPts[1], myPts[2], originalInputPtIndices, result);
    }

    vector<MyPoint_2>   convexHullList;

    Vasco_Rossi::computeConvexHull(myPts, convexHullList);

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
                if (Vasco_Rossi::pointsAreTooClose(myPts[ptInd], convexHullList[chInd]))
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
//        CGAL::cpp11::result_of<MyIntersect_2(MySegment_2, MySegment_2)>::type intRes = intersection(firstSeg, secondSeg);
        auto intRes = intersection(firstSeg, secondSeg);
        if (intRes)
        { //these two non-par segments will intersect... do i really have to test for result?
            result.medPoint = boost::get<MyPoint_2>(*intRes);
        } //TODO Else throw error or something...
    }

#if (MY_VERBOSE)
cout << "geom med 4 pts: " << result.medPoint << endl;
#endif

    return result;
}

static void findCollinearMedian_3Pts(const MyPoint_2& pointA, const MyPoint_2& pointB, const MyPoint_2& pointC, 
                                                const vector<size_t>& originalInputPtIndices, GeomMedianData& fillMe)
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
