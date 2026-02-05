#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <cstring>
#include <stdexcept>
#include "scram_formatter.h"

namespace
{
    const EVP_MD *getDigest(ScramMechanismType mech)
    {
        switch (mech)
        {
        case ScramMechanismType::SCRAM_MECHANISM_SHA_256:
            return EVP_sha256();
        case ScramMechanismType::SCRAM_MECHANISM_SHA_512:
            return EVP_sha512();
        default:
            throw std::invalid_argument("Unknown SCRAM mechanism");
        }
    }

    std::string hmacInternal(const EVP_MD *md, const std::string &key, const std::string &data)
    {
        std::string result;
        result.resize(EVP_MAX_MD_SIZE);
        unsigned int len = 0;

        HMAC(md, key.data(), static_cast<int>(key.size()),
             (unsigned char *)data.data(), data.size(),
             (unsigned char *)result.data(), &len);

        result.resize(len);
        return result;
    }
}

int ScramFormatter::Hmac(const std::string &key, const std::string &data, std::string &output) const
{
    try
    {
        const EVP_MD *md = getDigest(m_mechanism);
        output = hmacInternal(md, key, data);
        return ErrNoError;
    }
    catch (...)
    {

        return ErrUnknownScramMechanism;
    }
}

std::string ScramFormatter::ComputeMac(const std::string &key, const std::string &data) const
{
    std::string result;
    Hmac(key, data, result);
    return result;
}

void ScramFormatter::XorInPlace(std::string &result,
                                const std::string &second) const
{
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] ^= second[i];
    }
}

int ScramFormatter::SaltedPassword(const std::string &password, const std::string &salt, int iterations, std::string &output) const
{
    if (iterations < 1)
    {
        throw std::invalid_argument("Iterations must be >= 1");
    }

    // U1 = HMAC(password, salt || INT32_BE(1))
    std::string saltWithCounter = salt;
    saltWithCounter.push_back(0x00);
    saltWithCounter.push_back(0x00);
    saltWithCounter.push_back(0x00);
    saltWithCounter.push_back(0x01); // big-endian 1

    std::string u1 = ComputeMac(password, saltWithCounter);
    output = u1;
    std::string prev = u1;
    for (int i = 2; i <= iterations; ++i)
    {
        std::string ui = ComputeMac(password, prev);
        XorInPlace(output, ui);
        prev = std::move(ui);
    }

    return ErrNoError;
}