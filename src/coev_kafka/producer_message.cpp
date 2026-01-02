#include "producer_message.h"
#include <algorithm>
#include <cstdint>

ProducerMessage::ProducerMessage()
    : Offset(0),
      Partition(0),
      Retries(0),
      Flags(static_cast<FlagSet>(0)),
      SequenceNumber(0),
      ProducerEpoch(0),
      hasSequence(false)
{
}

int ProducerMessage::ByteSize(int version) const
{
    int size = 0;

    if (version >= 2)
    {
        size = maximumRecordOverhead;
        for (auto &h : Headers)
        {
            size += static_cast<int>(h.Key.size()) + static_cast<int>(h.Value.size()) + 2 * 5;
        }
    }
    else
    {
        size = producerMessageOverhead;
    }

    if (Key)
    {
        size += Key->Length();
    }
    if (Value)
    {
        size += Value->Length();
    }

    return size;
}

void ProducerMessage::Clear()
{
    Flags = static_cast<FlagSet>(0);
    Retries = 0;
    SequenceNumber = 0;
    ProducerEpoch = 0;
    hasSequence = false;
}
