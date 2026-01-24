#include <atomic>
#include <unordered_map>
#include <coev/coev.h>
#include "async_producer.h"
#include "sync_producer.h"
#include "producer_error.h"

static std::mutex poolMutex;
static coev::co_channel<std::shared_ptr<ProducerError>> expectationsPool;

static coev::awaitable<int> getExpectation(std::shared_ptr<ProducerError> &err)
{
    err = co_await expectationsPool.get();
    co_return 0;
}

static void putExpectation(std::shared_ptr<ProducerError> &p)
{
    expectationsPool = p;
}

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

SyncProducer::SyncProducer(std::shared_ptr<AsyncProducer> p) : m_producer(p)
{

    task << handleSuccesses();
    task << handleErrors();
}

SyncProducer::~SyncProducer()
{
    if (!closed)
    {
        Close();
    }
}

coev::awaitable<int> SyncProducer::SendMessage(std::shared_ptr<ProducerMessage> msg, int32_t &partition, int64_t &offset)
{

    co_await m_producer->dispatcher(msg);
    std::shared_ptr<ProducerError> pErr;
    auto err = co_await getExpectation(pErr);
    if (err != 0)
    {
        co_return err;
    }
    if (pErr != nullptr && pErr->m_err != 0)
    {
        partition = -1;
        offset = -1;
        co_return pErr->m_err;
    }
    partition = msg->m_partition;
    offset = msg->m_offset;
    co_return 0;
}

coev::awaitable<int> SyncProducer::SendMessages(const std::vector<std::shared_ptr<ProducerMessage>> &msgs)
{
    std::vector<std::shared_ptr<ProducerError>> expectations(msgs.size());
    for (size_t i = 0; i < msgs.size(); ++i)
    {
        std::shared_ptr<ProducerMessage> msg = msgs[i];
        co_await m_producer->dispatcher(msg);
        co_await getExpectation(expectations[i]);
    }
    for (size_t i = 0; i < expectations.size(); ++i)
    {
        putExpectation(expectations[i]);
    }

    co_return 0;
}

coev::awaitable<void> SyncProducer::handleSuccesses()
{
    while (!closed)
    {
        auto msg = co_await m_producer->m_successes.get();
        if (msg)
        {
            // putExpectation(msg);
        }
    }
}

coev::awaitable<void> SyncProducer::handleErrors()
{
    while (!closed)
    {
        auto err = co_await m_producer->m_errors.get();
        if (err && err->m_err != 0)
        {
        }
    }
}

int SyncProducer::Close()
{
    if (closed.exchange(true))
    {
        return 0;
    }
    m_producer->async_close();

    return 0;
}

ProducerTxnStatusFlag SyncProducer::TxnStatus()
{
    return m_producer->txn_status();
}

bool SyncProducer::IsTransactional()
{
    return m_producer->is_transactional();
}

int SyncProducer::BeginTxn()
{
    return m_producer->begin_txn();
}

coev::awaitable<int> SyncProducer::CommitTxn()
{
    return m_producer->commit_txn();
}

coev::awaitable<int> SyncProducer::AbortTxn()
{
    return m_producer->abort_txn();
}

int SyncProducer::AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id)
{
    return m_producer->add_offsets_to_txn(offsets, group_id);
}

int SyncProducer::AddMessageToTxn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id)
{
    return m_producer->add_message_to_txn(msg, metadata, group_id);
}

coev::awaitable<int> NewSyncProducer(const std::vector<std::string> &addrs, std::shared_ptr<Config> &config, std::shared_ptr<ISyncProducer> &producer)
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

coev::awaitable<int> NewSyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<ISyncProducer> &producer)
{

    int err = VerifyProducerConfig(client->GetConfig());
    if (err != 0)
    {
        co_return err;
    }

    auto p = std::make_shared<AsyncProducer>();
    err = co_await NewAsyncProducerFromClient(client, p);
    if (err != 0)
    {
        co_return err;
    }
    producer = std::make_shared<SyncProducer>(p);
    co_return 0;
}