#include "NghttpHeader.h"
#include <string.h>
#include <string>

namespace coev
{
    void NghttpHeader::push_back(const char *name, size_t namelen, const char *value, size_t valuelen)
    {
        m_headers.emplace_back(__nv{.h = {
                                        .name = name,
                                        .value = value,
                                        .namelen = namelen,
                                        .valuelen = valuelen,
                                        .flags = NGHTTP2_NV_FLAG_NONE}});
    }
    void NghttpHeader::push_back(const char *name, const char *value)
    {
        push_back(name, strlen(name), value, strlen(value));
    };
    NghttpHeader::operator nghttp2_nv *()
    {
        return &(m_headers.data()->nva);
    }
    int NghttpHeader::size() const
    {
        return m_headers.size();
    }
}