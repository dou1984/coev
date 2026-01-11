#pragma once

#include <chrono>

struct throttle_support
{
    virtual std::chrono::milliseconds throttle_time() const = 0;
};
