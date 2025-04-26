#include <string.h>
#include <string>
#include "NgHeader.h"


namespace coev::nghttp2
{
    void NgHeader::push_back(const char *name, size_t namelen, const char *value, size_t valuelen)
    {
        m_headers.emplace_back(__nv{.h = {
                                        .name = name,
                                        .value = value,
                                        .namelen = namelen,
                                        .valuelen = valuelen,
                                        .flags = NGHTTP2_NV_FLAG_NONE}});
    }
    void NgHeader::push_back(const char *name, const char *value)
    {
        push_back(name, strlen(name), value, strlen(value));
    };
    NgHeader::operator nghttp2_nv *()
    {
        return &(m_headers.data()->nva);
    }
    int NgHeader::size() const
    {
        return m_headers.size();
    }
}