#pragma once
#include <coev/coev.h>
#include <cosys/cosys.h>
#include <nghttp2/nghttp2.h>


namespace coev::nghttp2
{

    class NghttpServer final : public tcp::server
    {
    public:
        NghttpServer(const char *url); 
        
        
        
        
    };
}