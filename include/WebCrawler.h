#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <string>
#include <queue>
#include <curl/curl.h>
#include <iostream>
#include <unordered_set>

class WebCrawler {
public:
	WebCrawler(const std::string& seed_url, int max_pages);
	~WebCrawler();
	void run();

private: 
	void parsePage(const std::string& url);
	std::string fetchPage(const std::string& url);
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

	int m_max_pages_to_crawl;
	std::queue<std::string> m_frontier;
	std::unordered_set<std::string> m_crawled_pages;
	

};
#endif
