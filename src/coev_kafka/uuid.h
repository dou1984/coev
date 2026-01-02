#pragma once
#include <array>
#include <string>
#include <cstdint>

struct Uuid
{
    std::array<uint8_t, 16> data;
    std::string String() const;
};