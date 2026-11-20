#ifndef CONSTANTS_AND_UTILITIES_HPP
#define CONSTANTS_AND_UTILITIES_HPP

//See OneSteinerTreeProblem.cpp for project notes

// C++ standard library
#include <algorithm>    // std::sort
#include <cassert>      // assert
#include <chrono>       // std::chrono::steady_clock (memory checkpoints)
#include <cmath>        // std::fabs, std::floor, std::isfinite
#include <cstddef>      // std::size_t
#include <cstdlib>
#include <fstream>      // std::ifstream (memory checkpoints)
#include <functional>   // std::reference_wrapper
#include <iomanip>      // std::setprecision
#include <iostream>
#include <iterator>     // std::back_inserter, std::next
#include <limits>       // std::numeric_limits
#include <list>
#include <set>
#include <sstream>      // std::ostringstream
#include <stdexcept>    // std::invalid_argument
#include <string>
#include <unordered_map> // VertexIndexMap
#include <vector>

// CGAL
#include <CGAL/CORE_Expr.h>                  // CORE::Expr (used in the Lazy_FT typedef)
#include <CGAL/Gmpq.h>                       // CGAL::Gmpq (used in the OODC_FT typedef)
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Simple_cartesian.h>           // MyKernel, OODC_Kernel

#include <CGAL/Arr_segment_traits_2.h>       // OODC_Traits_2
#include <CGAL/Arr_extended_dcel.h>          // Arr_face_extended_dcel
#include <CGAL/Arrangement_2.h>              // MyArrangement_2
#include <CGAL/Arr_overlay_2.h>              // CGAL::overlay
#include <CGAL/Arr_default_overlay_traits.h> // Arr_face_overlay_traits

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
// Linux 4.19 added fs.protected_regular; distributions enable it by default
// (Ubuntu since 20.04). When enabled, it blocks cross-user O_CREAT opens in
// such directories. 
// If this program is ever run by a different user than the one who 
// created the lock file, open() will fail with EACCES.
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
constexpr const char* FLOCK_PATH = "/tmp/one-st-tree-mutex.lock";

// When set to 1 outputs debug messages
#define MY_VERBOSE                  (0)

// EXCLUDE_CELL_BOUNDARY has no visible effect on the output or runtime: it is only used
// by the complement-based ODC construction (the difference-based construction
// always uses INCLUDED), and cell boundaries become arrangement edges either
// way, while only faces yield candidates.
#define EXCLUDE_CELL_BOUNDARY       (1)  

// 1: build each ODC by subtracting the other sites' regions from the cone
//    (clipped to the hull first).
// 0: build the union of everything that is not the cell, then complement it.
// Both give identical output on the test suites; 1 was about 10% faster there
// (measured with rational OODC arithmetic).
// Can be overridden at build time, e.g. make release EXTRA_FLAGS=-DBUILD_ODCELL_BY_DIFFERENCES=0
#ifndef BUILD_ODCELL_BY_DIFFERENCES
#   define BUILD_ODCELL_BY_DIFFERENCES (1)
#endif

// When set to 1, prints memory checkpoints (current and peak memory use) to stderr
// at phase boundaries; see Utility_Functions::memoryCheckpoint. Linux only. Build
// with "make checkpoints" rather than changing this line: the extra stderr output
// would make test runs differ.
#ifndef MEMORY_CHECKPOINTS
#   define MEMORY_CHECKPOINTS          (0)
#endif

//magic numbers
constexpr int MIN_NUM_INPUT_POINTS      = 3;
constexpr int MAX_NUM_INPUT_POINTS      = 10;
constexpr int DEFAULT_NUM_INPUT_POINTS  = MIN_NUM_INPUT_POINTS;

constexpr int GRID_LENGTH_DEFAULT   = 10;
constexpr int GRID_LENGTH_MIN       = 1;

constexpr int  RAND_SEED_DEFAULT    = 10;
constexpr bool TRUE_RANDOM_DEFAULT  = false;

constexpr bool ONLY_POINTS_DEFAULT  = false;
constexpr bool BEST_ONLY_DEFAULT    = false;


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

// Project namespace: one Steiner point, via the Georgakopoulos-Papadimitriou algorithm (GPA)
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


//  Two exact kernels. Lazy_exact_nt evaluates with interval arithmetic first
//  and falls back to the exact number type only when the intervals can't
//  decide a comparison.
//
//  MyKernel (exact number type CORE::Expr) is for the geometric medians and
//  the MSTs: MST lengths are sums of square roots (CGAL::sqrt in
//  DelaunayTriEMST::findMST), and CORE::Expr represents square roots exactly.
//  In practice this was faster than EPECK_with_root_of; both are exact.
    typedef CGAL::Lazy_exact_nt<CORE::Expr>         Lazy_FT;
    typedef CGAL::Simple_cartesian< Lazy_FT >       MyKernel;

    typedef MyKernel::FT                            My_Number_type;
    typedef MyKernel::Point_2                       MyPoint_2;
    typedef MyKernel::Segment_2                     MySegment_2;
    typedef MyKernel::Line_2                        MyLine_2;
    typedef MyKernel::Intersect_2                   MyIntersect_2;
    typedef MyKernel::Direction_2                   MyDirection_2;

