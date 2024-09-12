#include "SearchEngineServer.h"
#include <functional>
#include <string>

SearchEngineServer::SearchEngineServer(SearchEngine searchEngine)
    : m_searchEngine(searchEngine)
{
}

SearchEngineServer::~SearchEngineServer()
{
}

void SearchEngineServer::start()
{
    crow::SimpleApp app;

    using std::placeholders::_1;
    CROW_ROUTE(app, "/").methods(crow::HTTPMethod::Get)(std::bind(&SearchEngineServer::rootAPIDocumentation, this, _1));
    CROW_ROUTE(app, "/search").methods(crow::HTTPMethod::Get)(std::bind(&SearchEngineServer::search, this, _1));
    CROW_ROUTE(app, "/crawl").methods(crow::HTTPMethod::Post)(std::bind(&SearchEngineServer::crawl, this, _1));
    CROW_ROUTE(app, "/indexDocuments").methods(crow::HTTPMethod::Post)(std::bind(&SearchEngineServer::indexDocument, this, _1));
    CROW_ROUTE(app, "/crawlAndIndexDocument").methods(crow::HTTPMethod::Post)(std::bind(&SearchEngineServer::crawlAndIndexDocument, this, _1));
    CROW_ROUTE(app, "/setRankerParameters").methods(crow::HTTPMethod::Post)(std::bind(&SearchEngineServer::setRankerParameters, this, _1));

    app.port(8080).multithreaded().run();
}

crow::response SearchEngineServer::search(const crow::request &req)
{
    try
    {
        std::string query = req.url_params.get("query");
        SearchEngine::SearchResultsDocument results = m_searchEngine.search(query);
        return crow::response(200, SearchResultsDocumentToJSON(results));
    }
    catch (std::exception &ex)
    {
        return crow::response(404, ex.what());
    }
}

crow::response SearchEngineServer::crawl(const crow::request &req)
{
    try
    {
        auto post_data = crow::json::load(req.body);
        if (!post_data)
        {
            return crow::response(400, "Invalid JSON data");
        }
        crow::json::wvalue seedUrlsJSON{post_data["seedUrls"]};
        std::queue<std::string> seed_urls = SeedUrlsJSONToQueue(seedUrlsJSON);
        crow::json::wvalue numberOfPagesJSON = post_data["numberOfPages"];

        std::string numberOfPages = numberOfPagesJSON.dump();

        m_searchEngine.crawl( std::stoi(numberOfPages),seed_urls);
        std::string response = "Successfully crawled the pages";
        return crow::response(200, response);
    }
    catch (std::exception &ex)
    {
        std::string error = "Error " + static_cast<std::string>(ex.what());
        return crow::response(500, error);
    }
}

crow::response SearchEngineServer::indexDocument(const crow::request &req)
{
    try
    {
        auto post_data = crow::json::load(req.body);
        if (!post_data)
        {
            return crow::response(400, "Invalid JSON data");
        }
        crow::json::wvalue clearJSON = post_data["clear"];
        std::string clear = clearJSON.dump();

        m_searchEngine.indexDocuments((bool)std::stoi(clear));
        std::string response = "Successfully created Inverted Index";
        return crow::response(200, response);
    }
    catch (std::exception &ex)
    {
        std::string error = "Error " + static_cast<std::string>(ex.what());
        return crow::response(500, error);
    }
}

crow::response SearchEngineServer::crawlAndIndexDocument(const crow::request &req)
{
    try
    {
        auto post_data = crow::json::load(req.body);
        if (!post_data)
        {
            return crow::response(400, "Invalid JSON data");
        }
        crow::json::wvalue seedUrlsJSON{post_data["seedUrls"]};
        std::queue<std::string> seed_urls = SeedUrlsJSONToQueue(seedUrlsJSON);
        crow::json::wvalue numberOfPagesJSON = post_data["numberOfPages"];

        std::string numberOfPages = numberOfPagesJSON.dump();

        crow::json::wvalue clearJSON = post_data["clear"];
        std::string clear = clearJSON.dump();

        m_searchEngine.crawlAndIndexDocuments(seed_urls, std::stoi(numberOfPages), (bool)std::stoi(clear));
        std::string response = "Successfully crawled the pages and created Inverted Index";
        return crow::response(200, response);
    }
    catch (std::exception &ex)
    {
        std::string error = "Error " + static_cast<std::string>(ex.what());
        return crow::response(500, error);
    }
}

