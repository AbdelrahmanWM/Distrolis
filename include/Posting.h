#pragma once
#include<string>

struct Posting {
    std::string docId;
    int count;
    Posting(const std::string& d, int c) : docId(d), count(c) {}
};