#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <unordered_set>
#include "DataBase.h"
#include "libstemmer.h"
#include "Posting.h"

class InvertedIndex
{
public:
    InvertedIndex(const DataBase *&db,const std::string& database_name, const std::string& collection_name,const std::string& documents_collection_name);
    void run(bool clear = false);
    void addDocument(const std::string docId, std::string &content);

private:
    std::unordered_map<std::string, std::vector<Posting>> m_index;
    const DataBase *&m_db;
    const std::string m_database_name;
    const std::string m_collection_name;
    const std::string m_documents_collection_name;
    std::vector<std::string> tokenize(std::string &content);
    std::string normalize(std::string &token);
    std::string stem(std::string &word);
    bool isStopWord(std::string &word);
    static const std::unordered_set<std::string>& getStopWords();
};

#endif
