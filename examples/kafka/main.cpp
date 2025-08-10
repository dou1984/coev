#include <coev/coev.h>
#include <coev_kafka/KafkaCli.h>
#include <coev_kafka/KafkaResult.h>

using namespace coev;

std::string host;
int port;
std::string topic;

int main(int argc, char **argv)
{

    // kafkacli host port topic
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
                KafkaCli cli;
                auto fd = co_await cli.connect(host.c_str(), port);
                if (fd == INVALID)
                {
                    co_return;
                }                

                co_await cli.fetch_api_versions();

                auto config = std::make_shared<kafka_config_t>();
                config->set_compress_type(Kafka_NoCompress);
                config->set_client_id("coev");

                auto record = std::make_shared<kafka_record_t>();

                record->set_key("hello");
                record->set_value("world");
                record->add_header_pair("hi", "everyone");

                co_await cli.produce_message(topic, -1, record);

                co_await sleep_for(1000);
            })
        .join();

    return 0;
}