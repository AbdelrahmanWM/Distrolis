#include "InvertedIndex.h"
#include "DataBase.h"
#include "WordProcessor.h"

InvertedIndex::InvertedIndex(DataBase *&db, const std::string &database_name, const std::string &collection_name, const std::string &documents_collection_name, const std::string &metadata_collection_name)
    : m_db{db}, m_database_name{database_name}, m_collection_name{collection_name}, m_documents_collection_name{documents_collection_name}, m_metadata_collection_name{metadata_collection_name}
{
}

void InvertedIndex::run(bool clear)
{
    std::vector<bson_t *> documents{};

    m_document_metadata = {};
    try
    {
        // m_index = m_db -> getDocument(m_database_name,m_collection_name);
        if(clear){
            m_db->clearCollection(m_database_name,m_collection_name);
        }else{
            // retrieveExistingIndex();
            retrieveExistingMetadataDocument();
        }
        int numberOfDocuments = m_db->getCollectionDocumentCount(m_database_name, m_documents_collection_name, BCON_NEW("processed", BCON_BOOL(false)));

        int numberOfDocumentsToIndex = std::min(1000, (2 * numberOfDocuments / m_number_of_threads));
        {
            ThreadPool thread_pool{m_number_of_threads};
            std::vector<bson_t*>documents;
            while (numberOfDocuments > 0)
            {   std::cout<<1<<"\n";
                documents=m_db->getLimitedDocuments(m_database_name,m_documents_collection_name,numberOfDocumentsToIndex,BCON_NEW("processed",BCON_BOOL(false)));
                if (documents.empty())
                {
                    std::cout << "No \"un-processed\" documents found to index\n";
                    return;
                }
                std::cout<<2<<"\n";
                m_db->markDocumentsProcessed(documents,m_database_name,m_documents_collection_name);
                std::cout<<3<<'\n';
                thread_pool.enqueue([this, documents]
                                    { this->index(documents); });
                for(auto document:documents){
                    bson_destroy(document);
                }
                numberOfDocuments -= numberOfDocumentsToIndex;
            }
        }
        saveMetadataDocument();
    }
    catch (std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
    }
}

void InvertedIndex::index(std::vector<bson_t *> documents)
{
   std::cout<<std::this_thread::get_id()<<'\n';

    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> index = {};
    document_metadata metadata_document = {};
    DataBase db{m_db->getConnectionString()};
    try
    {
        metadata_document.total_documents += documents.size();
        {
            for (auto document : documents)
            {
                processDocument(document, index, metadata_document);
            }
        }
        updateMetadataDocument(metadata_document);
        db.saveInvertedIndex(index, m_database_name, m_collection_name);
    }
    catch (std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
    }
}

void InvertedIndex::processDocument(bson_t *document, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> &index, document_metadata &metadata_document)
{
    std::string content = m_db->extractContentFromIndexDocument(document);
    std::string docId = m_db->extractIndexFromIndexDocument(document);
    addDocument(docId, content, index, metadata_document);
    // m_db->markDocumentProcessed(document, m_database_name, m_documents_collection_name);
}

InvertedIndex::document_metadata InvertedIndex::getMetadataDocument()
{
    return m_document_metadata;
}

void InvertedIndex::addDocument(const std::string docId, std::string &content, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> &index, document_metadata &document_metadata)
{
    std::cout<<std::this_thread::get_id()<<'\n';
    std::vector<std::string> tokens = WordProcessor::tokenize(content);
    std::string token{};
    document_metadata.doc_lengths[docId] = tokens.size();
    for (long long unsigned int i = 0; i < tokens.size(); i++)
    {
        token = WordProcessor::normalize(tokens[i]);

        // if (WordProcessor::isStopWord(token)) // Stop words will complicate phrase search
        //     continue;
        token = WordProcessor::stem(token);
        // if(WordProcessor::isValidWord(token)){
        index[token][docId].push_back(i);
        // }
    }
}

