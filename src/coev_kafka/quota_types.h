#pragma once

#include <string>

using QuotaEntityType = std::string;

enum QuotaMatchType : int8_t
{
    QuotaMatchExact = 0,
    QuotaMatchDefault = 1,
    QuotaMatchAny = 2
};

const QuotaEntityType QuotaEntityUser = "user";
const QuotaEntityType QuotaEntityClientID = "client-id";
const QuotaEntityType QuotaEntityIP = "ip";
