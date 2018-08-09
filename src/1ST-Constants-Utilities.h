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
#include <cstddef>        // std::size_t


//https://github.com/CGAL/cgal/blob/master/Generator/examples/Generator/random_grid.cpp
//https://doc.cgal.org/latest/Kernel_23/classCGAL_1_1Exact__predicates__exact__constructions__kernel.html
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/Random.h>
#include <CGAL/algorithm.h>
#include <CGAL/ch_akl_toussaint.h>

using namespace CGAL;


//https://doc.cgal.org/latest/Kernel_23/Kernel_23_2exact_8cpp-example.html#_a0
//https://doc.cgal.org/latest/Kernel_23/Kernel_23_2intersection_visitor_8cpp-example.html#_a0
//https://doc.cgal.org/latest/Kernel_d/group__PkgKernelDFunctions.html#ga0aa3e8b6bdf1bff509f8e2672ef194d1
typedef Exact_predicates_exact_constructions_kernel Kernel;
typedef Kernel::Point_2 MyPoint_2;
typedef Kernel::Segment_2 Segment_2;
typedef Kernel::Line_2 Line_2;
typedef Kernel::Intersect_2 Intersect_2;

//https://doc.cgal.org/latest/Generator/index.html
typedef Creator_uniform_2<double, MyPoint_2>             Creator;
typedef Random_points_in_square_2<MyPoint_2, Creator> Point_generator;


using namespace std;


#define MY_VERBOSE                  (0)

//magic numbers
#define MIN_NUM_INPUT_POINTS        (3)
#define MAX_NUM_INPUT_POINTS        (10)
#define DEFAULT_NUM_INPUT_POINTS    (MIN_NUM_INPUT_POINTS)

#define GRID_LENGTH_DEFAULT  (10)

#define RAND_SEED_DEFAULT    (10)
#define TRUE_RANDOM_DEFAULT  (false)

#define ONLY_POINTS_DEFAULT  (false)

#define OUTPUT_FILE                 ("-result.json")
#define INPUT_POINTS_NAME_STRING    ("inputPoints")


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

    sStream << "{\"x\":\"" << p.x() << "\", " << "\"y\":\"" << p.y() << "\"}";
    return sStream.str();
}

static inline string insertTabs(int level)
{   
    ostringstream sStream;
    for(int i = 0; i < level; ++i){
        sStream << "\t";
    }
    return sStream.str();
}

static inline void extractPointsFromJSON2DArrayString(string &inputString, vector<MyPoint_2> &result){
    //I'd like to use a regex here, but I don't know how to write the grammar...
    //http://www.cplusplus.com/reference/string/string/find_first_of/
    std::size_t strIndex = inputString.find_first_of("[");
    //I don't like the way I'm doing this parsing!    
    while (strIndex != std::string::npos)
    {
        strIndex = inputString.find_first_of("[", strIndex + 1);
        if(strIndex != string::npos){
            try{
                //http://www.cplusplus.com/reference/string/stod/
                string::size_type stodIndex;
                double firstD = stod(inputString.substr(strIndex + 1), &stodIndex);
                strIndex = inputString.find_first_of(",", stodIndex + strIndex);
                if(strIndex != string::npos){
                    double secondD = stod(inputString.substr(strIndex + 1));
                    result.push_back(MyPoint_2(firstD, secondD));
                }
            }
            catch (...) {
                cerr << "Input point list string malformed" <<endl;
                result.clear();
                return;
            }
        }
    }
    return;
}

#endif