/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_kafka/consumer.h>
#include <coev_kafka/message.h>
#include <coev_kafka/partition_consumer.h>
#include <coev_kafka/config.h>

using namespace coev;
using namespace coev::kafka;

extern std::string test_host;
extern int test_port;
extern std::string test_topic;

void run_consumer_test()
{
    runnable::instance()
        .start(
            []() -> awaitable<void>
            {
                std::shared_ptr<Config> conf = std::make_shared<Config>();
                std::shared_ptr<Consumer> consumer;
                std::vector<std::string> addrs = {test_host + ":" + std::to_string(test_port)};

                auto err = co_await NewConsumer(addrs, conf, consumer);
                if (err)
                {
                    LOG_ERR("NewConsumer error: %d", err);
                    co_return;
                }
                Consumer::MessageChannel ch;
                co_start << consumer->Consume(test_topic, ch);

                uint64_t message_count = 0;
                while (true)
                {
                    std::shared_ptr<ConsumerMessage> msg;
                    co_await ch.get(msg);
                    if (msg)
                    {
                        message_count++;
                        if (message_count % 500 == 0)
                        {
                            LOG_DBG("Consumed %lu messages", message_count);
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                LOG_DBG("Consumer finished, consumed %lu messages", message_count);
                co_return;
            }).wait();
}
