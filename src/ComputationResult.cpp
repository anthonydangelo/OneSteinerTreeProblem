#include "ComputationResult.hpp"

//See OneSteinerTreeProblem.cpp for project notes

ComputationResult::ComputationResult(int numInputPoints,
                                     int randomSeed,
                                     int inputGridLength,
                                     bool onlyProducePoints,
                                     const vector<MyPoint_2> &userPointList) : numPoints(numInputPoints),
                                                                               randSeed(randomSeed),
                                                                               gridLength(inputGridLength),
                                                                               onlyPoints(onlyProducePoints),
                                                                               rand(Random(randSeed)),
                                                                               //https://doc.cgal.org/latest/Generator/classCGAL_1_1Random__points__in__square__2.html
                                                                               randPointGen(Point_generator(gridLength, rand)),
                                                                               coneRays(6)
{

    preparePointSet(userPointList);

    if(onlyPoints){
        return;
    }

    Vasco_Rossi::computeConvexHull(inputPtVector, convexHullList);

    computeOODC();

    computeMSTAndStPts();
    
    return;
} //constructor


void ComputationResult::preparePointSet(const vector< MyPoint_2 >& userPointList)
{
    set< MyPoint_2 > pointSet;
    //sanitize input a bit
    for (const MyPoint_2& pt : userPointList)
    {
        if (pointSet.size() < numPoints)
        {
            std::pair<std::set< MyPoint_2 >::iterator, bool> ret;
            ret = pointSet.emplace(pt.x(), pt.y());
            if (ret.second == false)
            { //Maybe we should disable this output... don't want it messing up the json
                cerr << "tried to add an element that was already in there. size is " << pointSet.size() << endl;
            }
        }
        else
        {
            break;
        }
    }

    //https://doc.cgal.org/latest/Generator/index.html#GeneratorExample_2
    while (pointSet.size() < numPoints)
    {
        std::pair<std::set< MyPoint_2 >::iterator, bool> ret;
        MyPoint_2 temp = *randPointGen++;
        bool inSet = false;
        for (const MyPoint_2& pt : pointSet)
        {
            if (Vasco_Rossi::pointsAreTooClose(pt, temp))
            {
                inSet = true;
                break;
            }
        }
        if (inSet)
        {
            continue;
        }
        //does the move const get called w this insert? or does it have to be a heap obj for that?
        ret = pointSet.insert(temp);
        if (ret.second == false)
        {
            cerr << "tried to add an element that was already in there. size is " << pointSet.size() << endl;
        }
    }

    for (set< MyPoint_2 >::const_iterator it = pointSet.begin(); it != pointSet.end(); ++it)
    {
        //are the two calls to the members as costly as the push_back?
        inputPtVector.emplace_back(it->x(), it->y());
    }

    return;
}

//Pre-condition: convexHullList is populated
void ComputationResult::computeOODC()
{
    Vasco_Rossi::computeConeRays(MyDirection_2(1, 0), coneRays);
    size_t coneRaysSize = coneRays.size();
    assert(coneRaysSize == 6);
    
    //TODO try switching ch to const points
    const vector< MyPoint_2 > &clippingPolygonList = convexHullList;

    MyNef_polyhedron clippingPolygon(clippingPolygonList.begin(), clippingPolygonList.end(), MyNef_polyhedron::INCLUDED);
    MyOverlay_Traits overlayTraits;
    //The arrangements and their overlays are the same type...
    //don't know what overlay returns, better not const the array arrs
    array<MyArrangement_2, 2> myTempArrangements;
    size_t arrIndex = 0;
    //Which is better as the outer loop? Maybe doing all 6 directions for one point at a time will lead to fewer crossings in the overlay...
    for(size_t i = 0; i < inputPtVector.size(); ++i)
    {
        const MyPoint_2 &cellOrigin = inputPtVector[i];
//        size_t originPtIndex;
//        //TODO throw an error or something instead
//        assert(findOriginIndex(cellOrigin, inputPtVector, originPtIndex)); 
         for (size_t j = 0; j < coneRaysSize; ++j)
        {
            size_t nextJ = j + 1;
            if(nextJ == coneRaysSize)
            {
                nextJ = 0;
            }
            //https://doc.cgal.org/latest/Arrangement_on_surface_2/group__PkgArrangement2Funcs.html#ga339cdba93f54001be303595689002396
            //Self-overlay is not supported, so we need two temp arrangements to alternate between, and a reference to the solution...
            //https://doc.cgal.org/latest/Arrangement_on_surface_2/Arrangement_on_surface_2_2overlay_unbounded_8cpp-example.html#a6
            OrientedDirichletCell temp(coneRays.at(j), coneRays.at(nextJ), cellOrigin, i, inputPtVector, clippingPolygon);
            overlay( myTempArrangements.at(arrIndex % 2), temp.getCellArrangement(), myTempArrangements.at((arrIndex + 1) % 2), overlayTraits);
            ++arrIndex;
        }
    }
    resultODCArrangement = myTempArrangements.at(arrIndex % 2);
#if (MY_VERBOSE)
    cout << myTempArrangements.at(arrIndex % 2) << endl;
    size_t faceIndex = 0;
    for (auto fit = resultODCArrangement.faces_begin(); fit != resultODCArrangement.faces_end(); ++fit, ++faceIndex)
    {
        if (!fit->is_unbounded())
        {
            cout << "face # " << faceIndex << ": " << endl;
            Marisa_Tomei::print_ccb(fit->outer_ccb());
            //static cast instead?
            MyFaceData tempStruct = (MyFaceData)(fit->data());
            cout << "face data contains the following indices: [" << endl;
            for (size_t temp : tempStruct.myInputPointIndices)
            {
                cout << temp << endl;
            }
            cout << "]" << endl;
        }
    }
#endif    

    return;
}

