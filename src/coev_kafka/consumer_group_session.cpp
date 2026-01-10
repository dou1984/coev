#include "consumer_group_session.h"
#include "consumer_group.h"
#include "sleep_for.h"
#include "consumer_message.h"
#include "consumer_group_handler.h"

ConsumerGroupSession::ConsumerGroupSession(std::shared_ptr<ConsumerGroup> &_parent, const std::string &_memberID, int32_t _generationID, std::shared_ptr<ConsumerGroupHandler> _handler, std::shared_ptr<IOffsetManager> _offsets, std::map<std::string, std::vector<int32_t>> &_claims, std::shared_ptr<Context> &_ctx, std::function<void()> _cancel) : m_parent(_parent), m_member_id(_memberID), m_generation_id(_generationID), m_handler(_handler), m_offsets(_offsets), m_claims(_claims), m_context(_ctx), m_cancel(_cancel)
{

    m_task << ConsumerGroupSession::HeartbeatLoop_();
}

std::map<std::string, std::vector<int32_t>> ConsumerGroupSession::GetClaims()
{
    return m_claims;
}

std::string ConsumerGroupSession::MemberID()
{
    return m_member_id;
}

int32_t ConsumerGroupSession::GenerationID()
{
    return m_generation_id;
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
    MarkOffset(msg->m_topic, msg->m_partition, msg->m_offset + 1, metadata);
}
std::shared_ptr<Context> ConsumerGroupSession::GetContext()
{
    return m_context;
}

coev::awaitable<int> ConsumerGroupSession::Release_(bool withCleanup)
{
    m_cancel();

    int err = 0;

    if (withCleanup)
    {
        if (int e = m_handler->Cleanup(shared_from_this()); e != 0)
        {
            m_parent->HandleError(std::make_shared<ConsumerError>("", -1, e), "", -1);
            err = e;
        }
    }

    m_hd_dying.store(true);

    while (!m_hd_dead.load())
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
    m_hd_dead.store(true);
    co_return;
}
