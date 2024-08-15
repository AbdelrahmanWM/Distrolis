#include "WebCrawler.h"

std::vector<std::string> WebCrawler::proxiesList;

WebCrawler::WebCrawler(std::queue<std::string> &seed_urls, int max_pages, const DataBase *&database, const HTMLParser &parser, URLParser &urlParser, const std::string &database_name, const std::string &collection_name, const bool useProxy, const std::string &proxyAPIUrl)
	: m_db(database), m_parser(parser), m_urlParser(urlParser), m_max_pages_to_crawl(max_pages), m_frontier(), m_crawled_pages{}, m_visitedUrls{}, m_database_name{database_name}, m_collection_name{collection_name}, m_useProxy(useProxy)
{

	if (useProxy)
		fetchProxies(proxyAPIUrl);
	while (!seed_urls.empty())
	{
		m_frontier.push(m_urlParser.normalizeURL(seed_urls.front()));
		seed_urls.pop();
	}
}

WebCrawler::~WebCrawler()
{
}

void WebCrawler::run(bool clear)
{
	if (clear)
		m_db->clearCollection(m_database_name, m_collection_name); // clear previous crawling history

	while (!m_frontier.empty() && m_crawled_pages.size() < static_cast<size_t>(m_max_pages_to_crawl))
	{
		const std::string &url = m_frontier.front();
		m_urlParser.setBaseURL(url);
		if (isURLVisited(url))
		{ // if already visited
			m_frontier.pop();
			continue;
		}
		if (m_urlParser.isDomainURL())
		{
			fetchRobotsTxtContent();
		}

		std::string htmlContent = fetchPage(url, m_useProxy);
		parsePage(htmlContent, url);
		m_frontier.pop();
		std::cout << "m_frontier: " << m_frontier.size() << '\n';
		std::cout << "Pages fetched: " << m_crawled_pages.size() << '\n';
	}
	m_db->insertManyDocuments(m_crawled_pages, m_database_name, m_collection_name);
}
void WebCrawler::parsePage(const std::string &htmlContent, const std::string &url)
{
	std::vector<std::string> links;
	try
	{
		// m_parser.extractAndStorePageDetails(htmlContent, url, m_db,m_database_name,m_collection_name);
		links = m_parser.extractLinksFromHTML(htmlContent);
	}
	catch (std::runtime_error &ex)
	{
		std::cout << "Error: " << ex.what() << '\n';
	}
	std::string absoluteUrl{};

	for (const auto &link : links)
	{

		absoluteUrl = m_urlParser.convertToAbsoluteURL(link);
		if (isURLVisited(absoluteUrl))
		{
			continue;
		}
		std::cout << "Link: " << absoluteUrl << '\n';
		m_frontier.push(absoluteUrl);
	}
	markURLAsVisited(url);
	m_crawled_pages.push_back(m_parser.getPageDocument(htmlContent, url));
}
std::string WebCrawler::fetchPage(const std::string &url, bool useProxy)
{
	std::cout << "URL: " << url << '\n';
	// Checking for url existence for duplication.
	// Fetching the url
	CURL *curl;
	CURLcode res;
	std::string readBuffer;
	curl = curl_easy_init();
	if (!curl)
	{
		throw std::runtime_error("Failed to initialize libcurl for WebCrawler instance.\n");
	}
	// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); Debug mode!
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, getRandomUserAgent().c_str());
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

		if (useProxy)
		{
			bool proxySuccess = false;
			const int maxRetries = 5;
			for (int i = 0; i < maxRetries; ++i)
			{
				std::string proxy = getRandomProxy();
				curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
				res = curl_easy_perform(curl);
				if (res == CURLE_OK)
				{
					proxySuccess = true;
					break;
				}
				else
				{
					std::cerr << "curl_easy_perform() failed with proxy: " << proxy << " - " << curl_easy_strerror(res) << std::endl;
				}
			}
			if (!proxySuccess)
			{
				std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
			}
		}
	}
	curl_easy_cleanup(curl);
	return readBuffer;
}
size_t WebCrawler::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string *)userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}

void WebCrawler::fetchRobotsTxtContent()
{
	std::string content = fetchPage(m_urlParser.getRobotsTxtURL(), m_useProxy);
	std::vector<std::string> links{m_parser.extractRobotsTxtLinks(content)};
	std::string absoluteURL{};
	for (const auto &link : links)
	{
		absoluteURL = m_urlParser.convertToAbsoluteURL(link);
		markURLAsVisited(absoluteURL);
	}
}

std::string WebCrawler::getRandomUserAgent()
{
	static std::vector<std::string> userAgents = {
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:115.0) Gecko/20100101 Firefox/115.0",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36 Edg/114.0.1823.43"};
	return userAgents[rand() % userAgents.size()];
}

bool WebCrawler::isURLVisited(const std::string &absoluteURL)
{
	std::hash<std::string> hasher{};
	return m_visitedUrls.find(hasher(absoluteURL)) != m_visitedUrls.end();
}

void WebCrawler::markURLAsVisited(const std::string &absoluteURL)
{
	std::hash<std::string> hasher{};
	m_visitedUrls.insert(hasher(absoluteURL));
}

std::string WebCrawler::getRandomProxy()
{
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> uni(0, proxiesList.size() - 1);
	return proxiesList[uni(rng)];
}

void WebCrawler::fetchProxies(const std::string &url)
{
	try
	{
		proxiesList.clear();
		std::string proxies = fetchPage(url, false);
		std::istringstream stream(proxies);
		std::string proxy;
		while (stream >> proxy)
		{
			// std::cout << proxy << "\n";
			proxiesList.push_back("http://" + proxy);
		}
		if (proxiesList.empty())
		{
			std::cerr << "No proxies where fetched\n";
		}
	}
	catch (std::exception &e)
	{
		std::cerr << "Failed to fetch proxies: " << e.what() << '\n';
	}
}
