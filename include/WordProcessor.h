#ifndef WORDPROCESSOR_H
#define WORDPROCESSOR_H

#include "libstemmer.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <regex>
#include <queue>
#include "PhraseTypes.h"
class WordProcessor
{
public:

    static std::vector<std::string> tokenize(const std::string &content);
    static bool isValidWord(const std::string word);
    static std::string normalize(const std::string &token);
    static std::string stem(const std::string &word);
    static bool isStopWord(const std::string &word);
    static std::string normalizeQuotedPhrase(const std::string& text);
    static bool isQuotedPhrase(const std::string& text);
    static std::queue<std::pair<PhraseType,std::string>>tokenizeQueryPhrases(const std::string& query);
    static std::queue<std::string> extractSeedUrlsFromString(const std::string &string);

private:
    static const std::unordered_set<std::string>& getStopWords();
    static PhraseType StringToPhraseType(const std::string& string);
};

#endif