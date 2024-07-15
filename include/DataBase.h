#include <string>
#include<iostream>
#include <mongoc/mongoc.h>
#ifndef DATABASE_H
#define DATABASE_H


class DataBase {
public:
	~DataBase();
	static DataBase* getInstance(const std::string& connectionString);
	static void destroyInstance();
private:
	DataBase(const std::string& connectionString);
	static DataBase* db;
	mongoc_client_t*client;
	mongoc_database_t* database;
};
#endif