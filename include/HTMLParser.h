#ifndef HTML_PARSER_H
#define HTML_PARSER_H
#include <libxml/xpath.h>
#include<libxml/HTMLparser.h>
#include<vector>
#include<iostream>
#include "DataBase.h"

class HTMLParser {
public:
	struct documentStructure {
		std::string url;
		std::string title;
		std::string description;
		std::string keywords;
		std::string author;
		std::string publication_date;
		std::string last_modification_date;
		std::string language;
		std::string content_type;
		std::string tags;
		std::string image_links;
		std::string content;
	};
	HTMLParser();
	~HTMLParser();
	void extractAndStorePageDetails(const std::string& htmlContent, const std::string& url, const DataBase*& db);
	std::vector<std::string> extractLinksFromHTML(const std::string& htmlContent);
	
	

private:
	
	std::string htmlContent;
	documentStructure extractElements(const::std::string& htmlContent, const std::string& url);
	xmlDocPtr loadHtmlDocument(const std::string& htmlContent);
	std::string extractElement(htmlDocPtr doc, xmlXPathContextPtr xpathCtx, std::string xpathExpr);
	xmlXPathContextPtr createXPathContext(xmlDocPtr doc);
	xmlXPathObjectPtr evaluateXPathExpression(xmlXPathContextPtr xpathCtx, const std::string& xpathExpr);
	std::vector<std::string> extractLinks(const std::string& htmlContent);
	void freeHtmlDocumentContextObject(htmlDocPtr doc, xmlXPathContextPtr xpathCtx, xmlXPathObjectPtr xpathObject);
	const bson_t* createBSONFromDocument(const documentStructure& doc);
};




#endif