#include <gtest/gtest.h>
#include "init_producer_id_response.h"

TEST(InitProducerIdResponseTest, VersionCompatibility) {
    InitProducerIDResponse response;
    
    // Test all supported versions
    for (int16_t version = 0; version <= 2; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(InitProducerIdResponseTest, BasicFunctionality) {
    InitProducerIDResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}