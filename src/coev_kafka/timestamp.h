// timestamp.h
#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <coev/coev.h>
#include "packet_encoder.h"
#include "packet_decoder.h"

struct Timestamp
{

    Timestamp();
    Timestamp(const std::chrono::system_clock::time_point &t);

    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd);

    std::chrono::system_clock::time_point get_time() const;
    void set_time(const std::chrono::system_clock::time_point &t);

    std::chrono::system_clock::time_point time_;
};