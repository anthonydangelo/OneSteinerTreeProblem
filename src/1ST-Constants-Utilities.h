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
#include <cstddef> // std::size_t

#include <math.h> /* fabs */

#include <functional>   // std::reference_wrapper

#include <algorithm> //std::next_permutation, std::sort

#include <CGAL/Lazy_exact_nt.h>

#include <CGAL/Polygon_set_2.h>
#include <CGAL/Compute_cone_boundaries_2.h>

#include <CGAL/squared_distance_2.h> //for 2D functions
#include <CGAL/number_utils.h>       //for cgal::sqrt

#include <CGAL/ch_akl_toussaint.h>

#include <CGAL/Kernel/global_functions.h>
#include <CGAL/enum.h>
#include <CGAL/intersections.h>



// NOTE: 
// We use a file lock as a resource mutex (a blocking lock call).
// The lock file lives in /tmp, which is sticky + world-writable.
// On modern kernels (>= 4.19, e.g. Ubuntu 20.10+), fs.protected_regular
// blocks cross-user O_CREAT opens in such directories. If this program
// is ever run by a different user than the one who created the lock file,
// open() will fail with EACCES.
//
// Fix: move the lock file to a directory that is neither sticky nor
// world-writable (e.g. a group-owned directory with mode 0770), or ensure
// all invocations run as the same user.
// Example:
//
//   sudo mkdir -p /var/lib/myapp-locks
//   sudo groupadd myapp-group
//   sudo usermod -aG myapp-group <web_server_user>
//   sudo usermod -aG myapp-group <your_user>
//   sudo chgrp myapp-group /var/lib/myapp-locks
//   sudo chmod 0770 /var/lib/myapp-locks
//
// Then change the path in this file from "/tmp/myapp.lock" to
// "/var/lib/myapp-locks/myapp.lock".   
#define FLOCK_PATH ("/tmp/one-st-tree-mutex.lock")


#define MY_VERBOSE                  (0)
#define EXCLUDE_CELL_BOUNDARY       (1)   //doesn't seem to make a diff. thought it would at least include the isolated pts on ch boundary...
#define BUILD_ODCELL_BY_DIFFERENCES (1)

//magic numbers
constexpr int MIN_NUM_INPUT_POINTS      = 3;
constexpr int MAX_NUM_INPUT_POINTS      = 10;
constexpr int DEFAULT_NUM_INPUT_POINTS  = MIN_NUM_INPUT_POINTS;

constexpr int GRID_LENGTH_DEFAULT   = 10;

constexpr int  RAND_SEED_DEFAULT    = 10;
constexpr bool TRUE_RANDOM_DEFAULT  = false;

constexpr bool ONLY_POINTS_DEFAULT  = false;


