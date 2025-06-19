#pragma once
#include <string>
#include <unordered_map>

namespace coev::http
{
    struct request
    {
        std::string url;
        std::string body;
        std::string status;
        std::unordered_map<std::string, std::string> headers;
        request() = default;
        request(request &&o);
        ~request();
        request(const request &o) = delete;
        const request &operator=(request &&o);
    };
}