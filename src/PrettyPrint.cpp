//See OneSteinerTreeProblem.cpp for project notes

#include "ConstantsAndUtilities.hpp"
#include "DelaunayTriEMST.hpp"
#include "GeomMedianFinder.hpp"
#include "ComputationResult.hpp"


namespace One_St_Pt_GPA
{
    namespace Pretty_Print
    {
        using std::boolalpha;
        
        void writeMstEdgeJSON(std::ostream & out, const MstEdgeEndpoints & edgeData, int tabLevel)
        {
            out << insertTabs(tabLevel);
            out << wrapStringInQuotes(MST_EDGE_NAME_STRING) << ": { \n";

            out << insertTabs(tabLevel + 1);
            out << wrapStringInQuotes(MST_EDGE_ENDPOINT_INDICES_NAME_STRING) <<  ": [ \n";

            out << insertTabs(tabLevel + 3);
            out << "{\"index\":\"" << edgeData.first.index << "\"},\n";
            out << insertTabs(tabLevel + 3);
            out << "{\"index\":\"" << edgeData.second.index << "\"}\n";

            out << insertTabs(tabLevel+3);
            out << "],\n"; 

            out << boolalpha;
            out << insertTabs(tabLevel + 1);
            out << wrapStringInQuotes(MST_EDGE_FIRST_ENDPOINT_IS_STEINER_PT_NAME_STRING) << ": \"" 
                << (VertexRef::Kind::STEINER == edgeData.first.kind) << "\",\n";
            out << insertTabs(tabLevel + 1);
            out << wrapStringInQuotes(MST_EDGE_SECOND_ENDPOINT_IS_STEINER_PT_NAME_STRING) << ": \"" 
                << (VertexRef::Kind::STEINER == edgeData.second.kind) << "\"\n";

            out << insertTabs(tabLevel);
            out << "}";
            return;
        }

        void writeMstDataJSON(std::ostream & out, const MyEMSTData & mst, int tabLevel)
        {
            out << std::setprecision(std::numeric_limits<double>::max_digits10);
            out << insertTabs(tabLevel);
            out << wrapStringInQuotes(MST_NAME_STRING) << ": { ";

            out << wrapStringInQuotes(MST_LENGTH_NAME_STRING) << " : \"" << mst.length << "\",\n";

            out << insertTabs(tabLevel+2);
            out << wrapStringInQuotes(MST_EDGE_LIST_NAME_STRING) << ": [ \n";

            auto endIt = mst.mstEdgePointIndices.end();
            for (auto myIt = mst.mstEdgePointIndices.begin(); myIt != endIt; ++myIt)
            {
                out << insertTabs(tabLevel + 4);
                out << "{ \n";

                writeMstEdgeJSON(out, *myIt, tabLevel + 5);

                out << "\n";

                out << insertTabs(tabLevel + 4);
                out << "}";
                if (next(myIt) != endIt)
                {
                    out << ",";
                }
                out << "\n";
            }

            out << insertTabs(tabLevel+4);
            out << "]\n";

            out << insertTabs(tabLevel);
            out << "}";
            return;
        }

        void writeGeomMedDataJSON(std::ostream & out, const GeomMedianData & stPtData, int tabLevel)
        {
            out << insertTabs(tabLevel);
            out << wrapStringInQuotes(STEINER_PT_NAME_STRING) << ": { \n";

            out << insertTabs(tabLevel+1);
            out << wrapStringInQuotes(GEOM_MED_POINT_NAME_STRING) << " : " << point2ToJSON(stPtData.medPoint) << ",\n";

            out << insertTabs(tabLevel+1);
            out << wrapStringInQuotes(GEOM_MED_NEIGHBOUR_INDICES_NAME_STRING) << ": [ \n";

            auto endIt = stPtData.inputPtIndices.end();
            for (auto myIt = stPtData.inputPtIndices.begin(); myIt != endIt; ++myIt)
            {
                out << insertTabs(tabLevel + 3);
                out << "{\"index\":\"" << *myIt << "\"}";
                if (next(myIt) != endIt)
                {
                    out << ",";
                }
                out << "\n";        
            }

            out << insertTabs(tabLevel+3);
            out << "],\n"; 

            out << boolalpha;
            out << insertTabs(tabLevel+1);
            out << wrapStringInQuotes(GEOM_MED_COINCIDES_W_INPUT_PT_NAME_STRING) << ": \"" << stPtData.coincidesWithInputPt << "\",\n";    
            out << insertTabs(tabLevel+1);
            out << wrapStringInQuotes(GEOM_MED_COINCIDENT_INPUT_INDEX_NAME_STRING) << ": {\"index\":\"" << stPtData.coincidentInputPtIndex << "\"}\n";

            out << insertTabs(tabLevel);
            out << "}"; 
            return;    
        }

