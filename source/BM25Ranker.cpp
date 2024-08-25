#include "BM25Ranker.h"
#include "WordProcessor.h"
#include <math.h>
#include <algorithm>

BM25Ranker::BM25Ranker(const std::string &database_name, const std::string &documents_collection_name, const InvertedIndex &invertedIndex, double k1, double b)
    : m_database_name(database_name), m_documents_collection_name(documents_collection_name), m_invertedIndex(invertedIndex), m_k1(k1), m_b(b)
{
}

std::vector<std::pair<std::string, double>> BM25Ranker::run(const std::string &query_string)
{
    try
    {
        extractInvertedIndexAndMetadata();
        initializeDocumentScores();
        if (WordProcessor::isQuotedTerm(query_string))
        {
            std::cout<<"term: "<<query_string<<"\n";
            calculateTermFrequencyMetadata(tokenizeQuery(WordProcessor::normalizeQuotedTerm(query_string)));
            std::cout<<"term: "<<WordProcessor::normalizeQuotedTerm(query_string)<<'\n';
        }
        std::vector<std::string> query_terms = tokenizeQuery(query_string);
        for (const auto &term : query_terms)
        {
            calculateBM25ScoreForTerm(term);
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

    double idf = std::log((totalDocuments - docCount + 0.5) / (docCount + 0.5) + 1);

    for (const auto &documentPair : term_freq)
    { // documentPair = {id, positions}
        const std::string &docID = documentPair.first;
        const std::vector<int> &positions = documentPair.second;

        double termFrequency = static_cast<double>(positions.size());

        double docLength = static_cast<double>(m_metadata_document.doc_lengths[docID]);
        double avgDocLength = m_metadata_document.average_doc_length;

        double docNorm = (1 - m_b) + m_b * (docLength / avgDocLength);
        double tf = (termFrequency * (m_k1 + 1)) / (termFrequency + m_k1 * docNorm);

        m_documents_scores[docID] += idf * tf;
    }
}

std::unordered_map<std::string, int> BM25Ranker::calculateTermFrequencyMetadata(const std::vector<std::string> &term_vector)
{
    std::unordered_map<std::string, int> documents;
    std::unordered_map<std::string, std::vector<int>> documentsMetadata;
    for (const auto &documentPair : m_term_frequencies[term_vector[0]])
    {
        documentsMetadata[documentPair.first] = documentPair.second;
    }
    std::cout<<"term size:"<<term_vector.size()<<"\n";
    for (const auto &term : term_vector)
    {
        std::cout<<term<<'\n';
        for (const auto &documentPair : m_term_frequencies[term])
        {
            std::vector<int> positions;
            std::string documentId = documentPair.first;
            std::vector<int> wordOccurrences = documentPair.second;
            std::cout<<documentId<<'\n';
            
            for (int position : wordOccurrences)
            {
                std::cout<<position<<'\n';
                if (std::find(documentsMetadata[documentId].begin(), documentsMetadata[documentId].end(), position) != documentsMetadata[documentId].end())
                {
                    positions.push_back(position+1);
                    std::cout<<"true\n";
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
    std::cout<<"Documents containing the phrase:\n";
    for(const auto& element :documents){
        std::cout<<element.first<<","<<element.second<<'\n';
    }
    std::cout<<"end\n";
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
