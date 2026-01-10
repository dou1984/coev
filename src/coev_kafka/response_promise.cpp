#include "response_promise.h"

void ResponsePromise::Handle(std::string &packets_, KError err)
{
    if (m_handler)
    {
        m_handler(packets_, err);
        return;
    }
    if (err)
    {
        m_errors.set(err);
        return;
    }

    m_packets.set(std::move(packets_));
}
