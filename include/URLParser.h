#ifndef URLPARSER_H
#define URLPARSER_H
#include<string>
#include<regex>

class URLParser {
public:
	URLParser(const std::string& baseURL);
	std::string convertToAbsoluteURL(const std::string& url);
	void setBaseURL(const std::string& baseURL);
	bool isDomainURL()const;
	std::string getRobotsTxtURL()const;
	static std::string normalizeURL(const std::string& url);

private:
	std::string m_baseURL;

	bool isAbsoluteURL(const std::string& url);
	bool isProtocolRelativeURL(const std::string& url);
	bool isRootRelativeURL(const std::string& url);
	std::string resolveProtocolRelativeURL(const std::string& url);
	std::string resolveRootRelativeURL(const std::string& url);
	std::string resolvePathRelativeURL(const std::string& url);
	bool shouldIgnoreURL(const std::string& url);
	

};

#endif