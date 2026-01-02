#pragma once

#include <memory>
#include <coev.h>

struct ProducerMessage;
struct ConsumerMessage;

struct ProducerInterceptor
{
    virtual ~ProducerInterceptor() = default;
    virtual coev::awaitable<int> OnSend(std::shared_ptr<ProducerMessage> msg) = 0;
};

struct ConsumerInterceptor
{
    virtual ~ConsumerInterceptor() = default;
    virtual coev::awaitable<int> OnConsume(std::shared_ptr<ConsumerMessage> msg) = 0;
};

coev::awaitable<int> safelyApplyInterceptor(std::shared_ptr<ProducerMessage> &msg, std::shared_ptr<ProducerInterceptor> &interceptor);
coev::awaitable<int> safelyApplyInterceptor(std::shared_ptr<ConsumerMessage> &msg, std::shared_ptr<ConsumerInterceptor> &interceptor);
