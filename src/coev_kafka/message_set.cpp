#include <algorithm>
#include <utility>
#include <coev/coev.h>
#include "message.h"
#include "message_set.h"
#include "errors.h"
#include "length_field.h"
MessageBlock::MessageBlock(std::shared_ptr<Message> msg, int64_t offset) : m_message(msg), m_offset(offset)
{
}
MessageBlock::MessageBlock(std::shared_ptr<Message> msg) : m_message(msg), m_offset(0)
{
}
MessageSet::MessageSet(const MessageSet &o) : m_partial_trailing_message(o.m_partial_trailing_message), m_overflow_message(o.m_overflow_message)
{
    m_messages = o.m_messages;
}

std::vector<MessageBlock> MessageBlock::Messages()
{
    if (m_message)
    {
        return std::move(m_message->m_message_set.m_messages);
    }

    std::vector<MessageBlock> single;
    single.emplace_back(m_message, m_offset);
    return single;
}

int MessageBlock::encode(packet_encoder &pe) const
{
    pe.putInt64(m_offset);
    LengthField length_field;
    pe.push(length_field);
    int err = m_message->encode(pe);
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
    m_message = std::make_shared<Message>();
    err = m_message->decode(pd);
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
    for (auto &block : m_messages)
    {
        int err = block.encode(pe);
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
        int err = magic_value(pd, magic);
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

        MessageBlock msb;
        err = msb.decode(pd);
        if (err == 0)
        {
            m_messages.emplace_back(msb.m_message);
        }
        else if (err == ErrInsufficientData)
        {
            if (msb.m_offset == -1)
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
    m_messages.emplace_back(msg);
}

void MessageSet::clear()
{
    m_partial_trailing_message = false;
    m_overflow_message = false;
    m_messages.clear();
}