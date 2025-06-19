#include <string.h>
#include <string>
#include "request.h"

namespace coev::nghttp2
{
    static std::string empty_string = "";
    const char *g_path = ":path";
    const char *g_method = ":method";
    const char *g_scheme = ":scheme";
    const char *g_authority = ":authority";

    void request::add_header(const char *name, size_t namelen, const char *value, size_t valuelen)
    {
        if (strncmp(name, g_path, strlen(g_path)) == 0)
        {
            m_path = std::string(value, valuelen);
        }
        else if (strncmp(name, g_method, strlen(g_method)) == 0)

        {
            m_method = std::string(value, valuelen);
        }
        else if (strncmp(name, g_scheme, strlen(g_scheme)) == 0)
        {
            m_scheme = std::string(value, valuelen);
        }
        else if (strncmp(name, g_authority, strlen(g_authority)) == 0)
        {
            m_authority = std::string(value, valuelen);
        }
        else
        {
            m_headers.emplace(std::string(name, namelen), std::string(value, valuelen));
        }
    }

    void request::set_stream_id(int32_t id)
    {
        m_stream_id = id;
    }
    void request::append(const char *data, size_t size)
    {
        m_body.append(data, size);
    }
    const std::string &request::header(const std::string &key)
    {
        auto it = m_headers.find(key);
        return it != m_headers.end() ? it->second : empty_string;
    }
    const std::string &request::path()
    {
        return m_path;
    }
    const std::string &request::method()
    {
        return m_method;
    }
    const std::string &request::scheme()
    {
        return m_scheme;
    }
    const std::string &request::authority()
    {
        return m_authority;
    }
    const std::string &request::body()
    {
        return m_body;
    }
    int32_t request::id() const
    {
        return m_stream_id;
    }
}