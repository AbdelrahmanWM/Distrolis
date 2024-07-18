#include <iostream>
#include "WebCrawler.h"
#include "DataBase.h"
#include "HTMLParser.h"

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
	WebCrawler webCrawler{ static_cast<std::string>("https://www.bbc.com/news") ,10,db,htmlParser};
	webCrawler.run();
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