#pragma once
#include <memory>
#include <coev/coev.h>
#include "broker.h"
#include "request.h"
#include "protocol_body.h"

coev::awaitable<int> request_decode(std::shared_ptr<Broker> &broker, Request &req, int &size);
std::shared_ptr<protocol_body> allocate_body(int16_t key, int16_t version);