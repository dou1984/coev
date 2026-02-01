#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#include "acl_delete_request.h"
#include "real_encoder.h"
#include "real_decoder.h"

// Test data from Sarama's acl_delete_request_test.go
const unsigned char aclDeleteRequestNulls[] = {
    0x00, 0x00, 0x00, 0x01,  // Number of filters
    0x01,                    // ResourceType: Any
    0xFF, 0xFF,              // ResourceName: null
    0xFF, 0xFF,              // Principal: null
    0xFF, 0xFF,              // Host: null
    0x0B,                    // Operation: AlterConfigs
    0x03,                    // PermissionType: Allow
};

const unsigned char aclDeleteRequest[] = {
    0x00, 0x00, 0x00, 0x01,  // Number of filters
    0x01,                    // ResourceType: Any
    0x00, 0x06, 'f', 'i', 'l', 't', 'e', 'r',  // ResourceName: "filter"
    0x00, 0x09, 'p', 'r', 'i', 'n', 'c', 'i', 'p', 'a', 'l',  // Principal: "principal"
    0x00, 0x04, 'h', 'o', 's', 't',  // Host: "host"
    0x04,                    // Operation: Write
    0x03,                    // PermissionType: Allow
};

const unsigned char aclDeleteRequestArray[] = {
    0x00, 0x00, 0x00, 0x02,  // Number of filters
    // First filter
    0x01,                    // ResourceType: Any
    0x00, 0x06, 'f', 'i', 'l', 't', 'e', 'r',  // ResourceName: "filter"
    0x00, 0x09, 'p', 'r', 'i', 'n', 'c', 'i', 'p', 'a', 'l',  // Principal: "principal"
    0x00, 0x04, 'h', 'o', 's', 't',  // Host: "host"
    0x04,                    // Operation: Write
    0x03,                    // PermissionType: Allow
    // Second filter
    0x02,                    // ResourceType: Topic
    0x00, 0x05, 't', 'o', 'p', 'i', 'c',  // ResourceName: "topic"
    0xFF, 0xFF,              // Principal: null
    0xFF, 0xFF,              // Host: null
    0x06,                    // Operation: Delete
    0x02,                    // PermissionType: Deny
};

const unsigned char aclDeleteRequestNullsv1[] = {
    0x00, 0x00, 0x00, 0x01,  // Number of filters
    0x01,                    // ResourceType: Any
    0xFF, 0xFF,              // ResourceName: null
    0x01,                    // ResourcePatternTypeFilter: Any
    0xFF, 0xFF,              // Principal: null
    0xFF, 0xFF,              // Host: null
    0x0B,                    // Operation: AlterConfigs
    0x03,                    // PermissionType: Allow
};

const unsigned char aclDeleteRequestv1[] = {
    0x00, 0x00, 0x00, 0x01,  // Number of filters
    0x01,                    // ResourceType: Any
    0x00, 0x06, 'f', 'i', 'l', 't', 'e', 'r',  // ResourceName: "filter"
    0x01,                    // ResourcePatternTypeFilter: Any
    0x00, 0x09, 'p', 'r', 'i', 'n', 'c', 'i', 'p', 'a', 'l',  // Principal: "principal"
    0x00, 0x04, 'h', 'o', 's', 't',  // Host: "host"
    0x04,                    // Operation: Write
    0x03,                    // PermissionType: Allow
};

TEST(DeleteAclsRequestTest, DecodeRequestNulls) {
    real_decoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(aclDeleteRequestNulls), sizeof(aclDeleteRequestNulls));
    decoder.m_offset = 0;
    
    DeleteAclsRequest request;
    int result = request.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode delete acls request with nulls";
    EXPECT_EQ(request.m_filters.size(), 1) << "Decoding produced unexpected number of filters";
    
    auto filter = request.m_filters[0];
    EXPECT_EQ(filter->m_resource_type, AclResourceTypeAny) << "ResourceType mismatch";
    EXPECT_TRUE(filter->m_resource_name.empty()) << "ResourceName should be empty for null value";
    EXPECT_TRUE(filter->m_principal.empty()) << "Principal should be empty for null value";
    EXPECT_TRUE(filter->m_host.empty()) << "Host should be empty for null value";
    EXPECT_EQ(filter->m_operation, AclOperationAlterConfigs) << "Operation mismatch";
    EXPECT_EQ(filter->m_permission_type, AclPermissionTypeAllow) << "PermissionType mismatch";
}

