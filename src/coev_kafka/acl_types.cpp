#include "acl_types.h"
#include <algorithm>
#include <cctype>
#include <map>

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string toString(AclOperation op)
{
    switch (op)
    {
    case AclOperationUnknown:
        return "Unknown";
    case AclOperationAny:
        return "Any";
    case AclOperationAll:
        return "All";
    case AclOperationRead:
        return "Read";
    case AclOperationWrite:
        return "Write";
    case AclOperationCreate:
        return "Create";
    case AclOperationDelete:
        return "Delete";
    case AclOperationAlter:
        return "Alter";
    case AclOperationDescribe:
        return "Describe";
    case AclOperationClusterAction:
        return "ClusterAction";
    case AclOperationDescribeConfigs:
        return "DescribeConfigs";
    case AclOperationAlterConfigs:
        return "AlterConfigs";
    case AclOperationIdempotentWrite:
        return "IdempotentWrite";
    default:
        return "Unknown";
    }
}

std::string marshalText(AclOperation op)
{
    return toString(op);
}

bool unmarshalText(const std::string &text, AclOperation &out, std::string &error)
{
    std::string normalized = toLower(text);
    static const std::map<std::string, AclOperation> mapping = {
        {"unknown", AclOperationUnknown},
        {"any", AclOperationAny},
        {"all", AclOperationAll},
        {"read", AclOperationRead},
        {"write", AclOperationWrite},
        {"create", AclOperationCreate},
        {"delete", AclOperationDelete},
        {"alter", AclOperationAlter},
        {"describe", AclOperationDescribe},
        {"clusteraction", AclOperationClusterAction},
        {"describeconfigs", AclOperationDescribeConfigs},
        {"alterconfigs", AclOperationAlterConfigs},
        {"idempotentwrite", AclOperationIdempotentWrite}};
    auto it = mapping.find(normalized);
    if (it != mapping.end())
    {
        out = it->second;
        return true;
    }
    out = AclOperationUnknown;
    error = "no acl operation with name " + normalized;
    return false;
}
std::string toString(AclPermissionType pt)
{
    switch (pt)
    {
    case AclPermissionTypeUnknown:
        return "Unknown";
    case AclPermissionTypeAny:
        return "Any";
    case AclPermissionTypeDeny:
        return "Deny";
    case AclPermissionTypeAllow:
        return "Allow";
    default:
        return "Unknown";
    }
}

std::string marshalText(AclPermissionType pt)
{
    return toString(pt);
}

bool unmarshalText(const std::string &text, AclPermissionType &out, std::string &error)
{
    std::string normalized = toLower(text);
    static const std::map<std::string, AclPermissionType> mapping = {
        {"unknown", AclPermissionTypeUnknown},
        {"any", AclPermissionTypeAny},
        {"deny", AclPermissionTypeDeny},
        {"allow", AclPermissionTypeAllow}};
    auto it = mapping.find(normalized);
    if (it != mapping.end())
    {
        out = it->second;
        return true;
    }
    out = AclPermissionTypeUnknown;
    error = "no acl permission with name " + normalized;
    return false;
}

// AclResourceType
std::string toString(AclResourceType rt)
{
    switch (rt)
    {
    case AclResourceTypeUnknown:
        return "Unknown";
    case AclResourceTypeAny:
        return "Any";
    case AclResourceTypeTopic:
        return "Topic";
    case AclResourceTypeGroup:
        return "Group";
    case AclResourceTypeCluster:
        return "Cluster";
    case AclResourceTypeTransactionalID:
        return "TransactionalID";
    case AclResourceTypeDelegationToken:
        return "DelegationToken";
    default:
        return "Unknown";
    }
}

std::string marshalText(AclResourceType rt)
{
    return toString(rt);
}

bool unmarshalText(const std::string &text, AclResourceType &out, std::string &error)
{
    std::string normalized = toLower(text);
    static const std::map<std::string, AclResourceType> mapping = {
        {"unknown", AclResourceTypeUnknown},
        {"any", AclResourceTypeAny},
        {"topic", AclResourceTypeTopic},
        {"group", AclResourceTypeGroup},
        {"cluster", AclResourceTypeCluster},
        {"transactionalid", AclResourceTypeTransactionalID},
        {"delegationtoken", AclResourceTypeDelegationToken}};
    auto it = mapping.find(normalized);
    if (it != mapping.end())
    {
        out = it->second;
        return true;
    }
    out = AclResourceTypeUnknown;
    error = "no acl resource with name " + normalized;
    return false;
}

// AclResourcePatternType
std::string toString(AclResourcePatternType rpt)
{
    switch (rpt)
    {
    case AclResourcePatternTypeUnknown:
        return "Unknown";
    case AclResourcePatternTypeAny:
        return "Any";
    case AclResourcePatternTypeMatch:
        return "Match";
    case AclResourcePatternTypeLiteral:
        return "Literal";
    case AclResourcePatternTypePrefixed:
        return "Prefixed";
    default:
        return "Unknown";
    }
}

std::string marshalText(AclResourcePatternType rpt)
{
    return toString(rpt);
}

bool unmarshalText(const std::string &text, AclResourcePatternType &out, std::string &error)
{
    std::string normalized = toLower(text);
    static const std::map<std::string, AclResourcePatternType> mapping = {
        {"unknown", AclResourcePatternTypeUnknown},
        {"any", AclResourcePatternTypeAny},
        {"match", AclResourcePatternTypeMatch},
        {"literal", AclResourcePatternTypeLiteral},
        {"prefixed", AclResourcePatternTypePrefixed}};
    auto it = mapping.find(normalized);
    if (it != mapping.end())
    {
        out = it->second;
        return true;
    }
    out = AclResourcePatternTypeUnknown;
    error = "no acl resource pattern with name " + normalized;
    return false;
}