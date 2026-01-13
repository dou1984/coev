#include "timestamp.h"
#include "errors.h"
#include <cstdint>

Timestamp::Timestamp() : time_(std::chrono::system_clock::time_point{})
{
}

Timestamp::Timestamp(const std::chrono::system_clock::time_point &t) : time_(t)
{
}

int Timestamp::encode(packetEncoder &pe)
{
    int64_t timestamp = -1;

    auto epoch = std::chrono::system_clock::from_time_t(0);
    if (time_ >= epoch)
    {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(time_.time_since_epoch());
        timestamp = static_cast<int64_t>(duration.count());
    }
    else if (time_ != std::chrono::system_clock::time_point{})
    {
        return ErrEncodeError;
    }

    pe.putInt64(timestamp);
    return ErrNoError;
}

int Timestamp::decode(packetDecoder &pd)
{
    int64_t millis;
    auto err = pd.getInt64(millis);
    if (err != ErrNoError)
    {
        return err;
    }

    std::chrono::system_clock::time_point timestamp{};
    if (millis >= 0)
    {
        auto seconds = millis / 1000;
        auto nanos = (millis % 1000) * 1000000; // convert ms to ns
        timestamp = std::chrono::system_clock::from_time_t(static_cast<time_t>(seconds));
        timestamp += std::chrono::nanoseconds(nanos);
    }

    time_ = timestamp;
    return ErrNoError;
}

std::chrono::system_clock::time_point Timestamp::get_time() const
{
    return time_;
}

void Timestamp::set_time(const std::chrono::system_clock::time_point &t)
{
    time_ = t;
}