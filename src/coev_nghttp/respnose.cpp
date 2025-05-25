#include "response.h"

namespace coev::nghttp2
{
    
    static std::string empty_string = "";

    const std::string &response::status() const
    {
        return m_status;
    }

    const std::string &response::body() const
    {
        return m_body;
    }
    void response::add_header(const char *name, size_t namelen, const char *value, size_t valuelen)
    {
        m_headers.emplace(std::string(name, namelen), std::string(value, valuelen));
    }
    void response::append(const char *body, size_t length)
    {
        m_body.append(body, length);
    }
    const std::string &response::header(const std::string &key)
    {

        auto it = m_headers.find(key);
        return it != m_headers.end() ? it->second : empty_string;
    }
}