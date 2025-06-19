#include <coev/coev.h>
#include <coev_kafka/co_kafka.h>

using namespace coev;
int main(int argc, char **argv)
{

    // kafkacli host port topic

    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0] << " host port topic" << std::endl;
        return -1;
    }
    std::string host = argv[1];
    int port = std::stoi(argv[2]);
    std::string topic = argv[3];

    kafka::KafkaCli cli;

    runnable::instance()
        .start(
            [&]() -> awaitable<void>
            {
                co_await cli.connect(host, port);

                co_await sleep_for(100);
            })
        .join();

    return 0;
}