TEST(DeleteAclsRequestTest, DecodeRequest) {
    real_decoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(aclDeleteRequest), sizeof(aclDeleteRequest));
    decoder.m_offset = 0;
    
    DeleteAclsRequest request;
    int result = request.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode delete acls request";
    EXPECT_EQ(request.m_filters.size(), 1) << "Decoding produced unexpected number of filters";
    
    auto filter = request.m_filters[0];
    EXPECT_EQ(filter->m_resource_type, AclResourceTypeAny) << "ResourceType mismatch";
    EXPECT_EQ(filter->m_resource_name, "filter") << "ResourceName mismatch";
    EXPECT_EQ(filter->m_principal, "principal") << "Principal mismatch";
    EXPECT_EQ(filter->m_host, "host") << "Host mismatch";
    EXPECT_EQ(filter->m_operation, AclOperationWrite) << "Operation mismatch";
    EXPECT_EQ(filter->m_permission_type, AclPermissionTypeAllow) << "PermissionType mismatch";
}

TEST(DeleteAclsRequestTest, DecodeRequestArray) {
    real_decoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(aclDeleteRequestArray), sizeof(aclDeleteRequestArray));
    decoder.m_offset = 0;
    
    DeleteAclsRequest request;
    int result = request.decode(decoder, 0);
    ASSERT_EQ(result, 0) << "Failed to decode delete acls request array";
    EXPECT_EQ(request.m_filters.size(), 2) << "Decoding produced unexpected number of filters";
    
    // Check first filter
    auto filter1 = request.m_filters[0];
    EXPECT_EQ(filter1->m_resource_type, AclResourceTypeAny) << "First filter ResourceType mismatch";
    EXPECT_EQ(filter1->m_resource_name, "filter") << "First filter ResourceName mismatch";
    EXPECT_EQ(filter1->m_principal, "principal") << "First filter Principal mismatch";
    EXPECT_EQ(filter1->m_host, "host") << "First filter Host mismatch";
    EXPECT_EQ(filter1->m_operation, AclOperationWrite) << "First filter Operation mismatch";
    EXPECT_EQ(filter1->m_permission_type, AclPermissionTypeAllow) << "First filter PermissionType mismatch";
    
    // Check second filter
    auto filter2 = request.m_filters[1];
    EXPECT_EQ(filter2->m_resource_type, AclResourceTypeTopic) << "Second filter ResourceType mismatch";
    EXPECT_EQ(filter2->m_resource_name, "topic") << "Second filter ResourceName mismatch";
    EXPECT_TRUE(filter2->m_principal.empty()) << "Second filter Principal should be empty";
    EXPECT_TRUE(filter2->m_host.empty()) << "Second filter Host should be empty";
    EXPECT_EQ(filter2->m_operation, AclOperationDelete) << "Second filter Operation mismatch";
    EXPECT_EQ(filter2->m_permission_type, AclPermissionTypeDeny) << "Second filter PermissionType mismatch";
}

TEST(DeleteAclsRequestTest, DecodeRequestNullsv1) {
    real_decoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(aclDeleteRequestNullsv1), sizeof(aclDeleteRequestNullsv1));
    decoder.m_offset = 0;
    
    DeleteAclsRequest request;
    int result = request.decode(decoder, 1);
    ASSERT_EQ(result, 0) << "Failed to decode delete acls request nulls (version 1)";
    EXPECT_EQ(request.m_filters.size(), 1) << "Decoding produced unexpected number of filters";
    
    auto filter = request.m_filters[0];
    EXPECT_EQ(filter->m_resource_type, AclResourceTypeAny) << "ResourceType mismatch";
    EXPECT_TRUE(filter->m_resource_name.empty()) << "ResourceName should be empty for null value";
    EXPECT_EQ(filter->m_resource_pattern_type_filter, AclResourcePatternTypeAny) << "ResourcePatternTypeFilter mismatch";
    EXPECT_TRUE(filter->m_principal.empty()) << "Principal should be empty for null value";
    EXPECT_TRUE(filter->m_host.empty()) << "Host should be empty for null value";
    EXPECT_EQ(filter->m_operation, AclOperationAlterConfigs) << "Operation mismatch";
    EXPECT_EQ(filter->m_permission_type, AclPermissionTypeAllow) << "PermissionType mismatch";
}

TEST(DeleteAclsRequestTest, DecodeRequestv1) {
    real_decoder decoder;
    decoder.m_raw = std::string(reinterpret_cast<const char*>(aclDeleteRequestv1), sizeof(aclDeleteRequestv1));
    decoder.m_offset = 0;
    
    DeleteAclsRequest request;
    int result = request.decode(decoder, 1);
    ASSERT_EQ(result, 0) << "Failed to decode delete acls request (version 1)";
    EXPECT_EQ(request.m_filters.size(), 1) << "Decoding produced unexpected number of filters";
    
    auto filter = request.m_filters[0];
    EXPECT_EQ(filter->m_resource_type, AclResourceTypeAny) << "ResourceType mismatch";
    EXPECT_EQ(filter->m_resource_name, "filter") << "ResourceName mismatch";
    EXPECT_EQ(filter->m_resource_pattern_type_filter, AclResourcePatternTypeAny) << "ResourcePatternTypeFilter mismatch";
    EXPECT_EQ(filter->m_principal, "principal") << "Principal mismatch";
    EXPECT_EQ(filter->m_host, "host") << "Host mismatch";
    EXPECT_EQ(filter->m_operation, AclOperationWrite) << "Operation mismatch";
    EXPECT_EQ(filter->m_permission_type, AclPermissionTypeAllow) << "PermissionType mismatch";
}

