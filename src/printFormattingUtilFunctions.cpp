//See OneSteinerTreeProblem.cpp for project notes

#include "1ST-Constants-Utilities.h"
#include "DelaunayTriEMST.hpp"
#include "GeomMedianFinder.hpp"
#include "ComputationResult.hpp"

/*
**
** Why namespace Marisa_Tomei? 
** A few of her movies have been on tv lately. 
** They left me wanting to see more of her...
** I think it's a fitting name for pretty print functions.
**
** Plus, since it's likely that I'm the only one who's ever going
** to see/use this, I can call it whatever I want as long as she doesn't mind. I hope she doesn't...
** Plus, I'm a fan.
**
 */
namespace Marisa_Tomei
{

    string mstEdgeToJSONString(const pair< pair<size_t, size_t>, pair<bool, bool> >& edgeData, int tabLevel)
    {
        ostringstream sStream;
        sStream << insertTabs(tabLevel);
        sStream << wrapStringInQuotes(MST_EDGE_NAME_STRING) << ": { \n";

        sStream << insertTabs(tabLevel + 1);
        sStream << wrapStringInQuotes(MST_EDGE_ENDPOINT_INDICES_NAME_STRING) <<  ": [ \n";

        sStream << insertTabs(tabLevel + 3);
        sStream << "{\"index\":\"" << edgeData.first.first << "\"},\n";
        sStream << insertTabs(tabLevel + 3);
        sStream << "{\"index\":\"" << edgeData.first.second << "\"}\n";

        sStream << insertTabs(tabLevel+3);
        sStream << "],\n"; 

        sStream << boolalpha;
        sStream << insertTabs(tabLevel + 1);
        sStream << wrapStringInQuotes(MST_EDGE_FIRST_ENDPOINT_IS_STEINER_PT_NAME_STRING) << ": \"" << edgeData.second.first << "\",\n";
        sStream << insertTabs(tabLevel + 1);
        sStream << wrapStringInQuotes(MST_EDGE_SECOND_ENDPOINT_IS_STEINER_PT_NAME_STRING) << ": \"" << edgeData.second.second << "\"\n";

        sStream << insertTabs(tabLevel);
        sStream << "}";
        return sStream.str();
    }

    string mstDataToJSONString(const MyEMSTData& mst, int tabLevel)
    {
        ostringstream sStream;
        sStream << insertTabs(tabLevel);
        sStream << wrapStringInQuotes(MST_NAME_STRING) << ": { ";

        sStream << wrapStringInQuotes(MST_LENGTH_NAME_STRING) << " : \"" << mst.length << "\",\n";

        sStream << insertTabs(tabLevel+2);
        sStream << wrapStringInQuotes(MST_EDGE_LIST_NAME_STRING) << ": [ \n";

        auto endIt = mst.mstEdgePointIndices.end();
        for (auto myIt = mst.mstEdgePointIndices.begin(); myIt != endIt; ++myIt)
        {
            sStream << insertTabs(tabLevel + 4);
            sStream << "{ \n";

            sStream << mstEdgeToJSONString(*myIt, tabLevel + 5) << "\n";

            sStream << insertTabs(tabLevel + 4);
            sStream << "}";
            if (next(myIt) != endIt)
            {
                sStream << ",";
            }
            sStream << "\n";
        }

        sStream << insertTabs(tabLevel+4);
        sStream << "]\n";

        sStream << insertTabs(tabLevel);
        sStream << "}";
        return sStream.str();
    }

