#include <string>
#include <libmongoc-1.0/mongoc/mongoc.h>
#ifndef DATABASE_H
#define DATABASE_H


class DataBase {
public:
	DataBase(const std::string& connectionString);
	~DataBase();
private:
	DataBase* db;
};
#endif