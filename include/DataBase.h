#include <string>
#include<iostream>
#include <mongoc/mongoc.h>
#ifndef DATABASE_H
#define DATABASE_H


class DataBase {
public:
	static DataBase* getInstance(const std::string& connectionString);
	static void destroyInstance();
	void insertDocument(const std::string& collectionName, const bson_t* document);

private:
	DataBase(const std::string& connectionString);
	~DataBase();
	static DataBase* db;
	mongoc_client_t*client;
	mongoc_database_t* database;
};
#endif