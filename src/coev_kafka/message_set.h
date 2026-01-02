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
    int64_t Offset = 0;
    std::shared_ptr<Message> Msg;

    MessageBlock() = default;
    MessageBlock(std::shared_ptr<Message> msg, int64_t offset) : Msg(msg), Offset(offset)
    {
    }
    std::vector<std::shared_ptr<MessageBlock>> Messages();
    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};

struct MessageSet : IEncoder, IDecoder
{
    bool PartialTrailingMessage = false;
    bool OverflowMessage = false;
    std::vector<std::shared_ptr<MessageBlock>> Messages;
    MessageSet() = default;
    MessageSet(std::shared_ptr<MessageBlock> msg)
    {
        Messages.push_back(msg);
    }
    MessageSet(const MessageSet &o) : PartialTrailingMessage(o.PartialTrailingMessage), OverflowMessage(o.OverflowMessage)
    {
        Messages = o.Messages;
    }
    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
    void addMessage(std::shared_ptr<Message> msg);
};
