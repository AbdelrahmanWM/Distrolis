#include <iostream>
#include "WebCrawler.h"
#include "DataBase.h"

int main(int argc, char*argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <MongoDB connection string>"
			<< std::endl;
		return EXIT_FAILURE;
	}
	const std::string connectionString = argv[1];
	DataBase* db = DataBase::getInstance(connectionString);
	bson_t* doc = BCON_NEW(
		"name", BCON_UTF8("John Due"),
		"age", BCON_INT32(30),
		"email", BCON_UTF8("john.due@example.com")
	);
	db->insertDocument("test", doc);
	curl_global_init(CURL_GLOBAL_ALL);

	WebCrawler webCrawler{ static_cast<std::string>("https://www.youtube.com/") ,2};
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