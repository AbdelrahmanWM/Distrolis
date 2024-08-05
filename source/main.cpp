#include <iostream>
#include "WebCrawler.h"
#include "DataBase.h"
#include "HTMLParser.h"
#include "URLParser.h"
#include <queue>
#include "InvertedIndex.h"

static const std::string DATABASE = "SearchEngine";
static const std::string DOCUMENTS_COLLECTION = "pages";
static const std::string INVERTED_INDEX_COLLECTION = "Index";
static const int NUMBER_OF_PAGES = 10;
static std::queue<std::string>seed_urls({
	 "https://www.bbc.com",
	 "https://www.bbc.com",
	 "https://www.bbc.com",
    "https://www.bbc.com/bitesize/search",
    "https://www.cnn.com/",
	"https://www.cnn.com/ads",
    "https://techcrunch.com/",
    "https://www.wired.com/",
	"https://www.wired.com/user",
    "https://www.nytimes.com/",
    "https://arstechnica.com/",
    "https://www.khanacademy.org/",
    "https://www.coursera.org/",
	"https://www.coursera.org/api",
    "https://news.ycombinator.com/",
    "https://www.medium.com/"
});
int main(int argc, char*argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <MongoDB connection string>"
			<< std::endl;
		return EXIT_FAILURE;
	}
	const std::string connectionString = argv[1];
	const DataBase* db = DataBase::getInstance(connectionString);
	
	curl_global_init(CURL_GLOBAL_ALL);
	HTMLParser htmlParser{};
	URLParser urlParser{seed_urls.front()};
	WebCrawler webCrawler{ seed_urls ,NUMBER_OF_PAGES,db,htmlParser,urlParser,DATABASE,DOCUMENTS_COLLECTION};
	webCrawler.run(true);

	InvertedIndex invertedIndex{ db,DATABASE,INVERTED_INDEX_COLLECTION, DOCUMENTS_COLLECTION};
	invertedIndex.run(true);

	curl_global_cleanup();
}
//CURL* curl;
//CURLcode res;
//std::string response;
//
//curl_global_init(CURL_GLOBAL_ALL);
//
//curl = curl_easy_init();
//if (curl) {
//    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3000/api/users");
//    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
//    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
//    res = curl_easy_perform(curl);
//
//    if (res != CURLE_OK) {
//        std::cerr << "Failed to perform HTTP request: " << curl_easy_strerror(res) << std::endl;
//    }
//    else {
//        std::cout << "Response:\n" << response << std::endl;
//    }
//    curl_easy_cleanup(curl);
//}
///// my code
//
//curl_global_cleanup();