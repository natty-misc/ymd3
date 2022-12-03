//
// Created by Michal on 26.02.2021.
//

#include "youtuberetriever.h"
#include <regex>

YMD::RetrieveFailure::RetrieveFailure(std::string& what) : std::runtime_error(what)
{

}

YMD::RetrieveFailure::RetrieveFailure(const char* what) : std::runtime_error(what)
{

}

YMD::YouTubeRetriever::YouTubeRetriever(const std::string& url)
{
    std::regex videoIDPattern(R"(^.*(?:(?:youtu\.be/|v/|vi/|u/\w/|embed/)|(?:(?:watch)?\?v(?:i)?=|&v(?:i)?=))([^#&?]*).*)");

    for (std::sregex_iterator it(url.cbegin(), url.cend(), videoIDPattern), end; it != end; it++)
    {
        auto match = *it;
        auto idMatch = match[1];

        if (idMatch.matched)
        {
            const std::string& id = idMatch.str();

            if (id.size() == 11)
            {
                this->videoID = id;
                return;
            }
        }
    }

    throw YMD::RetrieveFailure("Could not find a valid video ID in that URL!");
}

const std::string& YMD::YouTubeRetriever::getVideoID() const
{
    return this->videoID;
}
