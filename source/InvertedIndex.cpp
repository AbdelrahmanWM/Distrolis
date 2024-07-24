#include "InvertedIndex.h"
#include "DataBase.h"

InvertedIndex::InvertedIndex(const DataBase *&db,const std::string& database_name, const std::string& collection_name,  const std::string& documents_collection_name)
    : m_index{}, m_db{db}, m_database_name{database_name},m_collection_name{collection_name},m_documents_collection_name{documents_collection_name}
{
}

void InvertedIndex::run(bool clear)
{
    std::vector<bson_t> documents{};

    try
    {   
        if(clear){
            m_db->clearCollection(m_database_name,m_collection_name);
        }
        documents = m_db->getAllDocuments(m_database_name,m_documents_collection_name);
        std::cout << "size: " << documents.size() << '\n';
        for (const auto &document : documents)
        {
            std::string content = m_db->extractContentFromIndexDocument(document);
            std::string docId = m_db->extractIndexFromIndexDocument(document);
            addDocument(docId, content);
        }
        m_db->saveInvertedIndex(m_index,m_database_name,m_collection_name);
    }
    catch (std::exception &ex)
    {
        std::cout << "Error: " << ex.what() << '\n';
    }
}

void InvertedIndex::addDocument(const std::string docId, std::string &content)
{
    std::vector<std::string> tokens = tokenize(content);
    for (std::string token : tokens)
    {
        token = normalize(token);
        if (isStopWord(token))
            continue;
        token = stem(token);
        std::cout<<"* "<<token<<"\n";
        m_index[token].push_back({docId, 1});
    }
}

std::vector<std::string> InvertedIndex::tokenize(std::string &content)
{
    std::istringstream stream(content);
    std::string token;
    std::vector<std::string> tokens;
    while (stream >> token)
    {
        if(token.size()>3&&token.size()<=20)// Temporary filtration step until better solution is found
        tokens.push_back(token);
    }
    return tokens;
}

std::string InvertedIndex::normalize(std::string &token)
{
    std::string normalized;
    std::transform(token.begin(), token.end(), std::back_inserter(normalized), ::tolower);
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), ::ispunct), normalized.end());
    return normalized;
}

std::string InvertedIndex::stem(std::string &word)
{

    sb_stemmer *stemmer = sb_stemmer_new("english", nullptr);
    if (!stemmer)
    {
        throw std::runtime_error("Failed to create stemmer.");
    }
    const sb_symbol *input = reinterpret_cast<const sb_symbol *>(word.c_str());
    const sb_symbol *stemmed = sb_stemmer_stem(stemmer, input, word.length());

    if (!stemmed)
    {
        sb_stemmer_delete(stemmer);
        throw std::runtime_error("Failed to stem word.");
    }
    std::string stemmed_word{reinterpret_cast<const char *>(stemmed)};
    sb_stemmer_delete(stemmer);
    return stemmed_word;
}

bool InvertedIndex::isStopWord(std::string &word)
{
    
    return InvertedIndex::getStopWords().find(word) !=InvertedIndex::getStopWords().end();
}

const std::unordered_set<std::string> &InvertedIndex::getStopWords()
{
    static const std::unordered_set<std::string> stopWords = {
    "a", "about", "above", "after", "again", "against", "all", "am", "an", "and", "any", "are", "aren't", "as", "at", 
    "be", "because", "been", "before", "being", "below", "between", "both", "but", "by", 
    "can't", "cannot", "could", "couldn't", "did", "didn't", "do", "does", "doesn't", "doing", "don't", "down", "during", 
    "each", "few", "for", "from", "further", 
    "had", "hadn't", "has", "hasn't", "have", "haven't", "having", "he", "he'd", "he'll", "he's", "her", "here", "here's", "hers", "herself", "him", "himself", "his", 
    "how", "how's", 
    "i", "i'd", "i'll", "i'm", "i've", "if", "in", "into", "is", "isn't", "it", "it's", "its", "itself", 
    "let's", 
    "me", "more", "most", "mustn't", "my", "myself", 
    "no", "nor", "not", 
    "of", "off", "on", "once", "only", "or", "other", "ought", "our", "ours", "ourselves", "out", "over", "own", 
    "same", "shan't", "she", "she'd", "she'll", "she's", "should", "shouldn't", "so", "some", "such", 
    "than", "that", "that's", "the", "their", "theirs", "them", "themselves", "then", "there", "there's", "these", "they", "they'd", "they'll", "they're", "they've", "this", "those", "through", "to", "too", 
    "under", "until", "up", 
    "very", 
    "was", "wasn't", "we", "we'd", "we'll", "we're", "we've", "were", "weren't", "what", "what's", "when", "when's", "where", "where's", "which", "while", "who", "who's", "whom", "why", "why's", "with", "won't", "would", "wouldn't", 
    "you", "you'd", "you'll", "you're", "you've", "your", "yours", "yourself", "yourselves"
};
return stopWords;
}
