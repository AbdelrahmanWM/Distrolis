#include "BM25Ranker.h"
#include "WordProcessor.h"
#include <math.h>
#include <algorithm>
#include <stack>

double BM25Ranker::K1 = 1.5;           // Default value for K1
double BM25Ranker::B = 0.75;           // Default ovalue for B
double BM25Ranker::PHRASE_BOOST = 1.3; // Default value for PHRASE_BOOST

void BM25Ranker::SetSearchEngineParameters(double BM25_K1, double BM25_B, double PHRASE_BOOST_VALUE)
{
    BM25Ranker::K1 = BM25_K1;
    BM25Ranker::B = BM25_B;
    BM25Ranker::PHRASE_BOOST = PHRASE_BOOST_VALUE;
}

BM25Ranker::BM25Ranker(const std::string &database_name, const std::string &documents_collection_name, const InvertedIndex &invertedIndex)
    : m_database_name(database_name), m_documents_collection_name(documents_collection_name), m_invertedIndex(invertedIndex), m_query_phrase_queue{}
{
}

std::vector<std::pair<std::string, double>> BM25Ranker::run(const std::string &query_string)
{
    try
    {
        extractInvertedIndexAndMetadata();

        BM25Ranker::ScoresDocument scores_document = ProcessQuery(query_string);
        std::vector<std::pair<std::string, double>> documents = sortDocumentScores(scores_document);
        return documents;
    }
    catch (std::exception &ex)
    {
        std::cerr << ex.what() << '\n';
        return std::vector<std::pair<std::string, double>>();
    }
}

BM25Ranker::ScoresDocument BM25Ranker::ProcessQuery(const std::string &query)
{
    std::stack<LogicalOperation> operations_stack{};
    std::stack<std::unordered_map<std::string, double>> phrase_documents_scores_stack;
    std::stack<std::unordered_map<std::string, double>> term_documents_scores_stack;
    std::queue<std::pair<PhraseType, std::string>> phrases_queue = WordProcessor::tokenizeQueryPhrases(query);
    LogicalOperation op = LogicalOperation::OTHER;
    LogicalOperation currentOp = LogicalOperation::OTHER;
    std::unordered_map<std::string, double> operand1;
    std::unordered_map<std::string, double> operand2;
    std::unordered_map<std::string, double> result;

    phrases_queue.push(std::make_pair(PhraseType::LOGICAL_OPERATION, "!")); // Weak operator
    while (!phrases_queue.empty())
    {
        const auto &pair = phrases_queue.front();
        phrases_queue.pop();
        if (pair.first == PhraseType::LOGICAL_OPERATION)
        {
            std::cout<<"HERE\n";
            if (!operations_stack.empty())
            {
                op = operations_stack.top();
                std::cout<<"op "<<(int)op<<'\n';
                currentOp = GetLogicalOperation(pair.second);
                std::cout<<"curr "<<(int)currentOp<<"\n";
                // std::cout<<"Current operation "<<(int)currentOp<<"\n";
                while (op <= currentOp)
                {
                    operations_stack.pop();
                    if (op == LogicalOperation::NOT)
                    {
                        operand1 = phrase_documents_scores_stack.top();
                        phrase_documents_scores_stack.pop();
                        result = documentNOTOperation(std::move(operand1));
                        phrase_documents_scores_stack.push(result);
                    }
                    else if (op == LogicalOperation::AND)
                    {
                        operand1 = phrase_documents_scores_stack.top();
                        phrase_documents_scores_stack.pop();
                        operand2 = phrase_documents_scores_stack.top();
                        phrase_documents_scores_stack.pop();
                        result = documentsANDOperation(std::move(operand1), std::move(operand2));
                        phrase_documents_scores_stack.push(result);
                    }
                    else if (op == LogicalOperation::OR)
                    {
            std::cout<<"HERE OR\n";

                        operand1 = phrase_documents_scores_stack.top();
                        phrase_documents_scores_stack.pop();
                        operand2 = phrase_documents_scores_stack.top();
                        phrase_documents_scores_stack.pop();
                        result = documentsOROperation(std::move(operand1), std::move(operand2));
                        phrase_documents_scores_stack.push(result);
                    }
                    if(operations_stack.empty())break;
                    op = operations_stack.top();
                }

                operations_stack.push(currentOp);

            }
            else
            {
                operations_stack.push(GetLogicalOperation(pair.second));
            }
        }
        else if (pair.first == PhraseType::PHRASE)
        {
            phrase_documents_scores_stack.push(calculatePhraseScore(pair.second));
        }
        else if (pair.first == PhraseType::TERM)
        {
            term_documents_scores_stack.push(calculateTermScore(pair.second));
        }
        else
        {
            continue;
        }
    }
    if (!term_documents_scores_stack.empty())
    {

        BM25Ranker::ScoresDocument result{term_documents_scores_stack.top()};
        term_documents_scores_stack.pop();
        while (!term_documents_scores_stack.empty())
        {
            result = documentsADDOperation(std::move(result), std::move(term_documents_scores_stack.top()));
            term_documents_scores_stack.pop();
        }
        result = documentNormalizeOperation(std::move(result));
        term_documents_scores_stack.push(result);
    }
    if (!phrase_documents_scores_stack.empty())
    {
        BM25Ranker::ScoresDocument result{phrase_documents_scores_stack.top()};
        phrase_documents_scores_stack.pop();
        while (!phrase_documents_scores_stack.empty())
        {
            result = documentsANDOperation(std::move(result), std::move(phrase_documents_scores_stack.top()));
            phrase_documents_scores_stack.pop();
        }
        phrase_documents_scores_stack.push(result);
    }
    if (term_documents_scores_stack.empty())
    {
        std::cout<<"ONLY PHRASES\n";
        return phrase_documents_scores_stack.top();
    }
    else if (phrase_documents_scores_stack.empty())
    {
        std::cout<<"ONLY TERMS\n";
        return term_documents_scores_stack.top();
    }
    else
    {
        std::cout<<"BOTH\n";
        return documentNormalizeOperation(documentsADDOperation(phrase_documents_scores_stack.top(), term_documents_scores_stack.top(), true));
    }
}