void ComputationResult::computeMSTAndStPts()
{
    DelaunayTriEMST myEMST(inputPtVector);
    origMST = myEMST.getEMSTData();

    vector<GeomMedianData> rawStPtList = computeStPtsForOODC();
    for (size_t ptIndex = 0; ptIndex < rawStPtList.size(); ++ptIndex)
    {
        const GeomMedianData& s = rawStPtList[ptIndex];
        //test its insertion into the pt set if it's not degenerate
        if(s.coincidesWithInputPt)
        {
            steinerPoints.emplace_back(s, origMST);
        }
        else
        {
//            MyEMSTData stMST = myEMST.testPointInsertion(s.medPoint);
//            steinerPoints.emplace_back(s, stMST);
            steinerPoints.emplace_back(s, myEMST.testPointInsertion(s.medPoint));
        }
    }
    sort(steinerPoints.begin(), steinerPoints.end());
    //Now we have to properly assign the st pt indices so they print out correctly
    for (size_t stPtIndex = 0; stPtIndex < steinerPoints.size(); ++stPtIndex)
    {
        vector<pair<pair<size_t, size_t>,
                    pair<bool, bool>>>& mstEdgePairData = steinerPoints[stPtIndex].mstData.mstEdgePointIndices;
        for (pair<pair<size_t, size_t>, pair<bool, bool>>& tempEdgeData : mstEdgePairData)
        {
            //there won't be any degenerate edges, so either the first is a st pt, or the second, or neither
            if (tempEdgeData.second.first)
            {
                tempEdgeData.first.first = stPtIndex;
            }
            else if (tempEdgeData.second.second)
            {
                tempEdgeData.first.second = stPtIndex;
            }
        }
    }

    return;
}

string ComputationResult::outputResultToJSONString() const
{
    ostringstream sStream;
    sStream << "{\n";  
    sStream << Marisa_Tomei::wrapStringInQuotes(RAND_SEED_NAME_STRING) << ": \"" << randSeed << "\"";  
    sStream << ",\n";
    sStream << Marisa_Tomei::wrapStringInQuotes(GRID_LENGTH_NAME_STRING) << ": \"" << gridLength << "\"";  
    sStream << ",\n";    
    sStream << Marisa_Tomei::pointVectorToJSONString(INPUT_POINTS_NAME_STRING, inputPtVector);
    if(!onlyPoints){
        sStream << ",\n";
        sStream << Marisa_Tomei::vertexIndicesToJSONString(CONVEX_HULL_POINTS_NAME_STRING, convexHullList, inputPtVector);
        sStream << ",\n";
        sStream << Marisa_Tomei::arrangementToJSONString(resultODCArrangement);
        sStream << ",\n";
        sStream << Marisa_Tomei::mstDataToJSONString(origMST);
        sStream << ",\n";
        sStream << Marisa_Tomei::steinerPointsToJSONString(steinerPoints);
    }
    sStream << "\n}";

    return sStream.str();
}

void ComputationResult::computePotentialStPts(const vector<size_t>& siteIndices, const int numComboPts,
                                              vector<GeomMedianData>& results, set<vector<size_t>>& seenList) const
{
    if (numComboPts > siteIndices.size())
    {
        //TODO throw some kind of error...
        return;
    }
    Combination_enumerator<vector<size_t>::const_iterator> combi(numComboPts, siteIndices.begin(), siteIndices.end());
    while (!combi.finished())
    {
        vector<size_t> siteComboIndices;
        for (int i = 0; i < numComboPts; ++i)
        {
            size_t myInd = *combi[i];
            siteComboIndices.push_back(myInd);
        }
        sort(siteComboIndices.begin(), siteComboIndices.end());
        pair<set<vector<size_t>>::iterator, bool> ret = seenList.insert(siteComboIndices);
        if (ret.second)
        {
            vector<MyPoint_2> siteTuples;
            for (int i = 0; i < numComboPts; ++i)
            {
                const MyPoint_2& tempSitePt = inputPtVector.at(siteComboIndices[i]);
                siteTuples.emplace_back(tempSitePt.x(), tempSitePt.y());
            }
            results.push_back(GeomMedianFinder::computeGeomMedian(siteTuples, siteComboIndices));
        }
        ++combi;
    }
    return;
}

vector<GeomMedianData> ComputationResult::computeStPtsForOODC() const
{
    vector<GeomMedianData> results;
    set< vector<size_t> > seenList;

    auto endFit = resultODCArrangement.faces_end();
    for (MyArrangement_2::Face_const_iterator fit = resultODCArrangement.faces_begin(); fit != endFit; ++fit)
    {
        if (!fit->is_unbounded())
        {
            vector<size_t> siteIndices;
            //static cast instead?
            MyFaceData myFaceStruct = (MyFaceData)(fit->data());
            auto endIt = end(myFaceStruct.myInputPointIndices);
            for (auto it = begin(myFaceStruct.myInputPointIndices); it != endIt; ++it)
            {
                siteIndices.push_back(*it);
            }
            //These can be extracted into a function...
            if (siteIndices.size() >= 3)
            {
                computePotentialStPts(siteIndices, 3, results, seenList);
            }
            if (siteIndices.size() >= 4)
            {
                computePotentialStPts(siteIndices, 4, results, seenList);
            }   
        } // if (!fit->is_unbounded())
    } // for (MyArrangement_2::Face_const_iterator fit = resultODCArrangement.faces_begin(); fit != endFit; ++fit)

    return results;
}
