#pragma once
#include <string>
#include "Ngheader.h"

namespace coev
{
    struct NgRequest : NgHeader
    {
        std::string m_body;
    };
}