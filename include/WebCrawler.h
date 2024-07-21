#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <string>
#include <queue>
#include <curl/curl.h>
#include <iostream>
#include <unordered_set>
#include "HTMLParser.h"

class WebCrawler {
public:
	WebCrawler(const std::string& seed_url, int max_pages,const DataBase*& database,HTMLParser& parser);
	~WebCrawler();
	void run();

private: 
	void parsePage(const std::string& htmlContent, const std::string& url);
	std::string fetchPage(const std::string& url);
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
	static bool isAbsoluteURL(const std::string& url);
	static bool shouldIgnoreURL(const std::string& url);
	static std::string convertToAbsoluteURL(const std::string& url, const std::string& baseUrl);

	const DataBase*& m_db;
	HTMLParser& m_parser;
	int m_max_pages_to_crawl;
	std::queue<std::string> m_frontier;
	std::unordered_set<std::string> m_crawled_pages;
	std::unordered_set<size_t> m_visitedUrls;
	

};
#endif