crow::json::wvalue SearchEngineServer::SearchResultsDocumentToJSON(SearchEngine::SearchResultsDocument &resultsDocument)
{
    crow::json::wvalue::list result{};

    for (const auto &pair : resultsDocument)
    {
        crow::json::wvalue item;
        item["key"] = pair.first;
        item["value"] = pair.second;
        result.push_back(item);
    }
    return result;
}

crow::response SearchEngineServer::setRankerParameters(const crow::request &req)
{
    auto post_data = crow::json::load(req.body);
    if (!post_data)
    {
        return crow::response(404, "Invalid JSON data");
    }
    try
    {
        crow::json::wvalue BM25_K1_JSON = post_data["BM25_K1"];
        double BM25_K1 = std::stod(BM25_K1_JSON.dump());
        crow::json::wvalue BM25_B_JSON = post_data["BM25_B"];
        double BM25_B = std::stod(BM25_B_JSON.dump());
        crow::json::wvalue PHRASE_BOOST_JSON = post_data["PHRASE_BOOST"];
        double PHRASE_BOOST = std::stod(PHRASE_BOOST_JSON.dump());
        crow::json::wvalue EXACT_MATCH_WEIGHT_JSON = post_data["EXACT_MATCH_WEIGHT"];
        double EXACT_MATCH_WEIGHT = std::stod(EXACT_MATCH_WEIGHT_JSON.dump());
        m_searchEngine.setRankerParameters(BM25_K1, BM25_B, PHRASE_BOOST, EXACT_MATCH_WEIGHT);
        std::string result = "Successfully set new Ranker parameters";
        return crow::response(200, result);
    }
    catch (std::exception &ex)
    {
        std::string error = "Error " + static_cast<std::string>(ex.what());
        return crow::response(500, error);
    }
}

crow::response SearchEngineServer::rootAPIDocumentation(const crow::request &req)
{
    crow::response res;
    res.set_header("Content-Type","text/html");
    return (200,getAPIDocumentationHTMLPage());
}

std::queue<std::string> SearchEngineServer::SeedUrlsJSONToQueue(crow::json::wvalue &list)
{
    std::queue<std::string> queue{};
    queue = WordProcessor::extractSeedUrlsFromString(list.dump());
    return queue;
}

std::string SearchEngineServer::getAPIDocumentationHTMLPage()
{
    const std::string HTMLPage = R"(
    <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Search Engine API Documentation</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            margin: 0;
            padding: 0;
            background-color: #f4f4f4;
        }
        .container {
            width: 80%;
            margin: auto;
            overflow: hidden;
        }
        header {
            background: #333;
            color: #fff;
            padding: 10px 0;
            text-align: center;
        }
        .api-section {
            margin: 20px 0;
            padding: 20px;
            background: #fff;
            border-radius: 5px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }
        h2 {
            color: #333;
        }
        pre {
            background: #f4f4f4;
            padding: 10px;
            border-radius: 5px;
            overflow-x: auto;
        }
    </style>
