#include "ComputationResult.hpp"

ComputationResult::ComputationResult(int numInputPoints,
                                     int randomSeed,
                                     int inputGridLength,
                                     bool trueRandomSeed,
                                     bool onlyProducePoints,
                                     string outputFilePrefix) : numPoints(numInputPoints),
                                                               randSeed(randomSeed),
                                                               gridLength(inputGridLength),
                                                               trueRandom(trueRandomSeed),
                                                               onlyPoints(onlyProducePoints),
                                                               rand(Random(randSeed)),
                                                               //https://doc.cgal.org/latest/Generator/classCGAL_1_1Random__points__in__square__2.html
                                                               randPointGen(Point_generator(gridLength, rand)),
                                                               outFilePrefix(outputFilePrefix)
{
    //https://doc.cgal.org/latest/Generator/index.html#GeneratorExample_2
    while (pointSet.size() < numPoints)
    {
        pointSet.insert(*randPointGen++);
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



void ComputationResult::outputResultToJSONFile() const
{
    ofstream myfile;

    myfile.open (outFilePrefix + OUTPUT_FILE);
    myfile << "{\n";
    myfile << outputCollectionToJSONString(INPUT_POINTS_NAME_STRING, pointSet);
    if(!onlyPoints){

    }
    myfile << "\n}"; 
    myfile.close();   

    return;
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
