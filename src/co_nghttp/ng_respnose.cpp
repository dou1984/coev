#include "ng_response.h"

namespace coev::nghttp2
{
    const std::string &ng_response::status() const
    {
        return m_status;
    }

    const std::string &ng_response::body() const
    {
        return m_body;
    }
}