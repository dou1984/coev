/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <memory>
#include <coev/coev.h>

namespace coev::kafka
{

    struct ProducerMessage;
    struct ConsumerMessage;

    struct ProducerInterceptor
    {
        virtual ~ProducerInterceptor() = default;
        virtual awaitable<int> OnSend(std::shared_ptr<ProducerMessage> msg) = 0;
    };

    struct ConsumerInterceptor
    {
        virtual ~ConsumerInterceptor() = default;
        virtual awaitable<int> OnConsume(std::shared_ptr<ConsumerMessage> msg) = 0;
    };

    awaitable<int> safely_apply_interceptor(std::shared_ptr<ProducerMessage> &msg, std::shared_ptr<ProducerInterceptor> &interceptor);
    awaitable<int> safely_apply_interceptor(std::shared_ptr<ConsumerMessage> &msg, std::shared_ptr<ConsumerInterceptor> &interceptor);

} // namespace coev::kafka
