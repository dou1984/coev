#include <gtest/gtest.h>
#include "alter_partition_reassignments_request.h"
#include "alter_partition_reassignments_response.h"

TEST(AlterPartitionReassignmentsTest, RequestVersionCompatibility)
{
    AlterPartitionReassignmentsRequest request;
    for (int16_t version = 0; version <= 0; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(AlterPartitionReassignmentsTest, ResponseVersionCompatibility)
{
    AlterPartitionReassignmentsResponse response;
    for (int16_t version = 0; version <= 0; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(AlterPartitionReassignmentsTest, BasicFunctionality)
{
    AlterPartitionReassignmentsRequest request;
    request.set_version(0);
    EXPECT_EQ(request.version(), 0);
    EXPECT_EQ(request.key(), 45);

    AlterPartitionReassignmentsResponse response;
    response.set_version(0);
    EXPECT_EQ(response.version(), 0);
    EXPECT_EQ(response.key(), 45);
}