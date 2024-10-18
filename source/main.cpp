#include <iostream>
#include "SearchEngine.h"
#include <iostream>
#include "SearchEngineServer.h"
#include <functional>
#include <chrono>
#include "SeedURLS.h"
// #include "ThreadPool.h"

static const std::string DATABASE = "SearchEngine";
static const std::string DOCUMENTS_COLLECTION = "pages";
static const std::string VISITED_URLS_COLLECTION = "VisitedUrls";
static const std::string INVERTED_INDEX_COLLECTION = "Index";
static const std::string METADATA_COLLECTION = "Metadata";
static const int NUMBER_OF_PAGES = 50000;
static const int NUMBER_OF_THREADS = 10;
static const bool USE_PROXY = false;
static const double BM25K1 = 1.5;
static const double BM25B = 0.75;
static const double PHRASE_BOOST = 1.35;
static const double EXACT_MATCH_WEIGHT = 0.6;
static std::queue<std::string>seed_url /*{SeedURLS::readSeedUrls("../seedUrls.txt")};*/
({
// 	// "http://localhost:3000/doc1",
// 	// "http://localhost:3000/doc2",
// 	// "http://localhost:3000/doc3",
// 	// "http://localhost:3000/doc4",
// 	// "http://localhost:3000/doc5"
// 	"https://www.bbc.com",
//     "https://www.cnn.com/",
//     "https://techcrunch.com/",
//     "https://www.wired.com/",
//     "https://www.nytimes.com/",
//     "https://arstechnica.com/",
//     "https://news.ycombinator.com/",
//     "https://www.medium.com/",
// 	    "https://www.khanacademy.org/",
// 		    "https://arstechnica.com",
//     "https://www.theverge.com",
//     "https://www.cnet.com",
//     "https://www.bloomberg.com",
//     "https://www.zdnet.com",
//     "https://www.engadget.com",
//     "https://www.polygon.com",
//     "https://www.scientificamerican.com"
//     // "https://www.coursera.org",
    "https://www.edx.org",
    "https://www.udemy.com",
    "https://www.academia.edu",
    "https://www.open.edu",
    "https://www.scholarly.org",
    "https://www.ted.com",
//     // "https://www.wolframalpha.com",
//     // "https://www.codecademy.com"

});
int main(int argc, char *argv[])
{

	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <MongoDB connection string>"
				  << std::endl;
		return EXIT_FAILURE;
	}

	const std::string connectionString = argv[1];
	const std::string proxyAPIUrl = argv[2];
	// static std::queue<std::string> seed_urls{SEEDURLS::readSeedUrls("../seedUrls.txt")};
	BM25Ranker::setRankerParameters(BM25K1, BM25B, PHRASE_BOOST, EXACT_MATCH_WEIGHT);
	DataBase db{connectionString};
	DataBase *dbPtr = &db;
	HTMLParser htmlParser{};
	WebCrawler webCrawler{dbPtr, htmlParser, DATABASE, DOCUMENTS_COLLECTION, VISITED_URLS_COLLECTION, USE_PROXY, proxyAPIUrl};
	// ThreadPool threadPool{NUMBER_OF_THREADS};

	InvertedIndex invertedIndex{dbPtr, DATABASE, INVERTED_INDEX_COLLECTION, DOCUMENTS_COLLECTION, METADATA_COLLECTION};

	// webCrawler.clearCrawledDocuments();
	// webCrawler.setNumberOfThreads(10);
	// auto start = std::chrono::high_resolution_clock::now();
	// webCrawler.run(NUMBER_OF_PAGES, seed_urls);
	// auto end = std::chrono::high_resolution_clock::now();
	// auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
	// std::cout << "Time taken: " << duration.count() << " seconds\n";

	BM25Ranker bm25Ranker{DATABASE, DOCUMENTS_COLLECTION, &invertedIndex};
	SearchEngine engine{&webCrawler, &invertedIndex, &bm25Ranker};

	// engine.setNumberOfThreads(6);
	// webCrawler.clearCrawledDocuments();
	// engine.crawl(100,seed_urls);
	/**********************************
	// engine.crawlAndIndexDocuments(seed_urls,10,true);
	// std::vector<std::pair<std::string,double>> documents = engine.search("\"\"Israel\"\"");
	// webCrawler.run(seed_urls,NUMBER_OF_PAGES,true);
	// invertedIndex.run(true);


	// std::vector<std::pair<std::string,double>> documents = bm25Ranker.run("(\"BBC news\" or \"CNN\") not \"England\"");
	// for(const auto&pair:documents){
	// 	std::cout<<"Document: "<<pair.first<<" -> "<<pair.second<<'\n';
	// }
	**********************************/

	SearchEngineServer server{engine};
	server.start();
	db.destroyConnection();
}
