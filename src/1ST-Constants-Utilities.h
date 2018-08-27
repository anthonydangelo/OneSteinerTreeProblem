#ifndef ONE_ST_CONSTANTS_UTILITIES
#define ONE_ST_CONSTANTS_UTILITIES

//#include <ostream>
#include <iostream>
#include <sstream> // for ostringstream
#include <string>
//#include <unordered_set> //need to provide a hasher or something for this...
#include <set>
#include <vector>

#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <ctime>
#include <cstddef> // std::size_t

#include <math.h> /* fabs */

#include <functional>   // std::reference_wrapper

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
//        set<size_t> resSet;
/*         copy(first.myInputPointIndices.begin(), first.myInputPointIndices.end(), back_inserter(result.myInputPointIndices) );
        
        for(const auto ind : second.myInputPointIndices)
        {
            bool addMe = false;
            for(const auto resInd : result.myInputPointIndices)
            {
            }
        } 
*/
        for(const auto ind : first.myInputPointIndices)
        {
            //resSet.insert(ind);
            result.myInputPointIndices.insert(ind);
        }
        for(const auto ind : second.myInputPointIndices)
        {
            //resSet.insert(ind);
            result.myInputPointIndices.insert(ind);
        }        
/*         for(const auto ind : resSet)
        {
            result.myInputPointIndices.push_back(ind);
        }  */       
        return result;
  }
};
typedef CGAL::Arr_face_overlay_traits<MyArrangement_2, MyArrangement_2, MyArrangement_2, Overlay_faces>  MyOverlay_Traits;


#define MY_VERBOSE (0)
#define EXCLUDE_CELL_BOUNDARY (1)   //doesn't seem to make a diff. thought it would at least include the isolated pts on ch boundary...
#define DEBUG_W_MY_BOUNDING_BOX (0)
#define TIME_PROGRAM (1)
#define DEBUG_OVERLAY (0)
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

#define OUTPUT_FILE ("-result.json")
#define INPUT_POINTS_NAME_STRING ("inputPoints")
#define CONVEX_HULL_POINTS_NAME_STRING ("convexHullPoints")
#define RAND_SEED_NAME_STRING ("randSeed")
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

#define DOUBLE_EPSILON (0.000001)

/* bool intInRangeInclusive(int x, int lowEnd, int highEnd){
    return ((x >= lowEnd) && (x <= highEnd));
}

string stringAsJSONString(string s)
{
    return ("\"" + s + "\"");
} */

//why do these need to be static?
static inline bool intInRangeInclusive(int x, int lowEnd, int highEnd)
{
    return ((x >= lowEnd) && (x <= highEnd));
}

//static inline string stringAsJSONString(string s)
static inline string wrapStringInQuotes(const string &s)
{
    return ("\"" + s + "\"");
}

static inline string point2ToJSON(const MyPoint_2 &p)
{
    ostringstream sStream;

    sStream << "{\"x\":\"" << p.x() << "\", "
            << "\"y\":\"" << p.y() << "\"}";
    return sStream.str();
}

static inline string insertTabs(int level)
{
    ostringstream sStream;
    for (int i = 0; i < level; ++i)
    {
        sStream << "\t";
    }
    return sStream.str();
}

static inline void extractPointsFromJSON2DArrayString(string &inputString, vector<MyPoint_2> &result)
{
    //I'd like to use a regex here, but I don't know how to write the grammar...
    //http://www.cplusplus.com/reference/string/string/find_first_of/
    std::size_t strIndex = inputString.find_first_of("[");
    //I don't like the way I'm doing this parsing!
    while (strIndex != std::string::npos)
    {
        strIndex = inputString.find_first_of("[", strIndex + 1);
        if (strIndex != string::npos)
        {
            try
            {
                //http://www.cplusplus.com/reference/string/stod/
                string::size_type stodIndex;
                double firstD = stod(inputString.substr(strIndex + 1), &stodIndex);
                strIndex = inputString.find_first_of(",", stodIndex + strIndex);
                if (strIndex != string::npos)
                {
                    double secondD = stod(inputString.substr(strIndex + 1));
                    result.push_back(MyPoint_2(My_Number_type(firstD), My_Number_type(secondD)));
                    //                    result.push_back(MyPoint_2(firstD, secondD ));
                }
            }
            catch (exception &e)
            {
                cerr << e.what() << endl;
                cerr << "Input point list string malformed" << endl;
                result.clear();
                return;
            }
        }
    }
    return;
}

//Assume the numbers aren't too large or small for doubles
static inline bool pointsAreTooClose(const MyPoint_2 &first, const MyPoint_2 &second)
{
    string ptString = point2ToJSON(first);
    string tempString = point2ToJSON(second);

    std::size_t strIndex = ptString.find_first_of(":");
    ptString = ptString.substr(strIndex + 1);
    strIndex = ptString.find_first_of("\"");
    ptString = ptString.substr(strIndex + 1);
    double ptX = stod(ptString);
    strIndex = ptString.find_first_of(":");
    ptString = ptString.substr(strIndex + 1);
    strIndex = ptString.find_first_of("\"");
    ptString = ptString.substr(strIndex + 1);
    double ptY = stod(ptString);

    strIndex = tempString.find_first_of(":");
    tempString = tempString.substr(strIndex + 1);
    strIndex = tempString.find_first_of("\"");
    tempString = tempString.substr(strIndex + 1);
    double tempX = stod(tempString);
    strIndex = tempString.find_first_of(":");
    tempString = tempString.substr(strIndex + 1);
    strIndex = tempString.find_first_of("\"");
    tempString = tempString.substr(strIndex + 1);
    double tempY = stod(tempString);

    return ( (fabs(ptX - tempX) < DOUBLE_EPSILON) && (fabs(ptY - tempY) < DOUBLE_EPSILON) );
}

static inline bool findOriginIndex(const MyPoint_2 &cellOrigin, 
                          const vector< reference_wrapper<const MyPoint_2> > &inputPointSet,
                          size_t &resultIndex)
{
    resultIndex = 0;
    for (auto it = inputPointSet.begin(); it != inputPointSet.end(); ++it, ++resultIndex)
    {
        if (pointsAreTooClose(*it, cellOrigin))
        {
            return true;
        }
    }
    return false;
}


//https://doc.cgal.org/latest/Arrangement_on_surface_2/index.html#title6 
//&& examples/Arrangement_on_surface_2/bgl_dual_adapter.cpp 
//&& examples/Arrangement_on_surface_2/arr_print.h
static inline void print_ccb (MyArrangement_2::Ccb_halfedge_const_circulator circ)
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