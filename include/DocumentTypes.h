#ifndef DOCUMENT_TYPES_H
#define DOCUMENT_TYPES_H

#include <string>
#include <unordered_map>
#include <functional>

struct SearchResultDocument
{
    std::string title;
    std::string body;
    std::string url;
    double score;
};
struct Document
{
    std::string _id;
    std::string title;
    std::string url;
    std::string content;
    std::string description;
};
inline std::unordered_map<std::string, std::function<void(Document &, const std::string &)>> documentFieldSetters = {
    {"_id", [](Document &d, const std::string &value)
     { d._id = value; }},
    {"title", [](Document &d, const std::string &value)
     { d.title = value; }},
    {"url", [](Document &d, const std::string &value)
     { d.url = value; }},
    {"content", [](Document &d, const std::string &value)
     { d.content = value; }},
    {"description", [](Document &d, const std::string &value)
     { d.description = value; }},

};
#endif