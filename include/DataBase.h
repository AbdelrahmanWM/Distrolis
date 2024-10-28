#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <mutex>
#include <thread>
#include <mongoc/mongoc.h>
#include <bson/bson.h>

class DataBase
{
public:

    DataBase(const std::string &connectionString);

    ~DataBase();
    void insertDocument(const bson_t *document, const std::string &database_name, const std::string &collection_name);
    void insertManyDocuments(std::vector<bson_t *>& documents, const std::string &database_name, const std::string &collection_name);
    void insertOrUpdateManyDocuments(std::vector<std::pair<bson_t*,bson_t*>>& filterAndUpdateDocuments,const std::string& database_name, const std::string& collection_name);
    bson_t *getDocument(const std::string &database_name, const std::string &collection_name);
    std::vector<bson_t *> getAllDocuments(const std::string &database_name, const std::string &collection_name, bson_t *filters = bson_new());
    std::vector<bson_t*> getLimitedDocuments(const std::string&database_name,const std::string&collection_name,int limit,bson_t *filters= bson_new());
    int getCollectionDocumentCount(const std::string &database_name, const std::string& collection_name,bson_t*filter=bson_new());
    std::vector<bson_t *> getDocumentsByIds(const std::string &database_name, const std::string &collection_name, const std::vector<std::string> &ids);
    bson_t *createIdFilter(const std::vector<std::string> &ids);
    void clearCollection(const std::string &database_name, const std::string collection_name);
    void saveInvertedIndex(const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> &index, const std::string &database_name, const std::string collection_name);
    void markDocumentsProcessed(std::vector<bson_t *>&documents, const std::string &database_name, const std::string &collection_name);
    std::string extractContentFromIndexDocument(const bson_t *document);
    std::string extractIndexFromIndexDocument(const bson_t *document);
    std::string getConnectionString();
    void destroyConnection();
private:
    std::string m_connection_string;
    void processWordDocuments(std::vector<std::pair<bson_t *,bson_t*>> &filterAndUpdateDocuments, const std::string &term, const std::unordered_map<std::string, std::vector<int>> &map);
    mongoc_client_t *m_client;
    std::mutex dbMutex;
};

#endif // DATABASE_H
