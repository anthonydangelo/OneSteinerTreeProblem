#ifndef ODC_HPP
#define ODC_HPP

#include "1ST-Constants-Utilities.h"

#include <CGAL/CORE_Expr.h>
#include <CGAL/Extended_cartesian.h>
#include <CGAL/Nef_polyhedron_2.h>

typedef CORE::Expr Nef_FT;
typedef Lazy_exact_nt<Nef_FT> Lazy_Nef_FT;
typedef Extended_cartesian<Lazy_Nef_FT> Nef_Extended_kernel;
typedef Nef_polyhedron_2<Nef_Extended_kernel> MyNef_polyhedron;

typedef MyNef_polyhedron::Point Nef_Point;
typedef MyNef_polyhedron::Line Nef_Line;
typedef MyNef_polyhedron::Direction Nef_Direction;
typedef MyNef_polyhedron::Explorer Nef_Explorer;

class OrientedDirichletCell
{

  public:
    OrientedDirichletCell(const MyDirection_2 &dirA, const MyDirection_2 &dirB,
                          const MyPoint_2 &cellOrigin, const set<MyPoint_2> &pointSet,
                          const vector<MyPoint_2> &clippingPolygonList);
};

#endif