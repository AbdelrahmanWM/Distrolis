#include "WebCrawler.h"
bool stopRequested = false;

std::vector<std::string> WebCrawler::m_proxiesList;

WebCrawler::WebCrawler(DataBase *&database, const HTMLParser &parser, const std::string &database_name, const std::string &collection_name, const std::string &visitedUrls_collection_name, const bool useProxy, const std::string &proxyAPIUrl, int numberOfThreads)
	: m_db(database), m_parser(parser), m_frontier(), m_crawled_pages{}, m_visitedUrls{}, m_database_name{database_name}, m_collection_name{collection_name}, m_visitedUrls_collection_name(visitedUrls_collection_name), m_useProxy(useProxy),m_numberOfThreads(numberOfThreads)
{
	CURL *curl;
	curl = curl_easy_init();
	if (useProxy)
	{
		fetchProxies(curl, proxyAPIUrl);
	}
	curl_global_init(CURL_GLOBAL_ALL);
	curl_easy_cleanup(curl);
}

WebCrawler::~WebCrawler()
{
	curl_global_cleanup();
}
void WebCrawler::run(int maximumNumberOfPagesToCrawl, std::queue<std::string> &seedUrls)
{
	addSeedUrls(seedUrls);
	retrieveVisitedUrls(); // when the clear documents is called??
	std::vector<std::thread> workers{};
	for (int i = 0; i < m_numberOfThreads; i++)
	{
		workers.push_back(std::thread([this, maximumNumberOfPagesToCrawl]
									  { this->crawl(maximumNumberOfPagesToCrawl); }));
	}
	for (int i = 0; i < m_numberOfThreads; i++)
	{
		workers[i].join();
	}
	m_db->clearCollection(m_database_name, m_visitedUrls_collection_name);
	if (m_crawled_pages.size() > 0)
	{
		m_db->insertManyDocuments(m_crawled_pages, m_database_name, m_collection_name);
	}
	std::cout << "visited urls: " << m_visitedUrls.size() << '\n';
	saveVisitedUrls();
	std::cout << "Crawled pages number: " << m_crawled_pages.size() << "\n";
}
void WebCrawler::crawl(int maximumNumberOfPagesToCrawl)
{
	std::unique_lock<std::mutex> stopLock(m_stopMutex);

	CURL *curl;
	curl = curl_easy_init();

	while (!stopRequested)
	{

		std::string url;
		{
			std::unique_lock<std::mutex> lock1(m_frontierMutex);
			{
				std::unique_lock<std::mutex> lock2(m_crawledPagesMutex);
				if (m_frontier.empty() || m_crawled_pages.size() >= static_cast<size_t>(maximumNumberOfPagesToCrawl))
				{
					stopRequested = true;
					m_stopCondition.notify_all();
					break;
				}
			}
			url = m_frontier.front();
			m_frontier.pop();
			if (isURLVisited(url))
			{ // if already visited
				continue;
			}
		}
		if (URLParser::isDomainURL(url))
		{
			fetchRobotsTxtContent(curl, url);
		}
		std::string htmlContent = fetchPage(curl, url, m_useProxy);
		if (htmlContent == "")
			continue;
		parsePage(htmlContent, url);
		{
			std::unique_lock<std::mutex> lock1(m_frontierMutex);
			std::unique_lock<std::mutex> lock2(m_crawledPagesMutex);
			// std::cout << "frontier: " << m_frontier.size() << '\n';
			// std::cout << "pages fetched: " << m_crawled_pages.size() << '\n';
		}
		if (stopRequested)
		{
			break;
		}
	}

	curl_easy_cleanup(curl);
}

// while (!m_frontier.empty() && m_crawled_pages.size() < static_cast<size_t>(maximumNumberOfPagesToCrawl))
// {
// 	const std::string &url = m_frontier.front();
// 	m_urlParser.setBaseURL(url);
// 	if (isURLVisited(url))
// 	{ // if already visited
// 		m_frontier.pop();
// 		continue;
// 	}
// if (m_urlParser.isDomainURL())
// {
// 	fetchRobotsTxtContent();
// }

// std::string htmlContent = fetchPage(url, m_useProxy);
// parsePage(htmlContent, url);
// m_frontier.pop();
// std::cout << "frontier: " << m_frontier.size() << '\n';
// std::cout << "pages fetched: " << m_crawled_pages.size() << '\n';
// }

// m_db->clearCollection(m_database_name, m_visitedUrls_collection_name);
// if (m_crawled_pages.size() > 0)
// {
// 	m_db->insertManyDocuments(m_crawled_pages, m_database_name, m_collection_name);
// }
// std::cout << "visited urls: " << m_visitedUrls.size() << '\n';
// saveVisitedUrls();