</head>
<body>
    <header>
        <h1>Search Engine API Documentation</h1>
    </header>
    <div class="container">
        <div class="api-section">
            <h2>1. Search</h2>
            <p><strong>Endpoint:</strong> /search</p>
            <p><strong>Method:</strong> GET</p>
            <p><strong>Parameters:</strong></p>
            <ul>
                <li><strong>query</strong> (string): The search query.</li>
            </ul>
            <p><strong>Response:</strong></p>
            <pre>{
    "results": [
        {
            "key": "Document ID or Key",
            "value": "Search Result"
        }
    ]
}</pre>
            <p><strong>Success Response:</strong></p>
            <pre>200 OK</pre>
            <p><strong>Error Response:</strong></p>
            <pre>404 Not Found</pre>
        </div>
        
        <div class="api-section">
            <h2>2. Crawl</h2>
            <p><strong>Endpoint:</strong> /crawl</p>
            <p><strong>Method:</strong> POST</p>
            <p><strong>Request Body:</strong></p>
            <pre>{
    "seedUrls": [
        "URL1",
        "URL2"
    ],
    "numberOfPages": 10,
    "clear": 1
}</pre>
            <p><strong>Parameters:</strong></p>
            <ul>
                <li><strong>seedUrls</strong> (array of strings): List of seed URLs to crawl.</li>
                <li><strong>numberOfPages</strong> (integer): Number of pages to crawl.</li>
                <li><strong>clear</strong> (integer): Flag to indicate if the index should be cleared (1 for true, 0 for false).</li>
            </ul>
            <p><strong>Response:</strong></p>
            <pre>200 OK: Successfully crawled the pages</pre>
            <p><strong>Error Response:</strong></p>
            <pre>400 Bad Request: Invalid JSON data</pre>
            <pre>500 Internal Server Error</pre>
        </div>

        <div class="api-section">
            <h2>3. Index Documents</h2>
            <p><strong>Endpoint:</strong> /indexDocuments</p>
            <p><strong>Method:</strong> POST</p>
            <p><strong>Request Body:</strong></p>
            <pre>{
    "clear": 1
}</pre>
            <p><strong>Parameters:</strong></p>
            <ul>
                <li><strong>clear</strong> (integer): Flag to indicate if the index should be cleared (1 for true, 0 for false).</li>
            </ul>
            <p><strong>Response:</strong></p>
            <pre>200 OK: Successfully created Inverted Index</pre>
            <p><strong>Error Response:</strong></p>
            <pre>400 Bad Request: Invalid JSON data</pre>
            <pre>500 Internal Server Error</pre>
        </div>

        <div class="api-section">
            <h2>4. Crawl and Index Document</h2>
            <p><strong>Endpoint:</strong> /crawlAndIndexDocument</p>
            <p><strong>Method:</strong> POST</p>
            <p><strong>Request Body:</strong></p>
            <pre>{
    "seedUrls": [
        "URL1",
        "URL2"
    ],
    "numberOfPages": 10,
    "clear": 1
}</pre>
            <p><strong>Parameters:</strong></p>
            <ul>
                <li><strong>seedUrls</strong> (array of strings): List of seed URLs to crawl.</li>
                <li><strong>numberOfPages</strong> (integer): Number of pages to crawl.</li>
                <li><strong>clear</strong> (integer): Flag to indicate if the index should be cleared (1 for true, 0 for false).</li>
            </ul>
            <p><strong>Response:</strong></p>
            <pre>200 OK: Successfully crawled the pages and created Inverted Index</pre>
            <p><strong>Error Response:</strong></p>
            <pre>400 Bad Request: Invalid JSON data</pre>
            <pre>500 Internal Server Error</pre>
        </div>

        <div class="api-section">
            <h2>5. Set Ranker Parameters</h2>
            <p><strong>Endpoint:</strong> /setRankerParameters</p>
            <p><strong>Method:</strong> POST</p>
            <p><strong>Request Body:</strong></p>
            <pre>{
    "BM25_K1": 1.5,
    "BM25_B": 0.75,
    "PHRASE_BOOST": 1.2,
    "EXACT_MATCH_WEIGHT": 2.0
}</pre>
            <p><strong>Parameters:</strong></p>
            <ul>
                <li><strong>BM25_K1</strong> (number): The BM25 parameter K1.</li>
                <li><strong>BM25_B</strong> (number): The BM25 parameter B.</li>
                <li><strong>PHRASE_BOOST</strong> (number): Boost value for phrase matches.</li>
                <li><strong>EXACT_MATCH_WEIGHT</strong> (number): Weight for exact matches.</li>
            </ul>
            <p><strong>Response:</strong></p>
            <pre>200 OK: Successfully set new Ranker parameters</pre>
            <p><strong>Error Response:</strong></p>
            <pre>404 Not Found: Invalid JSON data</pre>
            <pre>500 Internal Server Error</pre>
        </div>
    </div>
</body>
</html>
    )" ;
    return HTMLPage;
}
