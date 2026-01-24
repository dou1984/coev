#include <coev/coev.h>
#include <coev_kafka/consumer.h>
#include <coev_kafka/message.h>
#include <coev_kafka/partition_consumer.h>
#include <coev_kafka/async_producer.h>
#include <coev_kafka/partitioner.h>

using namespace coev;

std::string method;
std::string host;
int port;
std::string topic;
std::string data;

int main(int argc, char **argv)
{

    if (argc < 5)
    {
        std::cout << "Usage: " << argv[0] << "(pull|push) host port topic" << std::endl;
        return -1;
    }
    set_log_level(LOG_LEVEL_CORE);
    method = argv[1];
    host = argv[2];
    port = std::stoi(argv[3]);
    topic = argv[4];
    if (argc == 6)
    {
        data = argv[5];
    }

    if (method == "pull")
    {
        runnable::instance()
            .start(
                []() -> awaitable<void>
                {
                    std::shared_ptr<Config> conf = std::make_shared<Config>();

                    std::shared_ptr<Consumer> consumer;

                    std::vector<std::string> addrs = {host + ":" + std::to_string(port)};

                    auto err = co_await NewConsumer(addrs, conf, consumer);

                    if (err)
                    {
                        LOG_ERR("NewConsumer error: %d", err);
                        co_return;
                    }

                    std::vector<int32_t> partitions;
                    err = co_await consumer->Partitions(topic, partitions);
                    if (err)
                    {
                        LOG_ERR("Partitions error: %d", err);
                        co_return;
                    }

                    co_task task;
                    for (auto partition : partitions)
                    {
                        task << [](auto consumer, auto partition) -> awaitable<void>
                        {
                            std::shared_ptr<PartitionConsumer> partition_consumer;
                            auto err = co_await consumer->ConsumePartition(topic, partition, OffsetNewest, partition_consumer);
                            if (err)
                            {
                                LOG_ERR("ConsumePartition error: %d", err);
                                co_return;
                            }
                            while (true)
                            {
                                auto msg = co_await partition_consumer->m_messages;
                                if (msg)
                                {
                                    LOG_DBG("%s %s", msg->key().c_str(), msg->value().c_str());
                                }
                            }
                        }(consumer, partition);
                    }
                    co_await task.wait_all();
                    LOG_DBG("all task done");
                })
            .wait();
    }
    else if (method == "push")
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

                    std::vector<std::string> addrs = {host + ":" + std::to_string(port)};

                    std::shared_ptr<AsyncProducer> producer;

                    auto err = co_await NewAsyncProducer(addrs, conf, producer);

                    if (err)
                    {
                        std::cout << "NewAsyncProducer error: " << err << std::endl;
                        co_return;
                    }
                    defer(producer->async_close());

                    auto msg = std::make_shared<ProducerMessage>();
                    msg->m_topic = topic;
                    msg->m_key = std::make_shared<StringEncoder>("key");
                    msg->m_value = std::make_shared<ByteEncoder>("hello world");

                    co_await producer->dispatcher(msg);

                    int64_t offset = 0;
                    co_task task;
                    auto succ = task << [&]() -> awaitable<void>
                    {
                        auto suc_msg = co_await producer->m_successes.get();
                        offset = suc_msg->m_offset;

                        LOG_DBG("produced message at offset %ld", offset);
                    }();
                    auto fail = task << [&]() -> awaitable<void>
                    {
                        auto fail_msg = co_await producer->m_errors.get();
                        LOG_ERR("fail: %d", fail_msg->m_err);
                        err = fail_msg->m_err;
                    }();

                    co_await task.wait();
                })

            .wait();
    }

    return 0;
}