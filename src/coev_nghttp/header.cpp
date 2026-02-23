/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <string.h>
#include <string>
#include "header.h"

namespace coev::nghttp2
{
    void header::push_back(const char *name, size_t namelen, const char *value, size_t valuelen)
    {
        m_headers.emplace_back(__nv{.h = {
                                        .name = name,
                                        .value = value,
                                        .namelen = namelen,
                                        .valuelen = valuelen,
                                        .flags = NGHTTP2_NV_FLAG_NONE}});
    }
    void header::push_back(const char *name, const char *value)
    {
        push_back(name, strlen(name), value, strlen(value));
    };
    header::operator nghttp2_nv *()
    {
        return &(m_headers.data()->nva);
    }
    int header::size() const
    {
        return m_headers.size();
    }
}