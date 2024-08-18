#ifndef HTML_PARSER_H
#define HTML_PARSER_H
#include <libxml/xpath.h>
#include<libxml/HTMLparser.h>
#include<vector>
#include<iostream>
#include <unordered_set>
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
	bson_t * getPageDocument(const std::string &htmlContent, const std::string &url) const;
	void extractAndStorePageDetails(const std::string &htmlContent, const std::string &url, const DataBase *&db,const std::string&database_name, const std::string& collection_name) const;
	std::vector<std::string> extractLinksFromHTML(const std::string& htmlContent) const;
	std::vector<std::string> extractRobotsTxtLinks(const std::string& robotsTxt)const;
	

private:
	
	std::string htmlContent;
	documentStructure extractElements(const::std::string& htmlContent, const std::string& url) const;
	xmlDocPtr loadHtmlDocument(const std::string& htmlContent) const;
	std::string extractElement(const htmlDocPtr& doc, const xmlXPathContextPtr& xpathCtx, const std::string& xpathExpr)  const;
    std::string extractText(const htmlDocPtr& doc) const;
	xmlXPathContextPtr createXPathContext(const xmlDocPtr& doc)  const;
	xmlXPathObjectPtr evaluateXPathExpression(const xmlXPathContextPtr& xpathCtx, const std::string& xpathExpr) const;
	std::vector<std::string> extractLinks(const std::string& htmlContent) const;
	void freeHtmlDocumentContextObject(const htmlDocPtr& doc, const xmlXPathContextPtr& xpathCtx, const xmlXPathObjectPtr& xpathObject)  const;
	bson_t* createBSONFromDocument(const documentStructure& doc) const;
	void extractTextNodes(xmlNodePtr node, std::string &output) const;
	static const std::unordered_set<std::string>& getHTMLSkipTags();
};




#endif