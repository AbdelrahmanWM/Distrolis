#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <unordered_set>
#include "DataBase.h"
#include "ThreadPool.h"

class InvertedIndex
{
public:
    struct document_metadata
    {
        int total_documents;
        std::unordered_map<std::string, int> doc_lengths;
        double average_doc_length;
        document_metadata() : total_documents(0), average_doc_length(0.0) {}
    };
    InvertedIndex(DataBase *&db, const std::string &database_name, const std::string &collection_name, const std::string &documents_collection_name, const std::string &metadata_collection_name, int numberOfThreads = 1);
    void run(bool clear = false);
    void terminate(bool clear=true);
    void index(std::vector<bson_t *> documents);
    void processDocument(bson_t *document, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> &index, document_metadata &document_metadata);
    void addDocument(const std::string docId, std::string &content, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> &index, document_metadata &document_metadata);
    void retrieveExistingMetadataDocument();
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> retrieveExistingIndex();

    document_metadata getMetadataDocument();
    void setDatabaseName(const std::string &databaseName);
    void setInvertedIndexCollectionName(const std::string &collectionName);
    void setDocumentsCollectionName(const std::string &collectionName);
    void setMetadataCollectionName(const std::string &collectionName);
    bool setNumberOfThreads(int numberOfThreads);

private:
    void extractInvertedIndexDocument(bson_t *&document, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> &index);

    bson_t *getEmptyMetaDataDocument();
    void saveMetadataDocument();
    void updateMetadataDocument(document_metadata &document);

    static double getAverageDocumentSize(const document_metadata &document);
    // std::unordered_map<std::string, std::unordered_map<std::string,std::vector<int>>> m_index;
    document_metadata m_document_metadata;
    // document_metadata m_iteration_metadata;
    DataBase *&m_db;
    std::string m_database_name;
    std::string m_collection_name;
    std::string m_documents_collection_name;
    std::string m_metadata_collection_name;
    std::mutex m_metadata_document_mutex;
    std::mutex indexMutex;
    std::atomic<bool> m_stopRequest{};
    std::atomic<bool> m_clearHistory{};
    int m_number_of_threads;
};

#endif
