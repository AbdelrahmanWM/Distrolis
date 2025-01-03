#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include "InvertedIndex.h"
#include <stack>
#include "DocumentRetriever.h"
#include "PhraseTypes.h"
#ifndef RANKER_H
#define RANKER_H
class BM25Ranker
{
public:
    typedef std::unordered_map<std::string, double> ScoresDocument;
    static void setRankerParameters(double BM25_K1, double BM25_B, double PHRASE_BOOST_VALUE, double EXACT_MATCH_WEIGHT);
    BM25Ranker(const std::string &database_name, const std::string &documents_collection_name, InvertedIndex *invertedIndex, DocumentRetriever *dr);
    std::vector<SearchResultDocument> run(const std::string &query_string, double accuracy);
    void setDatabaseName(const std::string &databaseName);
    void setDocumentsCollectionName(const std::string &collectionName);
    void extractInvertedIndexAndMetadata();

private:
    ScoresDocument ProcessQuery(const std::string &query);
    ScoresDocument calculatePhraseScore(const std::string &phrase);
    ScoresDocument calculateTermScore(const std::string &term);
    ScoresDocument documentNOTOperation(const ScoresDocument &operand);
    void documentPrintOperation(const ScoresDocument &operand);
    ScoresDocument documentsANDOperation(const ScoresDocument &operand1, const ScoresDocument &operand2);
    ScoresDocument documentsADDOperation(const ScoresDocument &operand1, const ScoresDocument &operand2, double operand1Boost = 1, double operand2Boost = 1);
    ScoresDocument documentsOROperation(const ScoresDocument &operand1, const ScoresDocument &operand2);
    void documentsStackCombineOperation(std::stack<ScoresDocument> &documentsStack);
    ScoresDocument documentNormalizeOperation(const ScoresDocument &operand);
    ScoresDocument getEmptyScoresDocument();
    ScoresDocument calculateBM25ScoreForTerm(const std::string &term);
    ScoresDocument calculateBM25ScoreForPhrase(const std::unordered_map<std::string, int> &documentsMap);

    // void normalizePhraseBoostEffect();
    static double calculateBM25(int termFrequency, int documentLength, double averageDocumentLength, int totalDocumentsCount, int termDocumentsCount);
    std::unordered_map<std::string, int> calculateTermFrequencyMetadata(const std::vector<std::string> &term_vector, const std::string &phrase);
    std::vector<std::pair<std::string, double>> sortDocumentScores(ScoresDocument scoresDocument);
    std::vector<std::string> tokenizeQuery(const std::string &query);
    static void insertIntoDocumentsWordAndPhrasePositions(const std::string &documentID, const std::string &phrase, const std::vector<int> &positions);
    static void getWordsAndPhrasesWeight(std::queue<std::pair<PhraseType, std::string>> &phrases_queue);
    static std::pair<int, int> getBestDocumentSnippetPositions(std::string &documentID, std::unordered_map<std::string, int> &weights);
    static std::string constructDocumentSnippet(std::string &documentID, std::vector<std::string> &document_vector, std::pair<int, int> pos);
    static std::unordered_map<std::string, double> filterMapBasedOnValue(std::unordered_map<std::string, double> &map, double val);

    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> m_term_frequencies;
    InvertedIndex::document_metadata m_metadata_document;
    std::string m_database_name;
    std::string m_documents_collection_name;
    InvertedIndex *m_invertedIndex;
    DocumentRetriever *m_dr;
    std::queue<std::string> m_query_phrase_queue;
    static std::unordered_map<std::string, int> wordsAndPhrasesWeights;
    static double K1;
    static double B;
    static double TERM_FREQUENCY_WEIGHT;
    static double EXACT_MATCH_WEIGHT;
    static double PHRASE_BOOST;
};
#endif