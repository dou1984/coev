#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#include "acl_describe_request.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Test data from Sarama's acl_describe_request_test.go
const unsigned char aclDescribeRequest[] = {
    0x02,                                                    // ResourceType: Topic
    0x00, 0x05, 't', 'o', 'p', 'i', 'c',                     // ResourceName: "topic"
    0x00, 0x09, 'p', 'r', 'i', 'n', 'c', 'i', 'p', 'a', 'l', // Principal: "principal"
    0x00, 0x04, 'h', 'o', 's', 't',                          // Host: "host"
    0x05,                                                    // Operation: Create
    0x03                                                     // PermissionType: Allow
};

const unsigned char aclDescribeRequestV1[] = {
    0x02,                                                    // ResourceType: Topic
    0x00, 0x05, 't', 'o', 'p', 'i', 'c',                     // ResourceName: "topic"
    0x01,                                                    // ResourcePatternType: Any
    0x00, 0x09, 'p', 'r', 'i', 'n', 'c', 'i', 'p', 'a', 'l', // Principal: "principal"
    0x00, 0x04, 'h', 'o', 's', 't',                          // Host: "host"
    0x05,                                                    // Operation: Create
    0x03                                                     // PermissionType: Allow
};

TEST(DescribeAclsRequestTest, DecodeRequest)
{
    real_decoder decoder;
    std::string rawData(reinterpret_cast<const char *>(aclDescribeRequest), sizeof(aclDescribeRequest));
    decoder.m_raw = rawData;
    decoder.m_offset = 0;

    DescribeAclsRequest request;
    int result = request.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode ACL describe request";
    EXPECT_EQ(request.m_filter.m_resource_type, AclResourceTypeTopic) << "ResourceType mismatch";
    EXPECT_EQ(request.m_filter.m_resource_name, "topic") << "ResourceName mismatch";
    EXPECT_EQ(request.m_filter.m_principal, "principal") << "Principal mismatch";
    EXPECT_EQ(request.m_filter.m_host, "host") << "Host mismatch";
    EXPECT_EQ(request.m_filter.m_operation, AclOperationCreate) << "Operation mismatch";
    EXPECT_EQ(request.m_filter.m_permission_type, AclPermissionTypeAllow) << "PermissionType mismatch";
}

TEST(DescribeAclsRequestTest, DecodeRequestv1)
{
    real_decoder decoder;
    std::string rawData(reinterpret_cast<const char *>(aclDescribeRequestV1), sizeof(aclDescribeRequestV1));
    decoder.m_raw = rawData;
    decoder.m_offset = 0;

    DescribeAclsRequest request;
    int result = request.decode(decoder, 1);
    ASSERT_EQ(result, 0) << "Failed to decode ACL describe request v1";
    EXPECT_EQ(request.m_filter.m_resource_type, AclResourceTypeTopic) << "ResourceType mismatch";
    EXPECT_EQ(request.m_filter.m_resource_name, "topic") << "ResourceName mismatch";
    EXPECT_EQ(request.m_filter.m_resource_pattern_type_filter, AclResourcePatternTypeAny) << "ResourcePatternTypeFilter mismatch";
    EXPECT_EQ(request.m_filter.m_principal, "principal") << "Principal mismatch";
    EXPECT_EQ(request.m_filter.m_host, "host") << "Host mismatch";
    EXPECT_EQ(request.m_filter.m_operation, AclOperationCreate) << "Operation mismatch";
    EXPECT_EQ(request.m_filter.m_permission_type, AclPermissionTypeAllow) << "PermissionType mismatch";
}

TEST(DescribeAclsRequestTest, EncodeRequest)
{
    DescribeAclsRequest request;
    request.set_version(0);
    request.m_filter.m_resource_type = AclResourceTypeTopic;
    request.m_filter.m_resource_name = "topic";
    request.m_filter.m_principal = "principal";
    request.m_filter.m_host = "host";
    request.m_filter.m_operation = AclOperationCreate;
    request.m_filter.m_permission_type = AclPermissionTypeAllow;

    real_encoder encoder(1024);
    int result = request.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode ACL describe request";

    std::string expectedData(reinterpret_cast<const char *>(aclDescribeRequest), sizeof(aclDescribeRequest));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for ACL describe request";
}

TEST(DescribeAclsRequestTest, EncodeRequestv1)
{
    DescribeAclsRequest request;
    request.set_version(1);
    request.m_filter.m_resource_type = AclResourceTypeTopic;
    request.m_filter.m_resource_name = "topic";
    request.m_filter.m_resource_pattern_type_filter = AclResourcePatternTypeAny;
    request.m_filter.m_principal = "principal";
    request.m_filter.m_host = "host";
    request.m_filter.m_operation = AclOperationCreate;
    request.m_filter.m_permission_type = AclPermissionTypeAllow;

    real_encoder encoder(1024);
    int result = request.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode ACL describe request v1";

    std::string expectedData(reinterpret_cast<const char *>(aclDescribeRequestV1), sizeof(aclDescribeRequestV1));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for ACL describe request v1";
}

