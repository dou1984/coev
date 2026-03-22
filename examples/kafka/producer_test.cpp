/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_kafka/async_producer.h>
#include <coev_kafka/message.h>
#include <coev_kafka/partitioner.h>
#include <coev_kafka/config.h>
#include <chrono>

using namespace coev;
using namespace coev::kafka;

extern std::string test_host;
extern int test_port;
extern std::string test_topic;

void run_producer_test()
{
    runnable::instance()
        .start(
            []() -> awaitable<void>
            {
                std::shared_ptr<Config> conf = std::make_shared<Config>();
                conf->Producer.Acks = WaitForAll;
                conf->Producer.Partitioner = NewRandomPartitioner;
                conf->Producer.Return.Successes = true;
                conf->Producer.Return.Errors = true;
                conf->Version = V0_11_0_2;

                std::vector<std::string> addrs = {test_host + ":" + std::to_string(test_port)};

                std::shared_ptr<AsyncProducer> producer;
                auto err = co_await NewAsyncProducer(addrs, conf, producer);
                if (err)
                {
                    LOG_ERR("NewAsyncProducer error: %d", err);
                    co_return;
                }
                defer(producer->async_close());

                uint64_t index = 0;
                uint64_t max_messages = 2000;
                auto start_time = std::chrono::steady_clock::now();

                while (index < max_messages)
                {
                    auto msg = std::make_shared<ProducerMessage>();
                    msg->m_topic = test_topic;
                    msg->m_key.m_data = "key-" + std::to_string(index);
                    msg->m_value.m_data = "message-" + std::to_string(index);

                    std::shared_ptr<ProducerMessage> reply;
                    co_await producer->producer(msg, reply);
                    if (reply->m_err != ErrNoError)
                    {
                        LOG_ERR("produced message %lu failed: %d", index, reply->m_err);
                    }
                    else
                    {
                        if ((index + 1) % 10000 == 0)
                        {
                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                            double qps = (elapsed > 0) ? (index + 1) * 1000.0 / elapsed : 0;
                            LOG_DBG("produced %lu messages, QPS: %.2f", index + 1, qps);
                        }
                    }
                    index++;
                }

                auto end_time = std::chrono::steady_clock::now();
                auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
                double final_qps = (total_time > 0) ? max_messages * 1000.0 / total_time : 0;
                LOG_DBG("Producer finished producing %lu messages", max_messages);
                LOG_DBG("Total time: %lu ms", total_time);
                LOG_DBG("Average QPS: %.2f", final_qps);
                co_return;
            }).wait();
}
