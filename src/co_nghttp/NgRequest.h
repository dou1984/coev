#pragma once
#include <string>
#include "NgHeader.h"

namespace coev::nghttp2
{
    struct NgRequest final : NgHeader
    {
        uint32_t stream_id;
        std::string m_body;
    };
}