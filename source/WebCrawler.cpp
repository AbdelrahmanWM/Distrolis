#include "WebCrawler.h"


WebCrawler::WebCrawler(const std::string& seed_url, int max_pages_to_crawl)
	: m_max_pages_to_crawl(max_pages_to_crawl)
{

	m_frontier.push(seed_url);
	std::cout<<fetchPage(seed_url)<<'\n';/// TEST
}

WebCrawler::~WebCrawler() {

}

void WebCrawler::run() {
	while (!m_frontier.empty() && m_crawled_pages.size() < m_max_pages_to_crawl) {
		
	}
}
void WebCrawler::parsePage(const std::string& url) {

}
std::string WebCrawler::fetchPage(const std::string& url) {
	std::cout << "URL: " << url << '\n';
	CURL* curl;
	CURLcode res;
	std::string readBuffer;
	curl = curl_easy_init();
	if (!curl) {
		throw std::runtime_error("Failed to initialize libcurl for WebCrawler instance.\n");
	}
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
	}
	curl_easy_cleanup(curl);
	return readBuffer;
}
size_t WebCrawler::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}