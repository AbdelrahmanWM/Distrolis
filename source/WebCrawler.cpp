#include "WebCrawler.h"
#include "ThreadPool.h"

std::atomic<bool> stopRequested = false;
std::atomic<bool> clearRecord = false;
int maximumNumberOfPages;
// Just temporarily
std::vector<std::string> WebCrawler::m_proxies_list;

WebCrawler::WebCrawler(DataBase *&database, const HTMLParser &parser, const std::string &database_name, const std::string &collection_name, const std::string &visitedUrls_collection_name, const bool useProxy, const std::string &proxyAPIUrl, int numberOfThreads)
	: m_db(database), m_parser(parser), m_frontier(), m_visited_urls{}, m_database_name{database_name}, m_collection_name{collection_name}, m_visited_urls_collection_name(visitedUrls_collection_name), m_use_proxy(useProxy), m_number_of_threads(numberOfThreads)
{
	CURL *curl;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (useProxy)
	{
		fetchProxies(curl, proxyAPIUrl);
	}

	curl_easy_cleanup(curl);
}

WebCrawler::~WebCrawler()
{
	curl_global_cleanup();
}
void WebCrawler::run(int maximumNumberOfPagesToCrawl, std::queue<std::string> &seedUrls)
{
	m_frontier_limit = maximumNumberOfPagesToCrawl;
	maximumNumberOfPages = maximumNumberOfPagesToCrawl;
	m_number_of_pages_to_save = std::min(1000, (maximumNumberOfPages / m_number_of_threads));
	m_crawled_pages_number = 0;
	clearRecord = false;
	auto start = std::chrono::high_resolution_clock::now();
	addSeedUrls(seedUrls);
	retrieveVisitedUrls(); // when the clear documents is called??
	m_db->clearCollection(m_database_name, m_visited_urls_collection_name);
	std::cout << "Number of threads: " << m_number_of_threads << "\n";
	{
		ThreadPool threadPool{static_cast<size_t>(m_number_of_threads)};
		for (int i = 0; i < m_number_of_threads; i++)
		{
			threadPool.enqueue([this, maximumNumberOfPagesToCrawl]
							   { this->crawl(maximumNumberOfPagesToCrawl); });
		}
	}
	std::cout << "visited urls: " << m_visited_urls.size() << '\n';
	saveVisitedUrls();
	std::cout << "Crawled pages number: " << m_crawled_pages_number << "\n";

	std::deque<std::string> empty;
	std::swap(m_frontier, empty);
	m_visited_urls.clear();
	stopRequested = false;
	if (clearRecord)
	{ // in case a termination happens
		clearCrawledDocuments();
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
	std::cout << "Time taken: " << duration.count() << " seconds\n";
}
void WebCrawler::terminate(bool clearHistory)
{
	stopRequested = true;
	clearRecord = clearHistory;
}
void WebCrawler::crawl(int maximumNumberOfPagesToCrawl)
{
	// std::unique_lock<std::mutex> stopLock(m_stopMutex);
	CURL *curl;
	DataBase db{m_db->getConnectionString()}; // temporarily
	curl = curl_easy_init();
	std::vector<bson_t *> crawled_pages{};
	crawled_pages.reserve(m_number_of_pages_to_save + 5);

	while (!stopRequested)
	{
		// std::cout << std::this_thread::get_id() << '\n';
		std::string url;

		{ // Dont believe a lock is necessary here
			std::unique_lock<std::mutex> lock(m_frontier_mutex);
			if (m_frontier.empty())
			{
				// m_stop_condition.wait_for(lock, std::chrono::seconds(1), [this]
				// 						  { return !m_frontier.empty(); });
				// if (m_frontier.empty())
				// {
				stopRequested = true;
				break;
			}
			url = m_frontier.front();
			m_frontier.pop_front();
			// }
		}
		if (isURLVisited(url))
		{ // if already visited
			continue;
		}
		if (URLParser::isDomainURL(url))
		{
			fetchRobotsTxtContent(curl, url);
		}
		std::string htmlContent = fetchPage(curl, url, m_use_proxy);
		if (htmlContent == "")
			continue;
		parsePage(htmlContent, url, crawled_pages);
		htmlContent.clear();
		if (crawled_pages.size() >= static_cast<size_t>(m_number_of_pages_to_save))
		{
			db.insertManyDocuments(crawled_pages, m_database_name, m_collection_name);
			for (bson_t *document : crawled_pages)
			{
				bson_destroy(document);
			}
			crawled_pages.clear();
			crawled_pages.reserve(m_number_of_pages_to_save + 5);
		}
		if (stopRequested)
		{
			break; // temporarily
		}
	}
	if (crawled_pages.size() > 0)
	{
		db.insertManyDocuments(crawled_pages, m_database_name, m_collection_name);
		for (bson_t *document : crawled_pages)
		{
			bson_destroy(document);
		}
		crawled_pages.clear();
	}
	curl_easy_cleanup(curl);
}

void WebCrawler::clearCrawledDocuments()
{
	m_db->clearCollection(m_database_name, m_collection_name); // clear previous crawling history
	m_db->clearCollection(m_database_name, m_visited_urls_collection_name);
}
void WebCrawler::addSeedUrls(std::queue<std::string> &seedUrls)
{
	while (!seedUrls.empty())
	{
		m_frontier.push_back(URLParser::normalizeURL(seedUrls.front()));
		seedUrls.pop();
	}
}
void WebCrawler::parsePage(const std::string &htmlContent, const std::string &url, std::vector<bson_t *> &crawled_pages)
{
	if (stopRequested)
		return; // temporarily
	if (htmlContent == "")
	{
		return;
	}
	std::vector<std::string> links;
	try
	{
		// m_parser.extractAndStorePageDetails(htmlContent, url, m_db,m_database_name,m_collection_name);
		links = std::move(m_parser.extractLinksFromHTML(htmlContent));
	}
	catch (std::runtime_error &ex)
	{
		std::cerr << "Error: " << ex.what() << '\n';
	}
	std::string absoluteUrl{};
	std::vector<std::string> urlsToAdd{};
	for (const auto &link : links)
	{

		absoluteUrl = std::move(URLParser::convertToAbsoluteURL(link, url));
		if (isURLVisited(absoluteUrl) || !URLParser::isAbsoluteURL(absoluteUrl))
		{
			continue;
		}
		{
			std::unique_lock<std::mutex> lock(m_frontier_mutex);

			if (m_frontier.size() <= static_cast<size_t>(m_frontier_limit))
			{
				m_frontier.push_back(absoluteUrl);
			}
		}
		// urlsToAdd.push_back(absoluteUrl);
	}

	links.clear();
	if (!stopRequested)
		markURLAsVisited(url);
	crawled_pages.push_back(m_parser.getPageDocument(htmlContent, url));
	// std::cout << "crawled pages: " << crawled_pages.size() << '\n';
	{
		std::unique_lock<std::mutex> lock(m_crawled_pages_mutex);
		m_crawled_pages_number += 1;
		if (m_crawled_pages_number >= maximumNumberOfPages)
		{
			stopRequested = true;
		}
	}
}
std::string WebCrawler::fetchPage(CURL *curl, const std::string &url, bool useProxy)
{
	if (stopRequested || isURLVisited(url)) // temporarily
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
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, getRandomUserAgent().c_str());
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 7L);
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
	return readBuffer;
}
size_t WebCrawler::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string *)userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}

