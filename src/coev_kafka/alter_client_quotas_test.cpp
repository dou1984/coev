#include <gtest/gtest.h>
#include "alter_client_quotas_request.h"
#include "alter_client_quotas_response.h"

TEST(AlterClientQuotasTest, RequestVersionCompatibility)
{
    AlterClientQuotasRequest request;
    for (int16_t version = 0; version <= 0; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(AlterClientQuotasTest, ResponseVersionCompatibility)
{
    AlterClientQuotasResponse response;
    for (int16_t version = 0; version <= 0; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AlterClientQuotasTest, BasicFunctionality)
{
    AlterClientQuotasRequest request;
    request.set_version(0);
    EXPECT_EQ(request.version(), 0);
    EXPECT_EQ(request.key(), 49);

    AlterClientQuotasResponse response;
    response.set_version(0);
    EXPECT_EQ(response.version(), 0);
    EXPECT_EQ(response.key(), 49);
}