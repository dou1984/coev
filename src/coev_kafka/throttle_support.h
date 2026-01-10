#pragma once

#include <chrono>

struct throttle_support
{
    virtual std::chrono::milliseconds throttleTime() const = 0;
};
