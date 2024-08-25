#include<string>
#include<vector>
#include<unordered_map>
#include "InvertedIndex.h"
#ifndef RANKER_H
#define RANKER_H
class BM25Ranker
{
public:
    BM25Ranker(const std::string&database_name,const std::string&documents_collection_name,const InvertedIndex& invertedIndex,double k1,double b);
    std::vector<std::pair<std::string,double>> run(const std::string& query_string);

private:
    void extractInvertedIndexAndMetadata();
    void initializeDocumentScores();
    void calculateBM25ScoreForTerm(const std::string& term);
    std::unordered_map<std::string,int>calculateTermFrequencyMetadata(const std::vector<std::string>&term_vector);
    std::vector<std::pair<std::string,double>> sortDocumentScores();
    std::vector<std::string> tokenizeQuery(const std::string& query);
    std::unordered_map<std::string, std::unordered_map<std::string,std::vector<int>>> m_term_frequencies;
    InvertedIndex::document_metadata m_metadata_document;
    std::unordered_map<std::string,double>m_documents_scores;
    std::string m_database_name;
    std::string m_documents_collection_name;
    InvertedIndex m_invertedIndex;
    double m_k1;
    double m_b;
};
#endif