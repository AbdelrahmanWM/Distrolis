#include "BM25Ranker.h"
#include "WordProcessor.h"
#include <math.h>
#include <algorithm>

double BM25Ranker::K1 = 1.5;          // Default value for K1
double BM25Ranker::B = 0.75;          // Default ovalue for B
double BM25Ranker::PHRASE_BOOST = 1.3; // Default value for PHRASE_BOOST

void BM25Ranker::SetSearchEngineParameters(double BM25_K1, double BM25_B, double PHRASE_BOOST_VALUE)
{
    BM25Ranker::K1 = BM25_K1;
    BM25Ranker::B = BM25_B;
    BM25Ranker::PHRASE_BOOST = PHRASE_BOOST_VALUE;
}

BM25Ranker::BM25Ranker(const std::string &database_name, const std::string &documents_collection_name, const InvertedIndex &invertedIndex)
    : m_database_name(database_name), m_documents_collection_name(documents_collection_name), m_invertedIndex(invertedIndex)
{
}

std::vector<std::pair<std::string, double>> BM25Ranker::run(const std::string &query_string)
{
    try
    {
        extractInvertedIndexAndMetadata();
        initializeDocumentScores();
        if (WordProcessor::isQuotedPhrase(query_string))
        {
            addDocumentsPhraseBoost(query_string);
        }
        std::vector<std::string> query_terms = tokenizeQuery(query_string);
        for (const auto &term : query_terms)
        {
            calculateBM25ScoreForTerm(term);
        }
        if(WordProcessor::isQuotedPhrase(query_string)){
            normalizePhraseBoostEffect();
        }
        std::vector<std::pair<std::string, double>> documents = sortDocumentScores();
        return documents;
    }
    catch (std::exception &ex)
    {
        std::cerr << ex.what() << '\n';
        return std::vector<std::pair<std::string, double>>();
    }
}

void BM25Ranker::addDocumentsPhraseBoost(const std::string& phrase)
{
    std::string normalizePhrase = WordProcessor::normalizeQuotedPhrase(std::move(phrase));
    std::vector<std::string>phrase_vector = tokenizeQuery(std::move(normalizePhrase)); 
    std::unordered_map<std::string,int> documentsMetadata= calculateTermFrequencyMetadata(std::move(phrase_vector));
    calculateBM25ScoreForPhrase(std::move(documentsMetadata));
}

void BM25Ranker::extractInvertedIndexAndMetadata()
{
    m_invertedIndex.retrieveExistingIndex();
    m_invertedIndex.retrieveExistingMetadataDocument();
    m_term_frequencies = m_invertedIndex.getInvertedIndex();
    m_metadata_document = m_invertedIndex.getMetadataDocument();
}

void BM25Ranker::initializeDocumentScores()
{
    for (const auto &pair : m_metadata_document.doc_lengths)
    {
        m_documents_scores[pair.first] = 0.0;
    }
}

std::vector<std::string> BM25Ranker::tokenizeQuery(const std::string &query)
{
    std::vector<std::string> results{};

    std::vector<std::string> terms = WordProcessor::tokenize(query);
    std::string term;
    for (long long unsigned int i = 0; i < terms.size(); i++)
    {
        term = WordProcessor::normalize(terms[i]);

        // if (WordProcessor::isStopWord(term))  // stop words will complicate phrase search
        //     continue;
        term = WordProcessor::stem(term);
        // if (WordProcessor::isValidWord(term))
        // {
        results.push_back(term);
        // }
    }
    return results;
}

void BM25Ranker::calculateBM25ScoreForTerm(const std::string &term)
{

    const auto &term_freq = m_term_frequencies[term];
    const double totalDocuments = m_metadata_document.total_documents;
    const double docCount = static_cast<double>(term_freq.size());
    double avgDocLength = m_metadata_document.average_doc_length;

    for (const auto &documentPair : term_freq)
    { // documentPair = {id, positions}
        const std::string &docID = documentPair.first;
        const std::vector<int> &positions = documentPair.second;
        double termFrequency = static_cast<double>(positions.size());
        double docLength = static_cast<double>(m_metadata_document.doc_lengths[docID]);
        

        m_documents_scores[docID] += calculateBM25(termFrequency, docLength, avgDocLength, totalDocuments, docCount);
    }
}

