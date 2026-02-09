#pragma once
#include <memory>
#include <map>
#include <memory>
#include <coev/coev.h>

#include "broker.h"
#include "producer_message.h"
#include "broker_producer_response.h"
#include "produce_set.h"
#include "errors.h"
#include "async_producer.h"
#include "topic_type.h"
#
struct BrokerProducer
{
    BrokerProducer(std::shared_ptr<AsyncProducer> parent, std::shared_ptr<Broker> broker);
    coev::awaitable<void> run();
    coev::awaitable<void> bridge();

    coev::awaitable<void> shutdown();
    KError needs_retry(std::shared_ptr<ProducerMessage> msg);
    int wait_for_space(std::shared_ptr<ProducerMessage> msg, bool force_rollover);
    void rollover();
    void handle_response(std::shared_ptr<BrokerProducerResponse> response);
    coev::awaitable<void> handle_success(std::shared_ptr<ProduceSet> sent, std::shared_ptr<ProduceResponse> response);
    void handle_error(std::shared_ptr<ProduceSet> sent, KError err);

    std::shared_ptr<AsyncProducer> m_parent;
    std::shared_ptr<Broker> m_broker;
    std::shared_ptr<ProduceSet> m_buffer;
    bool m_timer_fired = false;
    KError m_closing = ErrNoError;
    std::map<topic_t, KError> m_current_retries;

    coev::co_channel<std::shared_ptr<ProducerMessage>> m_input;
    coev::co_channel<std::shared_ptr<ProduceSet>> m_output;
    coev::co_channel<std::shared_ptr<BrokerProducerResponse>> m_responses;
    coev::co_channel<bool> m_abandoned;
    coev::co_timer m_timer;
    co_task m_task;
};