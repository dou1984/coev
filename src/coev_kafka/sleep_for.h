#pragma once
#include <chrono>
#include <coev/coev.h>

coev::awaitable<void> sleep_for(std::chrono::milliseconds);
coev::awaitable<void> sleep_for(std::chrono::seconds);