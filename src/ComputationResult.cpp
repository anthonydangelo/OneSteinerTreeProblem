#include "ComputationResult.hpp"

//See OneSteinerTreeProblem.cpp for project notes

namespace One_St_Pt_GPA
{
    ComputationResult::ComputationResult(int numInputPoints,
                                        int randomSeed,
                                        int inputGridLength,
                                        bool onlyProducePoints,
                                        const vector<MyPoint_2> & userPointList) : numPoints(numInputPoints),
                                                                                randSeed(randomSeed),
                                                                                gridLength(inputGridLength),
                                                                                onlyPoints(onlyProducePoints),
                                                                                rand(CGAL::Random(randSeed)),
                                                                                //https://doc.cgal.org/latest/Generator/classCGAL_1_1Random__points__in__square__2.html
                                                                                randPointGen(Point_generator(gridLength, rand)),
                                                                                coneRays(6)
    {

        preparePointSet(userPointList);

        if(onlyPoints)
        {
            return;
        }

        Utility_Functions::computeConvexHull(inputPtVector, convexHullList);

        computeOODC();

        computeMSTAndStPts();
        
        return;
    } //constructor


    void ComputationResult::preparePointSet(const vector< MyPoint_2 > & userPointList)
    {
        set< MyPoint_2 > pointSet;
        //sanitize input a bit
        for (const MyPoint_2 & pt : userPointList)
        {
            if (pointSet.size() <  static_cast< size_t >(numPoints))
            {
                pair< set< MyPoint_2 >::iterator, bool> ret;
                ret = pointSet.emplace(pt.x(), pt.y());
                if (ret.second == false)
                { // Goes to stderr, so it doesn't affect the JSON on stdout.
                    cerr << "tried to add an element that was already in there. size is " << pointSet.size() << endl;
                }
            }
            else
            {
                // More user points than numPoints: the rest are ignored.
                cerr << "WARNING: using only the first " << numPoints << " of the "
                     << userPointList.size() << " input points (see --numPoints)." << endl;                
                break;
            }
        }

        //https://doc.cgal.org/latest/Generator/index.html#GeneratorExample_2
        while (pointSet.size() < static_cast< size_t >(numPoints))
        {
            pair< set< MyPoint_2 >::iterator, bool> ret;
            MyPoint_2 temp = *randPointGen;
            ++randPointGen;
            bool inSet = false;
            for (const MyPoint_2 & pt : pointSet)
            {
                if (Utility_Functions::pointsAreTooClose(pt, temp))
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

        for (set< MyPoint_2 >::const_iterator it = pointSet.begin(); it != pointSet.end(); ++it)
        {
            inputPtVector.emplace_back(it->x(), it->y());
        }

        return;
    }

    /*
    ** Pre-condition: convexHullList is populated
    **
    ** Computes the Overlaid Oriented Dirichlet Cell (OODC) / 
    ** overlaid oriented Voronoi diagram arrangement by overlaying
    ** the individual cell arrangements. 
    ** For each input point and each of the
    ** 6 cone directions, a single ODC cell is computed (via Nef_2), converted
    ** to an arrangement, and overlaid onto the running result.
    ** The arrangement is clipped to the convex hull of the input sites
    ** which is OK since an optimal Steiner point must lie in the convex hull
    ** (see project notes).
    **
    ** The overlay functor merges the site-index sets of intersecting faces,
    ** so each face in the final arrangement knows which input sites "see" it.  
    **
    ** In the future, this code should be replaced by an implementation
    ** of an O(n log n) time algorithm for computing oriented Voronoi diagrams.
    ** See project notes.
    */
    void ComputationResult::computeOODC()
    {
        Utility_Functions::computeConeRays(MyDirection_2(1, 0), coneRays);
        size_t coneRaysSize = coneRays.size();
        assert(coneRaysSize == 6);
        
        // The convex hull is the clipping polygon for every cell.
        const vector< MyPoint_2 > & clippingPolygonList = convexHullList;

        MyNef_polyhedron clippingPolygon(clippingPolygonList.begin(), clippingPolygonList.end(), MyNef_polyhedron::INCLUDED);
        MyOverlay_Traits overlayTraits;
        //The arrangements and their overlays are the same type...
        CGAL::array< MyArrangement_2, 2 > myTempArrangements;
        size_t arrIndex = 0;

        // Swapping these loops alone (still one running arrangement) helps only
        // a little: once a second cone direction is added, every overlay
        // re-sweeps a growing arrangement of up to O(n^2) size, and re-processes
        // crossings already found. Keeping six running arrangements, one per
        // cone direction (each stays O(n), since same-direction cells only share
        // boundaries), and combining them with 5 overlays at the end finds each
        // crossing once and reduces the overlay phase from O(n^3 log n) to
        // O(n^2 log n); see the file header's Future work.       
        for(size_t i = 0; i < inputPtVector.size(); ++i)
        {
            const MyPoint_2 & cellOrigin = inputPtVector[i];

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
#   if (MY_VERBOSE)
        cout << myTempArrangements.at(arrIndex % 2) << endl;
        size_t faceIndex = 0;
        for (MyArrangement_2::Face_const_iterator fit = resultODCArrangement.faces_begin(); fit != resultODCArrangement.faces_end(); ++fit, ++faceIndex)
        {
            if (!fit->is_unbounded())
            {
                cout << "face # " << faceIndex << ": " << endl;
                Pretty_Print::print_ccb(fit->outer_ccb());
                const MyFaceData & tempStruct = fit->data();
                cout << "face data contains the following indices: [" << endl;
                for (size_t temp : tempStruct.myInputPointIndices)
                {
                    cout << temp << endl;
                }
                cout << "]" << endl;
            }
        }
#   endif    

        return;
    }

    void ComputationResult::computeMSTAndStPts()
    {
        DelaunayTriEMST myEMST(inputPtVector);
        origMST = myEMST.getEMSTData();

        vector< GeomMedianData > rawStPtList = computeStPtsForOODC();
        for (size_t ptIndex = 0; ptIndex < rawStPtList.size(); ++ptIndex)
        {
            const GeomMedianData & s = rawStPtList[ptIndex];
            //test its insertion into the pt set if it's not degenerate
            if(s.coincidesWithInputPt)
            {
                steinerPoints.emplace_back(s, origMST);
            }
            else
            {
                steinerPoints.emplace_back(s, myEMST.testPointInsertion(s.medPoint));
            }
        }
        // stable_sort keeps candidates with equal MST length in a deterministic order.
        std::stable_sort(steinerPoints.begin(), steinerPoints.end());
        //Now we have to properly assign the st pt indices so they print out correctly
        for (size_t stPtIndex = 0; stPtIndex < steinerPoints.size(); ++stPtIndex)
        {
            vector< MstEdgeEndpoints > & mstEdgePairData = steinerPoints[stPtIndex].mstData.mstEdgePointIndices;
            for (MstEdgeEndpoints & tempEdgeData : mstEdgePairData)
            {
                // At most one endpoint of an edge is the Steiner point.
                if (VertexRef::Kind::STEINER == tempEdgeData.first.kind)
                {
                    tempEdgeData.first.index = stPtIndex;
                }
                else if (VertexRef::Kind::STEINER == tempEdgeData.second.kind)
                {
                    tempEdgeData.second.index = stPtIndex;
                }
            }
        }

        return;
    }

    string ComputationResult::outputResultToJSONString() const
    {
        ostringstream sStream;
        sStream << "{\n";  
        sStream << Pretty_Print::wrapStringInQuotes(RAND_SEED_NAME_STRING) << ": \"" << randSeed << "\"";  
        sStream << ",\n";
        sStream << Pretty_Print::wrapStringInQuotes(GRID_LENGTH_NAME_STRING) << ": \"" << gridLength << "\"";  
        sStream << ",\n";    
        sStream << Pretty_Print::pointVectorToJSONString(INPUT_POINTS_NAME_STRING, inputPtVector);
        if(!onlyPoints){
            sStream << ",\n";
            sStream << Pretty_Print::vertexIndicesToJSONString(CONVEX_HULL_POINTS_NAME_STRING, convexHullList, inputPtVector);
            sStream << ",\n";
            sStream << Pretty_Print::arrangementToJSONString(resultODCArrangement);
            sStream << ",\n";
            sStream << Pretty_Print::mstDataToJSONString(origMST);
            sStream << ",\n";
            sStream << Pretty_Print::steinerPointsToJSONString(steinerPoints);
        }
        sStream << "\n}";

        return sStream.str();
    }

    /*
    ** Called by computeStPtsForOODC() with numComboPts \in {3,4}.
    **
    ** Enumerate all C(k,numComboPts) combinations of the given site indices 
    ** (k = siteIndices.size(), at most 6: one site per cone direction).
    ** Compute the geometric median of each combination.
    ** In addition to storing the results in the "results" in/out variable,
    ** store which combinations of input points we've worked on by updating
    ** "seenList".
    */
    void ComputationResult::computePotentialStPts(const vector< size_t > & siteIndices, const int numComboPts,
                                                vector< GeomMedianData > & results, set< vector< size_t>> & seenList) const
    {
        if (static_cast< size_t >(numComboPts) > siteIndices.size())
        {
            //TODO throw some kind of error...
            return;
        }
        CGAL::Combination_enumerator< vector< size_t>::const_iterator> combi(numComboPts, siteIndices.begin(), siteIndices.end());
        while (!combi.finished())
        {
            vector< size_t > siteComboIndices;
            for (int i = 0; i < numComboPts; ++i)
            {
                size_t myInd = *combi[i];
                siteComboIndices.push_back(myInd);
            }
            sort(siteComboIndices.begin(), siteComboIndices.end());
            pair< set< vector< size_t>>::iterator, bool> ret = seenList.insert(siteComboIndices);
            if (ret.second)
            {
                vector<MyPoint_2> siteTuples;
                for (int i = 0; i < numComboPts; ++i)
                {
                    const MyPoint_2 & tempSitePt = inputPtVector.at(siteComboIndices[i]);
                    siteTuples.emplace_back(tempSitePt.x(), tempSitePt.y());
                }
                results.push_back(GeomMedianFinder::computeGeomMedian(siteTuples, siteComboIndices));
            }
            ++combi;
        }
        return;
    }

    /*
    ** For each bounded face of the OODC arrangement, 
    ** use computePotentialStPts() to compute the geometric median of 
    ** each combination of C(k,3) and C(k,4) of the face's site indices (where k is the number of
    ** sites defining that face in the OODC arrangement). 
    ** Deduplicate via `seenList` (the same 3- or 4-point set may appear in
    ** multiple faces).
    ** Each face has at most 6 site indices (see MyFaceData), so this is
    ** constant work per face.
    */
    vector< GeomMedianData > ComputationResult::computeStPtsForOODC() const
    {
        vector< GeomMedianData > results;
        set< vector< size_t > > seenList;

        auto endFit = resultODCArrangement.faces_end();
        for (MyArrangement_2::Face_const_iterator fit = resultODCArrangement.faces_begin(); fit != endFit; ++fit)
        {
            if (!fit->is_unbounded())
            {
                const MyFaceData & myFaceStruct = fit->data();
                const vector< size_t > siteIndices(myFaceStruct.myInputPointIndices.begin(),
                                                    myFaceStruct.myInputPointIndices.end()); 
                // An optimal Steiner point has degree 3 or 4 (see the file header), so
                // only the 3- and 4-site subsets of the face's label give candidates.
                for (int subsetSize = 3; subsetSize <= 4; ++subsetSize)
                {
                    if (siteIndices.size() >= static_cast< size_t >(subsetSize))
                    {
                        computePotentialStPts(siteIndices, subsetSize, results, seenList);
                    }
                }                
            } // if (!fit->is_unbounded())
        } // for (MyArrangement_2::Face_const_iterator fit = resultODCArrangement.faces_begin(); fit != endFit; ++fit)

        return results;
    }
} // namespace One_St_Pt_GPA
