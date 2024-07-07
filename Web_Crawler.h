#ifndef WEB_CRAWLER_H
#define WEB_CRAWLER_H

#include <string>
#include <queue>

class WebCrawler {
public:
	WebCrawler(const std::string& seed_url, int max_pages);

	void run();

private: 
	void parsePage(std::string& url);

	int m_max_pages_to_crawl;
	std::queue<std::string> m_frontier;

};
#endif
