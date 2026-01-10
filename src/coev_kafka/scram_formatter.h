#pragma once

#include <vector>
#include <cstdint>
#include <memory>

#include "version.h"
#include "errors.h"

enum ScramMechanismType : int8_t
{
    UNKNOWN = 0,
    SCRAM_MECHANISM_SHA_256 = 1,
    SCRAM_MECHANISM_SHA_512 = 2
};

struct ScramFormatter
{
    ScramMechanismType m_mechanism;
    ScramFormatter(ScramMechanismType mech) : m_mechanism(mech) {}

    int Hmac(const std::string &key, const std::string &data, std::string &) const;
    int SaltedPassword(const std::string &password, const std::string &salt, int iterations, std::string &) const;

    std::string ComputeMac(const std::string &key, const std::string &data) const;
    void XorInPlace(std::string &result, const std::string &second) const;
};
