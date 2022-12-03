//
// Created by Michal on 26.02.2021.
//

#ifndef YMD3_YOUTUBERETRIEVER_H
#define YMD3_YOUTUBERETRIEVER_H

#include <string>
#include <stdexcept>

namespace YMD
{
    class RetrieveFailure : public std::runtime_error
    {
        public:
            explicit RetrieveFailure(std::string& what);
            explicit RetrieveFailure(const char* what);
    };

    class YouTubeRetriever
    {
        public:
            explicit YouTubeRetriever(const std::string& url);
            [[nodiscard]] const std::string& getVideoID() const;

        private:
            std::string videoID;
    };
}


#endif //YMD3_YOUTUBERETRIEVER_H
