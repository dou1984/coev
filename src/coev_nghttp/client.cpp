/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "client.h"

namespace coev::pool::nghttp2
{
    awaitable<int> _Nghttp::connect()
    {
        return coev::nghttp2::session::connect(m_host.c_str(), m_port);
    }

}