TEST(DescribeAclsRequestTest, VersionProperties)
{
    DescribeAclsRequest request;

    // Test version 0
    request.set_version(0);
    EXPECT_EQ(request.version(), 0) << "Version not properly set";
    EXPECT_EQ(request.key(), 29) << "Key mismatch for version 0";
    EXPECT_EQ(request.header_version(), 1) << "Header version mismatch for version 0";
    EXPECT_TRUE(request.is_valid_version()) << "Version 0 should be valid";

    // Test version 1
    request.set_version(1);
    EXPECT_EQ(request.version(), 1) << "Version not properly set";
    EXPECT_EQ(request.key(), 29) << "Key mismatch for version 1";
    EXPECT_EQ(request.header_version(), 1) << "Header version mismatch for version 1";
    EXPECT_TRUE(request.is_valid_version()) << "Version 1 should be valid";

    // Test invalid version
    request.set_version(2);
    EXPECT_EQ(request.version(), 2) << "Version not properly set";
    EXPECT_FALSE(request.is_valid_version()) << "Version 2 should be invalid";
}

TEST(DescribeAclsRequestTest, RoundTripEncodingDecoding)
{
    // Create original request
    DescribeAclsRequest originalRequest;
    originalRequest.set_version(0);
    originalRequest.m_filter.m_resource_type = AclResourceTypeTopic;
    originalRequest.m_filter.m_resource_name = "test_topic";
    originalRequest.m_filter.m_principal = "test_principal";
    originalRequest.m_filter.m_host = "test_host";
    originalRequest.m_filter.m_operation = AclOperationWrite;
    originalRequest.m_filter.m_permission_type = AclPermissionTypeDeny;

    // Encode the request
    real_encoder encoder(1024);
    int result = originalRequest.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode request for round-trip test";

    // Decode the request
    real_decoder decoder;
    std::string rawData = encoder.m_raw.substr(0, encoder.m_offset);
    decoder.m_raw = rawData;
    decoder.m_offset = 0;

    DescribeAclsRequest decodedRequest;
    result = decodedRequest.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode request for round-trip test";

    // Verify the decoded request matches the original
    EXPECT_EQ(decodedRequest.m_filter.m_resource_type, originalRequest.m_filter.m_resource_type) << "ResourceType mismatch in round-trip";
    EXPECT_EQ(decodedRequest.m_filter.m_resource_name, originalRequest.m_filter.m_resource_name) << "ResourceName mismatch in round-trip";
    EXPECT_EQ(decodedRequest.m_filter.m_principal, originalRequest.m_filter.m_principal) << "Principal mismatch in round-trip";
    EXPECT_EQ(decodedRequest.m_filter.m_host, originalRequest.m_filter.m_host) << "Host mismatch in round-trip";
    EXPECT_EQ(decodedRequest.m_filter.m_operation, originalRequest.m_filter.m_operation) << "Operation mismatch in round-trip";
    EXPECT_EQ(decodedRequest.m_filter.m_permission_type, originalRequest.m_filter.m_permission_type) << "PermissionType mismatch in round-trip";

    // Test round-trip with version 1
    DescribeAclsRequest originalRequestV1;
    originalRequestV1.set_version(1);
    originalRequestV1.m_filter.m_resource_type = AclResourceTypeGroup;
    originalRequestV1.m_filter.m_resource_name = "test_group";
    originalRequestV1.m_filter.m_resource_pattern_type_filter = AclResourcePatternTypePrefixed;
    originalRequestV1.m_filter.m_principal = "group_principal";
    originalRequestV1.m_filter.m_host = "group_host";
    originalRequestV1.m_filter.m_operation = AclOperationRead;
    originalRequestV1.m_filter.m_permission_type = AclPermissionTypeAllow;

    real_encoder encoderV1(1024);
    result = originalRequestV1.encode(encoderV1);
    ASSERT_EQ(result, 0) << "Failed to encode V1 request for round-trip test";

    real_decoder decoderV1;
    std::string rawDataV1 = encoderV1.m_raw.substr(0, encoderV1.m_offset);
    decoderV1.m_raw = rawDataV1;
    decoderV1.m_offset = 0;

    DescribeAclsRequest decodedRequestV1;
    result = decodedRequestV1.decode(decoderV1, 1);
    ASSERT_EQ(result, 0) << "Failed to decode V1 request for round-trip test";

    EXPECT_EQ(decodedRequestV1.m_filter.m_resource_type, originalRequestV1.m_filter.m_resource_type) << "ResourceType mismatch in V1 round-trip";
    EXPECT_EQ(decodedRequestV1.m_filter.m_resource_name, originalRequestV1.m_filter.m_resource_name) << "ResourceName mismatch in V1 round-trip";
    EXPECT_EQ(decodedRequestV1.m_filter.m_resource_pattern_type_filter, originalRequestV1.m_filter.m_resource_pattern_type_filter) << "ResourcePatternTypeFilter mismatch in V1 round-trip";
    EXPECT_EQ(decodedRequestV1.m_filter.m_principal, originalRequestV1.m_filter.m_principal) << "Principal mismatch in V1 round-trip";
    EXPECT_EQ(decodedRequestV1.m_filter.m_host, originalRequestV1.m_filter.m_host) << "Host mismatch in V1 round-trip";
    EXPECT_EQ(decodedRequestV1.m_filter.m_operation, originalRequestV1.m_filter.m_operation) << "Operation mismatch in V1 round-trip";
    EXPECT_EQ(decodedRequestV1.m_filter.m_permission_type, originalRequestV1.m_filter.m_permission_type) << "PermissionType mismatch in V1 round-trip";
}
