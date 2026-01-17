#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <chrono>

#include "acl_delete_response.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Test data from Sarama's acl_delete_response_test.go
const unsigned char deleteAclsResponse[] = {
    0x00, 0x00, 0x00, 0x64,  // Throttle time: 100 ms
    0x00, 0x00, 0x00, 0x01,  // Number of filter responses
    0x00, 0x00,              // No error
    0xFF, 0xFF,              // No error message
    0x00, 0x00, 0x00, 0x01,  // 1 matching ACL
    0x00, 0x00,              // No error
    0xFF, 0xFF,              // No error message
    0x02,                    // Resource type: Topic
    0x00, 0x05, 't', 'o', 'p', 'i', 'c',  // Resource name: "topic"
    0x00, 0x09, 'p', 'r', 'i', 'n', 'c', 'i', 'p', 'a', 'l',  // Principal: "principal"
    0x00, 0x04, 'h', 'o', 's', 't',  // Host: "host"
    0x04,                    // Operation: Write
    0x03                     // PermissionType: Allow
};

TEST(DeleteAclsResponseTest, DecodeResponse) {
    realDecoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(deleteAclsResponse), sizeof(deleteAclsResponse));
    decoder.m_offset = 0;
    
    DeleteAclsResponse response;
    int result = response.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode delete acls response";
    EXPECT_EQ(response.m_throttle_time.count(), 100) << "Throttle time mismatch";
    EXPECT_EQ(response.m_filter_responses.size(), 1) << "Filter responses count mismatch";
    
    auto filterResp = response.m_filter_responses[0];
    EXPECT_EQ(filterResp->m_err, KError::ErrNoError) << "Filter response error mismatch";
    EXPECT_TRUE(filterResp->m_err_msg.empty()) << "Filter response error message should be empty";
    EXPECT_EQ(filterResp->m_matching_acls.size(), 1) << "Matching ACLs count mismatch";
    
    auto matchingAcl = filterResp->m_matching_acls[0];
    EXPECT_EQ(matchingAcl->m_err, KError::ErrNoError) << "Matching ACL error mismatch";
    EXPECT_TRUE(matchingAcl->m_err_msg.empty()) << "Matching ACL error message should be empty";
    EXPECT_EQ(matchingAcl->m_resource.m_resource_type, AclResourceTypeTopic) << "Resource type mismatch";
    EXPECT_EQ(matchingAcl->m_resource.m_resource_name, "topic") << "Resource name mismatch";
    EXPECT_EQ(matchingAcl->m_acl.m_principal, "principal") << "Principal mismatch";
    EXPECT_EQ(matchingAcl->m_acl.m_host, "host") << "Host mismatch";
    EXPECT_EQ(matchingAcl->m_acl.m_operation, AclOperationWrite) << "Operation mismatch";
    EXPECT_EQ(matchingAcl->m_acl.m_permission_type, AclPermissionTypeAllow) << "PermissionType mismatch";
}

TEST(DeleteAclsResponseTest, EncodeResponse) {
    DeleteAclsResponse response;
    response.set_version(0);
    response.m_throttle_time = std::chrono::milliseconds(100);
    
    // Create filter response
    auto filterResp = std::make_shared<FilterResponse>();
    response.m_filter_responses.push_back(filterResp);
    
    // Create matching ACL
    auto matchingAcl = std::make_shared<MatchingAcl>();
    matchingAcl->m_resource.m_resource_type = AclResourceTypeTopic;
    matchingAcl->m_resource.m_resource_name = "topic";
    matchingAcl->m_acl.m_principal = "principal";
    matchingAcl->m_acl.m_host = "host";
    matchingAcl->m_acl.m_operation = AclOperationWrite;
    matchingAcl->m_acl.m_permission_type = AclPermissionTypeAllow;
    filterResp->m_matching_acls.push_back(matchingAcl);
    
    realEncoder encoder(1024);
    int result = response.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode delete acls response";
    
    std::string expectedData(reinterpret_cast<const char*>(deleteAclsResponse), sizeof(deleteAclsResponse));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for delete acls response";
}

