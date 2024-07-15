#include "HTMLParser.h"

HTMLParser::HTMLParser()
{
    xmlInitParser();
}

HTMLParser::~HTMLParser()
{
    xmlCleanupParser();
}
std::vector<std::string> HTMLParser::extractLinksFromHTML(const std::string& htmlContent)
{
    try {
        std::vector<std::string> links = extractLinks(htmlContent);
        for (const auto& link : links) {
            std::cout << "Found link: " << link << std::endl;
        }
        return links;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}
std::string HTMLParser::extractTitle(const std::string& htmlContent)
{
    htmlDocPtr doc = loadHtmlDocument(htmlContent);
    std::string xpathExpr = "title";
    xmlXPathContextPtr xpathCtx = createXPathContext(doc);
    xmlXPathObjectPtr xpathObj = evaluateXPathExpression(xpathCtx,xpathExpr);
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    std::string title;
    if (nodes->nodeNr > 0) {
        title = (char*)xmlNodeGetContent(nodes->nodeTab[0]);
    }
    freeHtmlDocumentContextObject(doc, xpathCtx, xpathObj);
    return title;
}

std::string HTMLParser::extractContent(const std::string& htmlContext)
{
    htmlDocPtr doc = loadHtmlDocument(htmlContent);
    std::string xpathExpr = "body";
    xmlXPathContextPtr xpathCtx = createXPathContext(doc);
    xmlXPathObjectPtr xpathObj = evaluateXPathExpression(xpathCtx, xpathExpr);
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    std::string content;
    if (nodes->nodeNr > 0) {
        content = (char*)xmlNodeGetContent(nodes->nodeTab[0]);
    }
    freeHtmlDocumentContextObject(doc, xpathCtx, xpathObj);
    return content;
}

xmlDocPtr HTMLParser::loadHtmlDocument(const std::string& htmlContent) {
    xmlDocPtr doc = htmlReadMemory(htmlContent.c_str(), htmlContent.size(), nullptr, nullptr, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == nullptr) {
        throw std::runtime_error("Failed to parse HTML document.");
    }
    return doc;
}

xmlXPathContextPtr HTMLParser::createXPathContext(xmlDocPtr doc) {
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == nullptr) {
        throw std::runtime_error("Failed to create XPath context.");
    }
    return xpathCtx;
}

xmlXPathObjectPtr HTMLParser::evaluateXPathExpression(xmlXPathContextPtr xpathCtx, const std::string& xpathExpr) {
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(xpathExpr.c_str()), xpathCtx);
    if (xpathObj == nullptr) {
        throw std::runtime_error("Failed to evaluate xPath expression.");
    }
    return xpathObj;
}

std::vector<std::string> HTMLParser::extractLinks(const std::string& htmlContent) {
    std::vector<std::string>links;
    xmlDocPtr doc = loadHtmlDocument(htmlContent);
    xmlXPathContextPtr xpathCtx = createXPathContext(doc);
    std::string xpathEpxr = "//a[@href]";
    xmlXPathObjectPtr xpathObj = evaluateXPathExpression(xpathCtx, xpathEpxr);
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    for (int i = 0; i < nodes->nodeNr; ++i) {
        xmlNodePtr node = nodes->nodeTab[i];
        xmlChar* href = xmlGetProp(node, reinterpret_cast <const xmlChar*>("href"));
        if (href != nullptr) {
            links.push_back(reinterpret_cast<const char*> (href));
            xmlFree(href);
        }
    }

    return links;
    freeHtmlDocumentContextObject(doc, xpathCtx, xpathObj);
}

void HTMLParser::freeHtmlDocumentContextObject(htmlDocPtr doc, xmlXPathContextPtr xpathCtx, xmlXPathObjectPtr xpathObject)
{
    xmlXPathFreeObject(xpathObject);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
}
