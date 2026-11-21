//See OneSteinerTreeProblem.cpp for project notes

#include "ConstantsAndUtilities.hpp"

namespace One_St_Pt_GPA
{
    namespace Utility_Functions
    {

        bool extractPointsFromJSON2DArrayString(const string & inputString, vector< MyPoint_2 > & result)
        {
            //NB: The string processing is a bit more flexible than the PrintHelp message suggests
            //TODO: consider switching to using a regex here
            // Note: the parser is lenient (e.g. "[1x, 2]" reads as (1, 2), because stod
            // stops at the first non-numeric character). That is not part of the
            // supported input format; a stricter parser may reject such input.

            size_t strIndex = inputString.find_first_of("(");
            if (strIndex != string::npos) 
            {
                cerr << "\nERROR: found '(' at position " << strIndex
                    << " but only '[' is expected. No points were read in.\n";
                return false;
            }

            strIndex = inputString.find_first_of("[");
            if (string::npos == strIndex)
            {
                cerr << "\nERROR: Could not read input list. Expected '[' to open the list but got: " << endl;
                cerr << inputString << endl;
                return false;
            }

            while (strIndex != string::npos)
            {
                strIndex = inputString.find_first_of("[", strIndex + 1);
                if (strIndex != string::npos)
                {
                    try
                    {
                        string::size_type stodIndex;
                        double firstD = stod(inputString.substr(strIndex + 1), &stodIndex);
                        // stodIndex is relative to the substring, not the original string
                        // stodIndex is the position of the first character in the substring after the number.                        
                        strIndex = inputString.find_first_of(",", stodIndex + strIndex);
                        if (strIndex != string::npos)
                        {
                            double secondD = stod(inputString.substr(strIndex + 1));
                            // stod accepts "nan", "inf" and "infinity"; non-finite coordinates
                            // are undefined behaviour in the exact kernel, so reject them.
                            if (!std::isfinite(firstD) || !std::isfinite(secondD))
                            {
                                throw std::invalid_argument("coordinate is not a finite number");
                            }
                            result.emplace_back(firstD, secondD);
                        }
                    }
                    // stod throws invalid_argument or out_of_range, and the isfinite check
                    // above throws invalid_argument; both derive from std::logic_error.
                    catch (const std::logic_error & e)
                    {
                        cerr << endl << "Exception caught: " << e.what() << endl;
                        cerr << "ERROR: Input point list string malformed. Try surrounding the list in quotes." << endl;
                        if (result.empty())
                        {
                            cerr << "No points were read in." << endl;                          
                        } else
                        {
                            cerr << "Successfully read in " << result.size() << " point(s). The last point read in successfully was: ";
                            MyPoint_2 p = result.back();
                            cerr << "( " << p.x() << ", " << p.y() << " )" << endl;
                            cerr << "List cleared." << endl << endl;
                            result.clear();                              
                        }                  
                        return false;
                    }
                }
            }
            return true;
        }   

        // Currently unused; kept as a utility (finds a cell origin's index in the input list).
        bool findOriginIndex(const MyPoint_2 & cellOrigin, 
                            const vector< reference_wrapper< const MyPoint_2 > > & inputPointSet,
                            size_t & resultIndex)
        {
            resultIndex = 0;
            for (auto it = inputPointSet.begin(); it != inputPointSet.end(); ++it, ++resultIndex)
            {
                if (pointsAreTooClose(*it, cellOrigin))
                {
                    return true;
                }
            }
            return false;
        }    

        bool findPointIndex(const MyPoint_2 & pt, const vector< MyPoint_2 > & myColl, size_t & myIndex)
        {
            myIndex = 0;
            if(!myColl.empty()){
                auto endIt = end(myColl);
                for (auto it = begin(myColl); it != endIt; ++it, ++myIndex) {
                    if(pointsAreTooClose(*it, pt)){
                        return true;
                    }
                }    
            }
            return false;
        }

        // Sorting by exact coordinates makes the order independent of how the
        // arrangement was built. Faces then look up each corner's index by vertex,
        // exactly and in O(1). (Looking corners up by coordinates, within a
        // tolerance, gave distinct vertices closer than the tolerance the same index:
        // the double-precision cone directions create sliver faces whose corners are
        // about 1e-16 apart.)
        void indexArrangementVertices(const MyArrangement_2 & arr, VertexIndexMap & vertexIndices,
                                                                    vector< MyPoint_2 > & arrPoints)
        {
            vector< MyArrangement_2::Vertex_const_handle > vertices;
            for (MyArrangement_2::Vertex_const_iterator vit = arr.vertices_begin(); vit != arr.vertices_end(); ++vit)
            {
                if (!vit->is_isolated() && !vit->is_at_open_boundary())
                {
                    vertices.push_back(vit);
                }
            }
            std::sort(vertices.begin(), vertices.end(),
                [](MyArrangement_2::Vertex_const_handle a, MyArrangement_2::Vertex_const_handle b)
                {
                    return a->point() < b->point();   // exact; lexicographic (x, then y)
                });

            vertexIndices.clear();
            arrPoints.clear();
            for (size_t i = 0; i < vertices.size(); ++i)
            {
                const MyArrangement_2::Vertex & vertex = *vertices[i];
                vertexIndices[ & vertex] = i;
                arrPoints.push_back( approxMyPoint( vertex.point()));
            }

            return;
        }


        void indexArrangementVertices(const vector< MyArrangement_2::Face_const_handle > & faces,
                                      VertexIndexMap & vertexIndices, vector< MyPoint_2 > & arrPoints)
        {
            // Each face's corners, each vertex once (neighbouring faces share corners).
            vector< MyArrangement_2::Vertex_const_handle > vertices;
            std::set< const MyArrangement_2::Vertex * > seen;
            for (const MyArrangement_2::Face_const_handle & face : faces)
            {
                MyArrangement_2::Ccb_halfedge_const_circulator circ = face->outer_ccb();
                MyArrangement_2::Ccb_halfedge_const_circulator curr = circ;
                do {
                    const MyArrangement_2::Vertex_const_handle corner = curr->target();
                    if (seen.insert( & (*corner)).second)
                    {
                        vertices.push_back(corner);
                    }
                } while (++curr != circ);
            }
            std::sort(vertices.begin(), vertices.end(),
                [](MyArrangement_2::Vertex_const_handle a, MyArrangement_2::Vertex_const_handle b)
                {
                    return a->point() < b->point();   // exact; lexicographic (x, then y)
                });

            vertexIndices.clear();
            arrPoints.clear();
            for (size_t i = 0; i < vertices.size(); ++i)
            {
                const MyArrangement_2::Vertex & vertex = *vertices[i];
                vertexIndices[ & vertex] = i;
                arrPoints.push_back( approxMyPoint( vertex.point()));
            }

            return;
        }        

    } // namespace Utility_Functions
} // namespace One_St_Pt_GPA
