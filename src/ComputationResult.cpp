#include "ComputationResult.hpp"

ComputationResult::ComputationResult(int numInputPoints,
                                     int randomSeed,
                                     int inputGridLength,
                                     bool trueRandomSeed,
                                     bool onlyProducePoints) : numPoints(numInputPoints),
                                                               randSeed(randomSeed),
                                                               gridLength(inputGridLength),
                                                               trueRandom(trueRandomSeed),
                                                               onlyPoints(onlyProducePoints),
                                                               rand(Random(randSeed)),
                                                               //https://doc.cgal.org/latest/Generator/classCGAL_1_1Random__points__in__square__2.html
                                                               randPointGen(Point_generator(gridLength, rand))
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
}

void ComputationResult::computeConvexHull(const set<MyPoint_2> &pointSet, vector<MyPoint_2> &result)
{
    ch_akl_toussaint(pointSet.begin(), pointSet.end(), back_inserter(result));

    return;
}



void ComputationResult::outputResultToJSONFile()
{
    ofstream myfile;

    myfile.open (OUTPUT_FILE);
    myfile << "{\n";
//    myfile << outputCollectionToJSONFile("inputPoints", );
    if(!onlyPoints){

    }
    myfile << "}"; 
    myfile.close();   

    return;
}



//https://stackoverflow.com/questions/5451305/how-to-make-function-argument-container-independent
//https://stackoverflow.com/questions/34561190/pass-a-std-container-to-a-function
template <typename Container>
string ComputationResult::outputCollectionToJSONFile(string name, const Container &myColl)
{
    ostringstream sStream;
    sStream << stringAsJSONString(name) << ":"; 
    
    return sStream.str();
}
