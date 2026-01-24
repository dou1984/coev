#include "partition_producer.h"
#include "sleep_for.h"
#include "producer_message.h"

PartitionProducer::PartitionProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic, int32_t partition)
    : m_parent(parent), m_topic(topic), m_partition(partition), m_high_watermark(0), m_retry_state(parent->m_conf->Producer.Retry.Max + 1)
{
}
PartitionProducer::~PartitionProducer()
{
}

coev::awaitable<int> PartitionProducer::init()
{
    auto err = co_await m_parent->m_client->Leader(m_topic, m_partition, m_leader);
    if (err != 0)
    {
        co_return err;
    }
    if (m_leader)
    {
        auto broker_producer_ = m_parent->get_broker_producer(m_leader);
        m_parent->m_in_flight++;
        auto syn_msg = std::make_shared<ProducerMessage>();
        syn_msg->m_topic = m_topic;
        syn_msg->m_partition = m_partition;
        syn_msg->m_flags = FlagSet::Syn;
        broker_producer_->m_input.set(syn_msg);
    }
    co_return 0;
}
coev::awaitable<int> PartitionProducer::dispatch(std::shared_ptr<ProducerMessage> &msg)
{

    if (m_broker_producer)
    {
        bool _;
        if (m_broker_producer->m_abandoned.try_get(_))
        {
            m_parent->unref_broker_producer(m_leader, m_broker_producer);
            m_broker_producer.reset();
            co_await sleep_for(m_parent->m_conf->Producer.Retry.Backoff);
        }
    }

    if (msg->m_retries > m_high_watermark)
    {
        int err = co_await update_leader_if_broker_producer_is_nil(msg);
        if (err != 0)
        {
            co_return err;
        }
        new_high_watermark(msg->m_retries);
        co_await backoff(msg->m_retries);
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
            co_return 0;
        }
        else if (msg->m_flags & FlagSet::Fin)
        {
            m_retry_state[m_high_watermark].m_expect_chaser = false;
            co_await flush_retry_buffers();
            m_parent->m_in_flight--;
            co_return 0;
        }
    }

    int err = co_await update_leader_if_broker_producer_is_nil(msg);
    if (err != 0)
    {
        co_return err;
    }

    if (m_parent->m_conf->Producer.Idempotent && msg->m_retries == 0 && msg->m_flags == 0)
    {
        m_parent->m_txnmgr->GetAndIncrementSequenceNumber(msg->m_topic, msg->m_partition, msg->m_sequence_number, msg->m_producer_epoch);
        msg->m_has_sequence = true;
    }

    if (m_parent->is_transactional())
    {
        m_parent->m_txnmgr->MaybeAddPartitionToCurrentTxn(m_topic, m_partition);
    }

    m_broker_producer->m_input.set(msg);

    if (m_broker_producer)
    {
        m_parent->unref_broker_producer(m_leader, m_broker_producer);
    }
    co_return 0;
}

coev::awaitable<void> PartitionProducer::backoff(int retries)
{
    auto backoff = m_parent->m_conf->Producer.Retry.Backoff;
    if (backoff > std::chrono::milliseconds(0))
    {
        co_await sleep_for(backoff);
    }
}

coev::awaitable<int> PartitionProducer::update_leader_if_broker_producer_is_nil(std::shared_ptr<ProducerMessage> msg)
{
    if (!m_broker_producer)
    {
        int err = co_await update_leader();
        if (err != 0)
        {
            m_parent->return_error(msg, err);
            co_await backoff(msg->m_retries);
            co_return err;
        }
    }
    co_return 0;
}

void PartitionProducer::new_high_watermark(int hwm)
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

    m_parent->unref_broker_producer(m_leader, m_broker_producer);
    m_broker_producer.reset();
}

coev::awaitable<void> PartitionProducer::flush_retry_buffers()
{
    while (true)
    {
        m_high_watermark--;

        if (!m_broker_producer)
        {
            int err = co_await update_leader();
            if (err != 0)
            {
                m_parent->return_errors(m_retry_state[m_high_watermark].m_buf, err);
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

coev::awaitable<int> PartitionProducer::update_leader()
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

    m_broker_producer = m_parent->get_broker_producer(m_leader);
    m_parent->m_in_flight++;

    auto syn_msg = std::make_shared<ProducerMessage>();
    syn_msg->m_topic = m_topic;
    syn_msg->m_partition = m_partition;
    syn_msg->m_flags = FlagSet::Syn;
    m_broker_producer->m_input.set(syn_msg);

    co_return 0;
}