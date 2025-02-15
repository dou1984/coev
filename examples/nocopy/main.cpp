#include <coev/coev.h>
#include <cosys/cosys.h>

using namespace coev;

class nocopy
{
public:
    nocopy() = default;
    nocopy(nocopy &&) = default;

    nocopy(const nocopy &) = default;
    nocopy &operator=(const nocopy &) = default;
    nocopy &operator=(nocopy &&) = default;
    ~nocopy() = default;
};
awaitable<void> co_nocopy()
{
    auto f = []() -> awaitable<nocopy, nocopy>
    {
        nocopy a, b;
        co_return {a, b};
    };

    auto [x, y] = co_await f();

    co_return;
}
int main()
{
    coev::running::instance().add(co_nocopy).join();
    return 0;
}