//  OODC_Kernel (exact number type Gmpq, a GMP rational) is for the OODC: the
//  cells (Nef_2, see OrientedDirichletCell.hpp), their arrangements, and the
//  overlays. Every OODC coordinate is rational: the input points are doubles,
//  the cone directions are doubles (see computeConeRays), and every
//  construction is a line, a midpoint or a line intersection. Rationals are
//  cheaper to compute with than CORE::Expr, and Lazy_exact_nt discards a
//  value's construction history once its exact value is known, while a
//  CORE::Expr keeps its whole expression tree. (Profiling with CORE::Expr here
//  showed tiny arrangements but overlays taking seconds, nearly all of it
//  spent evaluating CORE expressions.)
    typedef CGAL::Lazy_exact_nt<CGAL::Gmpq>         OODC_FT;
    typedef CGAL::Simple_cartesian< OODC_FT >       OODC_Kernel;

    //https://doc.cgal.org/latest/Arrangement_on_surface_2/index.html
    typedef CGAL::Arr_segment_traits_2<OODC_Kernel> OODC_Traits_2;
    typedef OODC_Kernel::Point_2                    OODC_Point_2;
    typedef OODC_Kernel::Segment_2                  OODC_Segment_2;
    typedef OODC_Kernel::Direction_2                OODC_Direction_2;
    

    // Label of an arrangement face: the indices of the sites whose oriented
    // Dirichlet cell (ODC) contains the face. Cells of the same cone
    // direction have disjoint interiors (together they partition the convex
    // hull), so a face lies in at most one cell per direction: at most 6
    // labels per face. This is what makes the number of candidate Steiner
    // points per face constant (at most C(6,3) + C(6,4) = 35).
    struct MyFaceData 
    {
        set< size_t > myInputPointIndices;
        //TODO augment with something that lets us do a colouring of the faces
    };

    // Only faces carry data (their labels), so a face-extended DCEL is enough.
    // TODO: a fully extended DCEL (CGAL::Arr_extended_dcel) could also store
    // vertex data, e.g. the type of each point (input, best Steiner,
    // candidate Steiner, other), and edge data.
    // https://doc.cgal.org/latest/Arrangement_on_surface_2/classCGAL_1_1Arr__face__overlay__traits.html
    // https://doc.cgal.org/latest/Arrangement_on_surface_2/Arrangement_on_surface_2_2overlay_unbounded_8cpp-example.html#_a4
    typedef CGAL::Arr_face_extended_dcel<OODC_Traits_2, MyFaceData>    MyDcel;
    typedef CGAL::Arrangement_2<OODC_Traits_2, MyDcel>                 MyArrangement_2;

    // The JSON index of each arrangement vertex, keyed by the vertex's address
    // (which stays valid as long as the arrangement isn't modified).
    typedef std::unordered_map< const MyArrangement_2::Vertex *, size_t > VertexIndexMap;    

    // Overlay functor: creates the label of each overlay face. Each face of an
    // overlay lies inside one face of each input arrangement, so its label is
    // the union of those two labels. The unbounded face of a single cell's
    // arrangement has an empty label, so it adds nothing.
    struct Overlay_faces
    {
        MyFaceData operator() (const MyFaceData & first, const MyFaceData & second) const
        {
                MyFaceData result = first;
                result.myInputPointIndices.insert(second.myInputPointIndices.begin(),
                                                  second.myInputPointIndices.end());
                return result;
        }
    };
    typedef CGAL::Arr_face_overlay_traits<MyArrangement_2, MyArrangement_2, MyArrangement_2, Overlay_faces>  MyOverlay_Traits;



    ////// Forward-declare Structs used in printing and utils ///////
    struct GeomMedianData;
    struct MyEMSTData;
    struct MstEdgeEndpoints;
    struct CandidateSteinerPointData;

    /////////////////////
    // Namespaces for utilities and printing.

    // Utility_Functions: range checks, input parsing, and arrangement/point-set helpers.
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
            return ( (std::fabs(CGAL::to_double(first.x()) - CGAL::to_double(second.x())) < DOUBLE_EPSILON) 
                    && (std::fabs(CGAL::to_double(first.y()) - CGAL::to_double(second.y())) < DOUBLE_EPSILON) );
        }

        bool findOriginIndex(const MyPoint_2 & cellOrigin, 
                            const vector< reference_wrapper< const MyPoint_2 > > & inputPointSet,
                            size_t & resultIndex);

        bool findPointIndex(const MyPoint_2 & pt, const vector< MyPoint_2 > & myColl, size_t & myIndex);

        inline void computeConvexHull(const vector< MyPoint_2 > & pointVec, vector< MyPoint_2 > & convexHullList)
        {
            // Reads the points through const iterators; writes the hull vertices
            // (counter-clockwise; points in the interior of hull edges are omitted)
            // to convexHullList.
            ch_akl_toussaint(pointVec.begin(), pointVec.end(), back_inserter(convexHullList));
            return;
        }    

        // Note: for MyKernel and OODC_Kernel, CGAL's Compute_cone_boundaries_2 uses
        // its generic version, which rotates the initial direction using
        // std::sin/std::cos (double precision); the exact version exists only for
        // Exact_predicates_exact_constructions_kernel_with_root_of. So the six
        // boundaries are 60 degrees apart only up to double rounding (they are
        // stored exactly as rationals). Also used for the 60-degree rotations in
        // GeomMedianFinder, so 3-point geometric medians are likewise accurate
        // to double precision rather than exact.
        // A template so that it works for both kernels: the kernel is deduced
        // from the direction type.
        template < class Direction_2 >
        inline void computeConeRays(const Direction_2 & initialDirection, vector< Direction_2 > & coneRays)
        {
            typedef typename CGAL::Kernel_traits< Direction_2 >::Kernel Kernel;            
            //https://doc.cgal.org/latest/Cone_spanners_2/index.html  
            // Construct the functor. The functor writes the 6 rays through an output iterator, so coneRays
            // must already hold 6 (writable, so non-const) elements.            
            assert(coneRays.size() == 6);
            CGAL::Compute_cone_boundaries_2< Kernel > cones;    
            cones(6, initialDirection, coneRays.begin());

            return;
        }

        // Midpoint of the interval approximation that a lazy number carries. Cheap, and
        // never triggers an exact computation (CGAL::to_double may: Lazy_exact_nt.h
        // falls back to exact() when the interval is wider than 1e-5 relative).
        inline double approxValue(const My_Number_type & x)
        {
            const std::pair< double, double > interval = CGAL::to_interval(x);
            return (interval.first + interval.second) / 2;
        }        

        // Converts a MyKernel point to the OODC kernel. Only for points with double
        // coordinates (input points, convex hull vertices), so the conversion is
        // exact; the assert checks that each coordinate is exactly a double.
        inline OODC_Point_2 toOODCPoint(const MyPoint_2 & p)
        {
            assert(CGAL::to_interval(p.x()).first == CGAL::to_interval(p.x()).second);
            assert(CGAL::to_interval(p.y()).first == CGAL::to_interval(p.y()).second);
            return OODC_Point_2(CGAL::to_double(p.x()), CGAL::to_double(p.y()));
        }

        // Converts an OODC point (e.g. an arrangement vertex) to a MyKernel point
        // with double approximations of its coordinates. Approximate: only for output.
        inline MyPoint_2 approxMyPoint(const OODC_Point_2 & p)
        {
            return MyPoint_2(CGAL::to_double(p.x()), CGAL::to_double(p.y()));
        }        

        // Numbers the arrangement's vertices for the JSON output (isolated vertices
        // excluded): sorted by exact coordinates, each vertex's index is its position
        // in that order. Fills vertexIndices (vertex -> index) and arrPoints (each
        // vertex's double approximation, in index order); clears both first.
        void indexArrangementVertices(const MyArrangement_2 & arr, VertexIndexMap & vertexIndices,
                                                                    vector< MyPoint_2 > & arrPoints);