void WebCrawler::fetchRobotsTxtContent(CURL *curl, const std::string &url)
{
	std::string content = fetchPage(curl, URLParser::getRobotsTxtURL(url), m_use_proxy);
	std::vector<std::string> links{m_parser.extractRobotsTxtLinks(content)};
	content.clear();
	std::string absoluteURL{};
	for (const auto &link : links)
	{
		absoluteURL = std::move(URLParser::convertToAbsoluteURL(link, url));
		markURLAsVisited(absoluteURL);
	}
	links.clear();
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
	std::lock_guard<std::mutex> lock(m_visited_urls_mutex);
	return m_visited_urls.find(hashedUrl) != m_visited_urls.end();
}

void WebCrawler::markURLAsVisited(const std::string &absoluteURL)
{
	std::hash<std::string> hasher{};
	std::lock_guard<std::mutex> lock(m_visited_urls_mutex);
	m_visited_urls.insert(hasher(absoluteURL));
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
		for (size_t url : m_visited_urls)
		{
			int64_t value = static_cast<int64_t>(url);
			BSON_APPEND_INT64(array, std::to_string(index).c_str(), value);
			index++;
		}

		BSON_APPEND_ARRAY(document, "visitedUrls", array);

		m_db->insertDocument(document, m_database_name, m_visited_urls_collection_name);

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
	bson_t *document = m_db->getDocument(m_database_name, m_visited_urls_collection_name);
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
					m_visited_urls.insert(url);
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
	bson_destroy(document);
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
	m_visited_urls_collection_name = collectionName;
}

bool WebCrawler::setNumberOfThreads(int numberOfThreads)
{
	if (numberOfThreads > 0 && numberOfThreads < static_cast<int>(std::thread::hardware_concurrency()))
	{
		m_number_of_threads = numberOfThreads;
		return true;
	}
	return false;
}

std::string WebCrawler::getRandomProxy()
{
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> uni(0, m_proxies_list.size() - 1);
	return m_proxies_list[uni(rng)];
}

void WebCrawler::fetchProxies(CURL *curl, const std::string &url)
{
	try
	{
		m_proxies_list.clear();
		std::string proxies = fetchPage(curl, url, false);
		std::istringstream stream(proxies);
		std::string proxy;
		while (stream >> proxy)
		{
			// std::cout << proxy << "\n";
			m_proxies_list.push_back("http://" + proxy);
		}
		if (m_proxies_list.empty())
		{
			std::cerr << "No proxies where fetched\n";
		}
	}
	catch (std::exception &e)
	{
		std::cerr << "Failed to fetch proxies: " << e.what() << '\n';
	}
}
