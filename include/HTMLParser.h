#ifndef HTML_PARSER_H
#define HTML_PARSER_H
#include <libxml/xpath.h>
#include<libxml/HTMLparser.h>
#include<vector>
#include<iostream>

class HTMLParser {
public:
	HTMLParser();

	~HTMLParser();
	std::vector<std::string> extractLinksFromHTML(const std::string& htmlContent);
	std::string extractTitle(const std::string& htmlContent);
	std::string extractContent(const std::string& htmlContext);

private:
	std::string htmlContent;
	xmlDocPtr loadHtmlDocument(const std::string& htmlContent);
	xmlXPathContextPtr createXPathContext(xmlDocPtr doc);
	xmlXPathObjectPtr evaluateXPathExpression(xmlXPathContextPtr xpathCtx, const std::string& xpathExpr);
	std::vector<std::string> extractLinks(const std::string& htmlContent);
	void freeHtmlDocumentContextObject(htmlDocPtr doc, xmlXPathContextPtr xpathCtx, xmlXPathObjectPtr xpathObject);
};




#endif