TEST(DeleteAclsResponseTest, VersionProperties) {
    DeleteAclsResponse response;
    
    // Test version 0
    response.set_version(0);
    EXPECT_EQ(response.version(), 0) << "Version not properly set";
    EXPECT_EQ(response.key(), 31) << "Key mismatch for version 0";
    EXPECT_EQ(response.header_version(), 0) << "Header version mismatch for version 0";
    EXPECT_TRUE(response.is_valid_version()) << "Version 0 should be valid";
    
    // Test valid version 1
    response.set_version(1);
    EXPECT_EQ(response.version(), 1) << "Version not properly set";
    EXPECT_TRUE(response.is_valid_version()) << "Version 1 should be valid";
    EXPECT_EQ(response.key(), 31) << "Key mismatch for version 1";
    EXPECT_EQ(response.header_version(), 0) << "Header version mismatch for version 1";
    
    // Test invalid version
    response.set_version(2);
    EXPECT_EQ(response.version(), 2) << "Version not properly set";
    EXPECT_FALSE(response.is_valid_version()) << "Version 2 should be invalid";
}

TEST(DeleteAclsResponseTest, RoundTripEncodingDecoding) {
    // Create original response with multiple filter responses and matching ACLs
    DeleteAclsResponse originalResponse;
    originalResponse.set_version(0);
    originalResponse.m_throttle_time = std::chrono::milliseconds(100);
    
    // First filter response
    auto filterResp1 = std::make_shared<FilterResponse>();
    originalResponse.m_filter_responses.push_back(filterResp1);
    
    // Matching ACLs for first filter response
    auto matchingAcl1 = std::make_shared<MatchingAcl>();
    matchingAcl1->m_resource.m_resource_type = AclResourceTypeTopic;
    matchingAcl1->m_resource.m_resource_name = "test_topic";
    matchingAcl1->m_acl.m_principal = "user:test1";
    matchingAcl1->m_acl.m_host = "localhost";
    matchingAcl1->m_acl.m_operation = AclOperationRead;
    matchingAcl1->m_acl.m_permission_type = AclPermissionTypeAllow;
    filterResp1->m_matching_acls.push_back(matchingAcl1);
    
    auto matchingAcl2 = std::make_shared<MatchingAcl>();
    matchingAcl2->m_resource.m_resource_type = AclResourceTypeGroup;
    matchingAcl2->m_resource.m_resource_name = "test_group";
    matchingAcl2->m_acl.m_principal = "user:test2";
    matchingAcl2->m_acl.m_host = "127.0.0.1";
    matchingAcl2->m_acl.m_operation = AclOperationWrite;
    matchingAcl2->m_acl.m_permission_type = AclPermissionTypeDeny;
    filterResp1->m_matching_acls.push_back(matchingAcl2);
    
    // Second filter response with error
    auto filterResp2 = std::make_shared<FilterResponse>();
    filterResp2->m_err = KError::ErrInvalidTopic;
    filterResp2->m_err_msg = "Invalid topic";
    originalResponse.m_filter_responses.push_back(filterResp2);
    
    // Encode the response
    realEncoder encoder(1024);
    int result = originalResponse.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode response for round-trip test";
    
    // Decode the response
    realDecoder decoder;
    // Only pass the used portion of the encoder buffer
    decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_offset = 0;
    
    DeleteAclsResponse decodedResponse;
    result = decodedResponse.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode response for round-trip test";
    
    // Verify the decoded response matches the original
    EXPECT_EQ(decodedResponse.m_throttle_time, originalResponse.m_throttle_time) << "Throttle time mismatch in round-trip";
    EXPECT_EQ(decodedResponse.m_filter_responses.size(), originalResponse.m_filter_responses.size()) << "Filter responses count mismatch in round-trip";
    
    for (size_t i = 0; i < originalResponse.m_filter_responses.size(); ++i) {
        auto originalFilterResp = originalResponse.m_filter_responses[i];
        auto decodedFilterResp = decodedResponse.m_filter_responses[i];
        
        EXPECT_EQ(decodedFilterResp->m_err, originalFilterResp->m_err) << "Filter response error mismatch in round-trip, index: " << i;
        EXPECT_EQ(decodedFilterResp->m_err_msg, originalFilterResp->m_err_msg) << "Filter response error message mismatch in round-trip, index: " << i;
        EXPECT_EQ(decodedFilterResp->m_matching_acls.size(), originalFilterResp->m_matching_acls.size()) << "Matching ACLs count mismatch in round-trip, index: " << i;
        
        for (size_t j = 0; j < originalFilterResp->m_matching_acls.size(); ++j) {
            auto originalMatchingAcl = originalFilterResp->m_matching_acls[j];
            auto decodedMatchingAcl = decodedFilterResp->m_matching_acls[j];
            
            EXPECT_EQ(decodedMatchingAcl->m_err, originalMatchingAcl->m_err) << "Matching ACL error mismatch in round-trip, index: " << j;
            EXPECT_EQ(decodedMatchingAcl->m_err_msg, originalMatchingAcl->m_err_msg) << "Matching ACL error message mismatch in round-trip, index: " << j;
            EXPECT_EQ(decodedMatchingAcl->m_resource.m_resource_type, originalMatchingAcl->m_resource.m_resource_type) << "Resource type mismatch in round-trip, index: " << j;
            EXPECT_EQ(decodedMatchingAcl->m_resource.m_resource_name, originalMatchingAcl->m_resource.m_resource_name) << "Resource name mismatch in round-trip, index: " << j;
            EXPECT_EQ(decodedMatchingAcl->m_acl.m_principal, originalMatchingAcl->m_acl.m_principal) << "Principal mismatch in round-trip, index: " << j;
            EXPECT_EQ(decodedMatchingAcl->m_acl.m_host, originalMatchingAcl->m_acl.m_host) << "Host mismatch in round-trip, index: " << j;
            EXPECT_EQ(decodedMatchingAcl->m_acl.m_operation, originalMatchingAcl->m_acl.m_operation) << "Operation mismatch in round-trip, index: " << j;
            EXPECT_EQ(decodedMatchingAcl->m_acl.m_permission_type, originalMatchingAcl->m_acl.m_permission_type) << "PermissionType mismatch in round-trip, index: " << j;
        }
    }
}

