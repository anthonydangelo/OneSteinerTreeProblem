#include "OrientedDirichletCell.hpp"

OrientedDirichletCell::OrientedDirichletCell(const MyDirection_2 &dirA,
                                             const MyDirection_2 &dirB,
                                             const MyPoint_2 &cellOrigin,
                                             const set<MyPoint_2> &pointSet,
                                             const vector<MyPoint_2> &clippingPolygonList)
{
    MyNef_polyhedron myPolygon(clippingPolygonList.begin(), clippingPolygonList.end(), MyNef_polyhedron::INCLUDED);
    Nef_Point sitePoint(cellOrigin.x(), cellOrigin.y());
    myPolygon = myPolygon.intersection(MyNef_polyhedron(Nef_Line(sitePoint, Nef_Direction(dirA.dx(), dirA.dy())), MyNef_polyhedron::INCLUDED));    
    myPolygon = myPolygon.intersection(MyNef_polyhedron(Nef_Line(sitePoint, Nef_Direction(-dirB.dx(), -dirB.dy())), MyNef_polyhedron::INCLUDED));    

}
