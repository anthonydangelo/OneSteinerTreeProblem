#include "OrientedDirichletCell.hpp"

OrientedDirichletCell::OrientedDirichletCell(const MyDirection_2 &dirA,
                                             const MyDirection_2 &dirB,
                                             const MyPoint_2 &cellOrigin,
                                             const vector<reference_wrapper<const MyPoint_2>> &inputPointSet,
                                             const vector<MyPoint_2> &clippingPolygonList)
{
    MyNef_polyhedron myPolygon(clippingPolygonList.begin(), clippingPolygonList.end(), MyNef_polyhedron::INCLUDED);
    Nef_Point sitePoint(cellOrigin.x(), cellOrigin.y());
    myPolygon = myPolygon.intersection(MyNef_polyhedron(Nef_Line(sitePoint, Nef_Direction(dirA.dx(), dirA.dy())), MyNef_polyhedron::INCLUDED));
    myPolygon = myPolygon.intersection(MyNef_polyhedron(Nef_Line(sitePoint, Nef_Direction(-dirB.dx(), -dirB.dy())), MyNef_polyhedron::INCLUDED));

    Nef_Explorer myExplorer = myPolygon.explorer();

    size_t originPtIndex;
    //TODO throw an error or something instead
    assert(findOriginIndex(cellOrigin, inputPointSet, originPtIndex));
#if (MY_VERBOSE)
    cout << "input pt is: " << cellOrigin.x() << ", " << cellOrigin.y() << endl;
#endif
    MyArrangement_2 odcArr = extractArrangement(myExplorer, originPtIndex);
#if (1)        
    cout << odcArr << endl;
    for(auto fit = odcArr.faces_begin(); fit != odcArr.faces_end(); ++fit)
    {
        if(!fit->is_unbounded())
        {
            //static cast instead?
            MyFaceData tempStruct = (MyFaceData)(fit->data());
            cout << "face data contains the following indices: [" << endl;
            for(size_t temp : tempStruct.myInputPointIndices)
            {
                cout << temp << endl;
            }
            cout << "]" << endl;            
        }
    }
#endif     

}

bool OrientedDirichletCell::findOriginIndex(const MyPoint_2 &cellOrigin,
                                            const vector<reference_wrapper<const MyPoint_2>> &inputPointSet,
                                            size_t &resultIndex) 
{
    resultIndex = 0;
    for(auto it = inputPointSet.begin(); it != inputPointSet.end(); ++it, ++resultIndex)
    {
        if(pointsAreTooClose(*it, cellOrigin))
        {
            return true;
        }
    }
    return false;
}


MyArrangement_2 OrientedDirichletCell::extractArrangement(const Nef_Explorer &myExplorer, const size_t &originIndex) const
{
    MyArrangement_2 result;
    //According to info in a cgal example (which is not in the docs), the first face is the infinite one
    auto fit = myExplorer.faces_begin();
#if (MY_VERBOSE)
    //You really have to dig to find it, but apparently mark signifies set exclusion/inclusion
    cout << "face 'mark': " << myExplorer.mark(fit) << endl;
    cout << "num faces: " << myExplorer.number_of_faces() <<endl;    
#endif
//https://doc.cgal.org/latest/Nef_2/Nef_2_2nef_2_polylines_8cpp-example.html#_a2
//    assert(myExplorer.number_of_faces() == 2); //should be 2 but it's not...
    while(!myExplorer.mark(fit) && (fit != myExplorer.faces_end()))
    {
#if (MY_VERBOSE)   
        auto hafc = myExplorer.face_cycle(fit), done(hafc);
        if(hafc != nullptr)
        {
            do{
                auto sourceVH = myExplorer.source(hafc);
                auto targetVH = myExplorer.target(hafc);
                if(myExplorer.is_standard(sourceVH) && myExplorer.is_standard(targetVH))  
                {   
                    cout << myExplorer.point(sourceVH) << ", " << myExplorer.point(targetVH) << endl;
                }
                ++hafc;
            }while(hafc != done);
        }
#endif          
        ++fit;
    }

    if(fit != myExplorer.faces_end())
    {
        auto hafc = myExplorer.face_cycle(fit), done(hafc), nextHC(hafc);
        if(hafc != nullptr)
        {
            ++nextHC;
            vector< MyPoint_2 > boundaryList;
            bool firstTime = true;
            do
            {
                auto sourceVH = myExplorer.source(hafc);
                auto targetVH = myExplorer.target(hafc);
                if(myExplorer.is_standard(sourceVH) && myExplorer.is_standard(targetVH))  
                {                
#if (MY_VERBOSE)        
                    cout << myExplorer.point(sourceVH) << ", " << myExplorer.point(targetVH) << endl;
#endif        
                    if(firstTime)
                    {
                        //auto sourcePt = sourceVH->point();
                        auto sourcePt = myExplorer.point(sourceVH);
                        boundaryList.push_back(MyPoint_2(sourcePt.x(), sourcePt.y()) );
                        firstTime = false;
                    }
                    if(nextHC != done)
                    {
                        //auto targetPt = targetVH->point();
                        auto targetPt = myExplorer.point(targetVH);
                        boundaryList.push_back(MyPoint_2(targetPt.x(), targetPt.y()));
                    }
                }
                ++hafc;
                ++nextHC;
            }while(hafc != done);
            vector< MyArrangement_2::Vertex_handle > vertexHandles;
            MyArrangement_2::Face_handle    unboundedFace = result.unbounded_face();
            for(MyPoint_2 pt : boundaryList)
            {
                vertexHandles.push_back( result.insert_in_face_interior(pt, unboundedFace) );
            }
            auto endBLIt = boundaryList.end();
            size_t handleIndex = 0, nextHandleIndex = 1;
            for(auto it = boundaryList.begin(); it != endBLIt; ++it)
            {
                MyPoint_2 first = *it;
                MyPoint_2 second;
                if(next(it) == endBLIt)
                {
                    second = *(boundaryList.begin());
                    nextHandleIndex = 0;
                } else {
                    second = *(it + 1);
                }
                result.insert_at_vertices(MySegment_2(first, second), vertexHandles.at(handleIndex), vertexHandles.at(nextHandleIndex));
                ++handleIndex;
                ++nextHandleIndex;
            }
        } /* if(hafc != nullptr) */
    } /* if(fit != myExplorer.faces_end()) */

    assert(result.number_of_faces() == 2);

    for(auto fit = result.faces_begin(); fit != result.faces_end(); ++fit)
    {
        if( ! fit->is_unbounded() )
        {
            //unclear if the struct already exists...
            MyFaceData temp;
            temp.myInputPointIndices.insert(originIndex);
            fit->set_data(temp);
            //in theory we should only be doing this to one face anyway, so break.
            break;
        }
    }

    return result;
}
