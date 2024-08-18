#include "InvertedIndex.h"
#include "DataBase.h"
#include "WordProcessor.h"

InvertedIndex::InvertedIndex(const DataBase *&db,const std::string& database_name, const std::string& collection_name,  const std::string& documents_collection_name)
    : m_index{}, m_db{db}, m_database_name{database_name},m_collection_name{collection_name},m_documents_collection_name{documents_collection_name}
{
}

void InvertedIndex::run(bool clear)
{
    std::vector<bson_t*> documents{};

    try
    {  
        // m_index = m_db -> getDocument(m_database_name,m_collection_name);
        

        if(!clear){
            retrieveExistingIndex();
        }
        documents = m_db->getAllDocuments(m_database_name,m_documents_collection_name);
        std::cout << "size: " << documents.size() << '\n';
        for (auto document : documents)
        {
            std::string content = m_db->extractContentFromIndexDocument(document);
            std::string docId = m_db->extractIndexFromIndexDocument(document);
            addDocument(docId, content);
        }
        m_db->clearCollection(m_database_name,m_collection_name);// clear older documents
        m_db->saveInvertedIndex(m_index,m_database_name,m_collection_name); // save updated documents

    }
    catch (std::exception &ex)
    {
        std::cout << "Error: " << ex.what() << '\n';
    }
}

void InvertedIndex::addDocument(const std::string docId, std::string &content)
{   
    std::vector<std::string> tokens = WordProcessor::tokenize(content);
    std::string token{};
    for (int i=0;i<tokens.size();i++)
    {
        token = WordProcessor::normalize(tokens[i]);
        
        if (WordProcessor::isStopWord(token))
            continue;
        token = WordProcessor::stem(token);
        if(WordProcessor::isValidWord(token)){
        m_index[token][docId].push_back(i);
        }
    }
}

void InvertedIndex::retrieveExistingIndex()
{
    std::vector<bson_t*> documents = m_db->getAllDocuments(m_database_name,m_collection_name);

    for(const bson_t* document:documents){
        InvertedIndex::extractInvertedIndexDocument(document);
    }

}

void InvertedIndex::extractInvertedIndexDocument(const bson_t *document)
{
    bson_iter_t iter;
    if(!bson_iter_init(&iter,document)){
        std::cerr<<"Failed to initialize BSON iterator."<<std::endl;
        return;
    }
    while(bson_iter_next(&iter)){
        if(BSON_ITER_HOLDS_ARRAY(&iter)){
            std::string term = bson_iter_key(&iter);
            std::unordered_map<std::string,std::vector<int>> doc_map;
            bson_iter_t array_iter;
            bson_iter_recurse(&iter,&array_iter);
            while(bson_iter_next(&array_iter)){
                if(BSON_ITER_HOLDS_DOCUMENT(&array_iter)){
                    bson_iter_t doc_iter;
                    bson_iter_recurse(&array_iter,&doc_iter);

                    std::string doc_id;
                    std::vector<int> positions{};
                    doc_id = bson_iter_key(&array_iter);
                    while(bson_iter_next(&doc_iter)){
                        if(strcmp(bson_iter_key(&doc_iter),"docId") == 0){
                            doc_id = bson_iter_utf8(&doc_iter,nullptr);
                        }else if(strcmp(bson_iter_key(&doc_iter),"positions") == 0){
                            bson_iter_t positions_iter;
                            bson_iter_recurse(&doc_iter,&positions_iter);
                            while(bson_iter_next(&positions_iter)){
                                positions.push_back(bson_iter_int32(&positions_iter));
                            }
                        }
                    }
                    doc_map[doc_id]=positions;
                }
            }
            m_index[term] = doc_map;
        }
    }
}
