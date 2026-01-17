#include <gtest/gtest.h>
#include "describe_client_quotas_request.h"
#include "describe_client_quotas_response.h"
#include "api_versions.h"

TEST(DescribeClientQuotasTest, RequestVersionCompatibility) {
    DescribeClientQuotasRequest request;
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DescribeClientQuotasTest, ResponseVersionCompatibility) {
    DescribeClientQuotasResponse response;
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DescribeClientQuotasTest, BasicFunctionality) {
    DescribeClientQuotasRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), apiKeyDescribeClientQuotas);
    
    DescribeClientQuotasResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), apiKeyDescribeClientQuotas);
}