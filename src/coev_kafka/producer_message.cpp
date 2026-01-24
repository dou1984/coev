#include "producer_message.h"
#include <algorithm>
#include <cstdint>

ProducerMessage::ProducerMessage()
    : m_offset(0),
      m_partition(0),
      m_retries(0),
      m_flags(static_cast<FlagSet>(0)),
      m_sequence_number(0),
      m_producer_epoch(0),
      m_has_sequence(false)
{
}

int ProducerMessage::byteSize(int version) const
{
    int size = 0;

    if (version >= 2)
    {
        size = MaximumRecordOverhead;
        for (auto &h : m_headers)
        {
            size += static_cast<int>(h.Key.size()) + static_cast<int>(h.Value.size()) + 2 * 5;
        }
    }
    else
    {
        size = ProducerMessageOverhead;
    }

    if (m_key)
    {
        size += m_key->Length();
    }
    if (m_value)
    {
        size += m_value->Length();
    }

    return size;
}

void ProducerMessage::clear()
{
    m_flags = static_cast<FlagSet>(0);
    m_retries = 0;
    m_sequence_number = 0;
    m_producer_epoch = 0;
    m_has_sequence = false;
}
