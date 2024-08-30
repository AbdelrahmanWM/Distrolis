#include "InvertedIndex.h"
#include "DataBase.h"
#include "WordProcessor.h"

InvertedIndex::InvertedIndex(const DataBase *&db,const std::string& database_name, const std::string& collection_name,  const std::string& documents_collection_name,const std::string& metadata_collection_name)
    : m_index{}, m_db{db}, m_database_name{database_name},m_collection_name{collection_name},m_documents_collection_name{documents_collection_name},m_metadata_collection_name{metadata_collection_name}
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
            retrieveExistingMetadataDocument();
        }
        documents = m_db->getAllDocuments(m_database_name,m_documents_collection_name,BCON_NEW("processed",BCON_BOOL(false)));
        // std::cout << "size: " << documents.size() << '\n';
        m_iteration_metadata.total_documents+=documents.size();
        
        for (auto document : documents)
        {
            std::string content = m_db->extractContentFromIndexDocument(document);
            std::string docId = m_db->extractIndexFromIndexDocument(document);
            addDocument(docId, content);
            m_db->markDocumentProcessed(document,m_database_name,m_documents_collection_name);
        }
        updateMetadataDocument();
        // std::cout<<"Average: "<<m_document_metadata.average_doc_length<<"\n";
        // std::cout<<"Total: "<<m_document_metadata.total_documents<<'\n';
        // for(auto document:m_document_metadata.doc_lengths)std::cout<<document.first<<","<<document.second<<"\n";
        saveMetadataDocument();
        saveInvertedIndex();

    }
    catch (std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
    }
}

InvertedIndex::document_metadata InvertedIndex::getMetadataDocument()
{
    return m_document_metadata;
}

void InvertedIndex::addDocument(const std::string docId, std::string &content)
{   
    std::vector<std::string> tokens = WordProcessor::tokenize(content);
    std::string token{};
    m_iteration_metadata.doc_lengths[docId]=tokens.size();
    
    for (long long unsigned int i=0;i<tokens.size();i++)
    {
        token = WordProcessor::normalize(tokens[i]);
        
        // if (WordProcessor::isStopWord(token)) // Stop words will complicate phrase search
        //     continue;
        token = WordProcessor::stem(token);
        // if(WordProcessor::isValidWord(token)){
        m_index[token][docId].push_back(i);
        // }
    }
}

void InvertedIndex::retrieveExistingMetadataDocument()
{
    bson_t* document = m_db->getDocument(m_database_name,m_metadata_collection_name);
    bson_iter_t iter;
    if(!bson_iter_init(&iter,document)){
        std::cerr<<"Failed to initialize BSON iterator."<<std::endl;
        return;
    }
    while(bson_iter_next(&iter)){
        if(strcmp(bson_iter_key(&iter),"total_documents") == 0){
            m_document_metadata.total_documents = bson_iter_int64(&iter);
        }
        if(strcmp(bson_iter_key(&iter),"average_doc_length") == 0){
            m_document_metadata.average_doc_length = bson_iter_double(&iter);
        }
        if(strcmp(bson_iter_key(&iter),"doc_lengths") == 0){
            bson_iter_t doc_iter;
            bson_iter_recurse(&iter,&doc_iter);

            while(bson_iter_next(&doc_iter)){

                int length= bson_iter_int64(&doc_iter);
                auto documentId =bson_iter_key(&doc_iter); 
                // std::cout<<documentId<<","<<length<<"\n";
                m_document_metadata.doc_lengths[documentId] = length ;
            }
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

std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> InvertedIndex::getInvertedIndex()
{
    return m_index;
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

void InvertedIndex::saveInvertedIndex()
{
    m_db->clearCollection(m_database_name,m_collection_name);// clear older documents
    m_db->saveInvertedIndex(m_index,m_database_name,m_collection_name); // save updated documents
}

void InvertedIndex::saveMetadataDocument()
{
    bson_t *bson = bson_new();
    
    try
    {
        bson_init(bson);

        bson_append_int64(bson, "total_documents", -1, m_document_metadata.total_documents);
        bson_append_double(bson, "average_doc_length", -1, m_document_metadata.average_doc_length);
        bson_t documents_map;
        BSON_APPEND_DOCUMENT_BEGIN(bson,"doc_lengths",&documents_map);
        for(const auto&[docId,docSize]:m_document_metadata.doc_lengths){
            bson_append_int64(&documents_map,docId.c_str(),-1,docSize);
        }
        bson_append_document_end(bson,&documents_map);
        
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    m_db->clearCollection(m_database_name,m_metadata_collection_name);
    m_db->insertDocument(bson,m_database_name,m_metadata_collection_name);
}


void InvertedIndex::updateMetadataDocument()

{   
    m_iteration_metadata.average_doc_length=getAverageDocumentSize(m_iteration_metadata);
    if(m_document_metadata.total_documents==0){
       m_document_metadata = m_iteration_metadata;
    }
    else{
        m_document_metadata.average_doc_length = 
        (m_document_metadata.total_documents*m_document_metadata.average_doc_length+m_iteration_metadata.total_documents*m_iteration_metadata.average_doc_length)
        /(m_document_metadata.total_documents+m_iteration_metadata.total_documents);

        m_document_metadata.total_documents+=m_iteration_metadata.total_documents;
        // for(auto document: m_document_metadata.doc_lengths)std::cout<<document.first<<","<<document.second<<'\n';
        // for(auto document: m_iteration_metadata.doc_lengths)std::cout<<document.first<<","<<document.second<<'\n';
        m_document_metadata.doc_lengths.insert(m_iteration_metadata.doc_lengths.begin(),m_iteration_metadata.doc_lengths.end());
    }
}

void InvertedIndex::setDatabaseName(const std::string &databaseName)
{
    m_database_name = databaseName;
}

void InvertedIndex::setInvertedIndexCollectionName(const std::string &collectionName)
{
    m_collection_name = collectionName;
}

void InvertedIndex::setDocumentsCollectionName(const std::string &collectionName)
{
    m_documents_collection_name = collectionName;
}

void InvertedIndex::setMetadataCollectionName(const std::string &collectionName)
{
    m_metadata_collection_name = collectionName;
}

double InvertedIndex::getAverageDocumentSize(const document_metadata &document)
{
    double sum=0.0;
    for(const auto& doc:document.doc_lengths){
        sum+=(static_cast<double>(doc.second)/document.total_documents);
    }
    return sum;
}
