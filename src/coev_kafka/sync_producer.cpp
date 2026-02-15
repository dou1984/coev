#include <atomic>
#include <unordered_map>
#include <coev/coev.h>
#include "async_producer.h"
#include "sync_producer.h"
#include "producer_error.h"

static coev::guard::co_channel<std::shared_ptr<ProducerMessage>> expectations_pool;

int VerifyProducerConfig(std::shared_ptr<Config> config)
{
    if (config->Producer.Return.Errors)
    {
        return ErrSyncProducerError;
    }
    if (config->Producer.Return.Successes)
    {
        return ErrSyncProducerSuccess;
    }
    return ErrNoError;
}

SyncProducer::SyncProducer(std::shared_ptr<AsyncProducer> producer) : m_producer(producer)
{
    m_task << handle_replies();
}

SyncProducer::~SyncProducer()
{
    if (!closed)
    {
        close();
    }
}

coev::awaitable<int> SyncProducer::send_message(std::shared_ptr<ProducerMessage> msg, int32_t &partition, int64_t &offset)
{
    m_producer->m_input.set(msg);

    std::shared_ptr<ProducerMessage> _msg;
    co_await expectations_pool.get(_msg);
    if (msg == nullptr)
    {
        partition = -1;
        offset = -1;
        co_return INVALID;
    }
    partition = msg->m_partition;
    offset = msg->m_offset;
    co_return 0;
}

coev::awaitable<int> SyncProducer::send_messages(const std::vector<std::shared_ptr<ProducerMessage>> &msgs)
{

    coev::co_task task;
    task << [this](auto size) -> coev::awaitable<void>
    {
        for (auto i = 0; i < size; ++i)
        {
            std::shared_ptr<ProducerMessage> msg;
            co_await m_producer->m_replies.get(msg);
            if (msg->m_err)
            {
                LOG_CORE("send_messages err %d", msg->m_err);
            }
        }
    }(msgs.size());
    for (size_t i = 0; i < msgs.size(); ++i)
    {
        m_producer->m_input.set(msgs[i]);
    }
    co_await task.wait_all();
    co_return 0;
}

coev::awaitable<void> SyncProducer::handle_replies()
{
    while (!closed)
    {
        std::shared_ptr<ProducerMessage> msg;
        co_await m_producer->m_replies.get(msg);
        expectations_pool.set(msg);
    }
    co_return;
}

int SyncProducer::close()
{
    if (closed.exchange(true))
    {
        return 0;
    }
    m_producer->async_close();

    return 0;
}

ProducerTxnStatusFlag SyncProducer::txn_status()
{
    return m_producer->txn_status();
}

bool SyncProducer::is_transactional()
{
    return m_producer->is_transactional();
}

int SyncProducer::begin_txn()
{
    return m_producer->begin_txn();
}

coev::awaitable<int> SyncProducer::commit_txn()
{
    return m_producer->commit_txn();
}

coev::awaitable<int> SyncProducer::abort_txn()
{
    return m_producer->abort_txn();
}

int SyncProducer::add_offsets_to_txn(const std::map<std::string, std::vector<PartitionOffsetMetadata>> &offsets, const std::string &group_id)
{
    return m_producer->add_offsets_to_txn(offsets, group_id);
}

int SyncProducer::add_message_to_txn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id)
{
    return m_producer->add_message_to_txn(msg, metadata, group_id);
}

coev::awaitable<int> NewSyncProducer(const std::vector<std::string> &addrs, std::shared_ptr<Config> &config, std::shared_ptr<SyncProducer> &producer)
{
    producer = nullptr;
    if (config == nullptr)
    {
        config = std::make_shared<Config>();
        config->Producer.Return.Successes = true;
    }

    int err = VerifyProducerConfig(config);
    if (err != 0)
    {
        co_return err;
    }
    std::shared_ptr<AsyncProducer> p;
    err = co_await NewAsyncProducer(addrs, config, p);
    if (err != 0)
    {
        co_return err;
    }
    producer = std::make_shared<SyncProducer>(p);
    co_return 0;
}

coev::awaitable<int> NewSyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<SyncProducer> &producer)
{

    int err = VerifyProducerConfig(client->GetConfig());
    if (err != 0)
    {
        co_return err;
    }

    auto async_producer = std::make_shared<AsyncProducer>();
    err = co_await NewAsyncProducer(client, async_producer);
    if (err != 0)
    {
        co_return err;
    }
    producer = std::make_shared<SyncProducer>(async_producer);
    co_return 0;
}