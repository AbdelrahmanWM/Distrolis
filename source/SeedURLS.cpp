#include "SeedURLS.h"
#include <iostream>

std::queue<std::string> SeedURLS::readSeedUrls(std::string fileRelativePath)
{
    std::queue<std::string> seedUrls{};
    std::ifstream myFile;
    myFile.open(fileRelativePath);
    std::string url;
    if (!myFile.is_open())
    {
        std::cerr << "Error opening file: " << fileRelativePath << '\n';
        return seedUrls;
    }
    while (std::getline(myFile, url))
    {
        if (!url.empty())
        {
            seedUrls.push(url);
        }
    }
    if (myFile.bad())
    {
        std::cerr << "Error reading from file: " << fileRelativePath << std::endl;
    }
    return seedUrls;
}
