/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <memory>
#include <vector>
#include "producer_message.h"
#include "errors.h"

using ProducerError = ProducerMessage;
using ProducerErrors = std::vector<ProducerError>;