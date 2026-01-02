#include "version.h"
#include "alter_configs_response.h"
#include "api_versions.h"

std::string AlterConfigError::Error() const
{
    std::string text = KErrorToString(Err);
    if (!ErrMsg.empty())
    {
        text = text + " - " + ErrMsg;
    }
    return text;
}

int AlterConfigsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (!pe.putArrayLength(static_cast<int32_t>(Resources.size())))
    {
        return ErrEncodeError;
    }

    for (auto &r : Resources)
    {
        if (!r->encode(pe))
        {
            return ErrEncodeError;
        }
    }

    return true;
}

int AlterConfigsResponse::decode(PDecoder &pd, int16_t version)
{
    if (!pd.getDurationMs(ThrottleTime))
    {
        return ErrDecodeError;
    }

    int32_t responseCount;
    if (!pd.getArrayLength(responseCount))
    {
        return ErrDecodeError;
    }

    Resources.resize(responseCount);
    for (int32_t i = 0; i < responseCount; ++i)
    {
        Resources[i] = std::make_shared<AlterConfigsResourceResponse>();
        if (!Resources[i]->decode(pd, version))
        {
            return ErrDecodeError;
        }
    }

    return true;
}

int AlterConfigsResourceResponse::encode(PEncoder &pe)
{
    pe.putInt16(ErrorCode);
    if (!pe.putString(ErrorMsg))
    {
        return ErrEncodeError;
    }
    pe.putInt8(static_cast<int8_t>(Type));
    if (!pe.putString(Name))
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return true;
}

int AlterConfigsResourceResponse::decode(PDecoder &pd, int16_t version)
{
    int16_t errCode;
    if (!pd.getInt16(errCode))
    {
        return ErrDecodeError;
    }
    ErrorCode = errCode;

    if (!pd.getNullableString(ErrorMsg))
    {
        return ErrDecodeError;
    }

    int8_t t;
    if (!pd.getInt8(t))
    {
        return ErrDecodeError;
    }
    Type = static_cast<ConfigResourceType>(t);

    if (!pd.getString(Name))
    {
        return ErrDecodeError;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

void AlterConfigsResponse::setVersion(int16_t v)
{
    Version = v;
}

int16_t AlterConfigsResponse::key() const
{
    return apiKeyAlterConfigs;
}

int16_t AlterConfigsResponse::version() const
{
    return Version;
}

int16_t AlterConfigsResponse::headerVersion() const
{
    return 0;
}

bool AlterConfigsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion AlterConfigsResponse::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    case 0:
        return V0_11_0_0;
    default:
        return V2_0_0_0;
    }
}

std::chrono::milliseconds AlterConfigsResponse::throttleTime() const
{
    return ThrottleTime;
}