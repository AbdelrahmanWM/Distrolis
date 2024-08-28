#ifndef PHRASETYPES_H
#define PHRASETYPES_H

#include <regex>
#include <string>


enum class PhraseType {
    PHRASE,
    LOGICAL_OPERATION,
    TERM
};


enum class LogicalOperation {
    NOT,
    AND,
    OR,
    OTHER
};


extern std::regex combined_pattern;

extern PhraseType StringToPhraseType(const std::string& string);
extern LogicalOperation GetLogicalOperation(const std::string string);

#endif
