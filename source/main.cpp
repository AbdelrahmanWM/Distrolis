#include <iostream>
#include "SearchEngine.h"
#include <iostream>
#include "SearchEngineServer.h"
#include <functional>
#include <chrono>


static const std::string DATABASE = "SearchEngine";
static const std::string DOCUMENTS_COLLECTION = "pages";
static const std::string VISITED_URLS_COLLECTION = "VisitedUrls";
static const std::string INVERTED_INDEX_COLLECTION = "Index";
static const std::string METADATA_COLLECTION="Metadata";
static const int NUMBER_OF_PAGES = 300;
static const int NUMBER_OF_THREADS = 5;
static const bool USE_PROXY = false;
static const double BM25K1 = 1.5;
static const double BM25B = 0.75;
static const double PHRASE_BOOST = 1.35;
static const double EXACT_MATCH_WEIGHT = 0.6;

static std::queue<std::string>seed_urls({
	// "http://localhost:3000/doc1",
	// "http://localhost:3000/doc2",
	// "http://localhost:3000/doc3",
	// "http://localhost:3000/doc4",
	// "http://localhost:3000/doc5"
	"https://www.bbc.com",
    "https://www.cnn.com/",
    "https://techcrunch.com/",
    "https://www.wired.com/",
    "https://www.nytimes.com/",
    "https://arstechnica.com/",
    "https://www.coursera.org/",
    "https://news.ycombinator.com/",
    "https://www.medium.com/",
	    "https://www.khanacademy.org/"

});

int main(int argc, char*argv[])
{

	if (argc < 3) {
		std::cerr << "Usage: " << argv[0] << " <MongoDB connection string>"
			<< std::endl;
		return EXIT_FAILURE;
	}
	
	const std::string connectionString = argv[1];
	const std::string proxyAPIUrl = argv[2];
	BM25Ranker::setRankerParameters(BM25K1,BM25B,PHRASE_BOOST,EXACT_MATCH_WEIGHT);
	DataBase* db = DataBase::getInstance(connectionString);
	HTMLParser htmlParser{};
	WebCrawler webCrawler{db,htmlParser,DATABASE,DOCUMENTS_COLLECTION,VISITED_URLS_COLLECTION,USE_PROXY,proxyAPIUrl};
	InvertedIndex invertedIndex{ db,DATABASE,INVERTED_INDEX_COLLECTION, DOCUMENTS_COLLECTION, METADATA_COLLECTION};


	// auto start = std::chrono::high_resolution_clock::now();
	// webCrawler.run(NUMBER_OF_PAGES,seed_urls);
	// auto end = std::chrono::high_resolution_clock::now();
	// auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
	// std::cout<<"Time taken: "<<duration.count()<<" seconds\n";
	// webCrawler.clearCrawledDocuments();

	BM25Ranker bm25Ranker{DATABASE,DOCUMENTS_COLLECTION,invertedIndex};
    SearchEngine engine{&webCrawler,&invertedIndex,&bm25Ranker};
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
}
