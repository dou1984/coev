#include <gtest/gtest.h>
#include "produce_request.h"
#include "produce_response.h"

TEST(ProduceSetTest, RequestVersionCompatibility) {
    ProduceRequest request;
    
    // Test different versions
    for (int16_t version = 0; version <= 7; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ProduceSetTest, ResponseVersionCompatibility) {
    ProduceResponse response;
    
    // Test different versions
    for (int16_t version = 0; version <= 7; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ProduceSetTest, BasicFunctionality) {
    ProduceRequest request;
    request.set_version(7);
    EXPECT_EQ(7, request.version());
    
    ProduceResponse response;
    response.set_version(7);
    EXPECT_EQ(7, response.version());
}