#if (MEMORY_CHECKPOINTS)
        // Prints one line to stderr:
        //   MEMORY {"t_s": <seconds since the first checkpoint>, "current_kB": <n>,
        //           "peak_kB": <n>, "label": "<label>"}
        // "current" is the resident memory (VmRSS) and "peak" its maximum so far (VmHWM),
        // as Linux reports them in /proc/self/status. Memory the program frees usually
        // stays with the allocator for reuse, so "current" rarely goes down: what to look
        // for is where it grows.
        inline void memoryCheckpoint(const string & label)
        {
            static const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            long currentKB = -1;
            long peakKB = -1;
            std::ifstream status("/proc/self/status");
            string line;
            while (std::getline(status, line))
            {
                std::istringstream fields(line);
                string key;
                long value = 0;
                if (fields >> key >> value)
                {
                    if (key == "VmRSS:")
                    {
                        currentKB = value;
                    }
                    else if (key == "VmHWM:")
                    {
                        peakKB = value;
                    }
                }
            }

            const double seconds = std::chrono::duration< double >(std::chrono::steady_clock::now() - start).count();
            string escapedLabel;
            for (char c : label)
            {
                if (c == '"' || c == '\\')
                {
                    escapedLabel += '\\';      // JSON escapes " and \ with a backslash
                }
                escapedLabel += c;
            }
            ostringstream out;
            out << "MEMORY {\"t_s\": " << std::fixed << std::setprecision(1) << seconds
                << ", \"current_kB\": " << currentKB << ", \"peak_kB\": " << peakKB
                << ", \"label\": \"" << escapedLabel << "\"}";
            cerr << out.str() << endl;
            
            return;
        }

        // "V=<vertices> E=<edges> F=<faces>" for a checkpoint label.
        inline string arrangementSize(const MyArrangement_2 & arr)
        {
            return "V=" + std::to_string(arr.number_of_vertices()) +
                   " E=" + std::to_string(arr.number_of_edges()) +
                   " F=" + std::to_string(arr.number_of_faces());
        }
