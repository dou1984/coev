#include <algorithm>
#include <utility>
#include <coev/coev.h>
#include "message_set.h"
#include "errors.h"
#include "length_field.h"

std::vector<std::shared_ptr<MessageBlock>> MessageBlock::Messages()
{
    if (m_msg && m_msg->m_set)
    {
        return std::move(m_msg->m_set->m_messages);
    }
    auto self = std::make_shared<MessageBlock>(*this);

    std::vector<std::shared_ptr<MessageBlock>> single;
    auto clone = std::make_shared<MessageBlock>();
    clone->m_offset = m_offset;
    clone->m_msg = std::move(m_msg);
    single.emplace_back(clone);
    return single;
}

int MessageBlock::encode(packetEncoder &pe)
{
    pe.putInt64(m_offset);
    pe.push(std::make_shared<LengthField>());
    int err = m_msg->encode(pe);
    if (err != 0)
    {
        pe.pop();
        return err;
    }
    return pe.pop();
}

int MessageBlock::decode(packetDecoder &pd)
{
    int err = pd.getInt64(m_offset);
    if (err != 0)
        return err;

    auto lengthField = acquireLengthField();
    defer(releaseLengthField(lengthField));
    if ((err = pd.push(std::dynamic_pointer_cast<pushDecoder>(lengthField))) != 0)
    {
        return err;
    }

    m_msg = std::make_shared<Message>();
    err = m_msg->decode(pd);

    int popErr = pd.pop();
    if (err == 0)
        err = popErr;

    return err;
}

int MessageSet::encode(packetEncoder &pe)
{
    for (auto &msgBlock : m_messages)
    {
        int err = msgBlock->encode(pe);
        if (err != 0)
            return err;
    }
    return 0;
}

int MessageSet::decode(packetDecoder &pd)
{
    m_messages.clear();
    m_partial_trailing_message = false;
    m_overflow_message = false;

    while (pd.remaining() > 0)
    {
        int8_t magic;
        int err = magicValue(pd, magic);
        if (err != 0)
        {
            if (err == ErrInsufficientData)
            {
                m_partial_trailing_message = true;
                return 0;
            }
            return err;
        }

        if (magic > 1)
        {
            return 0;
        }

        auto msb = std::make_shared<MessageBlock>();
        err = msb->decode(pd);
        if (err == 0)
        {
            m_messages.push_back(std::move(msb));
        }
        else if (err == ErrInsufficientData)
        {
            // Server may send partial trailing message
            if (msb->m_offset == -1)
            {
                m_overflow_message = true;
            }
            else
            {
                m_partial_trailing_message = true;
            }
            return 0;
        }
        else
        {
            return err;
        }
    }

    return 0;
}

void MessageSet::addMessage(std::shared_ptr<Message> msg)
{
    auto block = std::make_shared<MessageBlock>();
    block->m_msg = std::move(msg);
    m_messages.emplace_back(std::move(block));
}