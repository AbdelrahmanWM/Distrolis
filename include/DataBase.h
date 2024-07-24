#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <mongoc/mongoc.h>
#include <bson/bson.h>
#include "Posting.h"

class DataBase
{
public:
    static DataBase *getInstance(const std::string &connectionString, const std::string &database_name, const std::string &collection_name);
    static void destroyInstance();

    void insertDocument(const bson_t *document, const std::string &collectionName = "") const;
    std::vector<bson_t> getAllDocuments() const;
    void clearCollection() const;
    void saveInvertedIndex(const std::unordered_map<std::string, std::vector<Posting>> &map) const;
    std::string extractContentFromIndexDocument(const bson_t &document) const;
    std::string extractIndexFromIndexDocument(const bson_t &document) const;

private:
    DataBase(const std::string &connectionString, const std::string &database_name, const std::string &collection_name);
    ~DataBase();

    static DataBase *db;
    std::string m_database_name;
    std::string m_collection_name;
    mongoc_client_t *m_client;
};

#endif // DATABASE_H
