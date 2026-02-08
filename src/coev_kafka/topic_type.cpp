#include "topic_type.h"

bool topic_t::operator==(const topic_t &other) const
{
    return (m_topic == other.m_topic && m_partition == other.m_partition);
}

bool topic_t::operator<(const topic_t &other) const
{
    return (m_topic < other.m_topic || (m_topic == other.m_topic && m_partition < other.m_partition));
}