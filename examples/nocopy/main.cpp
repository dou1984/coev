#include <coev/coev.h>

using namespace coev;

class nocopy
{
public:
    nocopy() = default;
    nocopy(nocopy &&) = default;

    nocopy(const nocopy &) = delete;
    nocopy &operator=(const nocopy &) = delete;
    nocopy &operator=(nocopy &&) = default;
    ~nocopy() = default;
};

awaitable<void> co_nocopy()
{
    auto f = []() -> awaitable<nocopy>
    {
        nocopy a;
        co_return std::move(a);
    };

    auto x = co_await f();

    co_return;
}
int main()
{
    coev::runnable::instance()
        .start(co_nocopy)
        .join();
    return 0;
}