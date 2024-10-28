#include "SearchEngine.h"



SearchEngine::SearchEngine(WebCrawler* webCrawler,InvertedIndex* invertedIndex, BM25Ranker* ranker )
:m_webCrawler(std::move(webCrawler)),m_invertedIndex(std::move(invertedIndex)),m_ranker(std::move(ranker))
{
}

SearchEngine::SearchResultsDocument SearchEngine::search(const std::string search_query)
{
    return m_ranker->run(search_query);
}

void SearchEngine::crawlAndIndexDocuments(std::queue<std::string> &seedUrls, int maximumNumberOfPages, bool clearHistory)
{
    crawl(maximumNumberOfPages,seedUrls);
    indexDocuments(clearHistory);
}

void SearchEngine::crawl(int maximumNumberOfPages,std::queue<std::string>& seedUrls)
{

    return m_webCrawler->run(maximumNumberOfPages,seedUrls); 
}

void SearchEngine::terminateCrawl(bool clearDocumentsHistory)
{
    m_webCrawler->terminate(clearDocumentsHistory);
}

void SearchEngine::indexDocuments(bool clearExistingInvertedIndexAndMetadata)
{
    return m_invertedIndex->run(clearExistingInvertedIndexAndMetadata);
}

void SearchEngine::setDatabaseAndCollectionsNames(const std::string &databaseName, const std::string &documentsCollectionName, const std::string &visitedUrlsCollectionName, const std::string &invertedIndexCollectionName, const std::string &metadataCollectionName)
{
    setDatabaseName(databaseName);
    setDocumentsCollectionName(documentsCollectionName);
    setVisitedUrlsCollectionName(visitedUrlsCollectionName);
    setInvertedIndexCollectionName(invertedIndexCollectionName);
    setMetadataCollectionName(metadataCollectionName);
}

void SearchEngine::setDatabaseName(const std::string &newDataBaseName)
{
    m_webCrawler->setDatabaseName(newDataBaseName);
    m_invertedIndex->setDatabaseName(newDataBaseName);
    m_ranker->setDatabaseName(newDataBaseName);
}

void SearchEngine::setDocumentsCollectionName(const std::string& collectionName)
{
    m_webCrawler->setDocumentsCollectionName(collectionName);
    m_invertedIndex->setDocumentsCollectionName(collectionName);
    m_ranker->setDocumentsCollectionName(collectionName);
}

void SearchEngine::setVisitedUrlsCollectionName(const std::string& collectionName)
{
    m_webCrawler->setVisitedUrlCollectionName(collectionName);
}

void SearchEngine::setInvertedIndexCollectionName(const std::string& collectionName)
{
    m_invertedIndex->setInvertedIndexCollectionName(collectionName);
}

void SearchEngine::setMetadataCollectionName(const std::string& collectionName)
{
    m_invertedIndex->setMetadataCollectionName(collectionName);
}

void SearchEngine::setRankerParameters(double BM25_K1, double BM25_B, double PHRASE_BOOST_VALUE, double EXACT_MATCH_WEIGHT)
{
    BM25Ranker::setRankerParameters(BM25_K1,BM25_B,PHRASE_BOOST_VALUE,EXACT_MATCH_WEIGHT);
}

bool SearchEngine::setNumberOfThreads(int numberOfThreads)
{
    return m_webCrawler->setNumberOfThreads(numberOfThreads)&&m_invertedIndex->setNumberOfThreads(numberOfThreads);
}

void SearchEngine::clearCrawlHistory()
{
    m_webCrawler->clearCrawledDocuments();
}
