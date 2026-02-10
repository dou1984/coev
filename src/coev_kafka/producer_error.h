#pragma once
#include <memory>
#include <vector>
#include "producer_message.h"
#include "errors.h"

using ProducerError = ProducerMessage;
using ProducerErrors = std::vector<ProducerError>;