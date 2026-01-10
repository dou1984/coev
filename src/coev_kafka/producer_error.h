#pragma once
#include <memory>
#include <vector>
#include "producer_message.h"
#include "errors.h"

struct ProducerError
{
    std::shared_ptr<ProducerMessage> m_msg;
    KError m_err;
};

using ProducerErrors = std::vector<ProducerError>;