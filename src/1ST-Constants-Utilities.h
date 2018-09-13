#ifndef ONE_ST_CONSTANTS_UTILITIES
#define ONE_ST_CONSTANTS_UTILITIES

//See OneSteinerTreeProblem.cpp for project notes

#include <iostream>
#include <sstream> // for ostringstream
#include <string>
#include <set>
#include <vector>

#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <ctime>
#include <cstddef> // std::size_t

#include <math.h> /* fabs */

#include <functional>   // std::reference_wrapper

#include <algorithm> //std::next_permutation, std::sort

//https://github.com/CGAL/cgal/blob/master/Generator/examples/Generator/random_grid.cpp
//https://doc.cgal.org/latest/Kernel_23/classCGAL_1_1Exact__predicates__exact__constructions__kernel.html
//https://doc.cgal.org/latest/Cone_spanners_2/index.html
//#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel_with_root_of.h>
#include <CGAL/algorithm.h>

#include <CGAL/Lazy_exact_nt.h>

#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Compute_cone_boundaries_2.h>

#include <CGAL/squared_distance_2.h> //for 2D functions
#include <CGAL/number_utils.h>       //for cgal::sqrt

#include <CGAL/ch_akl_toussaint.h>

#include <CGAL/Kernel/global_functions.h>
#include <CGAL/enum.h>
#include <CGAL/intersections.h>

//TODO might be good to remove these and explicitly list the things I use...
//e.g. why necessary: time() in main() might be CGAL, not std...
using namespace CGAL;
using namespace std;

#define MY_LAZY_COMPS   (1)

//https://doc.cgal.org/latest/Kernel_23/Kernel_23_2exact_8cpp-example.html#_a0
//https://doc.cgal.org/latest/Kernel_23/Kernel_23_2intersection_visitor_8cpp-example.html#_a0
//https://doc.cgal.org/latest/Kernel_d/group__PkgKernelDFunctions.html#ga0aa3e8b6bdf1bff509f8e2672ef194d1
//https://doc.cgal.org/latest/Cone_spanners_2/index.html
//typedef Exact_predicates_exact_constructions_kernel Kernel;
#if (MY_LAZY_COMPS)
    typedef Lazy_exact_nt<CORE::Expr> Lazy_FT;
    typedef Simple_cartesian< Lazy_FT >  MyKernel;
#else
    typedef CGAL::Exact_predicates_exact_constructions_kernel_with_root_of      MyKernel;
#endif
//https://doc.cgal.org/latest/Arrangement_on_surface_2/index.html
typedef MyKernel::FT                                                        My_Number_type;
typedef CGAL::Arr_segment_traits_2<MyKernel>                                MyTraits_2;

typedef MyTraits_2::Point_2                                                 MyPoint_2;
typedef MyTraits_2::Segment_2                                               MySegment_2;
typedef MyTraits_2::Line_2                                                  MyLine_2;
typedef MyTraits_2::Intersect_2                                             MyIntersect_2;

typedef MyTraits_2::Direction_2                                             MyDirection_2;

typedef CGAL::Polygon_2<MyKernel>                                           MyPolygon_2;
typedef CGAL::Polygon_with_holes_2<MyKernel>                                MyPolygon_with_holes_2;
typedef CGAL::Polygon_set_2<MyKernel>                                       MyPolygon_set_2;

//enum class PointTypeEnum {INPUT, BEST_STEINER, STEINER, OTHER};
typedef struct {
//    vector< size_t > myInputPointIndices;
    set< size_t > myInputPointIndices;
    //TODO augment with something that lets us do a colouring of the faces
} MyFaceData;

//https://doc.cgal.org/latest/Arrangement_on_surface_2/classCGAL_1_1Arr__face__overlay__traits.html
//https://doc.cgal.org/latest/Arrangement_on_surface_2/Arrangement_on_surface_2_2overlay_unbounded_8cpp-example.html#_a4
//Might only be able to use extended faces 
//typedef CGAL::Arr_extended_dcel<MyTraits_2, PointTypeEnum, bool, MyFaceData>       MyDcel; //vertices, edges, then faces
//typedef CGAL::Arrangement_2<MyTraits_2, MyDcel>                             MyArrangement_2;
typedef CGAL::Arr_face_extended_dcel<MyTraits_2, MyFaceData>       MyDcel;
typedef CGAL::Arrangement_2<MyTraits_2, MyDcel>                    MyArrangement_2;

// Define a functor for creating a label.
struct Overlay_faces
{
  MyFaceData operator() (MyFaceData first, MyFaceData second) const
  {
      //This should work out since the outer faces of the arrangements will have empty sets //vectors
        MyFaceData result;

        for(const auto ind : first.myInputPointIndices)
        {
            result.myInputPointIndices.insert(ind);
        }
        for(const auto ind : second.myInputPointIndices)
        {
            result.myInputPointIndices.insert(ind);
        }        

        return result;
  }
};
typedef CGAL::Arr_face_overlay_traits<MyArrangement_2, MyArrangement_2, MyArrangement_2, Overlay_faces>  MyOverlay_Traits;


