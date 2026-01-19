#include <gtest/gtest.h>
#include "list_partition_reassignments_request.h"
#include "list_partition_reassignments_response.h"

TEST(ListPartitionReassignmentsTest, RequestVersionCompatibility)
{
    ListPartitionReassignmentsRequest request;
    for (int16_t version = 0; version <= 1; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(ListPartitionReassignmentsTest, ResponseVersionCompatibility)
{
    ListPartitionReassignmentsResponse response;
    for (int16_t version = 0; version <= 1; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(ListPartitionReassignmentsTest, BasicFunctionality)
{
    ListPartitionReassignmentsRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), 46);

    ListPartitionReassignmentsResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), 46);
}