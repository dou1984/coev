#pragma once
#include <memory>
#include <vector>
#include "producer_message.h"
#include "errors.h"

// struct ProducerError : ProducerMessage
// {
//     KError m_err;
// };
using ProducerError = ProducerMessage;

using ProducerErrors = std::vector<ProducerError>;