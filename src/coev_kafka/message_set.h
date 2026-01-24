#pragma once

#include <vector>
#include <memory>
#include <cstdint>

#include "message.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"

struct MessageBlock : IEncoder, IDecoder
{
    int64_t m_offset = 0;
    std::shared_ptr<Message> m_msg;

    MessageBlock() = default;
    MessageBlock(std::shared_ptr<Message> msg, int64_t offset) : m_msg(msg), m_offset(offset)
    {
    }
    std::vector<std::shared_ptr<MessageBlock>> Messages();
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd);
};

struct MessageSet : IEncoder, IDecoder
{
    bool m_partial_trailing_message = false;
    bool m_overflow_message = false;
    std::vector<std::shared_ptr<MessageBlock>> m_messages;
    MessageSet() = default;
    MessageSet(std::shared_ptr<MessageBlock> msg)
    {
        m_messages.push_back(msg);
    }
    MessageSet(const MessageSet &o) : m_partial_trailing_message(o.m_partial_trailing_message), m_overflow_message(o.m_overflow_message)
    {
        m_messages = o.m_messages;
    }
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd);
    void add_message(std::shared_ptr<Message> msg);
};
