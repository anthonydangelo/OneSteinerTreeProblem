#include "ComputationResult.hpp"

ComputationResult::ComputationResult(int numInputPoints,
                                     int randomSeed,
                                     int inputGridLength,
                                     bool onlyProducePoints,
                                     string outputFilePrefix,
                                     const vector<reference_wrapper<const MyPoint_2>> &userPointList) : numPoints(numInputPoints),
                                                                               randSeed(randomSeed),
                                                                               gridLength(inputGridLength),
                                                                               onlyPoints(onlyProducePoints),
                                                                               rand(Random(randSeed)),
                                                                               //https://doc.cgal.org/latest/Generator/classCGAL_1_1Random__points__in__square__2.html
                                                                               randPointGen(Point_generator(gridLength, rand)),
                                                                               outFilePrefix(outputFilePrefix),
                                                                               coneRays(6)
{

    preparePointSet(userPointList);

    if(onlyPoints){
        return;
    }

    computeConvexHull(inputPtVector, convexHullList);

    computeOODC();

    computeMSTAndStPts();
    
    return;
} //constructor


void ComputationResult::preparePointSet(const vector< reference_wrapper<const MyPoint_2> >& userPointList)
{
    set< reference_wrapper<const MyPoint_2> > pointSet;
    //sanitize input a bit
    for (const auto pt : userPointList)
    {
        if (pointSet.size() < numPoints)
        {
            std::pair<std::set< reference_wrapper<const MyPoint_2> >::iterator, bool> ret;
            ret = pointSet.insert(pt);
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
        std::pair<std::set< reference_wrapper<const MyPoint_2> >::iterator, bool> ret;
        MyPoint_2 temp = *randPointGen++;
        bool inSet = false;
        for (const auto pt : pointSet)
        {
            if (pointsAreTooClose(pt, temp))
            {
                inSet = true;
                break;
            }
        }
        if (inSet)
        {
            continue;
        }
        ret = pointSet.insert(temp);
        if (ret.second == false)
        {
            cerr << "tried to add an element that was already in there. size is " << pointSet.size() << endl;
        }
    }

    for (set< reference_wrapper<const MyPoint_2> >::const_iterator it = pointSet.begin(); it != pointSet.end(); ++it)
    {
        inputPtVector.push_back(*it);
    }

    return;
}

//Pre-condition: convexHullList is populated
void ComputationResult::computeOODC()
{
    computeConeRays(MyDirection_2(1, 0), coneRays);
    size_t coneRaysSize = coneRays.size();
    assert(coneRaysSize == 6);
    
    const vector< MyPoint_2 > &clippingPolygonList = convexHullList;

    MyNef_polyhedron clippingPolygon(clippingPolygonList.begin(), clippingPolygonList.end(), MyNef_polyhedron::INCLUDED);
    MyOverlay_Traits overlayTraits;
    //The arrangements and their overlays are the same type...
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
            print_ccb(fit->outer_ccb());
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
            steinerPoints.push_back(CandidateSteinerPointData(s, origMST));
        }
        else
        {
            MyEMSTData stMST = myEMST.testPointInsertion(s.medPoint);
            steinerPoints.push_back(CandidateSteinerPointData(s, stMST));
        }
    }
    sort(steinerPoints.begin(), steinerPoints.end());

    return;
}

string ComputationResult::outputResultToJSONString() const
{
    ostringstream sStream;
    sStream << "{\n";  
    sStream << wrapStringInQuotes(RAND_SEED_NAME_STRING) << ": \"" << randSeed << "\"";  
    sStream << ",\n";
//    sStream << pointSetToJSONString(INPUT_POINTS_NAME_STRING, pointSet);
    sStream << pointVectorToJSONString(INPUT_POINTS_NAME_STRING, inputPtVector);
    if(!onlyPoints){
        sStream << ",\n";
        sStream << vertexIndicesToJSONString(CONVEX_HULL_POINTS_NAME_STRING, convexHullList, inputPtVector);
        sStream << ",\n";
        sStream << arrangementToJSONString();
        sStream << ",\n";
        sStream << mstDataToJSONString(origMST);
        sStream << ",\n";
        sStream << steinerPointsToJSONString();
    }
    sStream << "\n}";

    return sStream.str();
}

