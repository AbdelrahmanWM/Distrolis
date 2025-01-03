#include "DataBase.h"
#include "DocumentTypes.h"
#include <vector>

#ifndef DOCUMENTRETRIEVER_H
#define DOCUMENTRETRIEVER_H
class DocumentRetriever
{
public:
    DocumentRetriever(DataBase *db);
    std::vector<SearchResultDocument> getScoresDocuments(const std::string &database_name, const std::string &collection_name, const std::vector<std::pair<std::string, double>> &scoresDocuments, std::unordered_map<std::string,double>&scores_documents);

private:
    DataBase *m_db;
};

#endif