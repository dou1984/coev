#include "partition_producer.h"
#include "sleep_for.h"
#include "producer_message.h"

PartitionProducer::PartitionProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic, int32_t partition)
    : m_parent(parent), m_topic(topic), m_partition(partition), m_high_watermark(0), m_retry_state(parent->m_conf->Producer.Retry.Max + 1)
{
}

coev::awaitable<int> PartitionProducer::Dispatch()
{
    auto err = co_await m_parent->m_client->Leader(m_topic, m_partition, m_leader);
    if (err != 0)
    {
        co_return err;
    }
    if (m_leader)
    {
        auto broker_producer_ = m_parent->getBrokerProducer(m_leader);
        m_parent->m_in_flight++;
        auto syn_msg = std::make_shared<ProducerMessage>();
        syn_msg->m_topic = m_topic;
        syn_msg->m_partition = m_partition;
        syn_msg->m_flags = FlagSet::Syn;
        broker_producer_->m_input.set(syn_msg);
    }

    while (true)
    {
        auto msg = co_await m_input.get();
        if (m_broker_producer)
        {
            bool _;
            if (m_broker_producer->m_abandoned.try_get(_))
            {
                m_parent->unrefBrokerProducer(m_leader, m_broker_producer);
                m_broker_producer.reset();
                co_await sleep_for(m_parent->m_conf->Producer.Retry.Backoff);
            }
        }

        if (msg->m_retries > m_high_watermark)
        {
            int err = co_await UpdateLeaderIfBrokerProducerIsNil(msg);
            if (err != 0)
            {
                continue;
            }
            NewHighWatermark(msg->m_retries);
            co_await Backoff(msg->m_retries);
        }
        else if (m_high_watermark > 0)
        {
            if (msg->m_retries < m_high_watermark)
            {
                if (msg->m_flags & FlagSet::Fin)
                {
                    m_retry_state[msg->m_retries].m_expect_chaser = false;
                    m_parent->m_in_flight--;
                }
                else
                {
                    m_retry_state[msg->m_retries].m_buf.push_back(msg);
                }
                continue;
            }
            else if (msg->m_flags & FlagSet::Fin)
            {
                m_retry_state[m_high_watermark].m_expect_chaser = false;
                co_await FlushRetryBuffers();
                m_parent->m_in_flight--;
                continue;
            }
        }

        int err = co_await UpdateLeaderIfBrokerProducerIsNil(msg);
        if (err != 0)
        {
            continue;
        }

        if (m_parent->m_conf->Producer.Idempotent && msg->m_retries == 0 && msg->m_flags == 0)
        {
            m_parent->m_txnmgr->GetAndIncrementSequenceNumber(msg->m_topic, msg->m_partition, msg->m_sequence_number, msg->m_producer_epoch);
            msg->m_has_sequence = true;
        }

        if (m_parent->IsTransactional())
        {
            m_parent->m_txnmgr->MaybeAddPartitionToCurrentTxn(m_topic, m_partition);
        }

        m_broker_producer->m_input.set(msg);
    }

    if (m_broker_producer)
    {
        m_parent->unrefBrokerProducer(m_leader, m_broker_producer);
    }
}

coev::awaitable<void> PartitionProducer::Backoff(int retries)
{
    auto backoff = m_parent->m_conf->Producer.Retry.Backoff;
    if (backoff > std::chrono::milliseconds(0))
    {
        co_await sleep_for(backoff);
    }
}

coev::awaitable<int> PartitionProducer::UpdateLeaderIfBrokerProducerIsNil(std::shared_ptr<ProducerMessage> msg)
{
    if (!m_broker_producer)
    {
        int err = co_await UpdateLeader();
        if (err != 0)
        {
            m_parent->returnError(msg, err);
            co_await Backoff(msg->m_retries);
            co_return err;
        }
    }
    co_return 0;
}

void PartitionProducer::NewHighWatermark(int hwm)
{
    m_high_watermark = hwm;
    m_retry_state[m_high_watermark].m_expect_chaser = true;
    m_parent->m_in_flight++;

    auto fin_msg = std::make_shared<ProducerMessage>();
    fin_msg->m_topic = m_topic;
    fin_msg->m_partition = m_partition;
    fin_msg->m_flags = FlagSet::Fin;
    fin_msg->m_retries = m_high_watermark - 1;
    m_broker_producer->m_input.set(fin_msg);

    m_parent->unrefBrokerProducer(m_leader, m_broker_producer);
    m_broker_producer.reset();
}

coev::awaitable<void> PartitionProducer::FlushRetryBuffers()
{
    while (true)
    {
        m_high_watermark--;

        if (!m_broker_producer)
        {
            int err = co_await UpdateLeader();
            if (err != 0)
            {
                m_parent->returnErrors(m_retry_state[m_high_watermark].m_buf, err);
                goto flushDone;
            }
        }

        for (auto &msg : m_retry_state[m_high_watermark].m_buf)
        {
            if (m_parent->m_conf->Producer.Idempotent && msg->m_retries == 0 && msg->m_flags == 0 && !msg->m_has_sequence)
            {
                m_parent->m_txnmgr->GetAndIncrementSequenceNumber(msg->m_topic, msg->m_partition, msg->m_sequence_number, msg->m_producer_epoch);
                msg->m_has_sequence = true;
            }
            m_broker_producer->m_input.set(msg);
        }

    flushDone:
        m_retry_state[m_high_watermark].m_buf.clear();
        if (m_retry_state[m_high_watermark].m_expect_chaser)
        {
            break;
        }
        else if (m_high_watermark == 0)
        {
            break;
        }
    }
}

coev::awaitable<int> PartitionProducer::UpdateLeader()
{
    std::vector<std::string> topics = {m_topic};
    int err = co_await m_parent->m_client->RefreshMetadata(topics);
    if (err != 0)
    {
        co_return err;
    }

    err = co_await m_parent->m_client->Leader(m_topic, m_partition, m_leader);
    if (err != 0)
    {
        co_return err;
    }

    m_broker_producer = m_parent->getBrokerProducer(m_leader);
    m_parent->m_in_flight++;

    auto syn_msg = std::make_shared<ProducerMessage>();
    syn_msg->m_topic = m_topic;
    syn_msg->m_partition = m_partition;
    syn_msg->m_flags = FlagSet::Syn;
    m_broker_producer->m_input.set(syn_msg);

    co_return 0;
}