#include "URLParser.h"

URLParser::URLParser() {
    
}

void URLParser::checkBaseUrl(const std::string&base_url )
{
if (base_url.empty()) {
        throw std::invalid_argument("Base URL cannot be empty");
    }
}

bool URLParser::isAbsoluteURL(const std::string &url)
{
    return url.find("http://") == 0 || url.find("https://") == 0;
}

bool URLParser::isProtocolRelativeURL(const std::string& url) {
    return url.find("//") == 0;
}

bool URLParser::isRootRelativeURL(const std::string& url) {
    return !url.empty() && url[0] == '/';
}

bool URLParser::shouldIgnoreURL(const std::string& url) {
    return url.empty() || url == "/" || url[0] == '#';
}

std::string URLParser::normalizeURL(const std::string &url)
{
    std::string normalizeURL {url};
    char lastChar = normalizeURL.back();
    while(lastChar=='*'||lastChar=='$'||lastChar=='/'||lastChar=='?'){
        
        normalizeURL.pop_back();
        lastChar = normalizeURL.back();
    }
    size_t index = normalizeURL.find("/*");
    if(index!=std::string::npos){
        normalizeURL = normalizeURL.substr(0,index);
    }
    return normalizeURL;
}

std::string URLParser::resolveProtocolRelativeURL(const std::string& url,const std::string& base_url) {
    std::regex regex{ "^(http:|https:)" };
    std::smatch match;
    if (std::regex_search(base_url, match, regex)) {
        return match.str(1) + url;
    }
    else {
        throw std::runtime_error("Base URL missing scheme for protocol-relative URL");
    }
}

std::string URLParser::resolveRootRelativeURL(const std::string& url,const std::string& base_url) {
    std::regex domainRegex{ R"(^([a-zA-Z]+://[^/]+))" };
    std::smatch match;
    if (std::regex_search(base_url, match, domainRegex)) {
        return match.str(1) + url;
    }
    else {
        throw std::runtime_error("Base URL invalid for root-relative URL");
    }
}

std::string URLParser::resolvePathRelativeURL(const std::string& url,const std::string& base_url) {
    std::string base = base_url;
    std::size_t last_slash = base.find_last_of('/');
    if (last_slash != std::string::npos) {
        base = base.substr(0, last_slash + 1);
    }
    return base + url;
}

std::string URLParser::convertToAbsoluteURL(const std::string& url,const std::string& base_url) {
    std::string result{};
    if (shouldIgnoreURL(url)) {
        result = base_url;
    }

    else if (isAbsoluteURL(url)) {
        result = url;
    }
    else if (isProtocolRelativeURL(url)) {
        result = resolveProtocolRelativeURL(url,base_url);
    }
    else if (isRootRelativeURL(url)) {
        result = resolveRootRelativeURL(url,base_url);
    }
    else {
        result = resolvePathRelativeURL(url,base_url);
    }
    return normalizeURL(result);

}


bool URLParser::isDomainURL(const std::string& base_url)
{
    std::regex domainRegex {R"(^(https?:\/\/)?([a-zA-Z0-9-]+\.)+[a-zA-Z]{2,}\/?$)"};
    std::smatch match;
    return std::regex_match(base_url,domainRegex);
}

std::string URLParser::getRobotsTxtURL(const std::string& base_url)
{

    if(base_url[base_url.length()-1]=='/')return base_url+"robots.txt";
    else return base_url+"/robots.txt";
}
