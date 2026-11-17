#ifndef DTEMST_HPP
#define DTEMST_HPP
/*
** Euclidean Minimum Spanning Tree via Delaunay triangulation + Kruskal's.
**
** Built off of the CGAL BGL_triangulation_2/emst.cpp example.
** In addition to computing the EMST of a point set 
** (by creating the Delaunay triangulation then running Kruskal's),
** this is also being used to test candidate Steiner points.
** This is done by inserting the candidate point into the existing triangulation,
** whereby CGAL takes care of updating the triangulation, 
** recomputing the EMST on the updated triangulation (the "new" result is saved and returned),
** then the candidate is removed from the triangulation. 
** This is an inefficient way to test Steiner points, but it was quick to implement.
** For a much better way (implementing it is future work), 
** see the paper I co-authored (and its references):
**
** (Bose, P., D'Angelo, A., & Durocher, S.) 
**  "On the Restricted k-Steiner Tree Problem.", 
**    ( J. Comb. Optim. 44(4): 2893-2918 (2022))
**
** What is described in that paper is an improvement on what is done in 
** Georgakopoulos and Papadimitriou, "The 1-Steiner Tree Problem,"
** J. Algorithms 8(1):122-130, 1987.
** While it would not improve the asymptotic runtime of their algorithm, it would improve
** the asymptotic runtime of this implementation.
*/
//See OneSteinerTreeProblem.cpp for project notes

#include "1ST-Constants-Utilities.h"

#include <map>  // std::map for the vertex index map

#include <CGAL/Delaunay_triangulation_2.h> //cgal examples  BGL_triangulation_2/emst.cpp, https://doc.cgal.org/latest/BGL/index.html#title15
#include <CGAL/boost/graph/graph_traits_Delaunay_triangulation_2.h>
#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/graph/filtered_graph.hpp>

namespace One_St_Pt_GPA
{
    typedef CGAL::Delaunay_triangulation_2<MyKernel>      MyTriangulation;
    /*
    //cgal examples  BGL_triangulation_2/emst.cpp, https://doc.cgal.org/latest/BGL/index.html#title15
    */
    // "As we only consider finite vertices and edges
    // we need the following filter"
    template <typename T>
    struct Is_finite 
    {
        const T *t_;
        Is_finite()
            : t_(NULL)
        {}
        Is_finite(const T &t)
            : t_(&t)
        { }
        template <typename VertexOrEdge>
        bool operator()(const VertexOrEdge &voe) const {
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
    typedef std::map<MyVertex_descriptor, int>                    MyVertexIndexMap;
    // A std::map is not a property map, because it is not lightweight
    typedef boost::associative_property_map<MyVertexIndexMap>     MyVertexIdPropertyMap;

    // One endpoint of an MST edge: which list the index refers to, and the index.
    struct VertexRef
    {
        enum class Kind { INPUT, STEINER };
        Kind   kind;
        size_t index;  // into the input points, or (for Kind::STEINER) the candidate list
    };

    struct MstEdgeEndpoints 
    { 
        VertexRef first;
        VertexRef second; 
    };
    
    // A minimum spanning tree: its edges (as endpoint indices) and its total length.
    struct MyEMSTData
    {
        // Temporary: handles into the triangulation, valid only until it next changes.
        // Used only to fill mstEdgePointIndices, then cleared (see addPointSet and
        // testPointInsertion). Use mstEdgePointIndices for anything that is kept.       
        vector< MyEdge_descriptor > mstEdgeList;

        //indices for the endpoints of an edge, and whether or not the end pt is a st pt
        vector< MstEdgeEndpoints > mstEdgePointIndices; 

        My_Number_type length = 0.0;

        // Orders trees by total length; used to sort the candidates.
        bool operator<(const MyEMSTData & other) const
        {
            return length < other.length;
        }

    };

    class DelaunayTriEMST
    {
        protected:
            MyTriangulation             delaunayTri;
            MyTriFilter                 is_finite;
            MyFinite_triangulation      finiteDT;  
            MyVertexIndexMap            vertex_id_map;
            int                         idMapIndex;
            const vector< MyPoint_2 > & initialPointSet;    // not owned: the vector must outlive this object
            MyEMSTData                  emstData;

        public:
            explicit DelaunayTriEMST(const vector< MyPoint_2 > & pointSet);
            //adds point set and computes emst
            const MyEMSTData & getEMSTData();
            MyEMSTData testPointInsertion(const MyPoint_2 & myPoint);

            // Not copyable or movable: is_finite and finiteDT refer to this object's own
            // delaunayTri, so a copy would refer to the original's triangulation.
            DelaunayTriEMST(const DelaunayTriEMST &)                = delete;
            DelaunayTriEMST & operator=(const DelaunayTriEMST &)    = delete;
            DelaunayTriEMST(DelaunayTriEMST &&)                     = delete;
            DelaunayTriEMST & operator=(DelaunayTriEMST &&)         = delete;            

        protected:
            void findMST(MyEMSTData & fillMe);
            void findEdgePointIndices(MyEMSTData & fillMe, bool containsSteinerPoint = false);
            void addPointSet();
    };
} // namespace One_St_Pt_GPA
#endif