TEST(DeleteAclsResponseTest, DecodeWithError) {
    // Test decoding a response with error
    const unsigned char errorResponse[] = {
        0x00, 0x00, 0x00, 0x32,  // Throttle time: 50 ms
        0x00, 0x00, 0x00, 0x01,  // Number of filter responses
        0x00, 0x11,              // Error: InvalidTopic (17)
        0x00, 0x0D, 'I', 'n', 'v', 'a', 'l', 'i', 'd', ' ', 't', 'o', 'p', 'i', 'c',  // Error message: 13 characters
        0x00, 0x00, 0x00, 0x00   // 0 matching ACLs
    };
    
    realDecoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(errorResponse), sizeof(errorResponse));
    decoder.m_offset = 0;
    
    DeleteAclsResponse response;
    int result = response.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode delete acls response with error";
    EXPECT_EQ(response.m_throttle_time.count(), 50) << "Throttle time mismatch";
    EXPECT_EQ(response.m_filter_responses.size(), 1) << "Filter responses count mismatch";
    
    auto filterResp = response.m_filter_responses[0];
    EXPECT_EQ(filterResp->m_err, KError::ErrInvalidTopic) << "Filter response error mismatch";
    EXPECT_EQ(filterResp->m_err_msg, "Invalid topic") << "Filter response error message mismatch";
    EXPECT_EQ(filterResp->m_matching_acls.size(), 0) << "Matching ACLs count should be zero";
}
