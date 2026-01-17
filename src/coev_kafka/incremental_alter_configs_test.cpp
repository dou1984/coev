#include <gtest/gtest.h>
#include "incremental_alter_configs_request.h"
#include "incremental_alter_configs_response.h"

TEST(IncrementalAlterConfigsTest, RequestVersionCompatibility) {
    IncrementalAlterConfigsRequest request;
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(IncrementalAlterConfigsTest, ResponseVersionCompatibility) {
    IncrementalAlterConfigsResponse response;
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(IncrementalAlterConfigsTest, BasicFunctionality) {
    IncrementalAlterConfigsRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), 44);
    
    IncrementalAlterConfigsResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), 44);
}