#pragma once

#include <coev/coev.h>
#include <curl/curl.h>

namespace coev
{
    class CurlCli : public io_connect
    {
        CURLM *m_multi = nullptr;

    public:
        CurlCli();
        ~CurlCli();

        void download(const char *url, int num);
    }
}