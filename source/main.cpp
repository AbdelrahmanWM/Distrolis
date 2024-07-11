#include <iostream>
#include "WebCrawler.h";

int main()
{
	curl_global_init(CURL_GLOBAL_ALL);
	WebCrawler webCrawler{ static_cast<std::string>("https://github.com/AbdelrahmanWM/AbdelrahmanWM/blob/main/README.md") ,1};
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