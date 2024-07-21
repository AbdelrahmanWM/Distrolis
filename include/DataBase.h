#include <string>
#include<iostream>
#include <mongoc/mongoc.h>
#ifndef DATABASE_H
#define DATABASE_H


class DataBase {
public:
	static DataBase* getInstance(const std::string& connectionString, const std::string& database_name, const std::string& collection_name);
	static void destroyInstance();
	void insertDocument( const bson_t* document) const;
	void clearCollection() const;

private:
	DataBase(const std::string& connectionString, const std::string& database_name, const std::string& collection_name);
	~DataBase();
	static DataBase* db;
	const std::string m_database_name;
	const std::string m_collection_name;
	mongoc_client_t*m_client;
};
#endif