#include "InvertedIndex.h"
#include "DataBase.h"

InvertedIndex::InvertedIndex(const DataBase*&db)
    :m_db(db),m_index({})
{
}

void InvertedIndex::run()
{
    std::vector<bson_t*> documents{};
    try {
        documents = m_db->getAllDocuments();
        for (const auto& document : documents) {
            std::string content = m_db->extractContentFromIndexDocument(document);
            std::string docId = m_db->extractIndexFromIndexDocument(document);
                addDocument(docId, content);
        }
        m_db->saveInvertedIndex(m_index);
    }
    catch (std::exception& ex) {
        std::cout << "Error: " << ex.what() << '\n';
    }
}

void InvertedIndex::addDocument(const std::string&docId, std::string& content)
{
    std::vector<std::string> tokens = tokenize(content);
    for (std::string token : tokens) {
        token = normalize(token);
        if (isStopWord(token))continue;
        token = stem(token);
        m_index[token].emplace_back(docId,1);
    }
}

std::vector<std::string> InvertedIndex::tokenize(std::string& content)
{
    std::istringstream stream(content);
    std::string token;
    std::vector<std::string>tokens;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;

}

std::string InvertedIndex::normalize(std::string& token)
{
    std::string normalized;
    std::transform(token.begin(), token.end(), std::back_inserter(normalized), ::tolower);
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), ::ispunct),normalized.end());
    return normalized;
}

std::string InvertedIndex::stem(std::string& word)
{

    sb_stemmer* stemmer= sb_stemmer_new("english",nullptr);
    if (!stemmer) {
        throw std::runtime_error("Failed to create stemmer.");
    }
    const sb_symbol* input = reinterpret_cast<const sb_symbol*>(word.c_str());
    const sb_symbol* stemmed = sb_stemmer_stem(stemmer, input, word.length());

    if (!stemmed) {
        sb_stemmer_delete(stemmer);
        throw std::runtime_error("Failed to stem word.");
    }
    std::string stemmed_word{ reinterpret_cast<const char*>(stemmed) };
    sb_stemmer_delete(stemmer);
    return  stemmed_word;
}

bool InvertedIndex::isStopWord(std::string& word)
{
    static const std::unordered_set<std::string>stopWords = { "the", "and", "is", "in", "at", "of", "on" };
    return stopWords.find(word) != stopWords.end();

}