void BM25Ranker::calculateBM25ScoreForPhrase(const std::unordered_map<std::string, int> &documentsMap)
{
    const double totalDocumentsCount = m_metadata_document.total_documents;
    const double documentsCount = static_cast<double>(documentsMap.size());
        double avgDocLength = m_metadata_document.average_doc_length;

    for (const auto& documentPair:documentsMap){ // docId, phrase_freq
        const std::string &docID = documentPair.first;
        double docLength = static_cast<double>(m_metadata_document.doc_lengths[docID]);
        double diminishingFactor = log(1 + documentPair.second);
        m_documents_scores[docID] += BM25Ranker::PHRASE_BOOST * diminishingFactor*calculateBM25(documentPair.second,docLength,avgDocLength,totalDocumentsCount,documentsCount);
    }
}

void BM25Ranker::normalizePhraseBoostEffect()
{
    for(const auto& documentPair:m_documents_scores){
        m_documents_scores[documentPair.first]=documentPair.second/(BM25Ranker::PHRASE_BOOST+1.0);
    }
}

double BM25Ranker::calculateBM25(int termFrequency, int documentLength, double averageDocumentLength, int totalDocumentsCount, int termDocumentsCount)
{
    double idf = std::log((totalDocumentsCount - termDocumentsCount + 0.5) / (termDocumentsCount + 0.5) + 1);
    double docNorm = (1 - BM25Ranker::B) + BM25Ranker::B * (documentLength / averageDocumentLength);
    double tf = (termFrequency * (BM25Ranker::K1 + 1)) / (termFrequency + BM25Ranker::K1 * docNorm);
    return idf * tf;
}

std::unordered_map<std::string, int> BM25Ranker::calculateTermFrequencyMetadata(const std::vector<std::string> &term_vector)
{
    std::unordered_map<std::string, int> documents;
    std::unordered_map<std::string, std::vector<int>> documentsMetadata;
    for (const auto &documentPair : m_term_frequencies[term_vector[0]])
    {
        documentsMetadata[documentPair.first] = documentPair.second;
    }
    std::cout << "term size:" << term_vector.size() << "\n";
    for (const auto &term : term_vector)
    {
        for (const auto &documentPair : m_term_frequencies[term])
        {
            std::vector<int> positions;
            std::string documentId = documentPair.first;
            std::vector<int> wordOccurrences = documentPair.second;

            for (int position : wordOccurrences)
            {
                if (std::find(documentsMetadata[documentId].begin(), documentsMetadata[documentId].end(), position) != documentsMetadata[documentId].end())
                {
                    positions.push_back(position + 1);
                }
            }
            documentsMetadata[documentId] = positions;
        }
    }
    for (const auto &element : documentsMetadata)
    {
        if (element.second.size() > 0)
        {
            documents[element.first] = element.second.size();
        }
    }
    // std::cout<<"Documents containing the phrase:\n";
    // for(const auto& element :documents){
    //     std::cout<<element.first<<","<<element.second<<'\n';
    // }
    // std::cout<<"end\n";
    return documents;
}

std::vector<std::pair<std::string, double>> BM25Ranker::sortDocumentScores()
{
    std::vector<std::pair<std::string, double>> sortedDocuments{};
    for (const auto &pair : m_documents_scores)
    {
        sortedDocuments.push_back({pair.first, pair.second});
    }
    std::sort(sortedDocuments.begin(), sortedDocuments.end(), [](std::pair<std::string, double> pair1, std::pair<std::string, double> pair2)
            { return pair1.second > pair2.second; });
    return sortedDocuments;
}
