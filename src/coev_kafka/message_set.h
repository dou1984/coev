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
    std::shared_ptr<Message> m_message;
    MessageBlock() = default;
    MessageBlock(std::shared_ptr<Message> msg, int64_t offset);
    MessageBlock(std::shared_ptr<Message> msg);
    std::vector<MessageBlock> Messages();
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd);
};

struct MessageSet : IEncoder, IDecoder
{
    bool m_partial_trailing_message = false;
    bool m_overflow_message = false;
    std::vector<MessageBlock> m_messages;
    MessageSet() = default;
    MessageSet(const MessageSet &o);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd);
    void add_message(std::shared_ptr<Message> msg);
    void clear();
};