        void writeCandidateSteinerPtDataJSON(std::ostream & out, const CandidateSteinerPointData & stPtData, int tabLevel)
        {
            out << insertTabs(tabLevel);
            out << wrapStringInQuotes(CANDIDATE_STEINER_POINT_DATA_NAME_STRING) << ": { \n";

            writeGeomMedDataJSON(out, stPtData.steinerPt, tabLevel+1);

            out << ",\n";
            writeMstDataJSON(out, stPtData.mstData, tabLevel+1);
            out << "\n";

            out << insertTabs(tabLevel);
            out << "}"; 
            return;  
        }

        void writeSteinerPointsJSON(std::ostream & out, const vector< CandidateSteinerPointData > & steinerPoints, int tabLevel)
        {
            out << insertTabs(tabLevel);
            out << wrapStringInQuotes(CANDIDATE_ST_PT_DATA_LIST_NAME_STRING) << ": [ \n";

            auto endIt = end(steinerPoints);
            for (auto it = begin(steinerPoints); it != endIt; ++it)
            {
                out << insertTabs(tabLevel + 2);
                out << "{ \n";
                writeCandidateSteinerPtDataJSON(out, *it, tabLevel + 3);
                out << "\n";
                out << insertTabs(tabLevel + 2);
                out << "}";
                if (next(it) != endIt)
                {
                    out << ",";
                }
                out << "\n";
            }

            out << insertTabs(tabLevel+2);
            out << "]"; 
            return;     
        }

        void writePointVectorJSON(std::ostream & out, const string & name, const vector< MyPoint_2 > & myColl, int tabLevel)
        {
            out << insertTabs(tabLevel);
            out << wrapStringInQuotes(name) << ": [\n";
        
            if(!myColl.empty()){
                auto endIt = end(myColl);
                for (auto it = begin(myColl); it != endIt; ++it) {
                    out << insertTabs(tabLevel + 2);
                    out << point2ToJSON(*it); 
                    if(next(it) != endIt){
                        out << ",";
                    } 
                    out << "\n";
                }    
            }

            out << insertTabs(tabLevel+2);
            out << "]"; 
        
            return;
        }

        void writeVertexIndicesJSON(std::ostream & out, const string & name, const vector< MyPoint_2 > & myColl,
                                                              const vector< MyPoint_2 > & myPtSet, int tabLevel)
        {

            out << insertTabs(tabLevel);
            out << wrapStringInQuotes(name) << ": [\n";
        
            if(!myColl.empty()){
                auto endIt = end(myColl);
                for (auto it = begin(myColl); it != endIt; ++it) {
                    size_t index = 0;
                    bool found = Utility_Functions::findPointIndex(*it, myPtSet, index);
                    assert(found);
                    out << insertTabs(tabLevel + 2);
                    out << "{\"index\":\"" << index << "\"}"; 
                    if(next(it) != endIt){
                        out << ",";
                    }
                    out << "\n";
                }    
            }

            out << insertTabs(tabLevel+2);
            out << "]";
        
            return;
        }    

