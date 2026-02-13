#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <chrono>

#include "acl_create_response.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Test data from Sarama's acl_create_response_test.go
const unsigned char createResponseWithError[] = {
    0x00, 0x00, 0x00, 0x64,             // Throttle time: 100 ms
    0x00, 0x00, 0x00, 0x01,             // Number of ACL creation responses: 1
    0x00, 0x2A,                         // Error: InvalidRequest (42)
    0x00, 0x05, 'e', 'r', 'r', 'o', 'r' // Error message: "error"
};

const unsigned char createResponseArray[] = {
    0x00, 0x00, 0x00, 0x64,              // Throttle time: 100 ms
    0x00, 0x00, 0x00, 0x02,              // Number of ACL creation responses: 2
    0x00, 0x2A,                          // Error: InvalidRequest (42)
    0x00, 0x05, 'e', 'r', 'r', 'o', 'r', // Error message: "error"
    0x00, 0x00,                          // No error
    0xFF, 0xFF                           // No error message
};

TEST(CreateAclsResponseTest, DecodeWithError) {
    std::string rawData(reinterpret_cast<const char *>(createResponseWithError), sizeof(createResponseWithError));
    real_decoder decoder(rawData);

    CreateAclsResponse response;
    int result = response.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode create acls response with error";
    EXPECT_EQ(response.m_throttle_time.count(), 100) << "Throttle time mismatch";
    EXPECT_EQ(response.m_acl_creation_responses.size(), 1) << "ACL creation responses count mismatch";

    auto aclResp = response.m_acl_creation_responses[0];
    EXPECT_EQ(aclResp.m_err, KError::ErrInvalidRequest) << "ACL creation response error mismatch";
    EXPECT_EQ(aclResp.m_err_msg, "error") << "ACL creation response error message mismatch";
}

TEST(CreateAclsResponseTest, DecodeResponseArray) {
    std::string rawData(reinterpret_cast<const char *>(createResponseArray), sizeof(createResponseArray));
    real_decoder decoder(rawData);

    CreateAclsResponse response;
    int result = response.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode create acls response array";
    EXPECT_EQ(response.m_throttle_time.count(), 100) << "Throttle time mismatch";
    EXPECT_EQ(response.m_acl_creation_responses.size(), 2) << "ACL creation responses count mismatch";

    // First response with error
    auto aclResp1 = response.m_acl_creation_responses[0];
    EXPECT_EQ(aclResp1.m_err, KError::ErrInvalidRequest) << "First ACL creation response error mismatch";
    EXPECT_EQ(aclResp1.m_err_msg, "error") << "First ACL creation response error message mismatch";

    // Second response without error
    auto aclResp2 = response.m_acl_creation_responses[1];
    EXPECT_EQ(aclResp2.m_err, KError::ErrNoError) << "Second ACL creation response error mismatch";
    EXPECT_TRUE(aclResp2.m_err_msg.empty()) << "Second ACL creation response error message should be empty";
}

TEST(CreateAclsResponseTest, EncodeWithError)
{
    CreateAclsResponse response;
    response.set_version(0);
    response.m_throttle_time = std::chrono::milliseconds(100);

    AclCreationResponse aclResp;
    aclResp.m_err = KError::ErrInvalidRequest;
    aclResp.m_err_msg = "error";
    response.m_acl_creation_responses.push_back(aclResp);

    real_encoder encoder(1024);
    int result = response.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode create acls response with error";

    std::string expectedData(reinterpret_cast<const char *>(createResponseWithError), sizeof(createResponseWithError));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for create acls response with error";
}