#endif        

    } // namespace Utility_Functions

    // MEMORY_CHECKPOINT(label) records a checkpoint when MEMORY_CHECKPOINTS is 1; when it
    // is 0 it does nothing and the label is not even evaluated.
#if (MEMORY_CHECKPOINTS)
#   define MEMORY_CHECKPOINT(label) One_St_Pt_GPA::Utility_Functions::memoryCheckpoint(label)
#else
#   define MEMORY_CHECKPOINT(label) ((void)0)
#endif

    // Pretty_Print: JSON and text formatting of points, MSTs, candidates, and the arrangement.
    namespace Pretty_Print
    {
        inline string wrapStringInQuotes(const string & s)
        {
            return ("\"" + s + "\"");
        }

        inline string point2ToJSON(const MyPoint_2 & p)
        {
            ostringstream sStream;
            // Lazy_exact_nt prints its double approximation; the default stream
            // precision (6 significant digits) would drop most of it.
            sStream << std::setprecision(std::numeric_limits<double>::max_digits10);
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
            
            return;
        }
#   endif

        // RAII: saves a stream's formatting state (flags such as boolalpha, and the
        // precision) and restores it when the saver goes out of scope, on every
        // way out of the enclosing function, including exceptions.
        class StreamStateSaver
        {
            public:
                explicit StreamStateSaver(std::ostream & s)
                    : stream(s), flags(s.flags()), precision(s.precision()) {}
                ~StreamStateSaver()
                {
                    stream.flags(flags);
                    stream.precision(precision);
                }
                // A copy would restore the state a second time, at the wrong moment.
                StreamStateSaver(const StreamStateSaver &)             = delete;
                StreamStateSaver & operator=(const StreamStateSaver &) = delete;
                StreamStateSaver(StreamStateSaver &&)             = delete;
                StreamStateSaver & operator=(StreamStateSaver &&) = delete;                

            private:
                std::ostream &           stream;
                const std::ios::fmtflags flags;
                const std::streamsize    precision;
        };

        // The write*JSON functions write straight to the given stream (e.g. cout)
        // instead of building strings: the full output can be hundreds of MB, and
        // assembling it in memory took several copies of it. They set the stream's
        // precision (max_digits10) and boolalpha where they print doubles and bools.
        void writeMstEdgeJSON(std::ostream & out, const MstEdgeEndpoints & edgeData, int tabLevel = 0);

        void writeMstDataJSON(std::ostream & out, const MyEMSTData & mst, int tabLevel = 0);

        void writeGeomMedDataJSON(std::ostream & out, const GeomMedianData & stPtData, int tabLevel = 0);

        void writeCandidateSteinerPtDataJSON(std::ostream & out, const CandidateSteinerPointData & stPtData, int tabLevel = 0);

        void writeSteinerPointsJSON(std::ostream & out, const vector< CandidateSteinerPointData > & steinerPoints, int tabLevel = 0);

        void writePointVectorJSON(std::ostream & out, const string & name, const vector< MyPoint_2 > & myColl, int tabLevel = 0);

        void writeVertexIndicesJSON(std::ostream & out, const string & name, const vector< MyPoint_2 > & myColl,
                                                                             const vector< MyPoint_2 > & myPtSet, int tabLevel = 0);    

        void writeArrangementFaceJSON(std::ostream & out, const string & faceName, const MyArrangement_2::Face_const_iterator fit, 
                                                                                   const VertexIndexMap & vertexIndices, int tabLevel = 0);        

        void writeArrangementJSON(std::ostream & out, const MyArrangement_2 & resultODCArrangement, int tabLevel = 0);

    } // namespace Pretty_Print
} // namespace One_St_Pt_GPA
#endif
