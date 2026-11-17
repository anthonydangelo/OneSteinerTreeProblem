#ifndef COMPUTATION_RESULT_HPP
#define COMPUTATION_RESULT_HPP

//See OneSteinerTreeProblem.cpp for project notes

#include "1ST-Constants-Utilities.h"

#include <CGAL/point_generators_2.h>
#include <CGAL/Random.h>
#include "CGAL/Combination_enumerator.h"

#include "OrientedDirichletCell.hpp"
#include "DelaunayTriEMST.hpp"
#include "GeomMedianFinder.hpp"

namespace One_St_Pt_GPA
{
    //https://doc.cgal.org/latest/Generator/index.html
    typedef CGAL::Creator_uniform_2<double, MyPoint_2>          Creator;
    typedef CGAL::Random_points_in_square_2<MyPoint_2, Creator> Point_generator;

    struct CandidateSteinerPointData
    {
        
        GeomMedianData steinerPt;
        MyEMSTData mstData;

        bool operator<(const CandidateSteinerPointData & other) const
        {
            return mstData < other.mstData; //compares lengths
        }

        // Constructs both members directly from the given values. Needed because:
        //  - GeomMedianData has no default constructor (it must be given its input
        //    indices), so steinerPt can't be created empty and assigned later; and
        //  - emplace_back(steinerPt, mstData) constructs the element with parentheses,
        //    which (before C++20) requires a constructor rather than an aggregate.
        CandidateSteinerPointData(const GeomMedianData & myStPt, const MyEMSTData & myMstData) : steinerPt(myStPt), mstData(myMstData) {}        
    };

    class ComputationResult
    {

        private:
            //https://stackoverflow.com/questions/11594846/default-member-values-best-practice
            int numPoints;
            int randSeed;
            int gridLength;
            bool onlyPoints;

            CGAL::Random rand;
            Point_generator     randPointGen;
            vector< MyPoint_2 > convexHullList;

            vector< MyDirection_2 > coneRays;
            vector< MyPoint_2 >     inputPtVector;
            MyArrangement_2         resultODCArrangement;

            MyEMSTData origMST;

            vector< CandidateSteinerPointData > steinerPoints;

        public:
            explicit ComputationResult(int numInputPoints = DEFAULT_NUM_INPUT_POINTS,
                                        int randomSeed = RAND_SEED_DEFAULT,
                                        int inputGridLength = GRID_LENGTH_DEFAULT,
                                        bool onlyProducePoints = ONLY_POINTS_DEFAULT,
                                        const vector< MyPoint_2 > & userPointList = vector< MyPoint_2 >());
            
            string outputResultToJSONString() const;

            // Not copyable or movable: randPointGen refers to this object's own rand, so a
            // copy's generator would keep using the original's random number generator.
            ComputationResult(const ComputationResult &)                = delete;
            ComputationResult & operator=(const ComputationResult &)    = delete;
            ComputationResult(ComputationResult &&)                     = delete;
            ComputationResult & operator=(ComputationResult &&)         = delete;                        

        private:
            void preparePointSet(const vector< MyPoint_2 > & userPointList);
            void computeOODC();
            void computeMSTAndStPts(); 
            void computePotentialStPts(const vector<size_t > & siteIndices, const int numComboPts, 
                                        vector< GeomMedianData > & results, set< vector< size_t > > & seenList) const; 
            vector< GeomMedianData > computeStPtsForOODC() const;
    };
} // namespace One_St_Pt_GPA
#endif