constexpr const char* ELAPSED_TIME_NAME_STRING          = "elapsedTime";
constexpr const char* COMP_RESULT_NAME_STRING           = "compResult";
constexpr const char* INPUT_POINTS_NAME_STRING          = "inputPoints";
constexpr const char* CONVEX_HULL_POINTS_NAME_STRING    = "convexHullPoints";
constexpr const char* RAND_SEED_NAME_STRING             = "randSeed";
constexpr const char* GRID_LENGTH_NAME_STRING           = "gridLength";
//////////////////////////////
constexpr const char* ARR_NAME_STRING           = "odcArrangement";
constexpr const char* ARR_POINTS_NAME_STRING    = "arrangementPoints";
///////
constexpr const char* ARR_FACES_NAME_STRING                     = "arrangementFaces";
constexpr const char* ARR_IND_FACE_NAME_STRING                  = "arrFace";
constexpr const char* ARR_FACE_NAME_PREFIX_NAME_STRING          = "arrFace__";
constexpr const char* ARR_FACE_CCW_V_INDICES_NAME_STRING        = "arrangementFaceCCWVIndices";
constexpr const char* ARR_OODC_INPUT_SITE_INDICES_NAME_STRING   = "arrangementOODCSiteIndices";
//////////////////////////////
constexpr const char* MST_NAME_STRING                                       = "minSpanningTree";
constexpr const char* MST_LENGTH_NAME_STRING                                = "mstLength";
constexpr const char* MST_EDGE_LIST_NAME_STRING                             = "mstEdgeList";
constexpr const char* MST_EDGE_NAME_STRING                                  = "mstEdge";
constexpr const char* MST_EDGE_ENDPOINT_INDICES_NAME_STRING                 = "mstEdgeEndpointIndices";
constexpr const char* MST_EDGE_FIRST_ENDPOINT_IS_STEINER_PT_NAME_STRING     = "firstEndPointIsSteiner";
constexpr const char* MST_EDGE_SECOND_ENDPOINT_IS_STEINER_PT_NAME_STRING    = "secondEndPointIsSteiner";
//////////////////////////////
constexpr const char* STEINER_PT_NAME_STRING                        = "steinerPoint";
constexpr const char* GEOM_MED_POINT_NAME_STRING                    = "geomMedPoint";
constexpr const char* GEOM_MED_NEIGHBOUR_INDICES_NAME_STRING        = "geomMedNeighbourIndices";
constexpr const char* GEOM_MED_COINCIDES_W_INPUT_PT_NAME_STRING     = "geomMedCoincidesWithInputPt";
constexpr const char* GEOM_MED_COINCIDENT_INPUT_INDEX_NAME_STRING   = "geomMedCoincidentInputPtIndex";
//////////////////////////////
constexpr const char* CANDIDATE_ST_PT_DATA_LIST_NAME_STRING     = "candidateStPtDataList";
constexpr const char* CANDIDATE_STEINER_POINT_DATA_NAME_STRING  = "candidateStPtData";

constexpr double DOUBLE_EPSILON = 0.000001;

// project namespace: one steiner point Georgakopolous-Papadimitriou algorithm (GPA)
namespace One_St_Pt_GPA
{
    using std::cout;
    using std::cerr;
    using std::endl;
    using std::vector;
    using std::string;
    using std::reference_wrapper;
    using std::ostringstream;
    using std::list;
    using std::size_t;
    using std::pair;
    using std::set;


//  Exact kernel with lazy interval-arithmetic evaluation (faster than
//  "EPECK_with_root_of" for typical inputs; both are fully exact).
    typedef CGAL::Lazy_exact_nt<CORE::Expr>         Lazy_FT;
    typedef CGAL::Simple_cartesian< Lazy_FT >       MyKernel;

    //https://doc.cgal.org/latest/Arrangement_on_surface_2/index.html
    typedef MyKernel::FT                            My_Number_type;
    typedef CGAL::Arr_segment_traits_2<MyKernel>    MyTraits_2;

    typedef MyTraits_2::Point_2                     MyPoint_2;
    typedef MyTraits_2::Segment_2                   MySegment_2;
    typedef MyTraits_2::Line_2                      MyLine_2;
    typedef MyTraits_2::Intersect_2                 MyIntersect_2;

    typedef MyTraits_2::Direction_2                 MyDirection_2;
    

    //enum class PointTypeEnum {INPUT, BEST_STEINER, STEINER, OTHER};
    struct MyFaceData 
    {
    //    vector< size_t > myInputPointIndices;
        set< size_t > myInputPointIndices;
        //TODO augment with something that lets us do a colouring of the faces
    };

    //https://doc.cgal.org/latest/Arrangement_on_surface_2/classCGAL_1_1Arr__face__overlay__traits.html
    //https://doc.cgal.org/latest/Arrangement_on_surface_2/Arrangement_on_surface_2_2overlay_unbounded_8cpp-example.html#_a4
    //Might only be able to use extended faces 
    //typedef CGAL::Arr_extended_dcel<MyTraits_2, PointTypeEnum, bool, MyFaceData>       MyDcel; //vertices, edges, then faces
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



    ////// Forward-declare Structs used in printing and utils ///////
    struct GeomMedianData;
    struct MyEMSTData;
    struct CandidateSteinerPointData;

    /////////////////////
    // Namespaces for utilities and printing.

    /*
    **
    **
    */
    namespace Utility_Functions
    {

        inline bool intInRangeInclusive(int x, int lowEnd, int highEnd)
        {
            return ((x >= lowEnd) && (x <= highEnd));
        }

