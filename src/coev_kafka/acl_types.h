#pragma once

#include <string>
#include <vector>
#include <cstdint>

enum AclOperation : int
{
    AclOperationUnknown = 0,
    AclOperationAny = 1,
    AclOperationAll = 2,
    AclOperationRead = 3,
    AclOperationWrite = 4,
    AclOperationCreate = 5,
    AclOperationDelete = 6,
    AclOperationAlter = 7,
    AclOperationDescribe = 8,
    AclOperationClusterAction = 9,
    AclOperationDescribeConfigs = 10,
    AclOperationAlterConfigs = 11,
    AclOperationIdempotentWrite = 12
};

std::string toString(AclOperation op);
std::string marshalText(AclOperation op);
bool unmarshalText(const std::string &text, AclOperation &out, std::string &error);

enum AclPermissionType : int
{
    AclPermissionTypeUnknown = 0,
    AclPermissionTypeAny = 1,
    AclPermissionTypeDeny = 2,
    AclPermissionTypeAllow = 3
};

std::string toString(AclPermissionType pt);
std::string marshalText(AclPermissionType pt);
bool unmarshalText(const std::string &text, AclPermissionType &out, std::string &error);

enum AclResourceType : int
{
    AclResourceTypeUnknown = 0,
    AclResourceTypeAny = 1,
    AclResourceTypeTopic = 2,
    AclResourceTypeGroup = 3,
    AclResourceTypeCluster = 4,
    AclResourceTypeTransactionalID = 5,
    AclResourceTypeDelegationToken = 6
};

std::string toString(AclResourceType rt);
std::string marshalText(AclResourceType rt);
bool unmarshalText(const std::string &text, AclResourceType &out, std::string &error);

enum AclResourcePatternType : int
{
    AclResourcePatternTypeUnknown = 0,
    AclResourcePatternTypeAny = 1,
    AclResourcePatternTypeMatch = 2,
    AclResourcePatternTypeLiteral = 3,
    AclResourcePatternTypePrefixed = 4
};

std::string toString(AclResourcePatternType rpt);
std::string marshalText(AclResourcePatternType rpt);
bool unmarshalText(const std::string &text, AclResourcePatternType &out, std::string &error);
