#include<string>
#include<vector>
#include<unordered_map>
#include "InvertedIndex.h"
#ifndef RANKER_H
#define RANKER_H
class BM25Ranker
{
public:
    static void SetSearchEngineParameters(double BM25_K1,double BM25_B,double PHRASE_BOOST_VALUE);
    BM25Ranker(const std::string&database_name,const std::string&documents_collection_name,const InvertedIndex& invertedIndex);
    std::vector<std::pair<std::string,double>> run(const std::string& query_string);

private:
    void addDocumentsPhraseBoost(const std::string& phrase);
    void extractInvertedIndexAndMetadata();
    void initializeDocumentScores();
    void calculateBM25ScoreForTerm(const std::string& term);
    void calculateBM25ScoreForPhrase(const std::unordered_map<std::string,int>& documentsMap);
    void normalizePhraseBoostEffect();
    static double calculateBM25(int termFrequency,int documentLength,double averageDocumentLength,int totalDocumentsCount,int termDocumentsCount);
    std::unordered_map<std::string,int>calculateTermFrequencyMetadata(const std::vector<std::string>&term_vector);
    std::vector<std::pair<std::string,double>> sortDocumentScores();
    std::vector<std::string> tokenizeQuery(const std::string& query);
    std::unordered_map<std::string, std::unordered_map<std::string,std::vector<int>>> m_term_frequencies;
    InvertedIndex::document_metadata m_metadata_document;
    std::unordered_map<std::string,double>m_documents_scores;
    std::string m_database_name;
    std::string m_documents_collection_name;
    InvertedIndex m_invertedIndex;
    static double K1;
    static double B;
    static double PHRASE_BOOST;
};
#endif