#include "response_promise.h"

void ResponsePromise::Handle(std::string &packets_, KError err)
{
    if (Handler)
    {
        Handler(packets_, err);
        return;
    }
    if (err)
    {
        Errors.set(err);
        return;
    }

    Packets.set(std::move(packets_));
}
