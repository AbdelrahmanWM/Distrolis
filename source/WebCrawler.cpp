#include "WebCrawler.h"


WebCrawler::WebCrawler(const std::string& seed_url, int max_pages_to_crawl, const DataBase*& database, const HTMLParser& parser, URLParser& urlParser)
	: m_max_pages_to_crawl(max_pages_to_crawl), m_db(database), m_parser(parser), m_visitedUrls({}),m_urlPaser(urlParser)
{
	m_frontier.push(seed_url);
}

WebCrawler::~WebCrawler() {

}

void WebCrawler::run(bool clear) {
	if (clear)m_db->clearCollection();// clear previous crawling history
	while (!m_frontier.empty() && m_crawled_pages.size() < m_max_pages_to_crawl) {
		const std::string& url = m_frontier.front();
		std::string htmlContent = fetchPage(url);
		parsePage(htmlContent,url);
		m_frontier.pop();
		std::cout << "m_frontier: " << m_frontier.size() << '\n';
		std::cout << "Pages fetched: " << m_crawled_pages.size() << '\n';

	}
}
void WebCrawler::parsePage(const std::string& htmlContent,const std::string& url) {
	std::vector<std::string> links;
	try{
		m_parser.extractAndStorePageDetails(htmlContent, url, m_db);
		links = m_parser.extractLinksFromHTML(htmlContent);
	}
	catch (std::runtime_error& ex) {
		std::cout << "Error: " << ex.what() << '\n';
	}
	std::hash<std::string> hasher;
	std::string absoluteUrl{};
	
	m_urlPaser.setBaseURL(url);
	for (const auto& link : links) {
		
		absoluteUrl = m_urlPaser.convertToAbsoluteURL(link);
		size_t urlHash = hasher(absoluteUrl);
		if (m_visitedUrls.find(urlHash) != m_visitedUrls.end()) {
			continue;
		}
		std::cout << "Link: " << absoluteUrl << '\n';
		m_visitedUrls.insert(urlHash);
		m_frontier.push(absoluteUrl);
	}
	m_crawled_pages.insert(htmlContent);
	
}
std::string WebCrawler::fetchPage(const std::string& url) {
	std::cout << "URL: " << url << '\n';
	// Checking for url existence for duplication.

	// Fetching the url
	CURL* curl;
	CURLcode res;
	std::string readBuffer;
	curl = curl_easy_init();
	if (!curl) {
		throw std::runtime_error("Failed to initialize libcurl for WebCrawler instance.\n");
	}
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); Debug mode!
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


