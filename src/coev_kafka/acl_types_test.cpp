#include <gtest/gtest.h>
#include <string>

#include "acl_types.h"

TEST(AclTypesTest, AclOperationTextMarshal)
{
    for (int i = static_cast<int>(AclOperationUnknown); i <= static_cast<int>(AclOperationIdempotentWrite); ++i)
    {
        AclOperation op = static_cast<AclOperation>(i);

        // Test marshalText
        std::string text = marshalText(op);
        EXPECT_FALSE(text.empty()) << "marshaling AclOperation " << i << " should not return empty string";

        // Test unmarshalText
        AclOperation got;
        std::string error;
        bool success = unmarshalText(text, got, error);
        EXPECT_TRUE(success) << "unmarshaling " << text << " to AclOperation should succeed, got error: " << error;
        EXPECT_EQ(got, op) << "unmarshaled AclOperation should match original, got " << static_cast<int>(got) << ", want " << i;
    }
}

TEST(AclTypesTest, AclPermissionTypeTextMarshal)
{
    for (int i = static_cast<int>(AclPermissionTypeUnknown); i <= static_cast<int>(AclPermissionTypeAllow); ++i)
    {
        AclPermissionType pt = static_cast<AclPermissionType>(i);

        // Test marshalText
        std::string text = marshalText(pt);
        EXPECT_FALSE(text.empty()) << "marshaling AclPermissionType " << i << " should not return empty string";

        // Test unmarshalText
        AclPermissionType got;
        std::string error;
        bool success = unmarshalText(text, got, error);
        EXPECT_TRUE(success) << "unmarshaling " << text << " to AclPermissionType should succeed, got error: " << error;
        EXPECT_EQ(got, pt) << "unmarshaled AclPermissionType should match original, got " << static_cast<int>(got) << ", want " << i;
    }
}

TEST(AclTypesTest, AclResourceTypeTextMarshal)
{
    for (int i = static_cast<int>(AclResourceTypeUnknown); i <= static_cast<int>(AclResourceTypeDelegationToken); ++i)
    {
        AclResourceType rt = static_cast<AclResourceType>(i);

        // Test marshalText
        std::string text = marshalText(rt);
        EXPECT_FALSE(text.empty()) << "marshaling AclResourceType " << i << " should not return empty string";

        // Test unmarshalText
        AclResourceType got;
        std::string error;
        bool success = unmarshalText(text, got, error);
        EXPECT_TRUE(success) << "unmarshaling " << text << " to AclResourceType should succeed, got error: " << error;
        EXPECT_EQ(got, rt) << "unmarshaled AclResourceType should match original, got " << static_cast<int>(got) << ", want " << i;
    }
}

TEST(AclTypesTest, AclResourcePatternTypeTextMarshal)
{
    for (int i = static_cast<int>(AclResourcePatternTypeUnknown); i <= static_cast<int>(AclResourcePatternTypePrefixed); ++i)
    {
        AclResourcePatternType rpt = static_cast<AclResourcePatternType>(i);

        // Test marshalText
        std::string text = marshalText(rpt);
        EXPECT_FALSE(text.empty()) << "marshaling AclResourcePatternType " << i << " should not return empty string";

        // Test unmarshalText
        AclResourcePatternType got;
        std::string error;
        bool success = unmarshalText(text, got, error);
        EXPECT_TRUE(success) << "unmarshaling " << text << " to AclResourcePatternType should succeed, got error: " << error;
        EXPECT_EQ(got, rpt) << "unmarshaled AclResourcePatternType should match original, got " << static_cast<int>(got) << ", want " << i;
    }
}

TEST(AclTypesTest, AclOperationToString)
{
    for (int i = static_cast<int>(AclOperationUnknown); i <= static_cast<int>(AclOperationIdempotentWrite); ++i)
    {
        AclOperation op = static_cast<AclOperation>(i);
        std::string str = toString(op);
        EXPECT_FALSE(str.empty()) << "toString(AclOperation) should not return empty string for value " << i;
    }
}

TEST(AclTypesTest, AclPermissionTypeToString)
{
    for (int i = static_cast<int>(AclPermissionTypeUnknown); i <= static_cast<int>(AclPermissionTypeAllow); ++i)
    {
        AclPermissionType pt = static_cast<AclPermissionType>(i);
        std::string str = toString(pt);
        EXPECT_FALSE(str.empty()) << "toString(AclPermissionType) should not return empty string for value " << i;
    }
}

TEST(AclTypesTest, AclResourceTypeToString)
{
    for (int i = static_cast<int>(AclResourceTypeUnknown); i <= static_cast<int>(AclResourceTypeDelegationToken); ++i)
    {
        AclResourceType rt = static_cast<AclResourceType>(i);
        std::string str = toString(rt);
        EXPECT_FALSE(str.empty()) << "toString(AclResourceType) should not return empty string for value " << i;
    }
}

TEST(AclTypesTest, AclResourcePatternTypeToString)
{
    for (int i = static_cast<int>(AclResourcePatternTypeUnknown); i <= static_cast<int>(AclResourcePatternTypePrefixed); ++i)
    {
        AclResourcePatternType rpt = static_cast<AclResourcePatternType>(i);
        std::string str = toString(rpt);
        EXPECT_FALSE(str.empty()) << "toString(AclResourcePatternType) should not return empty string for value " << i;
    }
}
