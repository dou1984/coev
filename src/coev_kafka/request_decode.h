/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <memory>
#include <coev/coev.h>
#include "broker.h"
#include "request.h"
#include "protocol_body.h"

namespace coev::kafka
{
    awaitable<int> request_decode(std::shared_ptr<Broker> &broker, Request &req, int &size);
    std::shared_ptr<protocol_body> request_allocate(int16_t key, int16_t version);
}