#ifndef COMPUTATION_RESULT_HPP
#define COMPUTATION_RESULT_HPP

#include "1ST-Constants-Utilities.h"

#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/ch_akl_toussaint.h>
#include <CGAL/Compute_cone_boundaries_2.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/Random.h>

#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Polygon_set_2.h>

//https://doc.cgal.org/latest/Generator/index.html
typedef Creator_uniform_2<double, MyPoint_2> Creator;
typedef Random_points_in_square_2<MyPoint_2, Creator> Point_generator;

typedef Kernel::Direction_2 MyDirection_2;

typedef CGAL::Polygon_2<Kernel>                           MyPolygon_2;
typedef CGAL::Polygon_with_holes_2<Kernel>                MyPolygon_with_holes_2;
typedef CGAL::Polygon_set_2<Kernel>                       MyPolygon_set_2;


class ComputationResult
{

  protected:
    //https://stackoverflow.com/questions/11594846/default-member-values-best-practice
    int numPoints;
    int randSeed;
    int gridLength;
    bool onlyPoints;
    string outFilePrefix;

    Random rand;
    set<MyPoint_2> pointSet;
    Point_generator randPointGen;
    vector<MyPoint_2> convexHullList;

    vector<MyDirection_2> coneRays;
    MyPolygon_set_2 convexHull;

  public:
    ComputationResult(int numInputPoints = DEFAULT_NUM_INPUT_POINTS,
                      int randomSeed = RAND_SEED_DEFAULT,
                      int inputGridLength = GRID_LENGTH_DEFAULT,
                      bool onlyProducePoints = ONLY_POINTS_DEFAULT,
                      string outFilePrefix = "",
                      vector<MyPoint_2> userPointList = vector<MyPoint_2>());
    
    string outputResultToJSONString() const;

  protected:
    void computeConvexHull();
//    template <typename Container>
//    string outputCollectionToJSONFile(string name, const Container &myColl) const;
    string outputCollectionToJSONString(string name, const set<MyPoint_2> &myColl, int tabLevel=0) const;
    string outputCollectionToJSONString(string name, const vector<MyPoint_2> &myColl, int tabLevel=0) const;
    string outputConvexHullToJSONString(string name, const vector<MyPoint_2> &myColl, int tabLevel=0) const;
    void computeConeRays();    
    bool findPointIndex(const MyPoint_2 &pt, size_t &myIndex) const; 
};

#endif