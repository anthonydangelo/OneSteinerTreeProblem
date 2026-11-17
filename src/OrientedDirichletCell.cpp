#include "OrientedDirichletCell.hpp"

//See OneSteinerTreeProblem.cpp for project notes

namespace One_St_Pt_GPA
{
    // secondDir is dirB negated on purpose. A Nef_polyhedron_2 built from a
    // line is the half-plane to the line's left. Through the cell's origin,
    // the left side of the line with direction dirA is the counter-clockwise
    // side of dirA, and the left side of the line with direction -dirB is the
    // clockwise side of dirB. Intersecting the two gives the 60-degree wedge
    // from dirA counter-clockwise to dirB (the next cone boundary).    
    OrientedDirichletCell::OrientedDirichletCell(const MyDirection_2 & dirA,
                                                const MyDirection_2 & dirB,
                                                const MyPoint_2 & cellOrigin,
                                                size_t originPtIndex,
                                                const vector< MyPoint_2 > & inputPointSet,
                                                const MyNef_polyhedron & clippingPolygon) : firstDir(Nef_Direction(dirA.dx(), dirA.dy())),
                                                                                                secondDir(Nef_Direction(-dirB.dx(), -dirB.dy())),
                                                                                                cellOriginPoint(Nef_Point(cellOrigin.x(), cellOrigin.y()))
    {

#   if (MY_VERBOSE)
        cout << "input pt is: " << cellOrigin.x() << ", " << cellOrigin.y() << endl;
#   endif

        MyNef_polyhedron myCell;
        computeCell(myCell, clippingPolygon, inputPointSet, originPtIndex);

        Nef_Explorer myExplorer = myCell.explorer();

        extractArrangement(odcArr, myExplorer, originPtIndex);
        
#   if (MY_VERBOSE)
        cout << odcArr << endl;
        for (MyArrangement_2::Face_const_iterator fit = odcArr.faces_begin(); fit != odcArr.faces_end(); ++fit)
        {
            if (!fit->is_unbounded())
            {
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
    }

    /*
    ** Computes the oriented Dirichlet cell of site `originPtIndex` within the
    ** 60 degree cone defined by `firstDir` and `secondDir`, clipped to the convex
    ** hull polygon.
    ** 
    ** There are two methods implemented. The method used is toggled by a 
    ** compiler flag in "1ST-Constants-Utilities.h"
    ** Setting BUILD_ODCELL_BY_DIFFERENCES = 1 seems more practically efficient.
    **
    ** A future work would be to replace this code with an algorithm for computing
    ** the oriented Voronoi / Dirichlet diagram. See project notes.
    **
    ** Method: 
    **  (! BUILD_ODCELL_BY_DIFFERENCES):
    **      This method builds everything that is NOT the cell, then takes
    **      the complement.    
    **      -- start with the union of two halfplanes whose complement
    **      is the cone apexed at the origin (i.e., the point dictated by originPtIndex)
    **      -- then, for each other site, add (union) the region where
    **      that second site is also a candidate (based on the cone apexed 
    **      at that second site) and is closer. 
    **      To do this, build a cone apexed at that second point, then compute
    **      the perpendicular bisector of the origin point and this second point.
    **      Keep only the part of the second site's cone that lies on its side of 
    **      the bisector (i.e., closer to the second site than to the origin).
    **      -- finally, we take the complement of this union 
    **      (i.e., the cell we ARE looking for) and clip it to the convex hull.
    **
    **  BUILD_ODCELL_BY_DIFFERENCES:
    **      -- start with the intersection of two half-planes (the cone apexed at 
    **      the point dictated by originPtIndex) and clip to the convex hull
    **      -- call this the origin's region 
    **      -- then, for each other site, subtract the region where that
    **      site is also a candidate (based on its own cone) and is closer. 
    **      This is done by building a cone apexed at that second point.
    **      We then compute the perpendicular bisector of the origin point and this second point.
    **      We use the bisector to keep the part of the second cone that lies on the second site's 
    **      side of the bisector (i.e., the part that is closer to this second
    **      site than to the origin): in this region, the second site is a nearer candidate in 
    **      this cone direction. Finally, we subtract this shape (what's left of the second cone) from
    **      the origin's region.
    */
    void OrientedDirichletCell::computeCell(MyNef_polyhedron & result, const MyNef_polyhedron & clippingPolygon,
                                            const vector< MyPoint_2 > & inputPointSet, size_t originPtIndex)
    {
#   if ( ! BUILD_ODCELL_BY_DIFFERENCES)   
        //build odcell by complementing
        const Nef_Line oppositeFirstDirLine = Nef_Line(cellOriginPoint, firstDir).opposite();
        const Nef_Line oppositeSecondDirLine = Nef_Line(cellOriginPoint, secondDir).opposite();
        auto cellBoundaryInclusion = MyNef_polyhedron::INCLUDED;
#      if (EXCLUDE_CELL_BOUNDARY)  
            cellBoundaryInclusion = MyNef_polyhedron::EXCLUDED;
#      endif
        MyNef_polyhedron stencil(oppositeFirstDirLine, cellBoundaryInclusion);
        stencil = stencil.join(MyNef_polyhedron(oppositeSecondDirLine, cellBoundaryInclusion));
        for (size_t i = 0; i < inputPointSet.size(); ++i)
        {
            if (i == originPtIndex)
            {
                continue;
            }
            const MyPoint_2 & otherSite = inputPointSet[i];
            const Nef_Point otherSitePoint(otherSite.x(), otherSite.y());
            MyNef_polyhedron polygonToRemove(Nef_Line(otherSitePoint, firstDir),
                                            cellBoundaryInclusion);
            polygonToRemove = polygonToRemove.intersection(MyNef_polyhedron(Nef_Line(otherSitePoint, secondDir),
                                                                            cellBoundaryInclusion));
            //https://doc.cgal.org/latest/Arrangement_on_surface_2/Arrangement_on_surface_2_2dual_lines_8cpp-example.html
            MyKernel ker;
            const MyPoint_2 midPoint = ker.construct_midpoint_2_object()(cellOriginPoint, otherSite);
            const Nef_Point nefMP(midPoint.x(), midPoint.y());
            const Nef_Line supportLine(cellOriginPoint, otherSitePoint);
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

#   else

        const Nef_Line firstDirLine = Nef_Line(cellOriginPoint, firstDir);
        const Nef_Line secondDirLine = Nef_Line(cellOriginPoint, secondDir);
        auto cellBoundaryInclusion = MyNef_polyhedron::INCLUDED;
        MyNef_polyhedron stencil(firstDirLine, cellBoundaryInclusion);
        stencil = stencil.intersection(MyNef_polyhedron(secondDirLine, cellBoundaryInclusion));
        // Clip the cone to the convex hull before subtracting (the optimal Steiner
        // point lies inside the hull; see the file header). Clipping at the end
        // instead would give the same cell; which order is faster hasn't been measured.        
        stencil = stencil.intersection(clippingPolygon);
        for (size_t i = 0; i < inputPointSet.size(); ++i)
        {
            if (i == originPtIndex)
            {
                continue;
            }
            const MyPoint_2 & otherSite = inputPointSet[i];
            const Nef_Point otherSitePoint(otherSite.x(), otherSite.y());
            MyNef_polyhedron polygonToRemove(Nef_Line(otherSitePoint, firstDir),
                                            cellBoundaryInclusion);
            polygonToRemove = polygonToRemove.intersection(MyNef_polyhedron(Nef_Line(otherSitePoint, secondDir),
                                                                            cellBoundaryInclusion));
            //https://doc.cgal.org/latest/Arrangement_on_surface_2/Arrangement_on_surface_2_2dual_lines_8cpp-example.html
            MyKernel ker;
            const MyPoint_2 midPoint = ker.construct_midpoint_2_object()(cellOriginPoint, otherSite);
            const Nef_Point nefMP(midPoint.x(), midPoint.y());
            const Nef_Line supportLine(cellOriginPoint, otherSitePoint);
            Nef_Line perpBi = supportLine.perpendicular(nefMP);
            //We assume the points are different, so neither point lies on the bisector...
            if (!perpBi.has_on_positive_side(otherSitePoint))
            {
                perpBi = perpBi.opposite();
            }
            polygonToRemove = polygonToRemove.intersection(MyNef_polyhedron(perpBi, cellBoundaryInclusion));
            // No need to clip this to the hull: the difference only removes the part
            // that overlaps the stencil, which is already clipped.

            stencil = stencil.difference(polygonToRemove);
        }

        result = stencil;
#   endif

        return;
    }

    void OrientedDirichletCell::extractArrangement(MyArrangement_2 & result, const Nef_Explorer & myExplorer, size_t originIndex)
    {
        // Nef_2 with an extended kernel (Extended_cartesian) adds a frame: a box with
        // "infinitely far" corners that stands in for infinity. So the explorer has 3
        // faces for a bounded cell, not 2:
        //  - the face outside the frame: the map's unbounded face (CGAL's example calls
        //    it "the infinite one"); never part of the set; always first;
        //  - the face just inside the frame, outside the cell: it represents the rest
        //    of the (unbounded) plane; not part of the set (unmarked);
        //  - the cell itself (marked).
        // An empty cell leaves only the first two. The "first face" ordering is shown in
        // CGAL's Nef_2 polylines example (linked below); I couldn't find it stated in the
        // reference manual, so this code relies on it as observed.
        auto fit = myExplorer.faces_begin();
#   if (MY_VERBOSE)
        // mark(f) is true if face f belongs to the set (the cell).        
        cout << "face 'mark': " << myExplorer.mark(fit) << endl;
        cout << "num faces: " << myExplorer.number_of_faces() << endl;
#   endif
        //https://doc.cgal.org/latest/Nef_2/Nef_2_2nef_2_polylines_8cpp-example.html#_a2
        // We skip unmarked faces and take the marked one; a cell is connected, so
        // there is at most one.
#   ifndef NDEBUG
        size_t markedFaces = 0;
        for (auto f = myExplorer.faces_begin(); f != myExplorer.faces_end(); ++f)
        {
            if (myExplorer.mark(f)) { ++markedFaces; }
        }
        assert(markedFaces <= 1);
#   endif
        
        while (!myExplorer.mark(fit) && (fit != myExplorer.faces_end()))
        {
#   if (MY_VERBOSE)
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
#   endif
            ++fit;
        }

        if (fit != myExplorer.faces_end())
        {
            auto hafc = myExplorer.face_cycle(fit), done(hafc), nextHC(hafc);
            if (hafc != nullptr)
            {
                ++nextHC;
                vector< MyPoint_2> boundaryList;
                bool firstTime = true;
                do
                {
                    auto sourceVH = myExplorer.source(hafc);
                    auto targetVH = myExplorer.target(hafc);
                    if (myExplorer.is_standard(sourceVH) && myExplorer.is_standard(targetVH))
                    {
#   if (MY_VERBOSE)
                        cout << myExplorer.point(sourceVH) << ", " << myExplorer.point(targetVH) << endl;
#   endif
                        if (firstTime)
                        {
                            // Use the explorer's point(), not vh->point(): the latter
                            // is an extended-kernel point; point() converts it to a standard one.
                            auto sourcePt = myExplorer.point(sourceVH);
                            boundaryList.emplace_back(sourcePt.x(), sourcePt.y());
                            firstTime = false;
                        }
                        if (nextHC != done)
                        {
                            auto targetPt = myExplorer.point(targetVH);
                            boundaryList.emplace_back(targetPt.x(), targetPt.y());
                        }
                    }
                    ++hafc;
                    ++nextHC;
                } while (hafc != done);
                vector< MyArrangement_2::Vertex_handle > vertexHandles;
                MyArrangement_2::Face_handle unboundedFace = result.unbounded_face();
                for (const MyPoint_2 & pt : boundaryList)
                {
                    vertexHandles.push_back(result.insert_in_face_interior(pt, unboundedFace));
                }
                auto endBLIt = boundaryList.end();
                size_t handleIndex = 0, nextHandleIndex = 1;
                for (auto it = boundaryList.begin(); it != endBLIt; ++it)
                {
                    const MyPoint_2 first = *it;
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
                // Faces start with default (empty) data; label the cell with its site.
                MyFaceData temp;
                temp.myInputPointIndices.insert(originIndex);
                fit->set_data(temp);
                // The cell's arrangement has exactly one bounded face, so stop here.
                break;                
            }
        }

        return;
    }

    const MyArrangement_2 & OrientedDirichletCell::getCellArrangement() const
    {
        return odcArr;
    }
} // namespace One_St_Pt_GPA