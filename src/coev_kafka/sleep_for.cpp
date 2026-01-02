#include "sleep_for.h"

coev::awaitable<void> sleep_for(std::chrono::milliseconds ms)
{
    std::chrono::microseconds us = std::chrono::duration_cast<std::chrono::microseconds>(ms);
    return coev::usleep_for(us.count());
}
coev::awaitable<void> sleep_for(std::chrono::seconds s)
{
    return coev::sleep_for(s.count());
}