#ifndef ODC_HPP
#define ODC_HPP

#include "1ST-Constants-Utilities.h"

#include <CGAL/CORE_Expr.h>
#include <CGAL/Extended_cartesian.h>
#include <CGAL/Nef_polyhedron_2.h>

typedef CORE::Expr Nef_FT;
#if (MY_LAZY_COMPS)
  typedef Lazy_exact_nt<Nef_FT> Lazy_Nef_FT; //compiler can't convert non-lazy expr into lazy expr...
  typedef Extended_cartesian<Lazy_Nef_FT> Nef_Extended_kernel;
#else
  typedef Extended_cartesian<Nef_FT> Nef_Extended_kernel;
#endif
typedef Nef_polyhedron_2<Nef_Extended_kernel> MyNef_polyhedron;

typedef MyNef_polyhedron::Point Nef_Point;
typedef MyNef_polyhedron::Line Nef_Line;
typedef MyNef_polyhedron::Direction Nef_Direction;
typedef MyNef_polyhedron::Explorer Nef_Explorer;

class OrientedDirichletCell
{
  protected: 
    //member initialization order depends on the order we list them here, apparently
    Nef_Direction     firstDir;
    Nef_Direction     secondDir;
    Nef_Point         cellOriginPoint;
    MyArrangement_2   odcArr;

  public:
    OrientedDirichletCell(const MyDirection_2 &dirA, const MyDirection_2 &dirB,
                          const MyPoint_2 &cellOrigin, 
                          const size_t &originPtIndex,
                          const vector< reference_wrapper<const MyPoint_2> > &inputPointSet, //vector instead of a set so we can use 'reliable' indices
                          const MyNef_polyhedron &clippingPolygon);
    const MyArrangement_2 & getCellArrangement() const;

  protected:
    void computeCell(MyNef_polyhedron &result, const MyNef_polyhedron &clippingPolygon, 
                      const vector< reference_wrapper<const MyPoint_2> > &inputPointSet, size_t originPtIndex);  
    void extractArrangement( MyArrangement_2 &result, const Nef_Explorer &myExplorer, const size_t &originIndex);
};

#endif