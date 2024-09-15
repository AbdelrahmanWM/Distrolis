#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <string>
#include <queue>
#include <curl/curl.h>
#include <iostream>
#include <random>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unordered_set>
#include "HTMLParser.h"
#include "URLParser.h"

class WebCrawler {
public:
	WebCrawler(DataBase*& database,const HTMLParser& parser,const std::string& database_name, const std::string& collection_name, const std::string& visitedUrls_collection_name,const bool useProxy,const std::string& proxyAPIUrl="",int numberOfThreads=1);
	~WebCrawler();
	void run(int maximumNumberOfPagesToCrawl,std::queue<std::string>& seedUrl);

	void crawl(int maximumNumberOfPagesToCrawl);
	void clearCrawledDocuments();
 	void setDatabaseName(const std::string& databaseName);
	void setDocumentsCollectionName(const std::string& collectionName);
	void setVisitedUrlCollectionName(const std::string& collectionName);
	bool setNumberOfThreads(int numberOfThreads);
private: 
	void addSeedUrls(std::queue<std::string>& seedUrls);
	void parsePage(const std::string& htmlContent, const std::string& url);
	std::string fetchPage(CURL *curl,const std::string& url,bool useProxy=true);
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static std::string getRandomUserAgent();
	static std::string getRandomProxy();
	void fetchProxies(CURL *curl,const std::string& url);

	void fetchRobotsTxtContent(CURL *curl,const std::string&url);
    bool isURLVisited(const std::string& absoluteURL);
	void markURLAsVisited(const std::string& absoluteURL);
	void saveVisitedUrls();
	void retrieveVisitedUrls();
   
	DataBase*& m_db;
	const HTMLParser& m_parser;
	std::queue<std::string> m_frontier;
	std::vector<bson_t*> m_crawled_pages;
	std::unordered_set<size_t> m_visitedUrls;
	std::string m_database_name;
	std::string m_collection_name; 
	std::string m_visitedUrls_collection_name;
	const bool m_useProxy;
	static std::vector<std::string> m_proxiesList;
	std::mutex m_visitedUrlsMutex;
	std::mutex m_frontierMutex;
	std::mutex m_crawledPagesMutex;
	std::mutex m_stopMutex;
	std::condition_variable m_stopCondition;
	int m_numberOfThreads;

	

};
#endif
