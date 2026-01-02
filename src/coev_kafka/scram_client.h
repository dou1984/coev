#pragma once

#include <string>
#include <coev.h>
#include "errors.h"

struct SCRAMClient
{

    virtual ~SCRAMClient() = default;
    virtual int Begin(const std::string &userName, const std::string &password, const std::string &authzID) = 0;
    virtual int Step(const std::string &challenge, std::string &response) = 0;
    virtual bool Done() = 0;
};