string ComputationResult::mstEdgeToJSONString(const pair< pair<size_t, size_t>, pair<bool, bool> >& edgeData, int tabLevel=0) const
{
    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(MST_EDGE_NAME_STRING) << ": { ";

    sStream << insertTabs(tabLevel + 1);
    sStream << wrapStringInQuotes(MST_EDGE_ENDPOINT_INDICES_NAME_STRING) <<  ": [ \n";

    sStream << insertTabs(tabLevel + 3);
    sStream << "{\"index\":\"" << edgeData.first.first << "\"},\n";
    sStream << "{\"index\":\"" << edgeData.first.second << "\"}\n";

    sStream << insertTabs(tabLevel+3);
    sStream << "],\n"; 

    sStream << boolalpha;
    sStream << insertTabs(tabLevel + 1);
    sStream << wrapStringInQuotes(MST_EDGE_FIRST_ENDPOINT_IS_STEINER_PT_NAME_STRING) << ": \"" << edgeData.second.first << "\",\n";
    sStream << insertTabs(tabLevel + 1);
    sStream << wrapStringInQuotes(MST_EDGE_SECOND_ENDPOINT_IS_STEINER_PT_NAME_STRING) << ": \"" << edgeData.second.second << "\"\n";

    sStream << insertTabs(tabLevel);
    sStream << "}";
    return sStream.str();
}

string ComputationResult::mstDataToJSONString(const MyEMSTData& mst, int tabLevel=0) const
{
    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(MST_NAME_STRING) << ": { ";

    sStream << wrapStringInQuotes(MST_LENGTH_NAME_STRING) << " : \"" << mst.length << "\",\n";

    sStream << insertTabs(tabLevel+1);
    sStream << wrapStringInQuotes(MST_EDGE_LIST_NAME_STRING) << ": [ \n";

    auto endIt = mst.mstEdgePointIndices.end();
    for (auto myIt = mst.mstEdgePointIndices.begin(); myIt != endIt; ++myIt)
    {
        sStream << insertTabs(tabLevel + 2);
        sStream << "{ \n";
        
        sStream << mstEdgeToJSONString(*myIt, tabLevel + 3) << "\n";

        sStream << insertTabs(tabLevel + 2);
        sStream << "}";
        if (next(myIt) != endIt)
        {
            sStream << ",";
        }
        sStream << "\n";
    }

    sStream << insertTabs(tabLevel+2);
    sStream << "]\n";

    sStream << insertTabs(tabLevel);
    sStream << "}";
    return sStream.str();
}

string ComputationResult::geomMedDataToJSONString(const GeomMedianData& stPtData, int tabLevel=0) const
{
    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(STEINER_PT_NAME_STRING) << ": { \n";

    sStream << insertTabs(tabLevel+1);
    sStream << wrapStringInQuotes(GEOM_MED_POINT_NAME_STRING) << " : " << point2ToJSON(stPtData.medPoint) << ",\n";

    sStream << insertTabs(tabLevel+1);
    sStream << wrapStringInQuotes(GEOM_MED_NEIGHBOUR_INDICES_NAME_STRING) << ": [ \n";

    auto endIt = stPtData.inputPtIndices.end();
    for (auto myIt = stPtData.inputPtIndices.begin(); myIt != endIt; ++myIt)
    {
        sStream << insertTabs(tabLevel + 3);
        sStream << "{\"index\":\"" << *myIt << "\"}";
        if (next(myIt) != endIt)
        {
            sStream << ",";
        }
        sStream << "\n";        
    }

    sStream << insertTabs(tabLevel+3);
    sStream << "],\n"; 

    sStream << boolalpha;
    sStream << insertTabs(tabLevel+1);
    sStream << wrapStringInQuotes(GEOM_MED_COINCIDES_W_INPUT_PT_NAME_STRING) << ": \"" << stPtData.coincidesWithInputPt << "\",\n";    
    sStream << insertTabs(tabLevel+1);
    sStream << wrapStringInQuotes(GEOM_MED_COINCIDENT_INPUT_INDEX_NAME_STRING) << ": {\"index\":\"" << stPtData.coincidentInputPtIndex << "\"}\n";

    sStream << insertTabs(tabLevel);
    sStream << "}"; 
    return sStream.str();    
}

string ComputationResult::candidateSteinerPtDataToJSONString(const CandidateSteinerPointData& stPtData, int tabLevel=0) const
{
    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(CANDIDATE_STEINER_POINT_DATA_NAME_STRING) << ": { \n";

    sStream << geomMedDataToJSONString(stPtData.steinerPt, tabLevel+1) << ",\n";
    sStream << mstDataToJSONString(stPtData.mstData, tabLevel+1) << "\n";

    sStream << insertTabs(tabLevel);
    sStream << "}"; 
    return sStream.str();  
}

