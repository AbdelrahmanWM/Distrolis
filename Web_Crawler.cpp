#include "Web_Crawler.h"

WebCrawler::WebCrawler(const std::string& seed_url, int max_pages_to_crawl)
	: m_max_pages_to_crawl(max_pages_to_crawl)
{
	m_frontier.push(seed_url);
}