bson_t *InvertedIndex::getEmptyMetaDataDocument()
{

    bson_t *bson = bson_new();

    try
    {
        bson_init(bson);

        bson_append_int64(bson, "total_documents", -1, 0);
        bson_append_double(bson, "average_doc_length", -1, 0);
        bson_t documents_map;
        BSON_APPEND_DOCUMENT_BEGIN(bson, "doc_lengths", &documents_map);
        bson_append_document_end(bson, &documents_map);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    return bson;
}
void InvertedIndex::retrieveExistingMetadataDocument()
{
    bson_t *document = m_db->getDocument(m_database_name, m_metadata_collection_name);
    if (document == nullptr)
    {
        document = getEmptyMetaDataDocument();
    }
    bson_iter_t iter;
    if (!bson_iter_init(&iter, document))
    {
        std::cerr << "Failed to initialize BSON iterator." << std::endl;
        return;
    }
    while (bson_iter_next(&iter))
    {
        if (strcmp(bson_iter_key(&iter), "total_documents") == 0)
        {
            m_document_metadata.total_documents = bson_iter_int64(&iter);
        }
        if (strcmp(bson_iter_key(&iter), "average_doc_length") == 0)
        {
            m_document_metadata.average_doc_length = bson_iter_double(&iter);
        }
        if (strcmp(bson_iter_key(&iter), "doc_lengths") == 0)
        {
            bson_iter_t doc_iter;
            bson_iter_recurse(&iter, &doc_iter);

            while (bson_iter_next(&doc_iter))
            {

                int length = bson_iter_int64(&doc_iter);
                auto documentId = bson_iter_key(&doc_iter);
                // std::cout<<documentId<<","<<length<<"\n";
                m_document_metadata.doc_lengths[documentId] = length;
            }
        }
    }
    bson_destroy(document);
}

std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> InvertedIndex::retrieveExistingIndex()
{
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> index;
    std::vector<bson_t *> documents = std::move(m_db->getAllDocuments(m_database_name, m_collection_name));

    for (bson_t *document : documents)
    {
        InvertedIndex::extractInvertedIndexDocument(document,index);
    }
    for (auto document : documents)
    {
        bson_destroy(document);
    }
    documents.clear();
    return index;
}


void InvertedIndex::extractInvertedIndexDocument(bson_t *&document,std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>>&index)
{
    bson_iter_t iter;
    std::string term;
    if (!bson_iter_init(&iter, document))
    {
        std::cerr << "Failed to initialize BSON iterator." << std::endl;
        return;
    }

    if (bson_iter_find(&iter, "term") && BSON_ITER_HOLDS_UTF8(&iter))
    {
        term = bson_iter_utf8(&iter, NULL);
    }
    else
    {
        std::cerr << "Failed to find term.\n";
        return;
    }
    if (bson_iter_find(&iter, "documents") && BSON_ITER_HOLDS_ARRAY(&iter))
    {
        std::unordered_map<std::string, std::vector<int>> doc_map;
        bson_iter_t array_iter;
        bson_iter_recurse(&iter, &array_iter);
        while (bson_iter_next(&array_iter))
        {
            if (BSON_ITER_HOLDS_DOCUMENT(&array_iter))
            {
                bson_iter_t doc_iter;
                bson_iter_recurse(&array_iter, &doc_iter);

                std::string doc_id;
                std::vector<int> positions{};
                doc_id = bson_iter_key(&array_iter);
                while (bson_iter_next(&doc_iter))
                {
                    if (strcmp(bson_iter_key(&doc_iter), "docId") == 0)
                    {
                        doc_id = bson_iter_utf8(&doc_iter, nullptr);
                    }
                    else if (strcmp(bson_iter_key(&doc_iter), "positions") == 0)
                    {
                        bson_iter_t positions_iter;
                        bson_iter_recurse(&doc_iter, &positions_iter);
                        while (bson_iter_next(&positions_iter))
                        {
                            positions.push_back(bson_iter_int32(&positions_iter));
                        }
                    }
                }
                if (!doc_id.empty())
                {
                    doc_map[doc_id] = positions;
                }
            }
        }
        index[term] = doc_map;
    }
    else
    {
        std::cerr << "Failed to find documents.\n";
    }
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
        BSON_APPEND_DOCUMENT_BEGIN(bson, "doc_lengths", &documents_map);
        for (const auto &[docId, docSize] : m_document_metadata.doc_lengths)
        {
            bson_append_int64(&documents_map, docId.c_str(), -1, docSize);
        }
        bson_append_document_end(bson, &documents_map);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    m_db->clearCollection(m_database_name, m_metadata_collection_name);
    m_db->insertDocument(bson, m_database_name, m_metadata_collection_name);
}

void InvertedIndex::updateMetadataDocument(document_metadata &metadata_document)

{
    std::lock_guard<std::mutex> lock(m_metadata_document_mutex);
    metadata_document.average_doc_length = getAverageDocumentSize(metadata_document);
    if (m_document_metadata.total_documents == 0)
    {
        m_document_metadata = metadata_document;
    }
    else
    {
        m_document_metadata.average_doc_length =
            (m_document_metadata.total_documents * m_document_metadata.average_doc_length + metadata_document.total_documents * metadata_document.average_doc_length) / (m_document_metadata.total_documents + metadata_document.total_documents);

        m_document_metadata.total_documents += metadata_document.total_documents;
        m_document_metadata.doc_lengths.insert(metadata_document.doc_lengths.begin(), metadata_document.doc_lengths.end());
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
    double sum = 0.0;
    for (const auto &doc : document.doc_lengths)
    {
        sum += (static_cast<double>(doc.second) / document.total_documents);
    }
    return sum;
}
bool InvertedIndex::setNumberOfThreads(int numberOfThreads)
{
    if (numberOfThreads > 0 && numberOfThreads < static_cast<int>(std::thread::hardware_concurrency()))
    {
        m_number_of_threads = numberOfThreads;
        return true;
    }
    return false;
}