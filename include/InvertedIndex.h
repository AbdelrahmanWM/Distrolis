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
    InvertedIndex(const DataBase *&db);
    void run();
    void addDocument(const std::string docId, std::string &content);

private:
    std::unordered_map<std::string, std::vector<Posting>> m_index;
    const DataBase *&m_db;
    std::vector<std::string> tokenize(std::string &content);
    std::string normalize(std::string &token);
    std::string stem(std::string &word);
    bool isStopWord(std::string &word);
};

#endif