void WebCrawler::clearCrawledDocuments()
{
	m_db->clearCollection(m_database_name, m_collection_name); // clear previous crawling history
	m_db->clearCollection(m_database_name, m_visitedUrls_collection_name);
}
void WebCrawler::addSeedUrls(std::queue<std::string> &seedUrls)
{
	while (!seedUrls.empty())
	{
		m_frontier.push(URLParser::normalizeURL(seedUrls.front()));
		seedUrls.pop();
	}
}
void WebCrawler::parsePage(const std::string &htmlContent, const std::string &url)
{
	if (htmlContent == "")
	{
		return;
	}
	std::vector<std::string> links;
	try
	{
		// m_parser.extractAndStorePageDetails(htmlContent, url, m_db,m_database_name,m_collection_name);
		links = m_parser.extractLinksFromHTML(htmlContent);
	}
	catch (std::runtime_error &ex)
	{
		std::cerr << "Error: " << ex.what() << '\n';
	}
	std::string absoluteUrl{};

	for (const auto &link : links)
	{

		absoluteUrl = URLParser::convertToAbsoluteURL(link, url);
		if (isURLVisited(absoluteUrl))
		{
			continue;
		}
		// std::cout << "Link: " << absoluteUrl << '\n';
		m_frontier.push(absoluteUrl);
	}
	markURLAsVisited(url);
	{
		std::lock_guard<std::mutex> lock(m_crawledPagesMutex);

		m_crawled_pages.push_back(m_parser.getPageDocument(htmlContent, url));
	}
}
std::string WebCrawler::fetchPage(CURL *curl, const std::string &url, bool useProxy)
{
	// std::cout << "URL: " << url << '\n';
	// Checking for url existence for duplication.
	// Fetching the url
	if (isURLVisited(url))
	{
		return "";
	}

	CURLcode res;
	std::string readBuffer;
	if (!curl)
	{
		throw std::runtime_error("Failed to initialize libcurl for WebCrawler instance.\n");
	}
	// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); Debug mode!
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, getRandomUserAgent().c_str());
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		std::cout << url << '\n';

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

	return readBuffer;
}
size_t WebCrawler::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string *)userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}

void WebCrawler::fetchRobotsTxtContent(CURL *curl, const std::string &url)
{
	std::string content = fetchPage(curl, URLParser::getRobotsTxtURL(url), m_useProxy);
	std::vector<std::string> links{m_parser.extractRobotsTxtLinks(content)};
	std::string absoluteURL{};
	for (const auto &link : links)
	{
		absoluteURL = URLParser::convertToAbsoluteURL(link, url);
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
	size_t hashedUrl = hasher(absoluteURL);
	std::lock_guard<std::mutex> lock(m_visitedUrlsMutex);
	return m_visitedUrls.find(hashedUrl) != m_visitedUrls.end();
}

void WebCrawler::markURLAsVisited(const std::string &absoluteURL)
{
	std::hash<std::string> hasher{};
	std::lock_guard<std::mutex> lock(m_visitedUrlsMutex);
	m_visitedUrls.insert(hasher(absoluteURL));
}

void WebCrawler::saveVisitedUrls()
{
	bson_t *document = bson_new();
	if (!document)
	{
		std::cerr << "Failed to create BSON document." << std::endl;
		return;
	}

	try
	{

		bson_t *array = bson_new();
		if (!array)
		{
			std::cerr << "Failed to create BSON array." << std::endl;
			bson_destroy(document);
			return;
		}

		int index = 0;
		for (size_t url : m_visitedUrls)
		{
			int64_t value = static_cast<int64_t>(url);
			BSON_APPEND_INT64(array, std::to_string(index).c_str(), value);
			index++;
		}

		BSON_APPEND_ARRAY(document, "visitedUrls", array);

		m_db->insertDocument(document, m_database_name, m_visitedUrls_collection_name);

		bson_destroy(array);
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}

	// Destroy the BSON document
	bson_destroy(document);
}

void WebCrawler::retrieveVisitedUrls()
{
	bson_t *document = m_db->getDocument(m_database_name, m_visitedUrls_collection_name);
	if (!document)
	{
		std::cerr << "Failed to retrieve BSON document." << std::endl;
		return;
	}

	bson_iter_t iter;
	if (!bson_iter_init(&iter, document))
	{
		std::cerr << "Failed to initialize BSON iterator." << std::endl;
		return;
	}

	while (bson_iter_next(&iter))
	{
		if (strcmp(bson_iter_key(&iter), "visitedUrls") == 0)
		{
			bson_iter_t array_iter;
			if (bson_iter_recurse(&iter, &array_iter))
			{

				while (bson_iter_next(&array_iter))
				{
					int64_t url = bson_iter_int64(&array_iter);
					m_visitedUrls.insert(url);
				}
			}
			// std::cout << "Visited URLs:" << std::endl;
			// for (auto url : m_visitedUrls)
			// {
			// 	std::cout << url << std::endl;
			// }
		}
		else
		{
			std::cerr << "Expected 'visitedUrls' to be an array." << std::endl;
		}
	}
}

void WebCrawler::setDatabaseName(const std::string &databaseName)
{
	m_database_name = databaseName;
}

void WebCrawler::setDocumentsCollectionName(const std::string &collectionName)
{
	m_collection_name = collectionName;
}

void WebCrawler::setVisitedUrlCollectionName(const std::string &collectionName)
{
	m_visitedUrls_collection_name = collectionName;
}

void WebCrawler::setNumberOfThreads(int numberOfThreads)
{
	m_numberOfThreads = numberOfThreads>0?numberOfThreads:m_numberOfThreads;
}

std::string WebCrawler::getRandomProxy()
{
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> uni(0, m_proxiesList.size() - 1);
	return m_proxiesList[uni(rng)];
}

void WebCrawler::fetchProxies(CURL *curl, const std::string &url)
{
	try
	{
		m_proxiesList.clear();
		std::string proxies = fetchPage(curl, url, false);
		std::istringstream stream(proxies);
		std::string proxy;
		while (stream >> proxy)
		{
			// std::cout << proxy << "\n";
			m_proxiesList.push_back("http://" + proxy);
		}
		if (m_proxiesList.empty())
		{
			std::cerr << "No proxies where fetched\n";
		}
	}
	catch (std::exception &e)
	{
		std::cerr << "Failed to fetch proxies: " << e.what() << '\n';
	}
}