string ComputationResult::steinerPointsToJSONString(int tabLevel=0) const
{
    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(CANDIDATE_ST_PT_DATA_LIST_NAME_STRING) << ": [ \n";

    auto endIt = end(steinerPoints);
    for (auto it = begin(steinerPoints); it != endIt; ++it)
    {
        sStream << insertTabs(tabLevel + 2);
        sStream << candidateSteinerPtDataToJSONString(*it);
        if (next(it) != endIt)
        {
            sStream << ",";
        }
        sStream << "\n";
    }

    sStream << insertTabs(tabLevel);
    sStream << "]"; 
    return sStream.str();     
}

/* //https://stackoverflow.com/questions/5451305/how-to-make-function-argument-container-independent
//https://stackoverflow.com/questions/34561190/pass-a-std-container-to-a-function
template <typename Container>
string ComputationResult::outputCollectionToJSONFile(string name, const Container &myColl) const
{
    ostringstream sStream;
    sStream << wrapStringInQuotes(name) << ": [";
    
    if(!myColl.empty()){
        for (auto it = begin(myColl); it != end(vec); ++it) {
            typedef decltype(*it) T; // if needed
            sStream << *it << " "; 
        }    
    }

    sStream << "]" << endl; 
    
    return sStream.str();
} */


string ComputationResult::pointSetToJSONString(string name, const set<MyPoint_2> &myColl, int tabLevel) const
{
    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(name) << ": [";
    
    if(!myColl.empty()){
        auto endIt = end(myColl);
        for (auto it = begin(myColl); it != endIt; ++it) {
            sStream << point2ToJSON(*it); 
            if(next(it) != endIt){
                sStream << ",\n";
                sStream << insertTabs(tabLevel + 2);
            } else {
                sStream << "\n";
            }
        }    
    }

    sStream << insertTabs(tabLevel);
    sStream << "]";
    
    return sStream.str();
}

string ComputationResult::pointVectorToJSONString(string name, const vector< reference_wrapper<const MyPoint_2> >& myColl, int tabLevel) const
{
    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(name) << ": [\n";
    
    if(!myColl.empty()){
        auto endIt = end(myColl);
        for (auto it = begin(myColl); it != endIt; ++it) {
            sStream << insertTabs(tabLevel + 2);
            sStream << point2ToJSON(*it); 
            if(next(it) != endIt){
                sStream << ",";
            } 
            sStream << "\n";
        }    
    }

    sStream << insertTabs(tabLevel);
    sStream << "]"; 
    
    return sStream.str();
}

string ComputationResult::vertexIndicesToJSONString(string name, const vector<MyPoint_2> &myColl,
                                                    const vector<reference_wrapper<const MyPoint_2>>& myPtSet, int tabLevel) const
{

    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(name) << ": [";
    
    if(!myColl.empty()){
        auto endIt = end(myColl);
        for (auto it = begin(myColl); it != endIt; ++it) {
            size_t index = 0;
            assert(findPointIndex(*it, myPtSet, index));
            sStream << "{\"index\":\"" << index << "\"}"; 
            if(next(it) != endIt){
                sStream << ",\n";
                sStream << insertTabs(tabLevel + 2);
            } else {
                sStream << "\n";
            }
        }    
    }

    sStream << insertTabs(tabLevel);
    sStream << "]";
    
    return sStream.str();
}

//Assumption: no duplicate points. The "std::set" insertion can't reliably tell if there are doubles (I've experienced this...)
void ComputationResult::insertArrangementPointsIntoPointSet(vector< reference_wrapper<const MyPoint_2> >& arrPointsVec) const
{
    set<MyPoint_2> arrPoints;
    for(auto it = resultODCArrangement.vertices_begin(); it != resultODCArrangement.vertices_end(); ++it)
    {
        if (! it->is_isolated() && ! it->is_at_open_boundary())
        {
            arrPoints.insert(it->point());
        }
    }    
    for (MyPoint_2 p : arrPoints)
    {
        arrPointsVec.push_back(p);
    }
    return;
}

