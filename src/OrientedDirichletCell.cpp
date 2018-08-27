#include "OrientedDirichletCell.hpp"

OrientedDirichletCell::OrientedDirichletCell(const MyDirection_2 &dirA,
                                             const MyDirection_2 &dirB,
                                             const MyPoint_2 &cellOrigin,
                                             const size_t &originPtIndex,
                                             const vector<reference_wrapper<const MyPoint_2>> &inputPointSet,
                                             const MyNef_polyhedron &clippingPolygon) : firstDir(Nef_Direction(dirA.dx(), dirA.dy())),
                                                                                             secondDir(Nef_Direction(-dirB.dx(), -dirB.dy())),
                                                                                             cellOriginPoint(Nef_Point(cellOrigin.x(), cellOrigin.y()))
{

#if (MY_VERBOSE)
    cout << "input pt is: " << cellOrigin.x() << ", " << cellOrigin.y() << endl;
#endif

//    MyNef_polyhedron clippingPolygon(clippingPolygonList.begin(), clippingPolygonList.end(), MyNef_polyhedron::INCLUDED);
    MyNef_polyhedron myCell;
    computeCell(myCell, clippingPolygon, inputPointSet, originPtIndex);

    Nef_Explorer myExplorer = myCell.explorer();

    extractArrangement(odcArr, myExplorer, originPtIndex);
    
#if (MY_VERBOSE)
    cout << odcArr << endl;
    for (auto fit = odcArr.faces_begin(); fit != odcArr.faces_end(); ++fit)
    {
        if (!fit->is_unbounded())
        {
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
}

void OrientedDirichletCell::computeCell(MyNef_polyhedron &result, const MyNef_polyhedron &clippingPolygon,
                                        const vector<reference_wrapper<const MyPoint_2>> &inputPointSet, size_t originPtIndex)
{
#if ( ! BUILD_ODCELL_BY_DIFFERENCES)   
    Nef_Line oppositeFirstDirLine = Nef_Line(cellOriginPoint, firstDir).opposite();
    Nef_Line oppositeSecondDirLine = Nef_Line(cellOriginPoint, secondDir).opposite();
    auto cellBoundaryInclusion = MyNef_polyhedron::INCLUDED;
#   if (EXCLUDE_CELL_BOUNDARY)  
        cellBoundaryInclusion = MyNef_polyhedron::EXCLUDED;
#   endif
    MyNef_polyhedron stencil(oppositeFirstDirLine, cellBoundaryInclusion);
    stencil = stencil.join(MyNef_polyhedron(oppositeSecondDirLine, cellBoundaryInclusion));
    for (size_t i = 0; i < inputPointSet.size(); ++i)
    {
        if (i == originPtIndex)
        {
            continue;
        }
        const MyPoint_2 &otherSite = inputPointSet.at(i);
        Nef_Point otherSitePoint(otherSite.x(), otherSite.y());
        MyNef_polyhedron polygonToRemove(Nef_Line(otherSitePoint, firstDir),
                                         cellBoundaryInclusion);
        polygonToRemove = polygonToRemove.intersection(MyNef_polyhedron(Nef_Line(otherSitePoint, secondDir),
                                                                        cellBoundaryInclusion));
        //https://doc.cgal.org/latest/Arrangement_on_surface_2/Arrangement_on_surface_2_2dual_lines_8cpp-example.html
        MyKernel ker;
        MyPoint_2 midPoint = ker.construct_midpoint_2_object()(cellOriginPoint, otherSite);
        Nef_Point nefMP(midPoint.x(), midPoint.y());
        Nef_Line supportLine(cellOriginPoint, otherSitePoint);
        Nef_Line perpBi = supportLine.perpendicular(nefMP);
        //We assume the points are different, so neither point lies on the bisector...
        if (!perpBi.has_on_positive_side(otherSitePoint))
        {
            perpBi = perpBi.opposite();
        }
        polygonToRemove = polygonToRemove.intersection(MyNef_polyhedron(perpBi, cellBoundaryInclusion));
        stencil = stencil.join(polygonToRemove);
    }

    stencil = stencil.complement();
    result = stencil.intersection(clippingPolygon);

#else

    Nef_Line firstDirLine = Nef_Line(cellOriginPoint, firstDir);
    Nef_Line secondDirLine = Nef_Line(cellOriginPoint, secondDir);
    auto cellBoundaryInclusion = MyNef_polyhedron::INCLUDED;
    MyNef_polyhedron stencil(firstDirLine, cellBoundaryInclusion);
    stencil = stencil.intersection(MyNef_polyhedron(secondDirLine, cellBoundaryInclusion));
    //does clipping here speed it up?
    stencil = stencil.intersection(clippingPolygon);
    for (size_t i = 0; i < inputPointSet.size(); ++i)
    {
        if (i == originPtIndex)
        {
            continue;
        }
        const MyPoint_2 &otherSite = inputPointSet.at(i);
        Nef_Point otherSitePoint(otherSite.x(), otherSite.y());
        MyNef_polyhedron polygonToRemove(Nef_Line(otherSitePoint, firstDir),
                                         cellBoundaryInclusion);
        polygonToRemove = polygonToRemove.intersection(MyNef_polyhedron(Nef_Line(otherSitePoint, secondDir),
                                                                        cellBoundaryInclusion));
        //https://doc.cgal.org/latest/Arrangement_on_surface_2/Arrangement_on_surface_2_2dual_lines_8cpp-example.html
        MyKernel ker;
        MyPoint_2 midPoint = ker.construct_midpoint_2_object()(cellOriginPoint, otherSite);
        Nef_Point nefMP(midPoint.x(), midPoint.y());
        Nef_Line supportLine(cellOriginPoint, otherSitePoint);
        Nef_Line perpBi = supportLine.perpendicular(nefMP);
        //We assume the points are different, so neither point lies on the bisector...
        if (!perpBi.has_on_positive_side(otherSitePoint))
        {
            perpBi = perpBi.opposite();
        }
        polygonToRemove = polygonToRemove.intersection(MyNef_polyhedron(perpBi, cellBoundaryInclusion));
        //should i bother to clip the polygonToRemove?

        stencil = stencil.difference(polygonToRemove);
    }

#endif

    return;
}

void OrientedDirichletCell::extractArrangement(MyArrangement_2 &result, const Nef_Explorer &myExplorer, const size_t &originIndex)
{
    //According to info in a cgal example (which is not in the docs), the first face is the infinite one
    auto fit = myExplorer.faces_begin();
#if (MY_VERBOSE)
    //You really have to dig to find it, but apparently mark signifies set exclusion/inclusion
    cout << "face 'mark': " << myExplorer.mark(fit) << endl;
    cout << "num faces: " << myExplorer.number_of_faces() << endl;
#endif
    //https://doc.cgal.org/latest/Nef_2/Nef_2_2nef_2_polylines_8cpp-example.html#_a2
    //    assert(myExplorer.number_of_faces() == 2); //should be 2 but it's not...
    while (!myExplorer.mark(fit) && (fit != myExplorer.faces_end()))
    {
#if (MY_VERBOSE)
        auto hafc = myExplorer.face_cycle(fit), done(hafc);
        if (hafc != nullptr)
        {
            do
            {
                auto sourceVH = myExplorer.source(hafc);
                auto targetVH = myExplorer.target(hafc);
                if (myExplorer.is_standard(sourceVH) && myExplorer.is_standard(targetVH))
                {
                    cout << myExplorer.point(sourceVH) << ", " << myExplorer.point(targetVH) << endl;
                }
                ++hafc;
            } while (hafc != done);
        }
#endif
        ++fit;
    }

    if (fit != myExplorer.faces_end())
    {
        auto hafc = myExplorer.face_cycle(fit), done(hafc), nextHC(hafc);
        if (hafc != nullptr)
        {
            ++nextHC;
            vector<MyPoint_2> boundaryList;
            bool firstTime = true;
            do
            {
                auto sourceVH = myExplorer.source(hafc);
                auto targetVH = myExplorer.target(hafc);
                if (myExplorer.is_standard(sourceVH) && myExplorer.is_standard(targetVH))
                {
#if (MY_VERBOSE)
                    cout << myExplorer.point(sourceVH) << ", " << myExplorer.point(targetVH) << endl;
#endif
                    if (firstTime)
                    {
                        //auto sourcePt = sourceVH->point();
                        auto sourcePt = myExplorer.point(sourceVH);
                        boundaryList.push_back(MyPoint_2(sourcePt.x(), sourcePt.y()));
                        firstTime = false;
                    }
                    if (nextHC != done)
                    {
                        //auto targetPt = targetVH->point();
                        auto targetPt = myExplorer.point(targetVH);
                        boundaryList.push_back(MyPoint_2(targetPt.x(), targetPt.y()));
                    }
                }
                ++hafc;
                ++nextHC;
            } while (hafc != done);
            vector<MyArrangement_2::Vertex_handle> vertexHandles;
            MyArrangement_2::Face_handle unboundedFace = result.unbounded_face();
            for (MyPoint_2 pt : boundaryList)
            {
                vertexHandles.push_back(result.insert_in_face_interior(pt, unboundedFace));
            }
            auto endBLIt = boundaryList.end();
            size_t handleIndex = 0, nextHandleIndex = 1;
            for (auto it = boundaryList.begin(); it != endBLIt; ++it)
            {
                MyPoint_2 first = *it;
                MyPoint_2 second;
                if (next(it) == endBLIt)
                {
                    second = *(boundaryList.begin());
                    nextHandleIndex = 0;
                }
                else
                {
                    second = *(it + 1);
                }
                result.insert_at_vertices(MySegment_2(first, second), vertexHandles.at(handleIndex), vertexHandles.at(nextHandleIndex));
                ++handleIndex;
                ++nextHandleIndex;
            }
        } /* if(hafc != nullptr) */
    }     /* if(fit != myExplorer.faces_end()) */

    size_t numFaces = result.number_of_faces();
    assert(numFaces <= 2);

    for (auto fit = result.faces_begin(); fit != result.faces_end(); ++fit)
    {
        if (!fit->is_unbounded())
        {
            //unclear if the struct already exists...
            MyFaceData temp;
            temp.myInputPointIndices.insert(originIndex);
            fit->set_data(temp);
            //in theory we should only be doing this to one face anyway, so break.
            break;
        }
    }

    return;
}

const MyArrangement_2 & OrientedDirichletCell::getCellArrangement() const
{
    return odcArr;
}
