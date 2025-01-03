#include "BM25Ranker.h"
#include "WordProcessor.h"
#include <math.h>
#include <algorithm>

const double epsilon = 1e-5;
double BM25Ranker::TERM_FREQUENCY_WEIGHT = 0.4;
double BM25Ranker::EXACT_MATCH_WEIGHT = 0.6;
double BM25Ranker::K1 = 1.5;           // Default value for K1
double BM25Ranker::B = 0.75;           // Default ovalue for B
double BM25Ranker::PHRASE_BOOST = 1.3; // Default value for PHRASE_BOOST
std::unordered_map<std::string, int> BM25Ranker::wordsAndPhrasesWeights = {};
int negativeWeightMultiplier = 5; // for negative weight to positive weight ratio on snippet forming
std::unordered_map<std::string, std::unordered_map<int, std::string>> documentsWordAndPhrasePositions{};
std::unordered_map<std::string, std::vector<int>> documentsPositions{};

void BM25Ranker::setRankerParameters(double BM25_K1, double BM25_B, double PHRASE_BOOST_VALUE, double EXACT_MATCH_WEIGHT)
{
    BM25Ranker::K1 = BM25_K1;
    BM25Ranker::B = BM25_B;
    BM25Ranker::PHRASE_BOOST = PHRASE_BOOST_VALUE;
    BM25Ranker::EXACT_MATCH_WEIGHT = EXACT_MATCH_WEIGHT;
    BM25Ranker::TERM_FREQUENCY_WEIGHT = 1 - EXACT_MATCH_WEIGHT;
}

BM25Ranker::BM25Ranker(const std::string &database_name, const std::string &documents_collection_name, InvertedIndex *invertedIndex, DocumentRetriever *dr)
    : m_database_name(database_name), m_documents_collection_name(documents_collection_name), m_invertedIndex(invertedIndex), m_query_phrase_queue{}, m_dr{dr}
{
    extractInvertedIndexAndMetadata();
}

std::vector<SearchResultDocument> BM25Ranker::run(const std::string &query_string, double accuracy)
{
    try
    {
        documentsWordAndPhrasePositions.clear();
        documentsPositions.clear();

        BM25Ranker::ScoresDocument scores_document = ProcessQuery(query_string);
        scores_document = filterMapBasedOnValue(scores_document, accuracy);

        std::vector<std::pair<std::string, double>> documents = sortDocumentScores(scores_document);
        std::vector<SearchResultDocument> searchDocuments = m_dr->getScoresDocuments(m_database_name, m_documents_collection_name, documents, scores_document);
        for (auto &document : documentsPositions)
        {
            std::sort(document.second.begin(), document.second.end());
        }
        for (auto &document : searchDocuments)
        {
            std::vector<std::string> tokens = WordProcessor::tokenize(document.title + document.body);
            std::pair<int, int> pos = getBestDocumentSnippetPositions(document.id, wordsAndPhrasesWeights);
            document.body = constructDocumentSnippet(document.id, tokens, pos);
        }
        return searchDocuments;
    }
    catch (std::exception &ex)
    {
        std::cerr << ex.what() << '\n';
        std::unordered_map<std::string, double> scoresMap = std::unordered_map<std::string, double>();
        return m_dr->getScoresDocuments(m_database_name, m_documents_collection_name, std::vector<std::pair<std::string, double>>(), scoresMap);
    }
}

