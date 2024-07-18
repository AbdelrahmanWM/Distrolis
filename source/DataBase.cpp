#include "DataBase.h"
#include <cstdlib>
#include <iostream>

DataBase* DataBase::db = nullptr;

DataBase::DataBase(const std::string& connectionString) {
    mongoc_init();

    client = mongoc_client_new(connectionString.c_str());
    if (!client) {
        std::cerr << "Failed to create a new client instance." << std::endl;
        return;
    }

    database = mongoc_client_get_database(client, "SearchEngine");

    bson_t* command = BCON_NEW("ping", BCON_INT32(1));
    bson_error_t error;
    bool retval = mongoc_client_command_simple(client, "admin", command, nullptr, nullptr, &error);

    if (!retval) {
        std::cerr << "Failed to run command: " << error.message << std::endl;
    }
    else {
        std::cout << "Connected successfully." << std::endl;
    }

    bson_destroy(command);
}

DataBase::~DataBase() {
    mongoc_database_destroy(database);
    if (client) {
        mongoc_client_destroy(client);
    }
    mongoc_cleanup();
}

DataBase* DataBase::getInstance(const std::string& connectionString) {
    if (!db) {
        db = new DataBase(connectionString);
    }
    return db;
}

void DataBase::destroyInstance() {
    delete db;
    db = nullptr;
}

void DataBase::insertDocument(const std::string& collectionName, const bson_t* document) const
{
    std::cout << *bson_as_json(document,nullptr)<<"\n";
    mongoc_collection_t* collection = nullptr;
    bson_error_t error;
    bool insertSuccess = false;
    try {
        collection = mongoc_client_get_collection(client, "SearchEngine", collectionName.c_str());
        if (!collection) {
            std::cerr << "Failed to get collection: " << collectionName << std::endl;
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

