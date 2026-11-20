#ifndef ODC_HPP
#define ODC_HPP

//See OneSteinerTreeProblem.cpp for project notes

#include "ConstantsAndUtilities.hpp"

#include <CGAL/Extended_cartesian.h>
#include <CGAL/Nef_polyhedron_2.h>

namespace One_St_Pt_GPA
{
    // Same number type as OODC_Kernel, so a standard Nef point (Nef_Point) is
    // an OODC_Point_2 (Extended_cartesian<FT>'s standard kernel is
    // Simple_cartesian<FT>): no conversion between the cells and their
    // arrangements.
    typedef CGAL::Extended_cartesian<OODC_FT>           Nef_Extended_kernel;    
    typedef CGAL::Nef_polyhedron_2<Nef_Extended_kernel> MyNef_polyhedron;

    typedef MyNef_polyhedron::Point         Nef_Point;
    typedef MyNef_polyhedron::Line          Nef_Line;
    typedef MyNef_polyhedron::Direction     Nef_Direction;
    typedef MyNef_polyhedron::Explorer      Nef_Explorer;

    class OrientedDirichletCell
    {
        private: 
            //member initialization order depends on the order we list them here
            const Nef_Direction     firstDir;
            const Nef_Direction     secondDir;
            const Nef_Point         cellOriginPoint;
            MyArrangement_2   odcArr;

        public:
            OrientedDirichletCell(const OODC_Direction_2 & dirA, const OODC_Direction_2 & dirB,
                                const OODC_Point_2 & cellOrigin, 
                                size_t originPtIndex,
                                const vector< OODC_Point_2 > & inputPointSet, //vector instead of a set so we can use 'reliable' indices
                                const MyNef_polyhedron & clippingPolygon);
            const MyArrangement_2 & getCellArrangement() const;

        private:
            void computeCell(MyNef_polyhedron & result, const MyNef_polyhedron & clippingPolygon, 
                            const vector< OODC_Point_2 > & inputPointSet, size_t originPtIndex);  
            void extractArrangement( MyArrangement_2 & result, const Nef_Explorer & myExplorer, size_t originIndex);
            MyNef_polyhedron regionToRemove(const OODC_Point_2 & otherSite, MyNef_polyhedron::Boundary inclusion) const;
    };
} // namespace One_St_Pt_GPA
#endif