BM25Ranker::ScoresDocument BM25Ranker::calculatePhraseScore(const std::string &phrase)
{   
    // std::string normalizePhrase = WordProcessor::normalizeQuotedPhrase(std::move(phrase));
    // std::cout<<normalizePhrase<<"___\n";
    std::vector<std::string> phrase_vector = tokenizeQuery(std::move(phrase));
    std::unordered_map<std::string, int> documentsMetadata = calculateTermFrequencyMetadata(std::move(phrase_vector));
    return calculateBM25ScoreForPhrase(std::move(documentsMetadata));
}

BM25Ranker::ScoresDocument BM25Ranker::calculateTermScore(const std::string &term)
{
    return calculateBM25ScoreForTerm(term);
}

BM25Ranker::ScoresDocument BM25Ranker::documentNOTOperation(const ScoresDocument &operand)
{
    BM25Ranker::ScoresDocument result;
    for (const auto &document : operand)
    {

        result[document.first] = 1 - document.second;
    }
    return result;
}

BM25Ranker::ScoresDocument BM25Ranker::documentsANDOperation(const ScoresDocument &operand1, const ScoresDocument &operand2)
{
    BM25Ranker::ScoresDocument result;
    for (const auto &document : operand1)
    {
        double score2 = operand2.at(document.first);
        result[document.first] = std::min(document.second, score2);
    }
    return result;
}

BM25Ranker::ScoresDocument BM25Ranker::documentsADDOperation(const ScoresDocument &operand1, const ScoresDocument &operand2, bool operand1Boost)
{
    double operandBoost{1};
    if (operand1Boost)
    {
        operandBoost = PHRASE_BOOST;
    }
    BM25Ranker::ScoresDocument result;
    for (const auto &document : operand1)
    {
        double score2 = operand2.at(document.first);

        double addition = operandBoost * document.second + score2;
        result[document.first] = addition;
    }

    return result;
}

BM25Ranker::ScoresDocument BM25Ranker::documentsOROperation(const ScoresDocument &operand1, const ScoresDocument &operand2)
{
    std::cout<<"are we even here\n";
    BM25Ranker::ScoresDocument result;
    for (const auto &document : operand1)
    {
        double score2 = operand2.at(document.first);
        // std::cout<<"->"<<document.second<<","<<score2<<'\n';
        result[document.first] = std::max(document.second, score2);
    }
    return result;
}