        void writeArrangementFaceJSON(std::ostream & out, const string & faceName, const MyArrangement_2::Face_const_iterator fit, 
                                                                    const VertexIndexMap & vertexIndices, int tabLevel)        
        {

            out << insertTabs(tabLevel);
            out << wrapStringInQuotes(ARR_IND_FACE_NAME_STRING) << ": {";
        
            out << "\"name\": " << wrapStringInQuotes(faceName) << " ,\n";
            ////////////////////
            out << insertTabs(tabLevel + 1);
            out << wrapStringInQuotes(ARR_FACE_CCW_V_INDICES_NAME_STRING) <<  ": [ \n";

            MyArrangement_2::Ccb_halfedge_const_circulator circ = fit->outer_ccb();
            MyArrangement_2::Ccb_halfedge_const_circulator curr = circ;
            MyArrangement_2::Halfedge_const_handle heEnd = circ; //don't know if this does an implicit conversion; best it do it here, not in the expr
            do {
                MyArrangement_2::Halfedge_const_handle he = curr;
                const MyArrangement_2::Vertex & corner = *(he->target());
                const VertexIndexMap::const_iterator found = vertexIndices.find( & corner);                
                assert(found != vertexIndices.end());
                const size_t index = found->second;                
                out << insertTabs(tabLevel + 3);
                out << "{\"index\":\"" << index << "\"}";
                if(he->next() != heEnd){
                    out << ",";
                }        
                out << "\n";
            } while (++curr != circ);

            out << insertTabs(tabLevel+3);
            out << "],\n";
            //////////////////////

            out << insertTabs(tabLevel + 1);
            out << wrapStringInQuotes(ARR_OODC_INPUT_SITE_INDICES_NAME_STRING) <<  ": [ \n";

            const MyFaceData & myFaceStruct = fit->data();
            auto endIt = end(myFaceStruct.myInputPointIndices);
            for (auto it = begin(myFaceStruct.myInputPointIndices); it != endIt; ++it) {
                out << insertTabs(tabLevel + 3);
                out << "{\"index\":\"" << *it << "\"}"; 
                if(next(it) != endIt){
                    out << ",";
                }
                out << "\n";
            }             
            out << insertTabs(tabLevel + 3);
            out << "]\n";
            //////////////////////

            out << insertTabs(tabLevel);
            out << "}"; 
        
            return;
        }    

        void writeArrangementJSON(std::ostream & out, const MyArrangement_2 & resultODCArrangement, int tabLevel)
        {
            // Exact vertex indices, so each face corner can be looked up by vertex.
            VertexIndexMap vertexIndices;
            vector< MyPoint_2 > arrPoints;   // double approximations, for printing
            Utility_Functions::indexArrangementVertices(resultODCArrangement, vertexIndices, arrPoints);

            out << insertTabs(tabLevel);
            out << wrapStringInQuotes(ARR_NAME_STRING) << ": {\n";

            writePointVectorJSON(out, ARR_POINTS_NAME_STRING, arrPoints, tabLevel + 2);

            out << ",\n";

            out << insertTabs(tabLevel+2);
            out << wrapStringInQuotes(ARR_FACES_NAME_STRING) << ": [ \n";

            size_t faceIndex = 0;
            auto endFit = resultODCArrangement.faces_end();
            for (MyArrangement_2::Face_const_iterator fit = resultODCArrangement.faces_begin(); fit != endFit; ++fit)
            {
                if (!fit->is_unbounded())
                {
                    if (faceIndex > 0)
                    {
                        out << ",\n";   // separator before every face except the first
                    }
                    ostringstream faceSStream;
                    faceSStream << ARR_FACE_NAME_PREFIX_NAME_STRING << faceIndex++;
                    out << insertTabs(tabLevel + 4);
                    out << "{ \n";

                    writeArrangementFaceJSON(out, faceSStream.str(), fit, vertexIndices, tabLevel + 5);

                    out << "\n";

                    out << insertTabs(tabLevel + 4);
                    out << "}";
                }
            }
            out << "\n";

            out << insertTabs(tabLevel+4);
            out << "]\n";

            out << insertTabs(tabLevel);
            out << "}";
        
            return;
        }

    } // namespace Pretty_Print
} //namespace One_St_Pt_GPA
