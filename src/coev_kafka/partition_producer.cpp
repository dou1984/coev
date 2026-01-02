#include "partition_producer.h"
#include "sleep_for.h"
#include "producer_message.h"

PartitionProducer::PartitionProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic, int32_t partition)
    : Parent(parent), Topic(topic), Partition(partition), HighWatermark_(0), RetryState_(parent->conf_->Producer.Retry.Max + 1)
{
}

coev::awaitable<int> PartitionProducer::Dispatch()
{
    auto err = co_await Parent->client_->Leader(Topic, Partition, Leader);
    if (err != 0)
    {
        co_return err;
    }
    if (Leader)
    {
        auto broker_producer_ = Parent->getBrokerProducer(Leader);
        Parent->in_flight_++;
        auto syn_msg = std::make_shared<ProducerMessage>();
        syn_msg->Topic = Topic;
        syn_msg->Partition = Partition;
        syn_msg->Flags = FlagSet::Syn;
        broker_producer_->input_.set(syn_msg);
    }

    while (true)
    {
        auto msg = co_await Input.get();
        if (BrokerProducer_)
        {
            bool _;
            if (BrokerProducer_->abandoned_.try_get(_))
            {
                Parent->unrefBrokerProducer(Leader, BrokerProducer_);
                BrokerProducer_.reset();
                co_await sleep_for(Parent->conf_->Producer.Retry.Backoff);
            }
        }

        if (msg->Retries > HighWatermark_)
        {
            int err = co_await UpdateLeaderIfBrokerProducerIsNil(msg);
            if (err != 0)
            {
                continue;
            }
            NewHighWatermark(msg->Retries);
            co_await Backoff(msg->Retries);
        }
        else if (HighWatermark_ > 0)
        {
            if (msg->Retries < HighWatermark_)
            {
                if (msg->Flags & FlagSet::Fin)
                {
                    RetryState_[msg->Retries].expect_chaser = false;
                    Parent->in_flight_--;
                }
                else
                {
                    RetryState_[msg->Retries].buf.push_back(msg);
                }
                continue;
            }
            else if (msg->Flags & FlagSet::Fin)
            {
                RetryState_[HighWatermark_].expect_chaser = false;
                co_await FlushRetryBuffers();
                Parent->in_flight_--;
                continue;
            }
        }

        int err = co_await UpdateLeaderIfBrokerProducerIsNil(msg);
        if (err != 0)
        {
            continue;
        }

        if (Parent->conf_->Producer.Idempotent && msg->Retries == 0 && msg->Flags == 0)
        {
            Parent->txnmgr_->GetAndIncrementSequenceNumber(msg->Topic, msg->Partition, msg->SequenceNumber, msg->ProducerEpoch);
            msg->hasSequence = true;
        }

        if (Parent->IsTransactional())
        {
            Parent->txnmgr_->MaybeAddPartitionToCurrentTxn(Topic, Partition);
        }

        BrokerProducer_->input_.set(msg);
    }

    if (BrokerProducer_)
    {
        Parent->unrefBrokerProducer(Leader, BrokerProducer_);
    }
}

coev::awaitable<void> PartitionProducer::Backoff(int retries)
{
    auto backoff = Parent->conf_->Producer.Retry.Backoff;
    if (backoff > std::chrono::milliseconds(0))
    {
        co_await sleep_for(backoff);
    }
}

coev::awaitable<int> PartitionProducer::UpdateLeaderIfBrokerProducerIsNil(std::shared_ptr<ProducerMessage> msg)
{
    if (!BrokerProducer_)
    {
        int err = co_await UpdateLeader();
        if (err != 0)
        {
            Parent->returnError(msg, err);
            co_await Backoff(msg->Retries);
            co_return err;
        }
    }
    co_return 0;
}

void PartitionProducer::NewHighWatermark(int hwm)
{
    HighWatermark_ = hwm;
    RetryState_[HighWatermark_].expect_chaser = true;
    Parent->in_flight_++;

    auto fin_msg = std::make_shared<ProducerMessage>();
    fin_msg->Topic = Topic;
    fin_msg->Partition = Partition;
    fin_msg->Flags = FlagSet::Fin;
    fin_msg->Retries = HighWatermark_ - 1;
    BrokerProducer_->input_.set(fin_msg);

    Parent->unrefBrokerProducer(Leader, BrokerProducer_);
    BrokerProducer_.reset();
}

coev::awaitable<void> PartitionProducer::FlushRetryBuffers()
{
    while (true)
    {
        HighWatermark_--;

        if (!BrokerProducer_)
        {
            int err = co_await UpdateLeader();
            if (err != 0)
            {
                Parent->returnErrors(RetryState_[HighWatermark_].buf, err);
                goto flushDone;
            }
        }

        for (auto &msg : RetryState_[HighWatermark_].buf)
        {
            if (Parent->conf_->Producer.Idempotent && msg->Retries == 0 && msg->Flags == 0 && !msg->hasSequence)
            {
                Parent->txnmgr_->GetAndIncrementSequenceNumber(msg->Topic, msg->Partition, msg->SequenceNumber, msg->ProducerEpoch);
                msg->hasSequence = true;
            }
            BrokerProducer_->input_.set(msg);
        }

    flushDone:
        RetryState_[HighWatermark_].buf.clear();
        if (RetryState_[HighWatermark_].expect_chaser)
        {
            break;
        }
        else if (HighWatermark_ == 0)
        {
            break;
        }
    }
}

coev::awaitable<int> PartitionProducer::UpdateLeader()
{
    std::vector<std::string> topics = {Topic};
    int err = co_await Parent->client_->RefreshMetadata(topics);
    if (err != 0)
    {
        co_return err;
    }

    err = co_await Parent->client_->Leader(Topic, Partition, Leader);
    if (err != 0)
    {
        co_return err;
    }

    BrokerProducer_ = Parent->getBrokerProducer(Leader);
    Parent->in_flight_++;

    auto syn_msg = std::make_shared<ProducerMessage>();
    syn_msg->Topic = Topic;
    syn_msg->Partition = Partition;
    syn_msg->Flags = FlagSet::Syn;
    BrokerProducer_->input_.set(syn_msg);

    co_return 0;
}