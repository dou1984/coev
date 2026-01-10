#pragma once

#include <memory>
#include "errors.h"
#include "produce_set.h"
#include "producer_message.h"

struct BrokerProducerResponse
{
    std::shared_ptr<ProduceSet> set;
    std::shared_ptr<ProduceResponse> res;
    KError m_err;
};
