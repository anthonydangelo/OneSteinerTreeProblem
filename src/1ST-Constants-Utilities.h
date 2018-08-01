#ifndef ONE_ST_CONSTANTS_UTILITIES
#define ONE_ST_CONSTANTS_UTILITIES

#include <iostream>
#include <sstream> // for ostringstream
#include <string>
//#include <unordered_set> //need to provide a hasher or something for this...
#include <set>
#include <vector>

#include <unistd.h>
#include <cstdlib>
#include <fstream>


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


#define MY_VERBOSE                  (1)

//magic numbers
#define MIN_NUM_INPUT_POINTS        (3)
#define MAX_NUM_INPUT_POINTS        (10)
#define DEFAULT_NUM_INPUT_POINTS    (MIN_NUM_INPUT_POINTS)

#define GRID_LENGTH_DEFAULT  (10)

#define RAND_SEED_DEFAULT    (10)
#define TRUE_RANDOM_DEFAULT  (false)

#define ONLY_POINTS_DEFAULT  (false)

#define OUTPUT_FILE          ("result.json")


/* bool intInRangeInclusive(int x, int lowEnd, int highEnd){
    return ((x >= lowEnd) && (x <= highEnd));
}

string stringAsJSONString(string s)
{
    return ("\"" + s + "\"");
} */

//why do these need to be static?
static bool intInRangeInclusive(int x, int lowEnd, int highEnd){
    return ((x >= lowEnd) && (x <= highEnd));
}

static string stringAsJSONString(string s)
{
    return ("\"" + s + "\"");
}

#endif