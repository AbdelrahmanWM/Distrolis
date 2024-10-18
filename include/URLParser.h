#ifndef URLPARSER_H
#define URLPARSER_H
#include<string>
#include<regex>
#include <unordered_set>
class URLParser {
public:
	URLParser();
	static std::string convertToAbsoluteURL(const std::string& url,const std::string& base_url);
	static bool isDomainURL(const std::string& base_url);
	static std::string getRobotsTxtURL(const std::string& base_url);
	static std::string normalizeURL(const std::string& url);
	static bool isAbsoluteURL(const std::string& url);
    static bool isURL(const std::string& url);

private:

	static void checkBaseUrl(const std::string&base_url );
	static bool isProtocolRelativeURL(const std::string& url);
	static bool isRootRelativeURL(const std::string& url);
	static std::string resolveProtocolRelativeURL(const std::string& url,const std::string& base_url);
	static std::string resolveRootRelativeURL(const std::string& url,const std::string& base_url);
	static std::string resolvePathRelativeURL(const std::string& url,const std::string& base_url);
	static bool shouldIgnoreURL(const std::string& url);
	static std::unordered_set<std::string> getTLDsList();
	static bool isValidTLD(const std::string& domain);
};

#endif