TEST(CreateAclsResponseTest, EncodeResponseArray)
{
    CreateAclsResponse response;
    response.set_version(0);
    response.m_throttle_time = std::chrono::milliseconds(100);

    // First response with error
    AclCreationResponse aclResp1;
    aclResp1.m_err = KError::ErrInvalidRequest;
    aclResp1.m_err_msg = "error";
    response.m_acl_creation_responses.push_back(aclResp1);

    // Second response without error
    AclCreationResponse aclResp2;
    aclResp2.m_err = KError::ErrNoError;
    response.m_acl_creation_responses.push_back(aclResp2);

    real_encoder encoder(1024);
    int result = response.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode create acls response array";

    std::string expectedData(reinterpret_cast<const char *>(createResponseArray), sizeof(createResponseArray));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for create acls response array";
}

TEST(CreateAclsResponseTest, VersionProperties)
{
    CreateAclsResponse response;

    // Test version 0
    response.set_version(0);
    EXPECT_EQ(response.version(), 0) << "Version not properly set";
    EXPECT_EQ(response.key(), 30) << "Key mismatch for version 0";
    EXPECT_EQ(response.header_version(), 0) << "Header version mismatch for version 0";
    EXPECT_TRUE(response.is_valid_version()) << "Version 0 should be valid";

    // Test version 1
    response.set_version(1);
    EXPECT_EQ(response.version(), 1) << "Version not properly set";
    EXPECT_EQ(response.key(), 30) << "Key mismatch for version 1";
    EXPECT_EQ(response.header_version(), 0) << "Header version mismatch for version 1";
    EXPECT_TRUE(response.is_valid_version()) << "Version 1 should be valid";

    // Test invalid version
    response.set_version(2);
    EXPECT_EQ(response.version(), 2) << "Version not properly set";
    EXPECT_FALSE(response.is_valid_version()) << "Version 2 should be invalid";
}

TEST(CreateAclsResponseTest, RoundTripEncodingDecoding)
{
    // Create original response with multiple ACL creation responses
    CreateAclsResponse originalResponse;
    originalResponse.set_version(0);
    originalResponse.m_throttle_time = std::chrono::milliseconds(100);

    // First response with error
    AclCreationResponse aclResp1;
    aclResp1.m_err = KError::ErrInvalidTopic;
    aclResp1.m_err_msg = "Invalid topic error";
    originalResponse.m_acl_creation_responses.push_back(aclResp1);

    // Second response with different error
    AclCreationResponse aclResp2;
    aclResp2.m_err = KError::ErrTopicAuthorizationFailed;
    aclResp2.m_err_msg = "Topic authorization failed";
    originalResponse.m_acl_creation_responses.push_back(aclResp2);

    // Third response without error
    AclCreationResponse aclResp3;
    aclResp3.m_err = KError::ErrNoError;
    originalResponse.m_acl_creation_responses.push_back(aclResp3);

    // Encode the response
    real_encoder encoder(1024);
    int result = originalResponse.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode response for round-trip test";

    // Decode the response
    std::string encodedData = encoder.m_raw.substr(0, encoder.m_offset);
    real_decoder decoder(encodedData);

    CreateAclsResponse decodedResponse;
    result = decodedResponse.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode response for round-trip test";

    // Verify the decoded response matches the original
    EXPECT_EQ(decodedResponse.m_throttle_time, originalResponse.m_throttle_time) << "Throttle time mismatch in round-trip";
    EXPECT_EQ(decodedResponse.m_acl_creation_responses.size(), originalResponse.m_acl_creation_responses.size()) << "ACL creation responses count mismatch in round-trip";

    for (size_t i = 0; i < originalResponse.m_acl_creation_responses.size(); ++i)
    {
        auto originalAclResp = originalResponse.m_acl_creation_responses[i];
        auto decodedAclResp = decodedResponse.m_acl_creation_responses[i];

        EXPECT_EQ(decodedAclResp.m_err, originalAclResp.m_err) << "ACL creation response error mismatch in round-trip, index: " << i;
        EXPECT_EQ(decodedAclResp.m_err_msg, originalAclResp.m_err_msg) << "ACL creation response error message mismatch in round-trip, index: " << i;
    }
}
