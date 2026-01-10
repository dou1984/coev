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

struct SyncProducer : ISyncProducer
{

    std::shared_ptr<AsyncProducer> producer;
    std::atomic<bool> closed{false};
    coev::co_task task;

    coev::awaitable<void> handleSuccesses();
    coev::awaitable<void> handleErrors();

    SyncProducer(std::shared_ptr<AsyncProducer> p);
    ~SyncProducer();

    coev::awaitable<int> SendMessage(std::shared_ptr<ProducerMessage> msg, int32_t &, int64_t &);
    coev::awaitable<int> SendMessages(const std::vector<std::shared_ptr<ProducerMessage>> &msgs);
    int Close();
    ProducerTxnStatusFlag TxnStatus();
    bool IsTransactional();
    int BeginTxn();
    coev::awaitable<int> CommitTxn();
    coev::awaitable<int> AbortTxn();
    int AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id);
    int AddMessageToTxn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id);
};

SyncProducer::SyncProducer(std::shared_ptr<AsyncProducer> p) : producer(p)
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

    producer->m_input.set(msg);
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
        producer->m_input.set(msgs[i]);
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
        auto msg = co_await producer->m_successes.get();
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
        auto err = co_await producer->m_errors.get();
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
    producer->AsyncClose();

    return 0;
}

ProducerTxnStatusFlag SyncProducer::TxnStatus()
{
    return producer->TxnStatus();
}

bool SyncProducer::IsTransactional()
{
    return producer->IsTransactional();
}

int SyncProducer::BeginTxn()
{
    return producer->BeginTxn();
}

coev::awaitable<int> SyncProducer::CommitTxn()
{
    return producer->CommitTxn();
}

coev::awaitable<int> SyncProducer::AbortTxn()
{
    return producer->AbortTxn();
}

int SyncProducer::AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id)
{
    return producer->AddOffsetsToTxn(offsets, group_id);
}

int SyncProducer::AddMessageToTxn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id)
{
    return producer->AddMessageToTxn(msg, metadata, group_id);
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