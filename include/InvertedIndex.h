#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <unordered_set>
#include "DataBase.h"




class InvertedIndex
{
public:
    InvertedIndex(const DataBase *&db,const std::string& database_name, const std::string& collection_name,const std::string& documents_collection_name,const std::string& metadata_collection_name);
    void run(bool clear = false);
    

private:
    struct document_metadata{
        int total_documents;
        std::unordered_map<std::string,int> doc_lengths;
        double average_doc_length;
        document_metadata() : total_documents(0), average_doc_length(0.0) {}
    };
    void addDocument(const std::string docId, std::string &content);
    void retrieveExistingIndex();
    void extractInvertedIndexDocument(const bson_t*document);
    void saveInvertedIndex();
    void saveMetadataDocument();
    void updateMetadataDocument();
    static double getAverageDocumentSize(const document_metadata& document);
    std::unordered_map<std::string, std::unordered_map<std::string,std::vector<int>>> m_index;
    document_metadata m_document_metadata;
    document_metadata m_iteration_metadata;
    const DataBase *&m_db;
    const std::string m_database_name;
    const std::string m_collection_name;
    const std::string m_documents_collection_name;
    const std::string m_metadata_collection_name;
  
};

#endif
