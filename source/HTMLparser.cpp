#include "XmlParser.h"

XmlParser::XmlParser()
{
    xmlInitParser();
}

XmlParser::~XmlParser()
{
    xmlCleanupParser();
}
std::vector<std::string> XmlParser::extractLinksFromHTML(const std::string& htmlContent)
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
xmlDocPtr XmlParser::loadHtmlDocument(const std::string& htmlContent) {
    xmlDocPtr doc = htmlReadMemory(htmlContent.c_str(), htmlContent.size(), nullptr, nullptr, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (doc == nullptr) {
        throw std::runtime_error("Failed to parse HTML document.");
    }
    return doc;
}

xmlXPathContextPtr XmlParser::createXPathContext(xmlDocPtr doc) {
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == nullptr) {
        throw std::runtime_error("Failed to create XPath context.");
    }
    return xpathCtx;
}

xmlXPathObjectPtr XmlParser::evaluateXPathExpression(xmlXPathContextPtr xpathCtx, const std::string& xpathExpr) {
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(xpathExpr.c_str()), xpathCtx);
    if (xpathObj == nullptr) {
        throw std::runtime_error("Failed to evaluate xPath expression.");
    }
    return xpathObj;
}

std::vector<std::string> XmlParser::extractLinks(const std::string& htmlContent) {
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
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    return links;

}