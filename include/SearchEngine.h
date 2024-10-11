#include "WebCrawler.h"
#include "DataBase.h"
#include "HTMLParser.h"
#include "URLParser.h"
#include "InvertedIndex.h"
#include "BM25Ranker.h"
#include "WordProcessor.h"
#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H  

class SearchEngine{

public:
typedef  std::vector<std::pair<std::string,double>> SearchResultsDocument;

SearchEngine(WebCrawler* webCrawler,InvertedIndex* invertedIndex, BM25Ranker* ranker );
// ~SearchEngine();
SearchResultsDocument search(const std::string search_query);
void crawlAndIndexDocuments(std::queue<std::string>& seedUrls, int maximumNumberOfPages, bool clearHistory=false);
void crawl(int maximumNumberOfPages,std::queue<std::string>& seedUrls);
void terminateCrawl(bool clearDocumentsHistory=true);
void indexDocuments(bool clearExistingInvertedIndexAndMetadata=false);
void setDatabaseAndCollectionsNames(const std::string& databaseName, const std::string& documentsCollectionName, const std::string& visitedUrlsCollectionName, const std::string& invertedIndexCollectionName, const std::string& metadataCollectionName);
void setDatabaseName(const std::string& newDataBaseName);
void setDocumentsCollectionName(const std::string& collectionName);
void setVisitedUrlsCollectionName(const std::string& collectionName);
void setInvertedIndexCollectionName(const std::string& collectionName);
void setMetadataCollectionName(const std::string& collectionName);
void setRankerParameters(double BM25_K1, double BM25_B, double PHRASE_BOOST_VALUE, double EXACT_MATCH_WEIGHT);
bool setNumberOfThreads(int numberOfThreads);
void clearCrawlHistory();
private:

WebCrawler* m_webCrawler;
InvertedIndex* m_invertedIndex;
BM25Ranker* m_ranker;

};
#endif