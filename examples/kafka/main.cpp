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
#include <coev_kafka/async_producer.h>
#include <coev_kafka/partitioner.h>
#include <coev_kafka/admin.h>
#include <coev_kafka/client.h>
#include <cstring>
#include <cstdlib>
#include <iostream>

using namespace coev;
using namespace coev::kafka;

std::string test_host;
int test_port;
std::string test_topic;
std::string test_data;

void run_consumer_test();
void run_producer_test();

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        LOG_DBG("Usage: %s (pull|push) host port topic", argv[0]);
        return -1;
    }
    set_log_level(LOG_LEVEL_DEBUG);
    std::string method = argv[1];
    test_host = argv[2];
    test_port = std::stoi(argv[3]);
    test_topic = argv[4];
    if (argc == 6)
    {
        test_data = argv[5];
    }

    if (method == "pull")
    {
        run_consumer_test();
    }
    else if (method == "push")
    {
        run_producer_test();
    }
    else
    {
        LOG_ERR("Unknown method: %s", method.c_str());
        return -1;
    }

    return 0;
}
