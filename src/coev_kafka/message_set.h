/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <vector>
#include <memory>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
// #include "version.h"
#include "message.h"

namespace coev::kafka
{
    struct Message;

    struct MessageBlock : IEncoder, IDecoder
    {
        int64_t m_offset = 0;
        Message m_message;
        MessageBlock() = default;
        MessageBlock(const Message &msg, int64_t offset);
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

        auto &emplace_message()
        {
            m_messages.emplace_back();
            return m_messages.back();
        }
        void clear();
        int64_t last_offset() const;
    };

} // namespace coev::kafka
