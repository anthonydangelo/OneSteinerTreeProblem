#ifndef DTEMST_HPP
#define DTEMST_HPP

#include "1ST-Constants-Utilities.h"

#include <CGAL/Delaunay_triangulation_2.h> //cgal examples  BGL_triangulation_2/emst.cpp, https://doc.cgal.org/latest/BGL/index.html#title15
#include <CGAL/boost/graph/graph_traits_Delaunay_triangulation_2.h>
#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/graph/filtered_graph.hpp>

typedef CGAL::Delaunay_triangulation_2<MyKernel>      MyTriangulation;
/*
//cgal examples  BGL_triangulation_2/emst.cpp, https://doc.cgal.org/latest/BGL/index.html#title15
*/
// "As we only consider finite vertices and edges
// we need the following filter"
template <typename T>
struct Is_finite {
  const T* t_;
  Is_finite()
    : t_(NULL)
  {}
  Is_finite(const T& t)
    : t_(&t)
  { }
  template <typename VertexOrEdge>
  bool operator()(const VertexOrEdge& voe) const {
    return ! t_->is_infinite(voe);
  }
};
typedef Is_finite<MyTriangulation>                                        MyTriFilter;
typedef boost::filtered_graph<MyTriangulation,MyTriFilter,MyTriFilter>    MyFinite_triangulation;
typedef boost::graph_traits<MyFinite_triangulation>::vertex_descriptor    MyVertex_descriptor;
typedef boost::graph_traits<MyFinite_triangulation>::vertex_iterator      MyVertex_iterator;
typedef boost::graph_traits<MyFinite_triangulation>::edge_descriptor      MyEdge_descriptor;
// "The BGL makes use of indices associated to the vertices
// We use a std::map to store the index"
typedef std::map<MyVertex_descriptor,int>   MyVertexIndexMap;
// A std::map is not a property map, because it is not lightweight
typedef boost::associative_property_map<MyVertexIndexMap>     MyVertexIdPropertyMap;

typedef struct {
    vector<MyEdge_descriptor>       mstEdgeList;
    vector<pair<size_t, size_t>>    mstEdgePointIndices;
    My_Number_type                  length = 0.0;
} MyEMSTData;

class DelaunayTriEMST
{
  protected:
    MyTriangulation delaunayTri;
    MyTriFilter is_finite;
    MyFinite_triangulation finiteDT;  
    MyVertexIndexMap vertex_id_map;
    int idMapIndex;
    const set< MyPoint_2 >* cpInitialPointSet;
    MyEMSTData      emstData;

  public:
    DelaunayTriEMST();
    //adds point set and computes emst
    MyEMSTData addPointSet(const set< MyPoint_2 > &pointSet);
    MyEMSTData testPointInsertion(const MyPoint_2& myPoint);

  protected:
    void findMST(MyEMSTData &fillMe);
};

#endif