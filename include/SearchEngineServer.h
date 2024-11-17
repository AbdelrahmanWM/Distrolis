#include "SearchEngine.h"
#include "crow.h"
#ifndef SEARCHENGINESERVER_H
#define SEARCHENGINESERVER_H

class SearchEngineServer {
public:
    SearchEngineServer(SearchEngine searchEngine);
    ~SearchEngineServer();
    void start();
    
private: 
    
    crow::response search(const crow::request& req);
    crow::response setThreadsNumber(const crow::request& req);
    crow::response crawl(const crow::request& req);
    crow::response crawl_terminate(const crow::request&req);
    crow::response indexDocument(const crow::request& req);
    crow::response index_terminate(const crow::request&req);
    crow::response crawlAndIndexDocument(const crow::request& req);
    crow::response clearCrawlHistory(const crow::request& req);
    crow::response setRankerParameters(const crow::request& req);
    crow::response rootAPIDocumentation(const crow::request& req);
    crow::json::wvalue SearchResultsDocumentToJSON(std::vector<SearchResultDocument>& resultsDocument );
    std::queue<std::string> SeedUrlsJSONToQueue(crow::json::wvalue& list);
    std::string getAPIDocumentationHTMLPage();
    
    SearchEngine m_searchEngine;
};

#endif 