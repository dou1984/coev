#include <coev/coev.h>
#include <cosys/cosys.h>

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
    auto f = []() -> awaitable<nocopy, nocopy>
    {
        nocopy a, b;
        co_return {std::move(a), std::move(b)};
    };

    auto [x, y] = co_await f();

    co_return;
}
int main()
{
    coev::running::instance().add(co_nocopy).join();
    return 0;
}