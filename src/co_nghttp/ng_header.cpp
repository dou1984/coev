#include <string.h>
#include <string>
#include "ng_header.h"


namespace coev::nghttp2
{
    void ng_header::push_back(const char *name, size_t namelen, const char *value, size_t valuelen)
    {
        m_headers.emplace_back(__nv{.h = {
                                        .name = name,
                                        .value = value,
                                        .namelen = namelen,
                                        .valuelen = valuelen,
                                        .flags = NGHTTP2_NV_FLAG_NONE}});
    }
    void ng_header::push_back(const char *name, const char *value)
    {
        push_back(name, strlen(name), value, strlen(value));
    };
    ng_header::operator nghttp2_nv *()
    {
        return &(m_headers.data()->nva);
    }
    int ng_header::size() const
    {
        return m_headers.size();
    }
}