BM25Ranker::ScoresDocument BM25Ranker::ProcessQuery(const std::string &query)
{
    try
    {
        std::stack<LogicalOperation> operations_stack{};
        std::stack<ScoresDocument> phrase_documents_scores_stack;
        std::stack<ScoresDocument> term_documents_scores_stack;

        std::queue<std::pair<PhraseType, std::string>> phrases_queue = WordProcessor::tokenizeQueryPhrases(query);
        getWordsAndPhrasesWeight(phrases_queue);
        LogicalOperation op = LogicalOperation::OTHER;
        LogicalOperation currentOp = LogicalOperation::OTHER;
        ScoresDocument operand1;
        ScoresDocument operand2;
        ScoresDocument result;

        phrases_queue.push(std::make_pair(PhraseType::LOGICAL_OPERATION, "!")); // Weak operator
        while (!phrases_queue.empty())
        {
            const auto &pair = phrases_queue.front();
            phrases_queue.pop();

            if (pair.first == PhraseType::LOGICAL_OPERATION)
            {
                if (!operations_stack.empty())
                {
                    op = operations_stack.top();
                    currentOp = GetLogicalOperation(pair.second);
                    // std::cout<<"Current operation "<<(int)currentOp<<"\n";
                    while (op <= currentOp && currentOp != LogicalOperation::OPENING_BRACKET)
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
                            operand1 = phrase_documents_scores_stack.top();
                            phrase_documents_scores_stack.pop();
                            operand2 = phrase_documents_scores_stack.top();
                            phrase_documents_scores_stack.pop();
                            result = documentsOROperation(std::move(operand1), std::move(operand2));
                            phrase_documents_scores_stack.push(result);
                        }
                        else
                        {
                            break;
                        }
                        if (operations_stack.empty())
                            break;
                        op = operations_stack.top();
                    }
                    if (currentOp != LogicalOperation::CLOSING_BRACKET)
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
                ScoresDocument result = calculateTermScore(pair.second);
                term_documents_scores_stack.push(std::move(result));
            }
            else
            {
                continue;
            }
        }

        if (!term_documents_scores_stack.empty())
        {
            documentsStackCombineOperation(term_documents_scores_stack);
        }
        if (!phrase_documents_scores_stack.empty())
        {
            documentsStackCombineOperation(phrase_documents_scores_stack);
        }

        if (term_documents_scores_stack.empty())
        {
            return phrase_documents_scores_stack.top();
        }
        else if (phrase_documents_scores_stack.empty())
        {
            return term_documents_scores_stack.top();
        }
        else
        {
            ScoresDocument boostedPhraseScore = documentsADDOperation(phrase_documents_scores_stack.top(), phrase_documents_scores_stack.top(), PHRASE_BOOST, 0);
            term_documents_scores_stack.push(boostedPhraseScore); // Adding them in one stack to apply the combine operation on them
            documentsStackCombineOperation(term_documents_scores_stack);
            return documentNormalizeOperation(term_documents_scores_stack.top());
        }
    }
    catch (std::exception &ex)
    {
        std::cerr << "Failed to process query: " << ex.what();
        return getEmptyScoresDocument();
    }
}
void BM25Ranker::documentsStackCombineOperation(std::stack<ScoresDocument> &documentsStack)
{
    try
    {
        BM25Ranker::ScoresDocument termFrequency{documentsStack.top()}, exactMatch{documentsStack.top()};
        documentsStack.pop();
        while (!documentsStack.empty())
        {
            termFrequency = documentsADDOperation(std::move(termFrequency), documentsStack.top());
            exactMatch = documentsANDOperation(std::move(exactMatch), documentsStack.top());
            documentsStack.pop();
        }
        termFrequency = documentNormalizeOperation(std::move(termFrequency));
        ScoresDocument result = documentsADDOperation(termFrequency, exactMatch, BM25Ranker::TERM_FREQUENCY_WEIGHT, BM25Ranker::EXACT_MATCH_WEIGHT);
        documentsStack.push(std::move(result));
    }
    catch (std::exception &ex)
    {
        std::cerr << "Failed to combine documents: " << ex.what();
    }
}
BM25Ranker::ScoresDocument BM25Ranker::calculatePhraseScore(const std::string &phrase)
{

    std::string normalizePhrase = WordProcessor::normalizeQuotedPhrase(std::move(phrase));

    std::vector<std::string> phrase_vector = tokenizeQuery(std::move(phrase));
    std::unordered_map<std::string, int> documentsMetadata = calculateTermFrequencyMetadata(std::move(phrase_vector), phrase);
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

void BM25Ranker::documentPrintOperation(const ScoresDocument &operand)
{
    std::cout << "Document:\n";
    for (const auto &pair : operand)
    {
        std::cout << pair.first << ", " << pair.second << '\n';
    }
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

BM25Ranker::ScoresDocument BM25Ranker::documentsADDOperation(const ScoresDocument &operand1, const ScoresDocument &operand2, double operand1Boost, double operand2Boost)
{

    BM25Ranker::ScoresDocument result;
    for (const auto &document : operand1)
    {
        double score2 = operand2.at(document.first);

        double addition = operand1Boost * document.second + operand2Boost * score2;
        result[document.first] = addition;
    }

    return result;
}

BM25Ranker::ScoresDocument BM25Ranker::documentsOROperation(const ScoresDocument &operand1, const ScoresDocument &operand2)
{

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

    if (max > 0)
        for (auto &document : result)
        {

            document.second /= max;
        }
    return result;
}

void BM25Ranker::extractInvertedIndexAndMetadata()
{
    m_invertedIndex->retrieveExistingMetadataDocument();
    m_term_frequencies = m_invertedIndex->retrieveExistingIndex();
    m_metadata_document = m_invertedIndex->getMetadataDocument();
    std::cout << "term frequencies: " << m_term_frequencies.size() << '\n';
    std::cout << "metadata document: " << m_metadata_document.total_documents << '\n';
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
    try
    {
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
            if (!term.empty())
                results.push_back(term);
            // }
        }
    }
    catch (std::exception &ex)
    {
        std::cerr << "Error tokenizing query: " << ex.what();
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
        insertIntoDocumentsWordAndPhrasePositions(docID, term, positions);
        double termFrequency = static_cast<double>(positions.size());
        double docLength = static_cast<double>(m_metadata_document.doc_lengths[docID]);
        double bm25Score = calculateBM25(termFrequency, docLength, avgDocLength, totalDocuments, docCount);
        maximum = std::max(maximum, bm25Score);
        scores_document[docID] = bm25Score;
    }

    if (maximum > 0)
        for (auto &document : scores_document)
        { // normalizing the scores between 0 -> 1
            // document.second = (document.second+epsilon) / (maximum+epsilon);
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

    if (maximum > 0)
        for (auto &document : scores_document)
        { // normalizing the scores between 0 -> 1
            // document.second = (document.second+epsilon) / (maximum+epsilon);
            document.second = document.second / maximum;
        }

    return scores_document;
}

void BM25Ranker::setDatabaseName(const std::string &databaseName)
{
    m_database_name = databaseName;
    m_invertedIndex->setDatabaseName(databaseName);
}

void BM25Ranker::setDocumentsCollectionName(const std::string &collectionName)
{
    m_documents_collection_name = collectionName;
    m_invertedIndex->setDocumentsCollectionName(collectionName);
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

std::unordered_map<std::string, int> BM25Ranker::calculateTermFrequencyMetadata(const std::vector<std::string> &term_vector, const std::string &phrase)
{
    std::unordered_map<std::string, int> documents;
    try
    {

        std::unordered_map<std::string, std::vector<int>> documentsMetadata;
        if (term_vector.empty())
            return documents;
        for (const auto &documentPair : m_term_frequencies[term_vector[0]])
        {
            documentsMetadata[documentPair.first] = documentPair.second;
        }

        for (const auto &term : term_vector)
        {
            if (!term.empty())
                for (const auto &documentPair : m_term_frequencies[term])
                {
                    std::vector<int> positions;
                    std::string documentId = documentPair.first;
                    std::vector<int> wordOccurrences = documentPair.second;
                    insertIntoDocumentsWordAndPhrasePositions(documentId, term, wordOccurrences); ///////////////////

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
        for (auto &element : documentsMetadata)
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
    }
    catch (std::exception &ex)
    {
        std::cerr << ex.what();
    }
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
void BM25Ranker::insertIntoDocumentsWordAndPhrasePositions(const std::string &documentID, const std::string &phrase, const std::vector<int> &positions)
{
    if (documentsWordAndPhrasePositions.find(documentID) == documentsWordAndPhrasePositions.end())
    {
        documentsWordAndPhrasePositions[documentID] = std::unordered_map<int, std::string>();
    }
    if (documentsPositions.find(documentID) == documentsPositions.end())
    {
        documentsPositions[documentID] = std::vector<int>();
    }
    for (int position : positions)
    {
        documentsPositions[documentID].push_back(position);
        documentsWordAndPhrasePositions[documentID][position] = phrase;
    }
}

void BM25Ranker::getWordsAndPhrasesWeight(std::queue<std::pair<PhraseType, std::string>> &phrases_queue)
{
    try
    {
        int n = phrases_queue.size();
        bool negativeFlag{false};
        for (int i = 0; i < n; i++)
        {
            auto &pair = phrases_queue.front();
            phrases_queue.pop();
            phrases_queue.push(pair);
            if (negativeFlag)
            {
                if (pair.first == PhraseType::LOGICAL_OPERATION)
                {
                    LogicalOperation op = GetLogicalOperation(pair.second);
                    if (op == LogicalOperation::OPENING_BRACKET)
                    {
                        while (i < n && op != LogicalOperation::CLOSING_BRACKET)
                        {
                            auto &pair = phrases_queue.front();
                            phrases_queue.pop();
                            phrases_queue.push(pair);
                            if (pair.first == PhraseType::LOGICAL_OPERATION)
                            {
                                op = GetLogicalOperation(pair.second);
                            }
                            else
                            {
                                wordsAndPhrasesWeights[pair.second] = -1 * negativeWeightMultiplier;
                            }
                            i++;
                        }
                        negativeFlag = false;
                    }
                }
                else if (pair.first == PhraseType::PHRASE)
                {
                    wordsAndPhrasesWeights[WordProcessor::normalizeQuotedPhrase(pair.second)] = -1 * negativeWeightMultiplier;
                    negativeFlag = false;
                }
                else if (pair.first == PhraseType::TERM)
                {
                    wordsAndPhrasesWeights[pair.second] = -1 * negativeWeightMultiplier;
                    negativeFlag = false;
                }
            }
            else
            {
                if (pair.first == PhraseType::LOGICAL_OPERATION && GetLogicalOperation(pair.second) == LogicalOperation::NOT)
                {
                    negativeFlag = true;
                }
                else if (pair.first == PhraseType::TERM || pair.first == PhraseType::PHRASE)
                {
                    wordsAndPhrasesWeights[pair.second] = 1;
                }
            }
        }
    }
    catch (std::exception &ex)
    {
        std::cerr << ex.what();
    }
}

std::pair<int, int> BM25Ranker::getBestDocumentSnippetPositions(std::string &documentID, std::unordered_map<std::string, int> &weights)
{
    std::vector<int> documentPositions = documentsPositions[documentID];
    std::unordered_map<int, std::string> positionsToWords = documentsWordAndPhrasePositions[documentID];
    std::pair<int, int> maxPos{0, 0};
    int maxWeight = -10e10;
    std::vector<int> prefixSums(documentPositions.size() + 1, 0);
    for (size_t i = 0; i < documentPositions.size(); i++)
    {
        prefixSums[i + 1] = prefixSums[i] + weights[positionsToWords[documentPositions[i]]];
    }
    for (size_t i = 0; i < documentPositions.size(); i++)
    {
        for (size_t j = i; j < documentPositions.size() && j - i <= 40; j++)
        {
            int weight = prefixSums[j + 1] - prefixSums[i];
            if (weight > maxWeight)
            {
                maxWeight = weight;
                maxPos = {i, j};
            }
        }
    }
    return maxPos;
}
std::string BM25Ranker::constructDocumentSnippet(std::string &documentID, std::vector<std::string> &document_vector, std::pair<int, int> pos)
{
    std::vector<int> documentPositions = documentsPositions[documentID];
    size_t documentPositionsSize = documentPositions.size();

    size_t diff = pos.second - pos.first;
    if (diff < 40)
    {
        while (diff < 40 && pos.first > 0)
        {
            pos.first--;
            diff++;
        }
        while (diff < 40 && pos.second < document_vector.size() - 1)
        {
            pos.second++;
            diff++;
        }
    }
    int index = 0;
    while (index < documentPositionsSize && documentPositions[index] < pos.first)
    {
        index++;
    }

    bool partOfExpression = false;
    std::stringstream result{};
    for (int i = pos.first; i <= pos.second; i++)
    {
        if (index < documentPositionsSize && i == documentPositions[index])
        {
            if (!partOfExpression)
            {
                result << "<span>";
                partOfExpression = true;
            }
            result << document_vector[i];
            index++;
        }
        else if (i != documentPositions[index])
        {
            if (partOfExpression)
            {
                result << "</span>";
                partOfExpression = false;
            }
            result << document_vector[i];
        }
        else
        {
            result << document_vector[i];
        }
        if (i != pos.second)
        {
            result << " ";
        }
    }
    if (partOfExpression)
    {
        result << "</span>";
    }
    return result.str();
}

std::unordered_map<std::string, double> BM25Ranker::filterMapBasedOnValue(std::unordered_map<std::string, double> &map, double val)
{
    std::unordered_map<std::string, double> result{};
    std::copy_if(map.begin(), map.end(), std::inserter(result, result.end()), [val](const std::pair<std::string, double> &entry)
                 { return entry.second >= val; });
    return result;
}