#include "consumer_group_session.h"
#include "consumer_group.h"
#include "sleep_for.h"
#include "consumer_message.h"
#include "consumer_group_handler.h"

ConsumerGroupSession::ConsumerGroupSession(std::shared_ptr<ConsumerGroup> &_parent, const std::string &_memberID, int32_t _generationID, std::shared_ptr<ConsumerGroupHandler> _handler, std::shared_ptr<IOffsetManager> _offsets, std::map<std::string, std::vector<int32_t>> &_claims, std::shared_ptr<Context> &_ctx, std::function<void()> _cancel)
    : parent(_parent), memberID(_memberID), generationID(_generationID), handler(_handler), offsets(_offsets), claims(_claims), context(_ctx), cancel(_cancel)
{

    task_ << ConsumerGroupSession::HeartbeatLoop_();
}

std::map<std::string, std::vector<int32_t>> ConsumerGroupSession::GetClaims()
{
    return claims;
}

std::string ConsumerGroupSession::MemberID()
{
    return memberID;
}

int32_t ConsumerGroupSession::GenerationID()
{
    return generationID;
}

void ConsumerGroupSession::MarkOffset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata)
{
    // Implementation would go here
}

void ConsumerGroupSession::Commit()
{
    // Implementation would go here
}

void ConsumerGroupSession::ResetOffset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata)
{
    // Implementation would go here
}

void ConsumerGroupSession::MarkMessage(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata)
{
    MarkOffset(msg->Topic, msg->Partition, msg->Offset + 1, metadata);
}
std::shared_ptr<Context> ConsumerGroupSession::GetContext()
{
    return context;
}

coev::awaitable<int> ConsumerGroupSession::Release_(bool withCleanup)
{
    cancel();

    int err = 0;

    if (withCleanup)
    {
        if (int e = handler->Cleanup(shared_from_this()); e != 0)
        {
            parent->HandleError(std::make_shared<ConsumerError>("", -1, e), "", -1);
            err = e;
        }
    }

    hbDying.store(true);

    while (!hbDead.load())
    {
        co_await sleep_for(std::chrono::milliseconds(10));
    }

    co_return err;
}

coev::awaitable<void> ConsumerGroupSession::Consume_(const std::string &topic, int32_t partition)
{
    co_return;
}

coev::awaitable<void> ConsumerGroupSession::HeartbeatLoop_()
{
    hbDead.store(true);
    co_return;
}
