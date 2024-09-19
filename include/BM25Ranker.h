#include<string>
#include<vector>
#include<unordered_map>
#include <queue>
#include "InvertedIndex.h"
#include <stack>
#ifndef RANKER_H
#define RANKER_H
class BM25Ranker
{
public:
    typedef  std::unordered_map<std::string,double> ScoresDocument;
    static void setRankerParameters(double BM25_K1,double BM25_B,double PHRASE_BOOST_VALUE, double EXACT_MATCH_WEIGHT);
    BM25Ranker(const std::string&database_name,const std::string&documents_collection_name, InvertedIndex* invertedIndex);
    std::vector<std::pair<std::string,double>> run(const std::string& query_string);
    void setDatabaseName(const std::string&databaseName);
    void setDocumentsCollectionName(const std::string&collectionName);
private:
    ScoresDocument ProcessQuery(const std::string& query);
    ScoresDocument calculatePhraseScore(const std::string& phrase);
    ScoresDocument calculateTermScore(const std::string& term);
    ScoresDocument documentNOTOperation(const ScoresDocument& operand);
    void documentPrintOperation(const ScoresDocument& operand);
    ScoresDocument documentsANDOperation(const ScoresDocument& operand1,const ScoresDocument& operand2);
    ScoresDocument documentsADDOperation(const ScoresDocument& operand1,const ScoresDocument& operand2,double operand1Boost = 1, double operand2Boost=1);
    ScoresDocument documentsOROperation(const ScoresDocument& operand1,const ScoresDocument& operand2);
    void documentsStackCombineOperation(std::stack<ScoresDocument> &documentsStack);    
    ScoresDocument documentNormalizeOperation(const ScoresDocument& operand);
    void extractInvertedIndexAndMetadata();
    ScoresDocument getEmptyScoresDocument();
    ScoresDocument calculateBM25ScoreForTerm(const std::string& term);
    ScoresDocument calculateBM25ScoreForPhrase(const std::unordered_map<std::string,int>& documentsMap);
    
    // void normalizePhraseBoostEffect();
    static double calculateBM25(int termFrequency,int documentLength,double averageDocumentLength,int totalDocumentsCount,int termDocumentsCount);
    std::unordered_map<std::string,int>calculateTermFrequencyMetadata(const std::vector<std::string>&term_vector);
    std::vector<std::pair<std::string,double>> sortDocumentScores(ScoresDocument scoresDocument);
    std::vector<std::string> tokenizeQuery(const std::string& query);
    std::unordered_map<std::string, std::unordered_map<std::string,std::vector<int>>> m_term_frequencies;
    InvertedIndex::document_metadata m_metadata_document;
    std::string m_database_name;
    std::string m_documents_collection_name;
    InvertedIndex* m_invertedIndex;
    std::queue<std::string> m_query_phrase_queue;
    static double K1;
    static double B;
    static double TERM_FREQUENCY_WEIGHT;
    static double EXACT_MATCH_WEIGHT;
    static double PHRASE_BOOST;
};
#endif