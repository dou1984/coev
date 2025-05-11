#pragma once

namespace coev::nghttp2
{
    struct ng_response
    {
        std::string m_status;
        std::string m_body;

        const std::string &status() const;
        const std::string &body() const;
    };
}