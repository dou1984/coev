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
#include <coev_kafka/version.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include "supported_versions.h"
using namespace coev;
using namespace coev::kafka;

extern std::string test_host;
extern int test_port;
extern std::string test_topic;
struct ProducerVersionResult
{
    KafkaVersion version;
    bool ok = false;
    uint64_t messages = 0;
    uint64_t successes = 0;
    uint64_t failures = 0;
    uint64_t time_ms = 0;
    double qps = 0;
    int err = 0;
};

static awaitable<ProducerVersionResult> test_producer_version(const KafkaVersion &ver)
{
    ProducerVersionResult result;
    result.version = ver;

    std::shared_ptr<Config> conf = std::make_shared<Config>();
    conf->Version = ver;
    conf->Producer.Acks = WaitForAll;
    conf->Producer.Return.Successes = true;
    conf->Producer.Return.Errors = true;
    conf->Producer.Partitioner = NewRandomPartitioner;

    std::vector<std::string> addrs = {test_host + ":" + std::to_string(test_port)};

    std::shared_ptr<AsyncProducer> producer;
    auto err = co_await NewAsyncProducer(addrs, conf, producer);
    if (err)
    {
        result.err = err;
        co_return result;
    }
    finally(producer->async_close());

    uint64_t max_messages = 1000;
    result.messages = max_messages;
    auto start_time = std::chrono::steady_clock::now();

    for (uint64_t i = 0; i < max_messages; i++)
    {
        auto msg = std::make_shared<ProducerMessage>();
        msg->m_topic = test_topic;
        msg->m_key.m_data = "version-test-key";
        msg->m_value.m_data = "version-test-value-" + ver.String() + "-" + std::to_string(i);

        std::shared_ptr<ProducerMessage> reply;
        err = co_await producer->producer(msg, reply);
        if (err == 0 && reply && reply->m_err == ErrNoError)
        {
            result.successes++;
        }
        else
        {
            result.failures++;
            // Record the first error code
            if (result.err == 0 && reply)
            {
                result.err = reply->m_err;
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    result.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    result.qps = result.time_ms > 0 ? result.successes * 1000.0 / result.time_ms : 0;
    result.ok = result.failures == 0;

    co_return result;
}

void run_producer_test()
{
    runnable::instance()
        .start(
            []() -> awaitable<void>
            {
                std::cout << "=== Kafka Producer Version Test ===" << std::endl;
                std::cout << "Broker: " << test_host << ":" << test_port << std::endl;
                std::cout << "Topic: " << test_topic << std::endl;
                std::cout << "Total versions to test: " << SupportedVersions().size() << std::endl;
                std::cout << "========================================" << std::endl;

                std::vector<ProducerVersionResult> results;
                results.reserve(SupportedVersions().size());

                for (size_t i = 0; i < SupportedVersions().size(); ++i)
                {
                    const auto &ver = SupportedVersions()[i];
                    std::cout << "[" << i + 1 << "/" << SupportedVersions().size()
                              << "] Testing version: " << ver.String() << std::endl;

                    auto result = co_await test_producer_version(ver);
                    results.push_back(result);

                    if (result.ok)
                    {
                        std::cout << "  [PASS] Producer: " << result.successes
                                  << " messages, " << result.time_ms
                                  << " ms, QPS: " << std::fixed << std::setprecision(2) << result.qps << std::endl;
                    }
                    else
                    {
                        std::cout << "  [FAIL] Producer: error code " << result.err << std::endl;
                    }
                }

                // Print summary
                int total = results.size();
                int passed = 0;
                std::vector<std::string> failed_versions;

                for (const auto &r : results)
                {
                    if (r.ok)
                        passed++;
                    else
                        failed_versions.push_back(r.version.String());
                }

                std::cout << "\n=== Test Summary ===" << std::endl;
                std::cout << "Total versions tested: " << total << std::endl;
                std::cout << "Producer passed: " << passed << " (" << std::fixed << std::setprecision(1)
                          << (total > 0 ? 100.0 * passed / total : 0) << "%)" << std::endl;

                if (!failed_versions.empty())
                {
                    std::cout << "\nFailed versions (" << failed_versions.size() << "):" << std::endl;
                    for (size_t i = 0; i < failed_versions.size(); ++i)
                    {
                        if (i > 0)
                            std::cout << ", ";
                        std::cout << failed_versions[i];
                    }
                    std::cout << std::endl;
                }
                else
                {
                    std::cout << "\nAll versions passed!" << std::endl;
                }

                // Print detailed QPS table
                std::cout << "\n=== Detailed Results ===" << std::endl;
                std::cout << std::left << std::setw(12) << "Version"
                          << " | " << std::right << std::setw(8) << "Msgs"
                          << " | " << std::right << std::setw(10) << "Time(ms)"
                          << " | " << std::right << std::setw(10) << "QPS"
                          << " | " << std::right << std::setw(6) << "Status" << std::endl;
                std::cout << std::string(60, '-') << std::endl;

                for (const auto &r : results)
                {
                    std::cout << std::left << std::setw(12) << r.version.String()
                              << " | " << std::right << std::setw(8) << r.successes
                              << " | " << std::right << std::setw(10) << r.time_ms
                              << " | " << std::right << std::setw(10) << std::fixed << std::setprecision(2) << r.qps
                              << " | " << std::right << std::setw(6) << (r.ok ? "PASS" : "FAIL") << std::endl;
                }

                co_return;
            })
        .end();
}
