#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <string>
#include <queue>
#include <curl/curl.h>
#include <iostream>
#include <random>
#include <unordered_set>
#include "HTMLParser.h"
#include "URLParser.h"

class WebCrawler {
public:
	WebCrawler(std::queue<std::string>& seed_urls, int max_pages,const DataBase*& database,const HTMLParser& parser,URLParser& urlParser,const std::string& database_name, const std::string& collection_name,const bool useProxy,const std::string& proxyAPIUrl="");
	~WebCrawler();
	void run(bool clear = false);

private: 
	void parsePage(const std::string& htmlContent, const std::string& url);
	static std::string fetchPage(const std::string& url,bool useProxy=true);
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static std::string getRandomUserAgent();
	static std::string getRandomProxy();
	void fetchProxies(const std::string& url);

	void fetchRobotsTxtContent();
    bool isURLVisited(const std::string& absoluteURL);
	void markURLAsVisited(const std::string& absoluteURL);
	const DataBase*& m_db;
	const HTMLParser& m_parser;
	URLParser& m_urlParser;
	int m_max_pages_to_crawl;
	std::queue<std::string> m_frontier;
	std::vector<bson_t*> m_crawled_pages;
	std::unordered_set<size_t> m_visitedUrls;
	const std::string m_database_name;
	const std::string m_collection_name; 
	const bool m_useProxy;
	static std::vector<std::string> proxiesList;

	

};
#endif
