/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_kafka/consumer.h>

using namespace coev;
using namespace coev::kafka;

std::string test_host;
int test_port;
std::string test_topic;
std::string test_data;

void run_consumer_test();

int main(int argc, char *argv[])
{
    if (argc != 4) {
        LOG_ERR("Usage: consumer_test <host> <port> <topic>");
        return 1;
    }
    test_host = argv[1];
    test_port = std::stoi(argv[2]);
    test_topic = argv[3];
    
    run_consumer_test();
    return 0;
}