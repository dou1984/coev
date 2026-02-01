#pragma once
#include <memory>
#include "consumer_group_session.h"
#include "consumer_group_claim.h"

struct ConsumerGroupHandler
{
    ConsumerGroupHandler() = default;
    virtual ~ConsumerGroupHandler() = default;
    int setup(std::shared_ptr<ConsumerGroupSession>)
    {
        return 0;
    }

    int cleanup(std::shared_ptr<ConsumerGroupSession>)
    {
        return 0;
    }

    int consume_claim(std::shared_ptr<ConsumerGroupSession>, std::shared_ptr<ConsumerGroupClaim>)
    {
        return 0;
    }
};
