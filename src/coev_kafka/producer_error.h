#pragma once
#include <memory>
#include <vector>
#include "producer_message.h"
#include "errors.h"

struct ProducerError
{
    std::shared_ptr<ProducerMessage> Msg;
    KError Err;
};

using ProducerErrors = std::vector<ProducerError>;