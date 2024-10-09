#include "SeedURLS.h"
#include <iostream>

std::queue<std::string> SEEDURLS::readSeedUrls(std::string fileRelativePath)
{
    std::queue<std::string> seedUrls{};
    std::ifstream myFile;
    myFile.open(fileRelativePath);
    std::string url;
    try
    {
        if (myFile.is_open())
        {
            while (myFile)
            {
                std::getline(myFile, url);
                seedUrls.push(url);
            }
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what();
    }

    myFile.close();
    return seedUrls;
}
