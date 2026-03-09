/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "topic_type.h"

namespace coev::kafka
{

    bool topic_t::operator==(const topic_t &other) const
    {
        return (m_topic == other.m_topic && m_partition == other.m_partition);
    }

    bool topic_t::operator<(const topic_t &other) const
    {
        return (m_topic < other.m_topic || (m_topic == other.m_topic && m_partition < other.m_partition));
    }

} // namespace coev::kafka