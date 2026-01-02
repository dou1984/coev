#pragma once
#include <chrono>
#include <coev.h>

coev::awaitable<void> sleep_for(std::chrono::milliseconds);
coev::awaitable<void> sleep_for(std::chrono::seconds);