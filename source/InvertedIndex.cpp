#include "InvertedIndex.h"
#include "DataBase.h"
#include "WordProcessor.h"

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
    std::vector<std::string> tokens = WordProcessor::tokenize(content);
    for (std::string token : tokens)
    {
        token = WordProcessor::normalize(token);
        
        if (WordProcessor::isStopWord(token))
            continue;
        token = WordProcessor::stem(token);
        if(WordProcessor::isValidWord(token)){
        if(m_index[token][docId])m_index[token][docId]++;
        else m_index[token][docId]=1;
        }
    }
}










