#include <gtest/gtest.h>
#include "alter_configs_request.h"
#include "alter_configs_response.h"

TEST(AlterConfigsTest, RequestVersionCompatibility) {
    AlterConfigsRequest request;
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(AlterConfigsTest, ResponseVersionCompatibility) {
    AlterConfigsResponse response;
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AlterConfigsTest, BasicFunctionality) {
    AlterConfigsRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), 33);
    
    AlterConfigsResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), 33);
}