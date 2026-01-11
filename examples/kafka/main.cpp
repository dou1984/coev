#include <coev/coev.h>
#include <coev_kafka/consumer.h>
#include <coev_kafka/message.h>
#include <coev_kafka/partition_consumer.h>

using namespace coev;

std::string host;
int port;
std::string topic;

int main(int argc, char **argv)
{

    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0] << " host port topic" << std::endl;
        return -1;
    }
    host = argv[1];
    port = std::stoi(argv[2]);
    topic = argv[3];

    runnable::instance()
        .start(
            []() -> awaitable<void>
            {
                std::shared_ptr<Config> conf = std::make_shared<Config>();

                std::shared_ptr<Consumer> consumer = std::make_shared<Consumer>();

                std::vector<std::string> addrs = {host + ":" + std::to_string(port)};

                auto err = co_await NewConsumer(addrs, conf, consumer);

                if (err)
                {
                    std::cout << "NewConsumer error: " << err << std::endl;
                    co_return;
                }

                std::vector<int32_t> partitions;

                err = co_await consumer->Partitions(topic, partitions);
                if (err)
                {
                    std::cout << "Partitions error: " << err << std::endl;
                    co_return;
                }

                co_task task;
                for (auto partition : partitions)
                {
                    task << [consumer, partition]() -> awaitable<void>
                    {
                        std::shared_ptr<PartitionConsumer> partition_consumer;
                        auto err = co_await consumer->ConsumePartition(topic, partition, OffsetNewest, partition_consumer);
                        if (err)
                        {
                            std::cout << "ConsumePartition error: " << err << std::endl;
                            co_return;
                        }
                        while (true)
                        {
                            auto msg = co_await partition_consumer->Messages();

                            if (msg)
                            {
                                LOG_DBG("%s %s \n", msg->Key().c_str(), msg->Value().c_str());
                            }
                        }
                    }();
                }
                co_await task.wait_all();

                LOG_DBG("all task done\n");
            })
        .wait();

    return 0;
}