//http://man7.org/linux/man-pages/man7/sem_overview.7.html
#define SEMAPHORE_NAME ("/ONE_ST_TREE_SEMAPHORE")
#define SEMAPHORE_INIT_VALUE (1)

#define MY_VERBOSE (0)
#define EXCLUDE_CELL_BOUNDARY (1)   //doesn't seem to make a diff. thought it would at least include the isolated pts on ch boundary...
#define BUILD_ODCELL_BY_COMPLEMENTING (1)   //no effect right now
#define BUILD_ODCELL_BY_DIFFERENCES   (1)

//magic numbers
#define MIN_NUM_INPUT_POINTS (3)
#define MAX_NUM_INPUT_POINTS (10)
#define DEFAULT_NUM_INPUT_POINTS (MIN_NUM_INPUT_POINTS)

#define GRID_LENGTH_DEFAULT (10)

#define RAND_SEED_DEFAULT (10)
#define TRUE_RANDOM_DEFAULT (false)

#define ONLY_POINTS_DEFAULT (false)


#define ELAPSED_TIME_NAME_STRING ("elapsedTime")
#define COMP_RESULT_NAME_STRING ("compResult")
#define INPUT_POINTS_NAME_STRING ("inputPoints")
#define CONVEX_HULL_POINTS_NAME_STRING ("convexHullPoints")
#define RAND_SEED_NAME_STRING ("randSeed")
#define GRID_LENGTH_NAME_STRING ("gridLength")
//////////////////////////////
#define ARR_NAME_STRING ("odcArrangement")
#define ARR_POINTS_NAME_STRING ("arrangementPoints")
///////
#define ARR_FACES_NAME_STRING ("arrangementFaces")
#define ARR_IND_FACE_NAME_STRING ("arrFace")
#define ARR_FACE_NAME_PREFIX_NAME_STRING ("arrFace__")
#define ARR_FACE_CCW_V_INDICES_NAME_STRING ("arrangementFaceCCWVIndices")
#define ARR_OODC_INPUT_SITE_INDICES_NAME_STRING ("arrangementOODCSiteIndices")
//////////////////////////////
#define MST_NAME_STRING ("minSpanningTree")
#define MST_LENGTH_NAME_STRING ("mstLength")
#define MST_EDGE_LIST_NAME_STRING ("mstEdgeList")
#define MST_EDGE_NAME_STRING ("mstEdge")
#define MST_EDGE_ENDPOINT_INDICES_NAME_STRING ("mstEdgeEndpointIndices")
#define MST_EDGE_FIRST_ENDPOINT_IS_STEINER_PT_NAME_STRING ("firstEndPointIsSteiner")
#define MST_EDGE_SECOND_ENDPOINT_IS_STEINER_PT_NAME_STRING ("secondEndPointIsSteiner")
//////////////////////////////
#define STEINER_PT_NAME_STRING ("steinerPoint")
#define GEOM_MED_POINT_NAME_STRING ("geomMedPoint")
#define GEOM_MED_NEIGHBOUR_INDICES_NAME_STRING ("geomMedNeighbourIndices")
#define GEOM_MED_COINCIDES_W_INPUT_PT_NAME_STRING ("geomMedCoincidesWithInputPt")
#define GEOM_MED_COINCIDENT_INPUT_INDEX_NAME_STRING ("geomMedCoincidentInputPtIndex")
//////////////////////////////
#define CANDIDATE_ST_PT_DATA_LIST_NAME_STRING ("candidateStPtDataList")
#define CANDIDATE_STEINER_POINT_DATA_NAME_STRING ("candidateStPtData")

#define DOUBLE_EPSILON (0.000001)


////// Forward-declare Structs used in printing and utils ///////
struct GeomMedianData;
struct MyEMSTData;
struct CandidateSteinerPointData;

/////////////////////
// Namespaces for utilities and printing.

/*
**
** Why namespace Vasco_Rossi? 
** Apart from sounding like it could be the name of his next album,
** it's just more interesting than "Utility_Functions". 
** Plus, since it's likely that I'm the only one who's ever going
** to see/use this, I can call it whatever I want as long as he doesn't mind. I hope he doesn't...
** Plus I'm a fan.
**
 */
namespace Vasco_Rossi
{

    inline bool intInRangeInclusive(int x, int lowEnd, int highEnd)
    {
        return ((x >= lowEnd) && (x <= highEnd));
    }

    void extractPointsFromJSON2DArrayString(string &inputString, vector< MyPoint_2 >& result);

