#include "ng_response.h"

namespace coev::nghttp2
{
    static std::string empty_string = "";

    const std::string &ng_response::status() const
    {
        return m_status;
    }

    const std::string &ng_response::body() const
    {
        return m_body;
    }
    void ng_response::add_header(const char *name, size_t namelen, const char *value, size_t valuelen)
    {
        m_headers.emplace(std::string(name, namelen), std::string(value, valuelen));
    }
    void ng_response::append(const char *body, size_t length)
    {
        m_body.append(body, length);
    }
    const std::string &ng_response::header(const std::string &key)
    {

        auto it = m_headers.find(key);
        return it != m_headers.end() ? it->second : empty_string;
    }
}