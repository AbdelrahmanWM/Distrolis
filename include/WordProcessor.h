#ifndef WORDPROCESSOR_H
#define WORDPROCESSOR_H

#include "libstemmer.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <regex>

class WordProcessor
{
public:
    static std::vector<std::string> tokenize(std::string &content);
    static bool isValidWord(std::string word);
    static std::string normalize(std::string &token);
    static std::string stem(std::string &word);
    static bool isStopWord(std::string &word);
private:
    static const std::unordered_set<std::string>& getStopWords();
};

#endif