    string geomMedDataToJSONString(const GeomMedianData& stPtData, int tabLevel)
    {
        ostringstream sStream;
        sStream << insertTabs(tabLevel);
        sStream << wrapStringInQuotes(STEINER_PT_NAME_STRING) << ": { \n";

        sStream << insertTabs(tabLevel+1);
        sStream << wrapStringInQuotes(GEOM_MED_POINT_NAME_STRING) << " : " << point2ToJSON(stPtData.medPoint) << ",\n";

        sStream << insertTabs(tabLevel+1);
        sStream << wrapStringInQuotes(GEOM_MED_NEIGHBOUR_INDICES_NAME_STRING) << ": [ \n";

        auto endIt = stPtData.inputPtIndices.end();
        for (auto myIt = stPtData.inputPtIndices.begin(); myIt != endIt; ++myIt)
        {
            sStream << insertTabs(tabLevel + 3);
            sStream << "{\"index\":\"" << *myIt << "\"}";
            if (next(myIt) != endIt)
            {
                sStream << ",";
            }
            sStream << "\n";        
        }

        sStream << insertTabs(tabLevel+3);
        sStream << "],\n"; 

        sStream << boolalpha;
        sStream << insertTabs(tabLevel+1);
        sStream << wrapStringInQuotes(GEOM_MED_COINCIDES_W_INPUT_PT_NAME_STRING) << ": \"" << stPtData.coincidesWithInputPt << "\",\n";    
        sStream << insertTabs(tabLevel+1);
        sStream << wrapStringInQuotes(GEOM_MED_COINCIDENT_INPUT_INDEX_NAME_STRING) << ": {\"index\":\"" << stPtData.coincidentInputPtIndex << "\"}\n";

        sStream << insertTabs(tabLevel);
        sStream << "}"; 
        return sStream.str();    
    }

    string candidateSteinerPtDataToJSONString(const CandidateSteinerPointData& stPtData, int tabLevel)
    {
        ostringstream sStream;
        sStream << insertTabs(tabLevel);
        sStream << wrapStringInQuotes(CANDIDATE_STEINER_POINT_DATA_NAME_STRING) << ": { \n";

        sStream << geomMedDataToJSONString(stPtData.steinerPt, tabLevel+1) << ",\n";
        sStream << mstDataToJSONString(stPtData.mstData, tabLevel+1) << "\n";

        sStream << insertTabs(tabLevel);
        sStream << "}"; 
        return sStream.str();  
    }

    string steinerPointsToJSONString(const vector<CandidateSteinerPointData>& steinerPoints, int tabLevel)
    {
        ostringstream sStream;
        sStream << insertTabs(tabLevel);
        sStream << wrapStringInQuotes(CANDIDATE_ST_PT_DATA_LIST_NAME_STRING) << ": [ \n";

        auto endIt = end(steinerPoints);
        for (auto it = begin(steinerPoints); it != endIt; ++it)
        {
            sStream << insertTabs(tabLevel + 2);
            sStream << "{ \n";
            sStream << candidateSteinerPtDataToJSONString(*it, tabLevel + 3);
            sStream << "\n";
            sStream << insertTabs(tabLevel + 2);
            sStream << "}";
            if (next(it) != endIt)
            {
                sStream << ",";
            }
            sStream << "\n";
        }

        sStream << insertTabs(tabLevel+2);
        sStream << "]"; 
        return sStream.str();     
    }

    string pointVectorToJSONString(string name, const vector< MyPoint_2 >& myColl, int tabLevel)
    {
        ostringstream sStream;
        sStream << insertTabs(tabLevel);
        sStream << wrapStringInQuotes(name) << ": [\n";
    
        if(!myColl.empty()){
            auto endIt = end(myColl);
            for (auto it = begin(myColl); it != endIt; ++it) {
                sStream << insertTabs(tabLevel + 2);
                sStream << point2ToJSON(*it); 
                if(next(it) != endIt){
                    sStream << ",";
                } 
                sStream << "\n";
            }    
        }

        sStream << insertTabs(tabLevel+2);
        sStream << "]"; 
    
        return sStream.str();
    }

    string vertexIndicesToJSONString(string name, const vector<MyPoint_2> &myColl,
                                                    const vector<MyPoint_2>& myPtSet, int tabLevel)
    {

        ostringstream sStream;
        sStream << insertTabs(tabLevel);
        sStream << wrapStringInQuotes(name) << ": [\n";
    
        if(!myColl.empty()){
            auto endIt = end(myColl);
            for (auto it = begin(myColl); it != endIt; ++it) {
                size_t index = 0;
                assert(Vasco_Rossi::findPointIndex(*it, myPtSet, index));
                sStream << insertTabs(tabLevel + 2);
                sStream << "{\"index\":\"" << index << "\"}"; 
                if(next(it) != endIt){
                    sStream << ",";
                }
                sStream << "\n";
            }    
        }

        sStream << insertTabs(tabLevel+2);
        sStream << "]";
    
        return sStream.str();
    }    

