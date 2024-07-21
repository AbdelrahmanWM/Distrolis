#include "DataBase.h"
#include <cstdlib>
#include <iostream>

DataBase* DataBase::db = nullptr;

DataBase::DataBase(const std::string& connectionString, const std::string& database_name, const std::string& collection_name)
    :m_database_name(database_name), m_collection_name(collection_name)
{
    mongoc_init();

    m_client = mongoc_client_new(connectionString.c_str());
    if (!m_client) {
        std::cerr << "Failed to create a new client instance." << std::endl;
        return;
    }


    bson_t* command = BCON_NEW("ping", BCON_INT32(1));
    bson_error_t error;
    bool retval = mongoc_client_command_simple(m_client, "admin", command, nullptr, nullptr, &error);

    if (!retval) {
        std::cerr << "Failed to run command: " << error.message << std::endl;
    }
    else {
        std::cout << "Connected successfully." << std::endl;
    }

    bson_destroy(command);
}

DataBase::~DataBase() {

    if (m_client) {
        mongoc_client_destroy(m_client);
    }
    mongoc_cleanup();
}

DataBase* DataBase::getInstance(const std::string& connectionString,const std::string& database_name,const std::string& collection_name) {
    if (!db) {
        db = new DataBase(connectionString,database_name,collection_name);
    }
    return db;
}

void DataBase::destroyInstance() {
    delete db;
    db = nullptr;
}

void DataBase::insertDocument( const bson_t* document) const
{
    std::cout << *bson_as_json(document,nullptr)<<"\n";
    mongoc_collection_t* collection = nullptr;
    bson_error_t error;
    bool insertSuccess = false;
    try {
        collection = mongoc_client_get_collection(m_client, "SearchEngine", m_collection_name.c_str());
        if (!collection) {
            std::cerr << "Failed to get collection: " << m_collection_name << std::endl;
            return;
        }

        std::cout << "Collection obtained successfully." << std::endl;

        insertSuccess = mongoc_collection_insert_one(collection, document, nullptr, nullptr, &error);
        if (!insertSuccess) {
            std::cerr << "Failed to insert document: " << error.message << std::endl;
        }
        else {
            std::cout << "Document inserted successfully." << std::endl;
        }
    }
    catch (std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

    if (collection) {
        mongoc_collection_destroy(collection);
    }
}

void DataBase::clearCollection() const
{
    mongoc_collection_t* collection=nullptr;
    try {
        collection = mongoc_client_get_collection(m_client, m_database_name.c_str(), m_collection_name.c_str());
        if (!collection) {
            throw std::runtime_error("Failed to get collection.");
        }

        bson_t* filter = bson_new();

        bson_error_t error;
        if (!mongoc_collection_delete_many(collection, filter, nullptr, nullptr, &error)) {
            throw std::runtime_error(std::string("Failed to delete documents: ") + error.message);
        }
        bson_destroy(filter);
    }
    catch (std::exception & ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    if (collection) {
        mongoc_collection_destroy(collection);
    }
}



