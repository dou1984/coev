#include <gtest/gtest.h>
#include "delete_records_request.h"
#include "delete_records_response.h"
#include "api_versions.h"

TEST(DeleteRecordsTest, RequestVersionCompatibility)
{
    DeleteRecordsRequest request;
    for (int16_t version = 0; version <= 2; version++)
    {
        request.set_version(version);
        EXPECT_TRUE(request.is_valid_version());
    }
}

TEST(DeleteRecordsTest, ResponseVersionCompatibility)
{
    DeleteRecordsResponse response;
    for (int16_t version = 0; version <= 2; version++)
    {
        response.set_version(version);
        EXPECT_TRUE(response.is_valid_version());
    }
}

TEST(DeleteRecordsTest, BasicFunctionality)
{
    DeleteRecordsRequest request;
    request.set_version(1);
    EXPECT_EQ(request.version(), 1);
    EXPECT_EQ(request.key(), apiKeyDeleteRecords);

    DeleteRecordsResponse response;
    response.set_version(1);
    EXPECT_EQ(response.version(), 1);
    EXPECT_EQ(response.key(), apiKeyDeleteRecords);
}