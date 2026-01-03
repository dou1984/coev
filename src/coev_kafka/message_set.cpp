#include <algorithm>
#include <utility>
#include <coev/coev.h>
#include "message_set.h"
#include "errors.h"
#include "length_field.h"

std::vector<std::shared_ptr<MessageBlock>> MessageBlock::Messages()
{
    if (Msg && Msg->Set)
    {
        return std::move(Msg->Set->Messages);
    }
    auto self = std::make_shared<MessageBlock>(*this);

    std::vector<std::shared_ptr<MessageBlock>> result;
    if (Msg && Msg->Set)
    {
        return std::move(Msg->Set->Messages);
    }
    else
    {
        std::vector<std::shared_ptr<MessageBlock>> single;
        auto clone = std::make_shared<MessageBlock>();
        clone->Offset = Offset;
        clone->Msg = std::move(Msg);
        single.emplace_back(clone);
        return single;
    }
}

int MessageBlock::encode(PEncoder &pe)
{
    pe.putInt64(Offset);
    pe.push(std::make_shared<LengthField>());
    int err = Msg->encode(pe);
    if (err != 0)
    {
        pe.pop();
        return err;
    }
    return pe.pop();
}

int MessageBlock::decode(PDecoder &pd)
{
    int err = pd.getInt64(Offset);
    if (err != 0)
        return err;

    auto lengthField = acquireLengthField();
    defer(releaseLengthField(lengthField));
    if ((err = pd.push(std::dynamic_pointer_cast<pushDecoder>(lengthField))) != 0)
    {
        return err;
    }

    Msg = std::shared_ptr<Message>();
    err = Msg->decode(pd);

    int popErr = pd.pop();
    if (err == 0)
        err = popErr;

    return err;
}

int MessageSet::encode(PEncoder &pe)
{
    for (auto &msgBlock : Messages)
    {
        int err = msgBlock->encode(pe);
        if (err != 0)
            return err;
    }
    return 0;
}

int MessageSet::decode(PDecoder &pd)
{
    Messages.clear();
    PartialTrailingMessage = false;
    OverflowMessage = false;

    while (pd.remaining() > 0)
    {
        int8_t magic;
        int err = magicValue(pd, magic);
        if (err != 0)
        {
            if (err == ErrInsufficientData)
            {
                PartialTrailingMessage = true;
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
            Messages.push_back(std::move(msb));
        }
        else if (err == ErrInsufficientData)
        {
            // Server may send partial trailing message
            if (msb->Offset == -1)
            {
                OverflowMessage = true;
            }
            else
            {
                PartialTrailingMessage = true;
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
    block->Msg = std::move(msg);
    Messages.emplace_back(std::move(block));
}