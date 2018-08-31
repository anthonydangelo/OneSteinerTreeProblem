#include "ComputationResult.hpp"

ComputationResult::ComputationResult(int numInputPoints,
                                     int randomSeed,
                                     int inputGridLength,
                                     bool onlyProducePoints,
                                     string outputFilePrefix,
                                     vector<MyPoint_2> userPointList) : numPoints(numInputPoints),
                                                               randSeed(randomSeed),
                                                               gridLength(inputGridLength),
                                                               onlyPoints(onlyProducePoints),
                                                               rand(Random(randSeed)),
                                                               //https://doc.cgal.org/latest/Generator/classCGAL_1_1Random__points__in__square__2.html
                                                               randPointGen(Point_generator(gridLength, rand)),
                                                               outFilePrefix(outputFilePrefix),
                                                               coneRays(6),
                                                               myEMST(DelaunayTriEMST())
{

    for (const auto pt : userPointList)
    {
        if(pointSet.size() < numPoints){
            std::pair<std::set<MyPoint_2>::iterator,bool> ret;
            ret = pointSet.insert(pt);
            if (ret.second == false){
                cerr << "tried to add an element that was already in there. size is " << pointSet.size() << endl; 
            }            
        } else {
            break;
        }
    }

    //https://doc.cgal.org/latest/Generator/index.html#GeneratorExample_2
    while (pointSet.size() < numPoints)
    {
        std::pair<std::set<MyPoint_2>::iterator,bool> ret;
        MyPoint_2 temp = *randPointGen++;
        bool inSet = false;
        for(const auto pt : pointSet) 
        {
            if( pointsAreTooClose(pt, temp) ){
                inSet = true;
                break;
            }
        }
        if (inSet){
            continue;
        }
        ret = pointSet.insert(temp);
        if (ret.second == false){
            cerr << "tried to add an element that was already in there. size is " << pointSet.size() << endl; 
        }
    }

#if (MY_VERBOSE)

    /* std::ostream_iterator< MyPoint_2 > out( std::cout, " " );
 std::copy(pointSet.begin(), pointSet.end(), out); */

    for (const auto pt : pointSet)
    {
        cout << " " << pt << endl;
    }

    cout << outputCollectionToJSONString(INPUT_POINTS_NAME_STRING, pointSet, 1);
#endif

    if(onlyPoints){
        return;
    }


    computeConeRays();
    size_t coneRaysSize = coneRays.size();
    assert(coneRaysSize == 6);
    

    computeConvexHull(pointSet, convexHullList);

#if (MY_VERBOSE)    

/* std::ostream_iterator< MyPoint_2 > out( std::cout, " " );
 std::copy(pointSet.begin(), pointSet.end(), out); */
    cout << "Convex Hull\n";
    for(const auto pt : convexHullList)
    {
        cout << " " << pt << endl;
    }
#endif     

    vector< reference_wrapper<const MyPoint_2> > inputPtVector;
    for(set<MyPoint_2>::const_iterator it = pointSet.begin(); it != pointSet.end(); ++it)
    {
        inputPtVector.push_back(*it);
    }
#if (DEBUG_W_MY_BOUNDING_BOX)    
    vector<MyPoint_2> clippingPolygonList;
    clippingPolygonList.push_back(MyPoint_2(20.0,-2.0));
    clippingPolygonList.push_back(MyPoint_2(20.0,34.0));
    clippingPolygonList.push_back(MyPoint_2(-10.0,34.0));
    clippingPolygonList.push_back(MyPoint_2(-10.0,-2.0)); 
#else
    const vector< MyPoint_2 > &clippingPolygonList = convexHullList;
#endif
    MyNef_polyhedron clippingPolygon(clippingPolygonList.begin(), clippingPolygonList.end(), MyNef_polyhedron::INCLUDED);
    MyOverlay_Traits overlayTraits;
    //The arrangements and their overlays are the same type...
    array<MyArrangement_2, 2> myTempArrangements;
    size_t arrIndex = 0;
    //Which is better as the outer loop? Maybe doing all 6 directions for one point at a time will lead to fewer crossings in the overlay...
    for(size_t i = 0; i < inputPtVector.size(); ++i)
    {
        const MyPoint_2 &cellOrigin = inputPtVector.at(i);
//        size_t originPtIndex;
//        //TODO throw an error or something instead
//        assert(findOriginIndex(cellOrigin, inputPtVector, originPtIndex)); 
#if (!DEBUG_OVERLAY)
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
#   if (MY_VERBOSE)               
            cout << "size of temp: " << sizeof(temp) << endl;
            cout << "size of arrangements: " << sizeof(myTempArrangements.at(0)) << ", " << sizeof(myTempArrangements.at(1)) << endl;                                 
#   endif            
        }
#else           
            //https://doc.cgal.org/latest/Arrangement_on_surface_2/group__PkgArrangement2Funcs.html#ga339cdba93f54001be303595689002396
            //Self-overlay is not supported, so we need two temp arrangements to alternate between, and a reference to the solution...
            //https://doc.cgal.org/latest/Arrangement_on_surface_2/Arrangement_on_surface_2_2overlay_unbounded_8cpp-example.html#a6
            OrientedDirichletCell temp(coneRays.at(0), coneRays.at(1), cellOrigin, i, inputPtVector, clippingPolygon);
            overlay( myTempArrangements.at(arrIndex % 2), temp.getCellArrangement(), myTempArrangements.at((arrIndex + 1) % 2), overlayTraits);
            ++arrIndex;
            OrientedDirichletCell tempB(coneRays.at(1), coneRays.at(2), cellOrigin, i, inputPtVector, clippingPolygon);
            overlay( myTempArrangements.at(arrIndex % 2), tempB.getCellArrangement(), myTempArrangements.at((arrIndex + 1) % 2), overlayTraits);
            ++arrIndex;     
            cout << "size of temp and tempB: " << sizeof(temp) << ", " << sizeof(tempB) << endl;
            cout << "size of arrangements: " << sizeof(myTempArrangements.at(0)) << ", " << sizeof(myTempArrangements.at(1)) << endl;            
#endif                
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

    myEMST.addPointSet(pointSet);
    
    return;
} //constructor

