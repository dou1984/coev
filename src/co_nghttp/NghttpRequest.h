#pragma once
#include <string>
#include "NghttpHeader.h"

namespace coev
{
    struct NghttpRequest : NghttpHeader
    {
        std::string m_body;
    };
}