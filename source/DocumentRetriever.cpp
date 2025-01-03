#include "DocumentRetriever.h"

DocumentRetriever::DocumentRetriever(DataBase *db)
    : m_db{db}
{
}

std::vector<SearchResultDocument> DocumentRetriever::getScoresDocuments(const std::string &database_name, const std::string &collection_name, const std::vector<std::pair<std::string, double>> &scoresDocuments, std::unordered_map<std::string, double> &scoresMap)
{
    std::vector<std::string> ids;
    std::vector<SearchResultDocument> results{};
    for (const auto &pair : scoresDocuments)
        ids.push_back(pair.first);
    std::cout << "IDs " << ids.size() << '\n';

    std::vector<bson_t *> fetchedDocuments = m_db->getDocumentsByIds(database_name, collection_name, ids);

    std::cout << "fetched: " << fetchedDocuments.size() << '\n';

    for (int i = 0; i < fetchedDocuments.size(); i++)
    {
        Document documentObject = m_db->extractDocument(fetchedDocuments[i]);
        SearchResultDocument resultDocument{documentObject._id, documentObject.title, documentObject.content, documentObject.url, scoresMap[documentObject._id]};
        results.push_back(resultDocument);
    }
    return results;
}