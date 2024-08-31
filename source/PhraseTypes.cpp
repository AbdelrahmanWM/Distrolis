#include "PhraseTypes.h"


std::regex combined_pattern{
    R"((\"{1,})(.*?)\1|(AND|OR|NOT|\(|\))|([^\s"]+))",
    std::regex_constants::icase
};

// PhraseType StringToPhraseType(const std::string& string)
// {
//     if(std::regex_match(string,phrase_pattern)){
//         return PhraseType::PHRASE;
//     }
//     if(std::regex_match(string,logical_operations_pattern)){
//         return PhraseType::LOGICAL_OPERATION;
//     }
//     if(std::regex_match(string,other_pattern)){
//         return PhraseType::TERM;
//     }
    
//     return PhraseType();
// }

LogicalOperation GetLogicalOperation(const std::string string)
{   
    std::regex and_pattern{R"(AND)", std::regex_constants::icase};
    std::regex or_pattern{R"(OR)", std::regex_constants::icase};
    std::regex not_pattern{R"(NOT)", std::regex_constants::icase};
    std::regex opening_bracket_pattern{R"(\()"};
    std::regex closing_bracket_pattern{R"(\))"};
    if(std::regex_match(string,and_pattern))return LogicalOperation::AND;
    if(std::regex_match(string,or_pattern))return LogicalOperation::OR;
    if(std::regex_match(string,not_pattern))return LogicalOperation::NOT;
    if(std::regex_match(string,opening_bracket_pattern))return LogicalOperation::OPENING_BRACKET;
    if(std::regex_match(string,closing_bracket_pattern))return LogicalOperation::CLOSING_BRACKET;
    return LogicalOperation::OTHER;
}