string ComputationResult::arrangementFaceToJSONString(string faceName, const MyArrangement_2::Face_const_iterator fit, 
                                                        const vector<reference_wrapper<const MyPoint_2>>& myPtSet, int tabLevel) const
{

    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(ARR_IND_FACE_NAME_STRING) << ": {";
    
    sStream << "\"name\": " << wrapStringInQuotes(faceName) << " ,\n";
    ////////////////////
    sStream << insertTabs(tabLevel + 1);
    sStream << wrapStringInQuotes(ARR_FACE_CCW_V_INDICES_NAME_STRING) <<  ": [ \n";

    MyArrangement_2::Ccb_halfedge_const_circulator circ = fit->outer_ccb();
    MyArrangement_2::Ccb_halfedge_const_circulator curr = circ;
    MyArrangement_2::Halfedge_const_handle heEnd = circ; //don't know if this does an implicit conversion; best it do it here, not in the expr
    do {
        MyArrangement_2::Halfedge_const_handle he = curr;
        size_t index = 0;
        assert(findPointIndex(he->target()->point(), myPtSet, index));
        sStream << insertTabs(tabLevel + 3);
        sStream << "{\"index\":\"" << index << "\"}";
        if(he->next() != heEnd){
            sStream << ",";
        }        
        sStream << "\n";
    } while (++curr != circ);

    sStream << insertTabs(tabLevel+3);
    sStream << "],\n";
    //////////////////////

    sStream << insertTabs(tabLevel + 1);
    sStream << wrapStringInQuotes(ARR_OODC_INPUT_SITE_INDICES_NAME_STRING) <<  ": [ \n";

    //static cast instead?
    MyFaceData myFaceStruct = (MyFaceData)(fit->data());
    auto endIt = end(myFaceStruct.myInputPointIndices);
    for (auto it = begin(myFaceStruct.myInputPointIndices); it != endIt; ++it) {
        sStream << insertTabs(tabLevel + 3);
        sStream << "{\"index\":\"" << *it << "\"}"; 
        if(next(it) != endIt){
            sStream << ",";
        }
        sStream << "\n";
    }             
    sStream << insertTabs(tabLevel + 3);
    sStream << "]\n";
    //////////////////////

    sStream << insertTabs(tabLevel);
    sStream << "}"; 
    
    return sStream.str();
}

string ComputationResult::arrangementToJSONString(int tabLevel) const
{
    vector< reference_wrapper<const MyPoint_2> > arrPoints;
    insertArrangementPointsIntoPointSet(arrPoints);

    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(ARR_NAME_STRING) << ": {";

    //sStream << pointSetToJSONString(ARR_POINTS_NAME_STRING, arrPoints, tabLevel + 1);
/*     vector<reference_wrapper<const MyPoint_2>> tempVec;
    for (const MyPoint_2& p : arrPoints)
    {
        //this is annoying and should be unnecessary! god damn it.
        tempVec.push_back(p);
    }
    sStream << pointVectorToJSONString(ARR_POINTS_NAME_STRING, tempVec, tabLevel + 1); 
*/
    sStream << pointVectorToJSONString(ARR_POINTS_NAME_STRING, arrPoints, tabLevel + 1) << ",\n";

    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(ARR_FACES_NAME_STRING) << ": [ \n";

    size_t faceIndex = 0;
    auto endFit = resultODCArrangement.faces_end();
//    for (MyArrangement_2::Face_const_iterator fit = resultODCArrangement.faces_begin(); fit != resultODCArrangement.faces_end(); ++fit, ++faceIndex)
    for (MyArrangement_2::Face_const_iterator fit = resultODCArrangement.faces_begin(); fit != endFit; ++fit)
    {
        if (!fit->is_unbounded())
        {
            ostringstream faceSStream;
//            faceSStream << ARR_FACE_NAME_PREFIX_NAME_STRING << faceIndex;
            faceSStream << ARR_FACE_NAME_PREFIX_NAME_STRING << faceIndex++;
            sStream << insertTabs(tabLevel + 1);
            sStream << "{ \n";

            sStream << arrangementFaceToJSONString(faceSStream.str(), fit, arrPoints, tabLevel + 2) << "\n";
//            sStream << arrangementFaceToJSONString(faceSStream.str(), fit, tempVec, tabLevel + 1);

            sStream << insertTabs(tabLevel + 1);
            sStream << "}";
            if(next(fit) != endFit) 
            {
                sStream << ",";
            } 
            sStream << "\n";
        }
    }

    sStream << insertTabs(tabLevel+1);
    sStream << "]\n";

    sStream << insertTabs(tabLevel);
    sStream << "}";
    
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
            vector<reference_wrapper<const MyPoint_2>> siteTuples;
            for (int i = 0; i < numComboPts; ++i)
            {
                siteTuples.push_back(inputPtVector.at(siteComboIndices[i]));
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
