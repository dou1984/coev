/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <memory>
#include "errors.h"
#include "produce_set.h"
#include "producer_message.h"

namespace coev::kafka
{

    struct BrokerProducerResponse
    {
        std::shared_ptr<ProduceSet> m_produce_set;
        std::shared_ptr<ProduceResponse> m_produce_response;
        KError m_err;
    };

} // namespace coev::kafka
