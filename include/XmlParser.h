#ifndef XML_PARSER_H
#define XML_PARSER_H
#include <libxml/xpath.h>
#include<libxml/HTMLparser.h>
#include<vector>
#include<iostream>

class XmlParser {
public:
	XmlParser();

	~XmlParser();
	std::vector<std::string> extractLinksFromHTML(const std::string& htmlContent);

private:
	std::string htmlContent;
	xmlDocPtr loadHtmlDocument(const std::string& htmlContent);
	xmlXPathContextPtr createXPathContext(xmlDocPtr doc);
	xmlXPathObjectPtr evaluateXPathExpression(xmlXPathContextPtr xpathCtx, const std::string& xpathExpr);
	std::vector<std::string> extractLinks(const std::string& htmlContent);

};




#endif