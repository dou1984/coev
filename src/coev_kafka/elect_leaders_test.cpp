#include <gtest/gtest.h>
#include "elect_leaders_request.h"
#include "elect_leaders_response.h"

TEST(ElectLeadersTest, RequestVersionCompatibility) {
    ElectLeadersRequest request;
    for (int16_t version = 0; version <= 1; version++) {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ElectLeadersTest, ResponseVersionCompatibility) {
    ElectLeadersResponse response;
    for (int16_t version = 0; version <= 1; version++) {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ElectLeadersTest, BasicFunctionality) {
    ElectLeadersRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), 43);
    
    ElectLeadersResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), 43);
}