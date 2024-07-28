#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <mongoc/mongoc.h>
#include <bson/bson.h>

class DataBase
{
public:
    static DataBase *getInstance(const std::string &connectionString);
    static void destroyInstance();

    void insertDocument(const bson_t *document,const std::string& database_name, const std::string &collection_name) const;
    void insertManyDocuments(std::vector<const bson_t *>documents,const std::string& database_name, const std::string &collection_name) const;
    std::vector<bson_t> getAllDocuments(const std::string& database_name, const std::string& collection_name) const;
    void clearCollection(const std::string& database_name, const std::string collection_name) const;
    void saveInvertedIndex(const std::unordered_map<std::string,  std::unordered_map<std::string,int>> &index, const std::string&database_name,const std::string collection_name) const;
    std::string extractContentFromIndexDocument(const bson_t &document) const;
    std::string extractIndexFromIndexDocument(const bson_t &document) const;

private:
    DataBase(const std::string &connectionString);
    ~DataBase();
    static DataBase *db;
    mongoc_client_t *m_client;
};

#endif // DATABASE_H
