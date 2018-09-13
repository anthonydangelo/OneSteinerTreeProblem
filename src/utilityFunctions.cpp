//See OneSteinerTreeProblem.cpp for project notes

#include "1ST-Constants-Utilities.h"

/*
**
** Why namespace Vasco_Rossi? 
** Apart from sounding like it could be the name of his next album,
** it's just more interesting than "Utility_Functions". 
** Plus, since it's likely that I'm the only one who's ever going
** to see/use this, I can call it whatever I want as long as he doesn't mind. I hope he doesn't...
** Plus I'm a fan.
**
 */
namespace Vasco_Rossi
{

    void extractPointsFromJSON2DArrayString(string &inputString, vector< MyPoint_2 >& result)
    {
        //I'd like to use a regex here, but I don't know how to write the grammar...
        //http://www.cplusplus.com/reference/string/string/find_first_of/
        std::size_t strIndex = inputString.find_first_of("[");
        //I don't like the way I'm doing this parsing!
        while (strIndex != std::string::npos)
        {
            strIndex = inputString.find_first_of("[", strIndex + 1);
            if (strIndex != string::npos)
            {
                try
                {
                    //http://www.cplusplus.com/reference/string/stod/
                    string::size_type stodIndex;
                    double firstD = stod(inputString.substr(strIndex + 1), &stodIndex);
                    strIndex = inputString.find_first_of(",", stodIndex + strIndex);
                    if (strIndex != string::npos)
                    {
                        double secondD = stod(inputString.substr(strIndex + 1));
                        //the temp assigned to the const ref apparently dies when we leave the function, leaving dangling ref
//                        const MyPoint_2& temp = MyPoint_2(My_Number_type(firstD), My_Number_type(secondD));
//                        result.push_back(temp);
                        result.emplace_back(firstD, secondD);
                    }
                }
                catch (exception &e)
                {
                    cerr << e.what() << endl;
                    cerr << "Input point list string malformed" << endl;
                    result.clear();
                    return;
                }
            }
        }
        return;
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

    bool findPointIndex(const MyPoint_2 &pt, const vector< MyPoint_2 >& myColl, size_t &myIndex)
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
    void insertArrangementPointsIntoPointSet(const MyArrangement_2& resultODCArrangement, vector< MyPoint_2 >& arrPointsVec)
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


} // namespace Vasco_Rossi
