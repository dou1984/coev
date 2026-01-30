#pragma once

#include <vector>
#include <memory>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"

struct Message;
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
    MessageSet(std::shared_ptr<MessageBlock> msg);
    MessageSet(const MessageSet &o);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd);
    void add_message(std::shared_ptr<Message> msg);
    void clear();
};
