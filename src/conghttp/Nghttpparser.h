#pragma once

#include <nghttp2/nghttp2.h>

namespace coev ::http2
{
    class Nghttpparser
    {
    public:
        Nghttpparser();
        ~Nghttpparser();

    private:
        nghttp2_session m_session;        
        nghttp2_hd_deflater m_deflater;
        
        static nghttp2_session_callbacks m_callbacks;
    };
}