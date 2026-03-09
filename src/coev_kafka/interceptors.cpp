/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "interceptors.h"
#include "producer_message.h"
#include "consumer.h"
#include <exception>
#include <iostream>

namespace coev::kafka
{

    awaitable<int> safely_apply_interceptor(std::shared_ptr<ProducerMessage> &msg, std::shared_ptr<ProducerInterceptor> &interceptor)
    {
        return interceptor->OnSend(msg);
    }

    awaitable<int> safely_apply_interceptor(std::shared_ptr<ConsumerMessage> &msg, std::shared_ptr<ConsumerInterceptor> &interceptor)
    {
        return interceptor->OnConsume(msg);
    }

} // namespace coev::kafka