void ComputationResult::computeConeRays()
{
    //https://doc.cgal.org/latest/Cone_spanners_2/index.html  
    MyDirection_2 initialDirection = MyDirection_2(1, 0);
    // construct the functor
    Compute_cone_boundaries_2<MyKernel> cones;    
    cones(6, initialDirection, coneRays.begin());

    return;
}



string ComputationResult::outputResultToJSONString() const
{
    ostringstream sStream;
    sStream << "{\n";  
    sStream << wrapStringInQuotes(RAND_SEED_NAME_STRING) << ": \"" << randSeed << "\"";  
    sStream << "," << endl;
    sStream << pointSetToJSONString(INPUT_POINTS_NAME_STRING, pointSet);
    if(!onlyPoints){
        sStream << "," << endl;
        sStream << vertexIndicesToJSONString(CONVEX_HULL_POINTS_NAME_STRING, convexHullList, pointSet);
        sStream << "," << endl;
        sStream << arrangementToJSONString();
    }
    sStream << "\n}";

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
    sStream << "]" << endl; 
    
    return sStream.str();
}

string ComputationResult::pointVectorToJSONString(string name, const vector<MyPoint_2> &myColl, int tabLevel) const
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
    sStream << "]" << endl; 
    
    return sStream.str();
}

string ComputationResult::vertexIndicesToJSONString(string name, const vector<MyPoint_2> &myColl, const set<MyPoint_2> &myPtSet, int tabLevel) const
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
    sStream << "]" << endl; 
    
    return sStream.str();
}

//Assumption: no duplicate points. The "std::set" insertion can't reliably tell if there are doubles (I've experienced this...)
void ComputationResult::insertArrangementPointsIntoPointSet(set<MyPoint_2> &arrPoints) const
{
    for(auto it = resultODCArrangement.vertices_begin(); it != resultODCArrangement.vertices_end(); ++it)
    {
        if (! it->is_isolated() && ! it->is_at_open_boundary())
        {
            arrPoints.insert(it->point());
        }
    }    
    return;
}

string ComputationResult::arrangementFaceToJSONString(string faceName, const MyArrangement_2::Face_const_iterator fit, 
                                                        const set<MyPoint_2> &myPtSet, int tabLevel) const
{

    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << "{ " << wrapStringInQuotes(ARR_IND_FACE_NAME_STRING) << ": {";
    
    sStream << "\"name\": " << wrapStringInQuotes(faceName) << " ," << endl;
    ////////////////////
    sStream << insertTabs(tabLevel + 1);
    sStream << wrapStringInQuotes(ARR_FACE_CCW_V_INDICES_NAME_STRING) <<  ": [";

    MyArrangement_2::Ccb_halfedge_const_circulator circ = fit->outer_ccb();
    MyArrangement_2::Ccb_halfedge_const_circulator curr = circ;
    MyArrangement_2::Halfedge_const_handle heEnd = circ; //don't know if this does an implicit conversion; best it do it here, not in the expr
    do {
        MyArrangement_2::Halfedge_const_handle he = curr;
        size_t index = 0;
        assert(findPointIndex(he->target()->point(), myPtSet, index));
        sStream << "{\"index\":\"" << index << "\"}";
        if(he->next() != heEnd){
            sStream << ",\n";
            sStream << insertTabs(tabLevel + 3);
        } else {
            sStream << "\n";
        }        
    } while (++curr != circ);

    sStream << insertTabs(tabLevel+1);
    sStream << "]," << endl;
    //////////////////////

    sStream << insertTabs(tabLevel + 1);
    sStream << wrapStringInQuotes(ARR_OODC_INPUT_SITE_INDICES_NAME_STRING) <<  ": [";

    //static cast instead?
    MyFaceData myFaceStruct = (MyFaceData)(fit->data());
    auto endIt = end(myFaceStruct.myInputPointIndices);
    for (auto it = begin(myFaceStruct.myInputPointIndices); it != endIt; ++it) {
        sStream << "{\"index\":\"" << *it << "\"}"; 
        if(next(it) != endIt){
            sStream << ",\n";
            sStream << insertTabs(tabLevel + 3);
        } else {
            sStream << "\n";
        }
    }             
    sStream << insertTabs(tabLevel + 1);
    sStream << "]" << endl;
    //////////////////////

    sStream << insertTabs(tabLevel);
    sStream << "} }" << endl; 
    
    return sStream.str();
}

string ComputationResult::arrangementToJSONString(int tabLevel) const
{
    set<MyPoint_2> arrPoints;
    insertArrangementPointsIntoPointSet(arrPoints);

    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(ARR_NAME_STRING) << ": {";

    sStream << pointSetToJSONString(ARR_POINTS_NAME_STRING, arrPoints, tabLevel + 1);

    sStream << insertTabs(tabLevel);
    sStream << "," << endl;

    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(ARR_FACES_NAME_STRING) << ": [";


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
            sStream << arrangementFaceToJSONString(faceSStream.str(), fit, arrPoints, tabLevel + 1);
            if(next(fit) != endFit) 
            {
                sStream << insertTabs(tabLevel + 1);
                sStream << "," << endl;
            }
        }
    }

    sStream << insertTabs(tabLevel);
    sStream << "]" << endl; 

    sStream << insertTabs(tabLevel);
    sStream << "}" << endl; 
    
    return sStream.str();
}

