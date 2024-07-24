#include "URLParser.h"

URLParser::URLParser(const std::string& baseURL) : m_baseURL(baseURL) {
    if (m_baseURL.empty()) {
        throw std::invalid_argument("Base URL cannot be empty");
    }
}

bool URLParser::isAbsoluteURL(const std::string& url) {
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

std::string URLParser::resolveProtocolRelativeURL(const std::string& url) {
    std::regex regex{ "^(http:|https:)" };
    std::smatch match;
    if (std::regex_search(m_baseURL, match, regex)) {
        return match.str(1) + url;
    }
    else {
        throw std::runtime_error("Base URL missing scheme for protocol-relative URL");
    }
}

std::string URLParser::resolveRootRelativeURL(const std::string& url) {
    std::regex domainRegex{ R"(^([a-zA-Z]+://[^/]+))" };
    std::smatch match;
    if (std::regex_search(m_baseURL, match, domainRegex)) {
        return match.str(1) + url;
    }
    else {
        throw std::runtime_error("Base URL invalid for root-relative URL");
    }
}

std::string URLParser::resolvePathRelativeURL(const std::string& url) {
    std::string base = m_baseURL;
    std::size_t last_slash = base.find_last_of('/');
    if (last_slash != std::string::npos) {
        base = base.substr(0, last_slash + 1);
    }
    return base + url;
}

std::string URLParser::convertToAbsoluteURL(const std::string& url) {
    if (shouldIgnoreURL(url)) {
        return m_baseURL;
    }

    if (isAbsoluteURL(url)) {
        return url;
    }
    else if (isProtocolRelativeURL(url)) {
        return resolveProtocolRelativeURL(url);
    }
    else if (isRootRelativeURL(url)) {
        return resolveRootRelativeURL(url);
    }
    else {
        return resolvePathRelativeURL(url);
    }
}

void URLParser::setBaseURL(const std::string& baseURL) {
    if (baseURL.empty()) {
        throw std::invalid_argument("Base URL cannot be empty");
    }
    m_baseURL = baseURL;
}
