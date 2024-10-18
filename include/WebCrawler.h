#ifndef webcrawler_h
#define webcrawler_h

#include <string>
#include <queue>
#include <deque>
#include <curl/curl.h>
#include <iostream>
#include <random>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unordered_set>
#include "htmlparser.h"
#include "urlparser.h"

class WebCrawler
{
public:
	WebCrawler(DataBase *&database, const HTMLParser &parser, const std::string &database_name, const std::string &collection_name, const std::string &visitedUrls_collection_name, const bool useProxy, const std::string &proxyAPIUrl = "", int numberOfThreads = 1);
	~WebCrawler();
	void run(int maximumNumberOfPagesToCrawl, std::queue<std::string> &seedUrl);
    void terminate(bool clearHistory=true);

	void crawl(int maximumNumberOfPagesToCrawl);
	void clearCrawledDocuments();
	void setDatabaseName(const std::string &databaseName);
	void setDocumentsCollectionName(const std::string &collectionName);
	void setVisitedUrlCollectionName(const std::string &collectionName);
	bool setNumberOfThreads(int numberOfThreads);
	std::string fetchPage(CURL *curl, const std::string &url, bool useProxy = true);

private:
	void addSeedUrls(std::queue<std::string> &seedUrls);
	void parsePage(const std::string &htmlContent, const std::string &url, std::vector<bson_t *> &crawled_pages);
	static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
	static std::string getRandomUserAgent();
	static std::string getRandomProxy();
	void fetchProxies(CURL *curl, const std::string &url);

	void fetchRobotsTxtContent(CURL *curl, const std::string &url);
	bool isURLVisited(const std::string &absoluteURL);
	void markURLAsVisited(const std::string &absoluteURL);
	void saveVisitedUrls();
	void retrieveVisitedUrls();

	DataBase *&m_db;
	const HTMLParser &m_parser;
	std::deque<std::string> m_frontier;
	int m_crawled_pages_number;
	std::unordered_set<size_t> m_visited_urls;
	std::string m_database_name;
	std::string m_collection_name;
	std::string m_visited_urls_collection_name;
	const bool m_use_proxy;
	static std::vector<std::string> m_proxies_list;
	std::mutex m_visited_urls_mutex;
	std::mutex m_frontier_mutex;
	std::mutex m_crawled_pages_mutex;
	std::mutex m_stop_mutex;
	std::condition_variable m_stop_condition;
	int m_number_of_threads;
	int m_number_of_pages_to_save;
	int m_frontier_limit;
};
#endif