TEST(DeleteAclsRequestTest, EncodeRequestNulls) {
    DeleteAclsRequest request;
    request.set_version(0);
    
    auto filter = std::make_shared<AclFilter>(0);
    filter->m_resource_type = AclResourceTypeAny;
    filter->m_operation = AclOperationAlterConfigs;
    filter->m_permission_type = AclPermissionTypeAllow;
    request.m_filters.push_back(filter);
    
    real_encoder encoder(1024);
    int result = request.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode delete acls request with nulls";
    
    std::string expectedData(reinterpret_cast<const char*>(aclDeleteRequestNulls), sizeof(aclDeleteRequestNulls));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for delete acls request with nulls";
}

TEST(DeleteAclsRequestTest, EncodeRequest) {
    DeleteAclsRequest request;
    request.set_version(0);
    
    auto filter = std::make_shared<AclFilter>(0);
    filter->m_resource_type = AclResourceTypeAny;
    filter->m_resource_name = "filter";
    filter->m_principal = "principal";
    filter->m_host = "host";
    filter->m_operation = AclOperationWrite;
    filter->m_permission_type = AclPermissionTypeAllow;
    request.m_filters.push_back(filter);
    
    real_encoder encoder(1024);
    int result = request.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode delete acls request";
    
    std::string expectedData(reinterpret_cast<const char*>(aclDeleteRequest), sizeof(aclDeleteRequest));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for delete acls request";
}

TEST(DeleteAclsRequestTest, EncodeRequestArray) {
    DeleteAclsRequest request;
    request.set_version(0);
    
    // First filter
    auto filter1 = std::make_shared<AclFilter>(0);
    filter1->m_resource_type = AclResourceTypeAny;
    filter1->m_resource_name = "filter";
    filter1->m_principal = "principal";
    filter1->m_host = "host";
    filter1->m_operation = AclOperationWrite;
    filter1->m_permission_type = AclPermissionTypeAllow;
    request.m_filters.push_back(filter1);
    
    // Second filter
    auto filter2 = std::make_shared<AclFilter>(0);
    filter2->m_resource_type = AclResourceTypeTopic;
    filter2->m_resource_name = "topic";
    filter2->m_operation = AclOperationDelete;
    filter2->m_permission_type = AclPermissionTypeDeny;
    request.m_filters.push_back(filter2);
    
    real_encoder encoder(1024);
    int result = request.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode delete acls request array";
    
    std::string expectedData(reinterpret_cast<const char*>(aclDeleteRequestArray), sizeof(aclDeleteRequestArray));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for delete acls request array";
}

TEST(DeleteAclsRequestTest, EncodeRequestNullsv1) {
    DeleteAclsRequest request;
    request.set_version(1);
    
    auto filter = std::make_shared<AclFilter>(1);
    filter->m_resource_type = AclResourceTypeAny;
    filter->m_resource_pattern_type_filter = AclResourcePatternTypeAny;
    filter->m_operation = AclOperationAlterConfigs;
    filter->m_permission_type = AclPermissionTypeAllow;
    request.m_filters.push_back(filter);
    
    real_encoder encoder(1024);
    int result = request.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode delete acls request nulls (version 1)";
    
    std::string expectedData(reinterpret_cast<const char*>(aclDeleteRequestNullsv1), sizeof(aclDeleteRequestNullsv1));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for delete acls request nulls (version 1)";
}

TEST(DeleteAclsRequestTest, EncodeRequestv1) {
    DeleteAclsRequest request;
    request.set_version(1);
    
    auto filter = std::make_shared<AclFilter>(1);
    filter->m_resource_type = AclResourceTypeAny;
    filter->m_resource_name = "filter";
    filter->m_resource_pattern_type_filter = AclResourcePatternTypeAny;
    filter->m_principal = "principal";
    filter->m_host = "host";
    filter->m_operation = AclOperationWrite;
    filter->m_permission_type = AclPermissionTypeAllow;
    request.m_filters.push_back(filter);
    
    real_encoder encoder(1024);
    int result = request.encode(encoder);
    ASSERT_EQ(result, 0) << "Failed to encode delete acls request (version 1)";
    
    std::string expectedData(reinterpret_cast<const char*>(aclDeleteRequestv1), sizeof(aclDeleteRequestv1));
    std::string actualData(encoder.m_raw.data(), encoder.m_offset);
    EXPECT_EQ(actualData, expectedData) << "Encoding failed for delete acls request (version 1)";
}

