#include "interceptors.h"
#include "producer_message.h"
#include "consumer.h"
#include <exception>
#include <iostream>

coev::awaitable<int> safely_apply_interceptor(std::shared_ptr<ProducerMessage> &msg, std::shared_ptr<ProducerInterceptor> &interceptor)
{
    return interceptor->OnSend(msg);
}

coev::awaitable<int> safely_apply_interceptor(std::shared_ptr<ConsumerMessage> &msg, std::shared_ptr<ConsumerInterceptor> &interceptor)
{
    return interceptor->OnConsume(msg);
}