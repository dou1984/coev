/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_kafka/async_producer.h>
#include <coev_kafka/consumer.h>
#include <coev_kafka/message.h>
#include <coev_kafka/partition_consumer.h>
#include <coev_kafka/config.h>
#include <coev_kafka/version.h>
#include <coev_kafka/client.h>
#include <coev_kafka/partitioner.h>
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace coev;
using namespace coev::kafka;

extern const std::vector<KafkaVersion> SupportedVersions;

std::string test_host;
int test_port;
std::string test_topic;

struct VersionTestResult
{
    KafkaVersion version;
    bool producer_ok = false;
    bool consumer_ok = false;
    int producer_err = 0;
    int consumer_err = 0;
    std::string producer_msg;
    std::string consumer_msg;
};

static awaitable<int> test_producer_version(const KafkaVersion &ver)
{
    std::shared_ptr<Config> conf = std::make_shared<Config>();
    conf->Version = ver;
    conf->Producer.Acks = WaitForAll;
    conf->Producer.Return.Successes = true;
    conf->Producer.Return.Errors = true;
    conf->Producer.Partitioner = NewRandomPartitioner(test_topic);

    std::vector<std::string> addrs = {test_host + ":" + std::to_string(test_port)};

    std::shared_ptr<AsyncProducer> producer;
    auto err = co_await NewAsyncProducer(addrs, conf, producer);
    if (err)
    {
        co_return err;
    }
    finally(producer->async_close());

    auto msg = std::make_shared<ProducerMessage>();
    msg->m_topic = test_topic;
    msg->m_key.m_data = "version-test-key";
    msg->m_value.m_data = "version-test-value-" + ver.String();

    std::shared_ptr<ProducerMessage> reply;
    err = co_await producer->producer(msg, reply);
    if (err == 0 && reply && reply->m_err == ErrNoError)
    {
        co_return 0;
    }
    co_return reply ? reply->m_err : err;
}

static awaitable<int> test_consumer_version(const KafkaVersion &ver)
{
    std::shared_ptr<Config> conf = std::make_shared<Config>();
    conf->Version = ver;

    std::vector<std::string> addrs = {test_host + ":" + std::to_string(test_port)};

    std::shared_ptr<Consumer> consumer;
    auto err = co_await NewConsumer(addrs, conf, consumer);
    if (err)
    {
        co_return err;
    }
    finally(consumer->Close());

    // Consume messages from the test topic
    Consumer::MessageChannel ch;
    co_start << consumer->Consume(test_topic, ch);

    // Try to fetch at most 1 message within 5 seconds using wait_for_any
    std::shared_ptr<ConsumerMessage> msg;
    auto get_result = wait_for_any(
        ch.get(msg),
        sleep_for(5.0));
    auto winner = co_await get_result;
    if (winner == 0 && msg)
    {
        co_return 0;
    }
    co_return -1;
}

static awaitable<void> test_single_version(const KafkaVersion &ver, VersionTestResult &result)
{
    result.version = ver;

    LOG_DBG("Testing version: %s", ver.String().c_str());

    // Test Producer
    result.producer_err = co_await test_producer_version(ver);
    if (result.producer_err == 0)
    {
        result.producer_ok = true;
        LOG_DBG("  [PASS] Producer: sent message successfully");
    }
    else
    {
        result.producer_ok = false;
        result.producer_msg = "error code: " + std::to_string(result.producer_err);
        LOG_DBG("  [FAIL] Producer: %s", result.producer_msg.c_str());
    }

    // Test Consumer
    result.consumer_err = co_await test_consumer_version(ver);
    if (result.consumer_err == 0)
    {
        result.consumer_ok = true;
        LOG_DBG("  [PASS] Consumer: consumed message successfully");
    }
    else
    {
        result.consumer_ok = false;
        result.consumer_msg = "error code: " + std::to_string(result.consumer_err);
        LOG_DBG("  [FAIL] Consumer: %s", result.consumer_msg.c_str());
    }
}

void run_version_test()
{
    std::vector<VersionTestResult> results;
    results.resize(SupportedVersions.size());

    runnable::instance()
        .start(
            [&results]() -> awaitable<void>
            {
                LOG_DBG("=== Kafka Version Compatibility Test ===");
                LOG_DBG("Broker: %s:%d", test_host.c_str(), test_port);
                LOG_DBG("Topic: %s", test_topic.c_str());
                LOG_DBG("Total versions to test: %zu", SupportedVersions.size());

                for (size_t i = 0; i < SupportedVersions.size(); ++i)
                {
                    co_await test_single_version(SupportedVersions[i], results[i]);
                    LOG_DBG("\n");
                }

                // Print summary
                int total = 0;
                int producer_passed = 0;
                int consumer_passed = 0;
                int both_passed = 0;
                std::vector<std::string> failed_versions;

                for (const auto &r : results)
                {
                    total++;
                    if (r.producer_ok)
                        producer_passed++;
                    if (r.consumer_ok)
                        consumer_passed++;
                    if (r.producer_ok && r.consumer_ok)
                        both_passed++;
                    else
                        failed_versions.push_back(r.version.String());
                }

                LOG_DBG("=== Test Summary ===");
                LOG_DBG("Total versions tested: %d", total);
                LOG_DBG("Producer passed: %d (%.1f%%)", producer_passed,
                        total > 0 ? 100.0 * producer_passed / total : 0);
                LOG_DBG("Consumer passed: %d (%.1f%%)", consumer_passed,
                        total > 0 ? 100.0 * consumer_passed / total : 0);
                LOG_DBG("Both passed: %d (%.1f%%)", both_passed,
                        total > 0 ? 100.0 * both_passed / total : 0);

                if (!failed_versions.empty())
                {
                    LOG_DBG("Failed versions (%zu):", failed_versions.size());
                    std::string failed_str;
                    for (size_t i = 0; i < failed_versions.size(); ++i)
                    {
                        if (i > 0)
                            failed_str += ", ";
                        failed_str += failed_versions[i];
                    }
                    LOG_DBG("  %s", failed_str.c_str());
                }
                else
                {
                    LOG_DBG("All versions passed!");
                }

                co_return;
            })
        .end();
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        LOG_DBG("Usage: %s <host> <port> <topic>", argv[0]);
        return -1;
    }
    set_log_level(LOG_LEVEL_DEBUG);
    // set_log_level(LOG_LEVEL_CORE);

    test_host = argv[1];
    test_port = std::stoi(argv[2]);
    test_topic = argv[3];

    run_version_test();

    return 0;
}