    //Assume the numbers aren't too large or small for doubles
    inline bool pointsAreTooClose(const MyPoint_2 &first, const MyPoint_2 &second)
    {
        return ( (fabs(CGAL::to_double(first.x()) - CGAL::to_double(second.x())) < DOUBLE_EPSILON) 
                && (fabs(CGAL::to_double(first.y()) - CGAL::to_double(second.y())) < DOUBLE_EPSILON) );
    }

    bool findOriginIndex(const MyPoint_2 &cellOrigin, 
                          const vector< reference_wrapper<const MyPoint_2> > &inputPointSet,
                          size_t &resultIndex);

    bool findPointIndex(const MyPoint_2 &pt, const vector< MyPoint_2 >& myColl, size_t &myIndex);

    inline void computeConvexHull(const vector< MyPoint_2 >& pointVec, vector<MyPoint_2>& convexHullList)
    {
        //TODO does convex hull work w const mypoints?
        ch_akl_toussaint(pointVec.begin(), pointVec.end(), back_inserter(convexHullList));
        return;
    }    

    inline void computeConeRays(const MyDirection_2& initialDirection, vector<MyDirection_2>& coneRays)
    {
        //https://doc.cgal.org/latest/Cone_spanners_2/index.html  
        // construct the functor
        //TODO does the cones function work w vector of const dirs?
        Compute_cone_boundaries_2<MyKernel> cones;    
        cones(6, initialDirection, coneRays.begin());

        return;
    }

    //Assumption: no duplicate points. The "std::set" insertion can't reliably tell if there are doubles (I've experienced this...)
    void insertArrangementPointsIntoPointSet(const MyArrangement_2& resultODCArrangement, vector< MyPoint_2 >& arrPointsVec);

} // namespace Vasco_Rossi


/*
**
** Why namespace Marisa_Tomei? 
** A few of her movies have been on tv lately. 
** They left me wanting to see more of her...
** I think it's a fitting name for pretty print functions.
**
** Plus, since it's likely that I'm the only one who's ever going
** to see/use this, I can call it whatever I want as long as she doesn't mind. I hope she doesn't...
** Plus, I'm a fan.
**
 */
namespace Marisa_Tomei
{
    inline string wrapStringInQuotes(const string &s)
    {
        return ("\"" + s + "\"");
    }

    inline string point2ToJSON(const MyPoint_2 &p)
    {
        ostringstream sStream;

        sStream << "{\"x\":\"" << p.x() << "\", "
                << "\"y\":\"" << p.y() << "\"}";
        return sStream.str();
    } 

    inline string insertTabs(int level)
    {
        ostringstream sStream;
        for (int i = 0; i < level; ++i)
        {
            sStream << "\t";
        }
        return sStream.str();
    }

#if (MY_VERBOSE)
    //https://doc.cgal.org/latest/Arrangement_on_surface_2/index.html#title6 
    //&& examples/Arrangement_on_surface_2/bgl_dual_adapter.cpp 
    //&& examples/Arrangement_on_surface_2/arr_print.h
    inline void print_ccb (MyArrangement_2::Ccb_halfedge_const_circulator circ)
    {
        MyArrangement_2::Ccb_halfedge_const_circulator curr = circ;
        std::cout << "(" << curr->source()->point() << ")";
        do {
            MyArrangement_2::Halfedge_const_handle he = curr;
            std::cout << " [" << he->curve() << "] "
              << "(" << he->target()->point() << ")";
        } while (++curr != circ);
        std::cout << std::endl;
    }
#endif

    string mstEdgeToJSONString(const pair< pair<size_t, size_t>, pair<bool, bool> >& edgeData, int tabLevel = 0);

    string mstDataToJSONString(const MyEMSTData& mst, int tabLevel = 0);

    string geomMedDataToJSONString(const GeomMedianData& stPtData, int tabLevel = 0);

    string candidateSteinerPtDataToJSONString(const CandidateSteinerPointData& stPtData, int tabLevel = 0);

    string steinerPointsToJSONString(const vector<CandidateSteinerPointData>& steinerPoints, int tabLevel = 0);

    string pointVectorToJSONString(string name, const vector< MyPoint_2 >& myColl, int tabLevel = 0);

    string vertexIndicesToJSONString(string name, const vector<MyPoint_2> &myColl,
                                                    const vector<MyPoint_2>& myPtSet, int tabLevel = 0);    

    string arrangementFaceToJSONString(string faceName, const MyArrangement_2::Face_const_iterator fit, 
                                                        const vector<MyPoint_2>& myPtSet, int tabLevel = 0);

    string arrangementToJSONString(const MyArrangement_2& resultODCArrangement, int tabLevel = 0);

} // namespace Marisa_Tomei

#endif