    string arrangementFaceToJSONString(string faceName, const MyArrangement_2::Face_const_iterator fit, 
                                                        const vector<MyPoint_2>& myPtSet, int tabLevel)
    {

        ostringstream sStream;
        sStream << insertTabs(tabLevel);
        sStream << wrapStringInQuotes(ARR_IND_FACE_NAME_STRING) << ": {";
    
        sStream << "\"name\": " << wrapStringInQuotes(faceName) << " ,\n";
        ////////////////////
        sStream << insertTabs(tabLevel + 1);
        sStream << wrapStringInQuotes(ARR_FACE_CCW_V_INDICES_NAME_STRING) <<  ": [ \n";

        MyArrangement_2::Ccb_halfedge_const_circulator circ = fit->outer_ccb();
        MyArrangement_2::Ccb_halfedge_const_circulator curr = circ;
        MyArrangement_2::Halfedge_const_handle heEnd = circ; //don't know if this does an implicit conversion; best it do it here, not in the expr
        do {
            MyArrangement_2::Halfedge_const_handle he = curr;
            size_t index = 0;
            assert(Vasco_Rossi::findPointIndex(he->target()->point(), myPtSet, index));
            sStream << insertTabs(tabLevel + 3);
            sStream << "{\"index\":\"" << index << "\"}";
            if(he->next() != heEnd){
                sStream << ",";
            }        
            sStream << "\n";
        } while (++curr != circ);

        sStream << insertTabs(tabLevel+3);
        sStream << "],\n";
        //////////////////////

        sStream << insertTabs(tabLevel + 1);
        sStream << wrapStringInQuotes(ARR_OODC_INPUT_SITE_INDICES_NAME_STRING) <<  ": [ \n";

        //static cast instead?
        MyFaceData myFaceStruct = (MyFaceData)(fit->data());
        auto endIt = end(myFaceStruct.myInputPointIndices);
        for (auto it = begin(myFaceStruct.myInputPointIndices); it != endIt; ++it) {
            sStream << insertTabs(tabLevel + 3);
            sStream << "{\"index\":\"" << *it << "\"}"; 
            if(next(it) != endIt){
                sStream << ",";
            }
            sStream << "\n";
        }             
        sStream << insertTabs(tabLevel + 3);
        sStream << "]\n";
        //////////////////////

        sStream << insertTabs(tabLevel);
        sStream << "}"; 
    
        return sStream.str();
    }    

    string arrangementToJSONString(const MyArrangement_2& resultODCArrangement, int tabLevel)
    {
        vector< MyPoint_2 > arrPoints;
        Vasco_Rossi::insertArrangementPointsIntoPointSet(resultODCArrangement, arrPoints);

        ostringstream sStream;
        sStream << insertTabs(tabLevel);
        sStream << wrapStringInQuotes(ARR_NAME_STRING) << ": {\n";

        sStream << pointVectorToJSONString(ARR_POINTS_NAME_STRING, arrPoints, tabLevel + 2) << ",\n";

        sStream << insertTabs(tabLevel+2);
        sStream << wrapStringInQuotes(ARR_FACES_NAME_STRING) << ": [ \n";

        size_t faceIndex = 0;
        auto endFit = resultODCArrangement.faces_end();
        for (MyArrangement_2::Face_const_iterator fit = resultODCArrangement.faces_begin(); fit != endFit; ++fit)
        {
            if (!fit->is_unbounded())
            {
                ostringstream faceSStream;
                faceSStream << ARR_FACE_NAME_PREFIX_NAME_STRING << faceIndex++;
                sStream << insertTabs(tabLevel + 4);
                sStream << "{ \n";

                sStream << arrangementFaceToJSONString(faceSStream.str(), fit, arrPoints, tabLevel + 5) << "\n";

                sStream << insertTabs(tabLevel + 4);
                sStream << "}";
                if(next(fit) != endFit) 
                {
                    sStream << ",";
                } 
                sStream << "\n";
            }
        }

        sStream << insertTabs(tabLevel+4);
        sStream << "]\n";

        sStream << insertTabs(tabLevel);
        sStream << "}";
    
        return sStream.str();
    }

} // namespace Marisa_Tomei
