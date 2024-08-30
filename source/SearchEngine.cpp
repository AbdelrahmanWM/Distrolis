#include "SearchEngine.h"



SearchEngine::SearchEngine(const WebCrawler & webCrawler, const InvertedIndex & invertedIndex, const BM25Ranker & ranker)
:m_webCrawler(webCrawler),m_invertedIndex(invertedIndex),m_ranker(ranker)
{
}

SearchEngine::SearchResultsDocument SearchEngine::search(const std::string search_query)
{
    return m_ranker.run(search_query);
}

void SearchEngine::crawl(std::queue<std::string>& seedUrls,int maximumNumberOfPages,bool clearPreviouslyCrawledPages)
{

    return m_webCrawler.run(seedUrls, maximumNumberOfPages,clearPreviouslyCrawledPages); 
}

void SearchEngine::indexDocuments(bool clearExistingInvertedIndexAndMetadata)
{
    return m_invertedIndex.run(clearExistingInvertedIndexAndMetadata);
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
    m_webCrawler.setDatabaseName(newDataBaseName);
    m_invertedIndex.setDatabaseName(newDataBaseName);
    m_ranker.setDatabaseName(newDataBaseName);
}

void SearchEngine::setDocumentsCollectionName(const std::string& collectionName)
{
    m_webCrawler.setDocumentsCollectionName(collectionName);
    m_invertedIndex.setDocumentsCollectionName(collectionName);
    m_ranker.setDocumentsCollectionName(collectionName);
}

void SearchEngine::setVisitedUrlsCollectionName(const std::string& collectionName)
{
    m_webCrawler.setVisitedUrlCollectionName(collectionName);
}

void SearchEngine::setInvertedIndexCollectionName(const std::string& collectionName)
{
    m_invertedIndex.setInvertedIndexCollectionName(collectionName);
}

void SearchEngine::setMetadataCollectionName(const std::string& collectionName)
{
    m_invertedIndex.setMetadataCollectionName(collectionName);
}

void SearchEngine::setRankerParameters(double BM25_K1, double BM25_B, double PHRASE_BOOST_VALUE, double EXACT_MATCH_WEIGHT)
{
    BM25Ranker::setRankerParameters(BM25_K1,BM25_B,PHRASE_BOOST_VALUE,EXACT_MATCH_WEIGHT);
}