TEST(DeleteAclsRequestTest, VersionProperties) {
    DeleteAclsRequest request;
    
    // Test version 0
    request.set_version(0);
    EXPECT_EQ(request.version(), 0) << "Version not properly set";
    EXPECT_EQ(request.key(), 31) << "Key mismatch for version 0";
    EXPECT_EQ(request.header_version(), 1) << "Header version mismatch for version 0";
    EXPECT_TRUE(request.is_valid_version()) << "Version 0 should be valid";
    
    // Test version 1
    request.set_version(1);
    EXPECT_EQ(request.version(), 1) << "Version not properly set";
    EXPECT_EQ(request.key(), 31) << "Key mismatch for version 1";
    EXPECT_EQ(request.header_version(), 1) << "Header version mismatch for version 1";
    EXPECT_TRUE(request.is_valid_version()) << "Version 1 should be valid";
    
    // Test invalid version
    request.set_version(2);
    EXPECT_EQ(request.version(), 2) << "Version not properly set";
    EXPECT_FALSE(request.is_valid_version()) << "Version 2 should be invalid";
}

TEST(DeleteAclsRequestTest, RoundTripEncodingDecoding) {
    // Test round-trip encoding and decoding for both versions
    for (int16_t version = 0; version <= 1; ++version) {
        DeleteAclsRequest originalRequest;
        originalRequest.set_version(version);
        
        // Create multiple filters with different values
        auto filter1 = std::make_shared<AclFilter>(version);
        filter1->m_resource_type = AclResourceTypeTopic;
        filter1->m_resource_name = "test_topic";
        filter1->m_principal = "user:test";
        filter1->m_host = "localhost";
        filter1->m_operation = AclOperationRead;
        filter1->m_permission_type = AclPermissionTypeAllow;
        if (version >= 1) {
            filter1->m_resource_pattern_type_filter = AclResourcePatternTypeLiteral;
        }
        originalRequest.m_filters.push_back(filter1);
        
        auto filter2 = std::make_shared<AclFilter>(version);
        filter2->m_resource_type = AclResourceTypeGroup;
        filter2->m_operation = AclOperationAll;
        filter2->m_permission_type = AclPermissionTypeDeny;
        if (version >= 1) {
            filter2->m_resource_pattern_type_filter = AclResourcePatternTypePrefixed;
        }
        originalRequest.m_filters.push_back(filter2);
        
        // Encode the request
        real_encoder encoder(1024);
        int result = originalRequest.encode(encoder);
        ASSERT_EQ(result, 0) << "Failed to encode request for round-trip test, version: " << version;
        
        // Decode the encoded request
        real_decoder decoder;
        decoder.m_raw = encoder.m_raw.substr(0, encoder.m_offset);
        decoder.m_offset = 0;
        
        DeleteAclsRequest decodedRequest;
        result = decodedRequest.decode(decoder, version);
        ASSERT_EQ(result, 0) << "Failed to decode request for round-trip test, version: " << version;
        
        // Verify the decoded request matches the original
        EXPECT_EQ(decodedRequest.m_filters.size(), originalRequest.m_filters.size()) << "Filter count mismatch in round-trip test, version: " << version;
        
        for (size_t i = 0; i < originalRequest.m_filters.size(); ++i) {
            auto originalFilter = originalRequest.m_filters[i];
            auto decodedFilter = decodedRequest.m_filters[i];
            
            EXPECT_EQ(decodedFilter->m_resource_type, originalFilter->m_resource_type) << "ResourceType mismatch in round-trip test, version: " << version << ", filter: " << i;
            EXPECT_EQ(decodedFilter->m_resource_name, originalFilter->m_resource_name) << "ResourceName mismatch in round-trip test, version: " << version << ", filter: " << i;
            EXPECT_EQ(decodedFilter->m_principal, originalFilter->m_principal) << "Principal mismatch in round-trip test, version: " << version << ", filter: " << i;
            EXPECT_EQ(decodedFilter->m_host, originalFilter->m_host) << "Host mismatch in round-trip test, version: " << version << ", filter: " << i;
            EXPECT_EQ(decodedFilter->m_operation, originalFilter->m_operation) << "Operation mismatch in round-trip test, version: " << version << ", filter: " << i;
            EXPECT_EQ(decodedFilter->m_permission_type, originalFilter->m_permission_type) << "PermissionType mismatch in round-trip test, version: " << version << ", filter: " << i;
            
            if (version >= 1) {
                EXPECT_EQ(decodedFilter->m_resource_pattern_type_filter, originalFilter->m_resource_pattern_type_filter) << "ResourcePatternTypeFilter mismatch in round-trip test, version: " << version << ", filter: " << i;
            }
        }
    }
}
