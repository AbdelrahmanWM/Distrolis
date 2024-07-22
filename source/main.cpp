#include <iostream>
#include "WebCrawler.h"
#include "DataBase.h"
#include "HTMLParser.h"
#include "URLParser.h"
#include "InvertedIndex.h"

int main(int argc, char*argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <MongoDB connection string>"
			<< std::endl;
		return EXIT_FAILURE;
	}
	const std::string connectionString = argv[1];
	const DataBase* db = DataBase::getInstance(connectionString,"SearchEngine","pages");
	
	curl_global_init(CURL_GLOBAL_ALL);
	HTMLParser htmlParser{};
	std::string seed_url = "https://github.com";
	URLParser urlParser{seed_url};

	WebCrawler webCrawler{ seed_url ,10,db,htmlParser,urlParser};
	webCrawler.run(true);

	InvertedIndex invertedIndex{ db };
	invertedIndex.run();

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