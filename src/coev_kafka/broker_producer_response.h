#pragma once

#include <memory>
#include "errors.h"
#include "produce_set.h"
#include "producer_message.h"

struct BrokerProducerResponse
{
    std::shared_ptr<ProduceSet> m_pset;
    std::shared_ptr<ProduceResponse> m_res;
    KError m_err;
};
