#ifndef GMFINDER_HPP
#define GMFINDER_HPP

#include "1ST-Constants-Utilities.h"

#include <CGAL/Exact_circular_kernel_2.h>
#include <CGAL/Circle_2.h>
#include <CGAL/Circular_kernel_intersections.h>
#include <CGAL/intersections.h>


//CGAL::Gmpq is the parameter to the Algebraic and kernel in the predefined filtered exact kernel...
//also, they use cartesian kernel, i use simple_cartesian, which apparently means the objects aren't ref. counted. 
//"This eases debugging, but may slow down algorithms that copy objects intensively."
typedef Filtered_bbox_circular_kernel_2<
                                        Circular_kernel_2<MyKernel, Algebraic_kernel_for_circles_2_2<Lazy_FT> > >   MyCircular_k;

typedef CGAL::Point_2<MyCircular_k>                 CircularPoint_2; //apparently necessary for circle intersection points
typedef CGAL::Circle_2<MyCircular_k>                MyCircle_2;
typedef CGAL::Circular_arc_2<MyCircular_k>          MyCircular_arc_2;

typedef typename CGAL::CK2_Intersection_traits<MyCircular_k, MyCircle_2, MyCircle_2>::type  CircleIntersection_result;

typedef struct GeomMedianData{
    MyPoint_2 medPoint;
    const vector<size_t>& inputPtIndices;
    bool coincidesWithInputPt = false;
    size_t coincidentInputPtIndex = 0;
    GeomMedianData(const vector<size_t>& inputSet) :inputPtIndices(inputSet){}
} GeomMedianData;

class GeomMedianFinder
{
  public:
    GeomMedianFinder();
    GeomMedianData computeGeomMedian3Pts(const vector<MyPoint_2>& myPts, const vector<size_t>& originalInputPtIndices) const;
};

#endif