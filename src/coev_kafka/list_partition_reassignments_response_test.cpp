#include <gtest/gtest.h>
#include "list_partition_reassignments_response.h"

TEST(ListPartitionReassignmentsResponseTest, VersionCompatibility)
{
    ListPartitionReassignmentsResponse response;

    // Test all supported versions
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ListPartitionReassignmentsResponseTest, BasicFunctionality)
{
    ListPartitionReassignmentsResponse response;
    response.set_version(1);
    EXPECT_EQ(1, response.version());
}