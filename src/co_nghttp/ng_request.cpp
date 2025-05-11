#include <string.h>
#include <string>
#include "ng_request.h"

namespace coev::nghttp2
{
    static std::string empty_string = "";

    void ng_request::add_header(const char *name, size_t namelen, const char *value, size_t valuelen)
    {
        m_headers.emplace(std::string(name, namelen), std::string(value, valuelen));
    }

    void ng_request::set_stream_id(int32_t id)
    {
        m_stream_id = id;
    }
    void ng_request::append(const char *data, size_t size)
    {
        m_body.append(data, size);
    }
    const std::string &ng_request::header(const std::string &key)
    {
    
        auto it = m_headers.find(key);
        return it != m_headers.end() ? it->second : empty_string;
    }
    const std::string &ng_request::path()
    {
        auto it = m_headers.find(":path");
        return it != m_headers.end() ? it->second : empty_string;
    }
    const std::string &ng_request::body()
    {
        return m_body;
    }
    int32_t ng_request::id() const
    {
        return m_stream_id;
    }
}