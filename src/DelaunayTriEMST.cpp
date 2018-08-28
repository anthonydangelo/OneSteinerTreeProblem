#include "DelaunayTriEMST.hpp"


//https://doc.cgal.org/latest/BGL/index.html#title15 and cgal examples BGL_triangulation_2/emst.cpp
DelaunayTriEMST::DelaunayTriEMST() : is_finite(MyTriFilter(delaunayTri)),
                                     finiteDT(MyFinite_triangulation(delaunayTri, is_finite, is_finite)),
                                     idMapIndex(0),
                                     cpInitialPointSet(nullptr)
{
    return;
}

//adds point set and computes emst
// Why use a pointer instead of a ref? refs have to be assigned on initialization, 
// which means setting it to some kind of null since the pointset may not be available when the object is created...
// !!!IMPORTANT!!! Because of the ptr, the set needs to continue to exist until such time as the obj is no longer used/destroyed
MyEMSTData DelaunayTriEMST::addPointSet(const set< MyPoint_2 >& pointSet)
{
    cpInitialPointSet = &pointSet;
    for(auto pt : *cpInitialPointSet)
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

    MyVertexIdPropertyMap vertex_index_pmap(vertex_id_map);
    // We use the default edge weight which is the squared length of the edge
    // This property map is defined in graph_traits_Triangulation_2.h

    // In the function call you can see a named parameter: vertex_index_map
    
    boost::kruskal_minimum_spanning_tree(finiteDT,
                                         std::back_inserter(emstData.mstEdgeList),
                                         vertex_index_map(vertex_index_pmap));

    auto endELIt = emstData.mstEdgeList.end();
    for (auto it = emstData.mstEdgeList.begin(); it != endELIt; ++it)
    {
        MyEdge_descriptor ed = *it;
        //MyVertex_descriptor and MyTriangulation::Vertex_handle seem to have the same interface...
        MyVertex_descriptor tempSource = source(ed, delaunayTri);
        MyVertex_descriptor tempTarget = target(ed, delaunayTri);
        MyTriangulation::Vertex_handle vhSource = tempSource;
        MyTriangulation::Vertex_handle vhTarget = tempTarget;
        //todo throw error if this fails...
        size_t sourceIndex, targetIndex;
        findPointIndex(vhSource->point(),  *cpInitialPointSet, sourceIndex);
        findPointIndex(vhTarget->point(),  *cpInitialPointSet, targetIndex);
        emstData.mstEdgePointIndices.push_back( std::pair<size_t, size_t>(sourceIndex, targetIndex) );
        emstData.length += CGAL::sqrt(squared_distance(vhSource->point(), vhTarget->point()));
    }
    
    return emstData;
}
//TODO Most of these two functions can be extracted into one that comps the MST and struct data and returns that...
MyEMSTData DelaunayTriEMST::testPointInsertion(const MyPoint_2& myPoint)
{
    MyEMSTData result;

    MyTriangulation::Vertex_handle sPointHandle = delaunayTri.insert(myPoint);
    //vertex handles and vertex descriptors are typedef'd to the same thing... 
    //I don't know why they use both in the mst example to print things out...
    vertex_id_map[sPointHandle] = idMapIndex; //no need to post-increment, we'd just decrement it later...

    findMST(result);    

    vertex_id_map.erase(sPointHandle);

    return result;
}

void DelaunayTriEMST::findMST(MyEMSTData &fillMe)
{
    MyVertexIdPropertyMap vertex_index_pmap(vertex_id_map);
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
        //todo throw error if this fails...
        size_t sourceIndex, targetIndex;
        findPointIndex(vhSource->point(),  *cpInitialPointSet, sourceIndex);
        findPointIndex(vhTarget->point(),  *cpInitialPointSet, targetIndex);
        fillMe.mstEdgePointIndices.push_back( std::pair<size_t, size_t>(sourceIndex, targetIndex) );
        fillMe.length += CGAL::sqrt(squared_distance(vhSource->point(), vhTarget->point()));
    }    

    return;
}

