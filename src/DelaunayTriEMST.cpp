#include "DelaunayTriEMST.hpp"
// See DelaunayTriEMST.hpp for an overview of this class and its limitations.
// See OneSteinerTreeProblem.cpp for project notes


namespace One_St_Pt_GPA
{
    //https://doc.cgal.org/latest/BGL/index.html#title15 and cgal examples BGL_triangulation_2/emst.cpp
    DelaunayTriEMST::DelaunayTriEMST(const vector< MyPoint_2 > & pointSet) : is_finite(MyTriFilter(delaunayTri)),
                                                                        finiteDT(MyFinite_triangulation(delaunayTri, is_finite, is_finite)),
                                                                        idMapIndex(0),
                                                                        initialPointSet(pointSet)
    {
        addPointSet();
        return;
    }

    const MyEMSTData & DelaunayTriEMST::getEMSTData() const
    {
        return emstData;
    }

    //adds point set and computes emst
    void DelaunayTriEMST::addPointSet()
    {
        for(const MyPoint_2 & pt : initialPointSet)
        {
            delaunayTri.insert(pt);
        }

        MyVertex_iterator   vit, ve;
        // Associate indices to the vertices
        // boost::tie assigns the first and second element of the std::pair
        // returned by boost::vertices to the variables vit and ve
        for (boost::tie(vit, ve) = boost::vertices(finiteDT); vit != ve; ++vit)
        {
            MyVertex_descriptor vd = *vit;
            vertex_id_map[vd] = idMapIndex++;
        }

        findMST(emstData);
        findEdgePointIndices(emstData);
        emstData.mstEdgeList.clear();   // handles go stale once candidates are inserted/removed
        
        return;
    }


    // myPoint must not coincide with an input point: insert() would then return
    // the existing input vertex, and the remove() below would delete it.
    // (Coincident candidates are filtered out in ComputationResult::computeMSTAndStPts.)    
    MyEMSTData DelaunayTriEMST::testPointInsertion(const MyPoint_2 & myPoint)
    {
        MyEMSTData result;

        MyTriangulation::Vertex_handle sPointHandle = delaunayTri.insert(myPoint);
        // Kruskal's needs an index for every vertex: give the candidate the next one
        // (idMapIndex = number of input points). It's erased again after the removal
        // below, so idMapIndex itself never changes.
        vertex_id_map[sPointHandle] = idMapIndex;        

        findMST(result);    
        findEdgePointIndices(result, true);
        result.mstEdgeList.clear();     // these handles go stale when the candidate is removed

        delaunayTri.remove(sPointHandle);
        vertex_id_map.erase(sPointHandle);

        return result;
    }

    void DelaunayTriEMST::findMST(MyEMSTData & fillMe)
    {
        MyVertexIdPropertyMap vertex_index_pmap(vertex_id_map);
        //from cgal example comments: "This property map is defined in graph_traits_Triangulation_2.h "
        // In the function call you can see a named parameter: vertex_index_map

        // Kruskal's algorithm only compares edge weights, so the default weight
        // (squared edge length, from CGAL's graph traits for Triangulation_2)
        // gives the same MST as true lengths: squaring preserves the order of
        // non-negative numbers, and it avoids square roots inside Kruskal's.
        // The square roots are taken once per MST edge below, for the total.        
        boost::kruskal_minimum_spanning_tree(finiteDT,
                                            std::back_inserter(fillMe.mstEdgeList),
                                            vertex_index_map(vertex_index_pmap));

        auto endELIt = fillMe.mstEdgeList.end();
        for (auto it = fillMe.mstEdgeList.begin(); it != endELIt; ++it)
        {
            MyEdge_descriptor ed = *it;
            //vertex handles and vertex descriptors are typedef'd to the same thing...
            //MyVertex_descriptor and MyTriangulation::Vertex_handle seem to have the same interface...
            //the cgal example has us use an intermediary MyVertex_descriptor...
            MyTriangulation::Vertex_handle vhSource = source(ed, delaunayTri);
            MyTriangulation::Vertex_handle vhTarget = target(ed, delaunayTri);
            fillMe.length += CGAL::sqrt(squared_distance(vhSource->point(), vhTarget->point()));
        }    

        return;
    }


    // Converts each MST edge's triangulation handles into indices of input points
    // (or marks the endpoint as the Steiner point). Each endpoint is found by
    // comparing coordinates with every input point: a linear scan, so O(n^2) per
    // MST, the dominant per-candidate cost. The Future work section in the header
    // of OneSteinerTreeProblem.cpp gives two ways to avoid it: store each vertex's
    // index in the vertex (Triangulation_vertex_base_with_info_2), or build each
    // candidate's MST from the original MST's edges, where indices are known.
    void DelaunayTriEMST::findEdgePointIndices(MyEMSTData & fillMe, bool containsSteinerPoint)
    {
        auto endELIt = fillMe.mstEdgeList.end();
        for (auto it = fillMe.mstEdgeList.begin(); it != endELIt; ++it)
        {
            MyEdge_descriptor ed = *it;
            // For Triangulation_2, vertex descriptors and vertex handles are the same type.
            // source()/target() are the Boost Graph Library's generic functions; they return
            // graph_traits<...>::vertex_descriptor (CGAL's emst example uses that name so the
            // code works for any BGL graph). For Triangulation_2 that type is Vertex_handle,
            // so we use Vertex_handle directly.
            MyTriangulation::Vertex_handle vhSource = source(ed, delaunayTri);
            MyTriangulation::Vertex_handle vhTarget = target(ed, delaunayTri);

            VertexRef sourceV  = { VertexRef::Kind::INPUT, 0 };
            VertexRef targetV = { VertexRef::Kind::INPUT, 0 };

            // Only one point is added for a candidate, so an endpoint not found among the
            // input points is the Steiner point. Its index is set after the candidates are
            // sorted (see ComputationResult::computeMSTAndStPts).
            bool foundSrc = Utility_Functions::findPointIndex(vhSource->point(),  initialPointSet, sourceV.index);
            if( !foundSrc && containsSteinerPoint)
            {
                sourceV.kind = VertexRef::Kind::STEINER;
                foundSrc = true;
            }
            bool foundTarget = Utility_Functions::findPointIndex(vhTarget->point(),  initialPointSet, targetV.index);
            if( !foundTarget && containsSteinerPoint && (sourceV.kind != VertexRef::Kind::STEINER))
            {
                targetV.kind = VertexRef::Kind::STEINER;
                foundTarget = true;
            }        
            assert(foundSrc && foundTarget);
            fillMe.mstEdgePointIndices.push_back(MstEdgeEndpoints{ sourceV, targetV });
        }  

        return;    
    }
} // namespace One_St_Pt_GPA
