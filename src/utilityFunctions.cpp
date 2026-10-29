//See OneSteinerTreeProblem.cpp for project notes

#include "1ST-Constants-Utilities.h"

namespace One_St_Pt_GPA
{
    /*
    **
    **
    */
    namespace Utility_Functions
    {
        using std::exception;

        bool extractPointsFromJSON2DArrayString(const string &inputString, vector< MyPoint_2 > &result)
        {
            //NB: The string processing is a bit more flexible than the PrintHelp message suggests
            //TODO: consider switching to using a regex here

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

            while (strIndex != std::string::npos)
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
                            result.emplace_back(firstD, secondD);
                        }
                    }
                    catch (exception &e)
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

        bool findOriginIndex(const MyPoint_2 &cellOrigin, 
                            const vector< reference_wrapper<const MyPoint_2> > &inputPointSet,
                            size_t &resultIndex)
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

        bool findPointIndex(const MyPoint_2 &pt, const vector< MyPoint_2 > &myColl, size_t &myIndex)
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

        //Assumption: no duplicate points. The "std::set" insertion can't reliably tell if there are doubles (I've experienced this...)
        void insertArrangementPointsIntoPointSet(const MyArrangement_2 &resultODCArrangement, vector< MyPoint_2 > &arrPointsVec)
        {
            set<MyPoint_2> arrPoints;
            for(auto it = resultODCArrangement.vertices_begin(); it != resultODCArrangement.vertices_end(); ++it)
            {
                if (! it->is_isolated() && ! it->is_at_open_boundary())
                {
                    const MyPoint_2& tempPt = it->point();
                    arrPoints.emplace(tempPt.x(), tempPt.y());
                }
            }    
            for (const MyPoint_2& p : arrPoints)
            {
                //we could probably turn this into a vec of refs, but it's not clear if the obvious way works
                arrPointsVec.emplace_back(p.x(), p.y());
            }
            return;
        }    


    } // namespace Utility_Functions
} // namespace One_St_Pt_GPA
