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
#
struct BrokerProducer
{
    BrokerProducer(std::shared_ptr<AsyncProducer> parent, std::shared_ptr<Broker> broker);
    coev::awaitable<void> run();

    coev::awaitable<void> shutdown();
    KError needsRetry(std::shared_ptr<ProducerMessage> msg);
    int waitForSpace(std::shared_ptr<ProducerMessage> msg, bool force_rollover);
    void rollOver();
    void handleResponse(std::shared_ptr<BrokerProducerResponse> response);
    coev::awaitable<void> handleSuccess(std::shared_ptr<ProduceSet> sent, std::shared_ptr<ProduceResponse> response);
    void handleError(std::shared_ptr<ProduceSet> sent, KError err);

    std::shared_ptr<AsyncProducer> parent_;
    std::shared_ptr<Broker> broker_;
    std::shared_ptr<ProduceSet> buffer_;
    bool timer_fired_;
    KError closing_;
    std::map<std::string, std::map<int32_t, KError>> current_retries_;

    coev::co_channel<std::shared_ptr<ProducerMessage>> input_;
    coev::co_channel<std::shared_ptr<ProduceSet>> output_;
    coev::co_channel<std::shared_ptr<BrokerProducerResponse>> responses_;
    coev::co_channel<bool> abandoned_;
    coev::co_timer timer_;
};