#include "ComputationResult.hpp"

ComputationResult::ComputationResult(int numInputPoints,
                                     int randomSeed,
                                     int inputGridLength,
                                     bool onlyProducePoints,
                                     string outputFilePrefix,
                                     vector<MyPoint_2> userPointList) : numPoints(numInputPoints),
                                                               randSeed(randomSeed),
                                                               gridLength(inputGridLength),
                                                               onlyPoints(onlyProducePoints),
                                                               rand(Random(randSeed)),
                                                               //https://doc.cgal.org/latest/Generator/classCGAL_1_1Random__points__in__square__2.html
                                                               randPointGen(Point_generator(gridLength, rand)),
                                                               outFilePrefix(outputFilePrefix),
                                                               rays(6)
{

    for (const auto pt : userPointList)
    {
        if(pointSet.size() < numPoints){
            std::pair<std::set<MyPoint_2>::iterator,bool> ret;
            ret = pointSet.insert(pt);
            if (ret.second == false){
                cerr << "tried to add an element that was already in there. size is " << pointSet.size() << endl; 
            }            
        } else {
            break;
        }
    }

    //https://doc.cgal.org/latest/Generator/index.html#GeneratorExample_2
    while (pointSet.size() < numPoints)
    {
        std::pair<std::set<MyPoint_2>::iterator,bool> ret;
        MyPoint_2 temp = *randPointGen++;
        bool inSet = false;
        for(const auto pt : pointSet) 
        {
            if( pointsAreTooClose(pt, temp) ){
                inSet = true;
                break;
            }
        }
        if (inSet){
            continue;
        }
        ret = pointSet.insert(temp);
        if (ret.second == false){
            cerr << "tried to add an element that was already in there. size is " << pointSet.size() << endl; 
        }
    }

#if (MY_VERBOSE)

    /* std::ostream_iterator< MyPoint_2 > out( std::cout, " " );
 std::copy(pointSet.begin(), pointSet.end(), out); */

    for (const auto pt : pointSet)
    {
        cout << " " << pt << endl;
    }

    cout << outputCollectionToJSONString(INPUT_POINTS_NAME_STRING, pointSet, 1);
#endif

    if(onlyPoints){
        return;
    }

    computeConvexHull(pointSet, convexHullList);
#if (MY_VERBOSE)    

/* std::ostream_iterator< MyPoint_2 > out( std::cout, " " );
 std::copy(pointSet.begin(), pointSet.end(), out); */
    cout << "Convex Hull\n";
    for(const auto pt : convexHullList)
    {
        cout << " " << pt << endl;
    }
#endif     

    return;
} //constructor


void ComputationResult::computeConvexHull(const set<MyPoint_2> &pointSet, vector<MyPoint_2> &result)
{
    ch_akl_toussaint(pointSet.begin(), pointSet.end(), back_inserter(result));

    return;
}



string ComputationResult::outputResultToJSONString() const
{
    ostringstream sStream;
    sStream << "{\n";  
    sStream << outputCollectionToJSONString(INPUT_POINTS_NAME_STRING, pointSet);
    if(!onlyPoints){

    }
    sStream << "\n}";

    return sStream.str();
}



/* //https://stackoverflow.com/questions/5451305/how-to-make-function-argument-container-independent
//https://stackoverflow.com/questions/34561190/pass-a-std-container-to-a-function
template <typename Container>
string ComputationResult::outputCollectionToJSONFile(string name, const Container &myColl) const
{
    ostringstream sStream;
    sStream << wrapStringInQuotes(name) << ": [";
    
    if(!myColl.empty()){
        for (auto it = begin(myColl); it != end(vec); ++it) {
            typedef decltype(*it) T; // if needed
            sStream << *it << " "; 
        }    
    }

    sStream << "]" << endl; 
    
    return sStream.str();
} */


string ComputationResult::outputCollectionToJSONString(string name, const set<MyPoint_2> &myColl, int tabLevel) const
{
    ostringstream sStream;
    sStream << insertTabs(tabLevel);
    sStream << wrapStringInQuotes(name) << ": [";
    
    if(!myColl.empty()){
        auto endIt = end(myColl);
        for (auto it = begin(myColl); it != endIt; ++it) {
            sStream << point2ToJSON(*it); 
            if(next(it) != endIt){
                sStream << ",\n";
                sStream << insertTabs(tabLevel + 2);
            } else {
                sStream << "\n" << insertTabs(tabLevel);
            }
        }    
    }

    sStream << "]" << endl; 
    
    return sStream.str();
}
