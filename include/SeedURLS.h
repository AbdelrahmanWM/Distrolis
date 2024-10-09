#ifndef SEEDURL_H
#define SEEDURL_H
#include <queue>
#include <string>
#include <fstream>
class SEEDURLS
{
public:
    static std::queue<std::string> readSeedUrls(std::string fileRelativePath);
};
#endif