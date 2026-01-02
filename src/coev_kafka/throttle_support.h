#pragma once

#include <chrono>

struct throttleSupport
{
    virtual std::chrono::milliseconds throttleTime() const = 0;
};
