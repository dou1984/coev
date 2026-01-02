#pragma once
#include <memory>
#include "consumer_group_session.h"
#include "consumer_group_claim.h"

struct ConsumerGroupHandler
{
    ConsumerGroupHandler() = default;
    virtual ~ConsumerGroupHandler() = default;
    int Setup(std::shared_ptr<IConsumerGroupSession>)
    {
        return 0;
    }

    int Cleanup(std::shared_ptr<IConsumerGroupSession>)
    {
        return 0;
    }

    int ConsumeClaim(std::shared_ptr<IConsumerGroupSession>, std::shared_ptr<IConsumerGroupClaim>)
    {
        return 0;
    }
};