BM25Ranker::ScoresDocument BM25Ranker::documentNormalizeOperation(const ScoresDocument &operand)
{
    BM25Ranker::ScoresDocument result;
    double max = 0;
    for (const auto &document : operand)
    {
        max = std::max(max, document.second);
        result[document.first] = document.second;
    }
    std::cout<<"max: "<<max<<'\n';
    if(max>0)
    for (auto &document : result)
    {std::cout<<document.second<<"\n";

        document.second /= max;
    }
    return result;
}

void BM25Ranker::extractInvertedIndexAndMetadata()
{
    m_invertedIndex.retrieveExistingIndex();
    m_invertedIndex.retrieveExistingMetadataDocument();
    m_term_frequencies = m_invertedIndex.getInvertedIndex();
    m_metadata_document = m_invertedIndex.getMetadataDocument();
}

BM25Ranker::ScoresDocument BM25Ranker::getEmptyScoresDocument()
{
    std::unordered_map<std::string, double> scoresDocument{};
    for (const auto &pair : m_metadata_document.doc_lengths)
    {
        scoresDocument[pair.first] = 0.0;
    }
    return scoresDocument;
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

BM25Ranker::ScoresDocument BM25Ranker::calculateBM25ScoreForTerm(const std::string &term)
{
    std::unordered_map<std::string, double> scores_document{BM25Ranker::getEmptyScoresDocument()};
    const auto &term_freq = m_term_frequencies[term];
    const double totalDocuments = m_metadata_document.total_documents;
    const double docCount = static_cast<double>(term_freq.size());
    double avgDocLength = m_metadata_document.average_doc_length;
    double maximum = 0;
    for (const auto &documentPair : term_freq)
    { // documentPair = {id, positions}
        const std::string &docID = documentPair.first;
        const std::vector<int> &positions = documentPair.second;
        double termFrequency = static_cast<double>(positions.size());
        double docLength = static_cast<double>(m_metadata_document.doc_lengths[docID]);
        double bm25Score = calculateBM25(termFrequency, docLength, avgDocLength, totalDocuments, docCount);
        maximum = std::max(maximum, bm25Score);
        scores_document[docID] = bm25Score;
    }
    if(maximum>0)
    for (auto &document : scores_document)
    { // normalizing the scores between 0 -> 1
        document.second = document.second / maximum;
    }
    return scores_document;
}

BM25Ranker::ScoresDocument BM25Ranker::calculateBM25ScoreForPhrase(const std::unordered_map<std::string, int> &documentsMap)
{
    std::unordered_map<std::string, double> scores_document{BM25Ranker::getEmptyScoresDocument()};
    const double totalDocumentsCount = m_metadata_document.total_documents;
    const double documentsCount = static_cast<double>(documentsMap.size());
    double avgDocLength = m_metadata_document.average_doc_length;
    double maximum = 0;

    for (const auto &documentPair : documentsMap)
    { // docId, phrase_freq
    
        const std::string &docID = documentPair.first;
        double docLength = static_cast<double>(m_metadata_document.doc_lengths[docID]);
        // double diminishingFactor = log(1 + documentPair.second);
        double bm25Score = calculateBM25(documentPair.second, docLength, avgDocLength, totalDocumentsCount, documentsCount);

        maximum = std::max(maximum, bm25Score);
        // m_documents_scores[docID] += BM25Ranker::PHRASE_BOOST * diminishingFactor*calculateBM25(documentPair.second,docLength,avgDocLength,totalDocumentsCount,documentsCount);
        scores_document[docID] = bm25Score;
    }
    if(maximum>0)
    for (auto &document : scores_document)
    { // normalizing the scores between 0 -> 1
        document.second = document.second / maximum;
    }
    return scores_document;
}

// void BM25Ranker::normalizePhraseBoostEffect()
// {
//     for (const auto &documentPair : m_documents_scores)
//     {
//         m_documents_scores[documentPair.first] = documentPair.second / (BM25Ranker::PHRASE_BOOST + 1.0);
//     }
// }

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

std::vector<std::pair<std::string, double>> BM25Ranker::sortDocumentScores(ScoresDocument scoresDocument)
{
    std::vector<std::pair<std::string, double>> sortedDocuments{};
    for (const auto &pair : scoresDocument)
    {
        sortedDocuments.push_back({pair.first, pair.second});
    }
    std::sort(sortedDocuments.begin(), sortedDocuments.end(), [](std::pair<std::string, double> pair1, std::pair<std::string, double> pair2)
              { return pair1.second > pair2.second; });
    return sortedDocuments;
}
