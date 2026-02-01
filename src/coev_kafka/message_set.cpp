#include <algorithm>
#include <utility>
#include <coev/coev.h>
#include "message.h"
#include "message_set.h"
#include "errors.h"
#include "length_field.h"

MessageSet::MessageSet(std::shared_ptr<MessageBlock> msg)
{
    m_messages.push_back(msg);
}
MessageSet::MessageSet(const MessageSet &o) : m_partial_trailing_message(o.m_partial_trailing_message), m_overflow_message(o.m_overflow_message)
{
    m_messages = o.m_messages;
}
std::vector<std::shared_ptr<MessageBlock>> MessageBlock::Messages()
{
    if (m_msg)
    {
        return std::move(m_msg->m_message_set.m_messages);
    }

    std::vector<std::shared_ptr<MessageBlock>> single;
    auto clone = std::make_shared<MessageBlock>();
    clone->m_offset = m_offset;
    clone->m_msg = std::move(m_msg);
    single.emplace_back(clone);
    return single;
}

int MessageBlock::encode(packet_encoder &pe) const
{
    pe.putInt64(m_offset);
    LengthField length_field;
    pe.push(length_field);
    int err = m_msg->encode(pe);
    if (err != 0)
    {
        pe.pop();
        return err;
    }
    return pe.pop();
}

int MessageBlock::decode(packet_decoder &pd)
{
    int err = pd.getInt64(m_offset);
    if (err != 0)
    {
        return err;
    }
    LengthField length_field;
    if ((err = pd.push(length_field)) != 0)
    {
        return err;
    }
    m_msg = std::make_shared<Message>();
    err = m_msg->decode(pd);
    if (err != 0)
    {
        return err;
    }
    err = pd.pop();
    if (err == 0)
    {
        return err;
    }
    return err;
}

int MessageSet::encode(packet_encoder &pe) const
{
    for (auto &message_block : m_messages)
    {
        int err = message_block->encode(pe);
        if (err != 0)
        {
            return err;
        }
    }
    return 0;
}

int MessageSet::decode(packet_decoder &pd)
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
            m_messages.emplace_back(msb);
        }
        else if (err == ErrInsufficientData)
        {
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

void MessageSet::add_message(std::shared_ptr<Message> msg)
{
    auto block = std::make_shared<MessageBlock>();
    block->m_msg = std::move(msg);
    m_messages.emplace_back(std::move(block));
}

void MessageSet::clear()
{
    m_partial_trailing_message = false;
    m_overflow_message = false;
    m_messages.clear();
}