#include "consumer_group_session.h"
#include "consumer_group.h"
#include "sleep_for.h"
#include "consumer_message.h"
#include "consumer_group_handler.h"
#include "offset_manager.h"

ConsumerGroupSession::ConsumerGroupSession(std::shared_ptr<ConsumerGroup> &_parent, const std::string &_memberID, int32_t _generationID, std::shared_ptr<ConsumerGroupHandler> _handler, std::shared_ptr<OffsetManager> _offsets, std::map<std::string, std::vector<int32_t>> &_claims, std::shared_ptr<Context> &_ctx, std::function<void()> _cancel) : m_parent(_parent), m_member_id(_memberID), m_generation_id(_generationID), m_handler(_handler), m_offsets(_offsets), m_claims(_claims), m_context(_ctx), m_cancel(_cancel)
{

    m_task << ConsumerGroupSession::_heartbeat_loop();
}

std::map<std::string, std::vector<int32_t>> ConsumerGroupSession::get_claims()
{
    return m_claims;
}

std::string ConsumerGroupSession::member_id()
{
    return m_member_id;
}

int32_t ConsumerGroupSession::generation_id()
{
    return m_generation_id;
}

void ConsumerGroupSession::mark_offset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata)
{
}

void ConsumerGroupSession::commit()
{
}

void ConsumerGroupSession::reset_offset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata)
{
}

void ConsumerGroupSession::mark_message(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata)
{
    mark_offset(msg->m_topic, msg->m_partition, msg->m_offset + 1, metadata);
}
std::shared_ptr<Context> ConsumerGroupSession::get_context()
{
    return m_context;
}

coev::awaitable<void> ConsumerGroupSession::hander_error(std::shared_ptr<PartitionOffsetManager> pom, const std::string &topic, int32_t partition)
{
    co_return;
}
coev::awaitable<int> ConsumerGroupSession::_release(bool withCleanup)
{
    m_cancel();

    int err = 0;

    if (withCleanup)
    {
        if (int e = m_handler->cleanup(shared_from_this()); e != 0)
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

coev::awaitable<void> ConsumerGroupSession::_consume(const std::string &topic, int32_t partition)
{
    co_return;
}

coev::awaitable<void> ConsumerGroupSession::_heartbeat_loop()
{
    m_hd_dead.store(true);
    co_return;
}