        bool extractPointsFromJSON2DArrayString(const string & inputString, vector< MyPoint_2 > & result);

        //Assume the numbers aren't too large or small for doubles
        inline bool pointsAreTooClose(const MyPoint_2 & first, const MyPoint_2 & second)
        {
            return ( (fabs(CGAL::to_double(first.x()) - CGAL::to_double(second.x())) < DOUBLE_EPSILON) 
                    && (fabs(CGAL::to_double(first.y()) - CGAL::to_double(second.y())) < DOUBLE_EPSILON) );
        }

        bool findOriginIndex(const MyPoint_2 & cellOrigin, 
                            const vector< reference_wrapper< const MyPoint_2 > > & inputPointSet,
                            size_t & resultIndex);

        bool findPointIndex(const MyPoint_2 & pt, const vector< MyPoint_2 > & myColl, size_t & myIndex);

        inline void computeConvexHull(const vector< MyPoint_2 > & pointVec, vector< MyPoint_2 > & convexHullList)
        {
            //TODO does convex hull work w const mypoints?
            ch_akl_toussaint(pointVec.begin(), pointVec.end(), back_inserter(convexHullList));
            return;
        }    

        inline void computeConeRays(const MyDirection_2 & initialDirection, vector< MyDirection_2 > & coneRays)
        {
            //https://doc.cgal.org/latest/Cone_spanners_2/index.html  
            // construct the functor
            //TODO does the cones function work w vector of const dirs?
            CGAL::Compute_cone_boundaries_2<MyKernel> cones;    
            cones(6, initialDirection, coneRays.begin());

            return;
        }

        //Assumption: no duplicate points. The "std::set" insertion can't reliably tell if there are doubles 
        void insertArrangementPointsIntoPointSet(const MyArrangement_2 & resultODCArrangement, vector< MyPoint_2 > & arrPointsVec);

    } // namespace Utility_Functions


    /*
    **
    **
    */
    namespace Pretty_Print
    {
        inline string wrapStringInQuotes(const string & s)
        {
            return ("\"" + s + "\"");
        }

        inline string point2ToJSON(const MyPoint_2 & p)
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

#   if (MY_VERBOSE)
        //https://doc.cgal.org/latest/Arrangement_on_surface_2/index.html#title6 
        //&& examples/Arrangement_on_surface_2/bgl_dual_adapter.cpp 
        //&& examples/Arrangement_on_surface_2/arr_print.h
        inline void print_ccb (MyArrangement_2::Ccb_halfedge_const_circulator circ)
        {
            MyArrangement_2::Ccb_halfedge_const_circulator curr = circ;
            cout << "(" << curr->source()->point() << ")";
            do {
                MyArrangement_2::Halfedge_const_handle he = curr;
                cout << " [" << he->curve() << "] "
                << "(" << he->target()->point() << ")";
            } while (++curr != circ);
            cout << endl;
        }
#   endif

        string mstEdgeToJSONString(const pair< pair< size_t, size_t>, pair< bool, bool> > & edgeData, int tabLevel = 0);

        string mstDataToJSONString(const MyEMSTData & mst, int tabLevel = 0);

        string geomMedDataToJSONString(const GeomMedianData & stPtData, int tabLevel = 0);

        string candidateSteinerPtDataToJSONString(const CandidateSteinerPointData & stPtData, int tabLevel = 0);

        string steinerPointsToJSONString(const vector< CandidateSteinerPointData > & steinerPoints, int tabLevel = 0);

        string pointVectorToJSONString(string name, const vector< MyPoint_2 > & myColl, int tabLevel = 0);

        string vertexIndicesToJSONString(string name, const vector< MyPoint_2 > & myColl,
                                                        const vector< MyPoint_2 > & myPtSet, int tabLevel = 0);    

        string arrangementFaceToJSONString(string faceName, const MyArrangement_2::Face_const_iterator fit, 
                                                            const vector< MyPoint_2 > & myPtSet, int tabLevel = 0);

        string arrangementToJSONString(const MyArrangement_2 & resultODCArrangement, int tabLevel = 0);

    } // namespace Pretty_Print
} // namespace One_St_Pt_GPA
#endif
