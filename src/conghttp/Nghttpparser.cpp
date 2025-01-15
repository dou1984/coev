#include "Nghttpparser.h"

namespace coev::http2
{
    nghttp2_session_callbacks Nghttpparser::m_callbacks;
    Nghttpparser::Nghttpparser()
    {
        nghttp2_session_server_new(&m_session, &m_callbacks, this);
    }
    Nghttpparser::~Nghttpparser()
    {
        if (m_session != nullptr)
        {
            nghttp2_session_del(m_session);
        }
    }

}