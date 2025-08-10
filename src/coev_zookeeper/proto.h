#pragma once
#include <sys/types.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace coev
{

    struct Id
    {
        std::string scheme;
        std::string id;
        ~Id();
    };

    struct ACL
    {
        int32_t perms;
        Id id;
        ~ACL();
    };

    struct Stat
    {
        int64_t czxid;
        int64_t mzxid;
        int64_t ctime;
        int64_t mtime;
        int32_t version;
        int32_t cversion;
        int32_t aversion;
        int64_t ephemeralOwner;
        int32_t dataLength;
        int32_t numChildren;
        int64_t pzxid;
    };

    struct StatPersisted
    {
        int64_t czxid;
        int64_t mzxid;
        int64_t ctime;
        int64_t mtime;
        int32_t version;
        int32_t cversion;
        int32_t aversion;
        int64_t ephemeralOwner;
        int64_t pzxid;
    };
    struct ClientInfo
    {
        std::string authScheme;
        std::string user;
    };
    struct ConnectRequest
    {
        int32_t protocolVersion;
        int64_t lastZxidSeen;
        int32_t timeout;
        int64_t sessionId;
        std::string passwd;
        int32_t readOnly;
    };
    struct ConnectResponse
    {
        int32_t protocolVersion;
        int32_t timeout;
        int64_t sessionId;
        std::string passwd;
        int32_t readOnly;
    };
    struct StringVec : std::vector<std::string>
    {
        auto operator=(std::vector<std::string> &&o)
        {
            std::vector<std::string>::operator=(std::move(o));
            return *this;
        }
        ~StringVec();
    };
    struct SetWatches
    {
        int64_t relativeZxid;
        StringVec dataWatches;
        StringVec existWatches;
        StringVec childWatches;
    };
    struct SetWatches2
    {
        int64_t relativeZxid;
        StringVec dataWatches;
        StringVec existWatches;
        StringVec childWatches;
        StringVec persistentWatches;
        StringVec persistentRecursiveWatches;
    };
    struct RequestHeader
    {
        int32_t xid;
        int32_t type;
    };
    struct MultiHeader
    {
        int32_t type;
        int32_t done;
        int32_t err;
    };
    struct AuthPacket
    {
        int32_t type;
        std::string scheme;
        std::string auth;
    };
    struct ReplyHeader
    {
        int32_t xid;
        int64_t zxid;
        int32_t err;
    };
    struct GetDataRequest
    {
        std::string path;
        int32_t watch;
    };
    struct SetDataRequest
    {
        std::string path;
        std::string data;
        int32_t version;
    };
    struct ReconfigRequest
    {
        std::string joiningServers;
        std::string leavingServers;
        std::string newMembers;
        int64_t curConfigId;
    };
    struct SetDataResponse
    {
        struct Stat stat;
    };
    struct GetSASLRequest
    {
        std::string token;
    };
    struct SetSASLRequest
    {
        std::string token;
    };
    struct SetSASLResponse
    {
        std::string token;
    };
    struct ACLVec : std::vector<ACL>
    {
    };

    struct CreateRequest
    {
        std::string path;
        std::string data;
        ACLVec acl;
        int32_t flags;
        void clear();
    };
    struct CreateTTLRequest
    {
        std::string path;
        std::string data;
        ACLVec acl;
        int32_t flags;
        int64_t ttl;
        ~CreateTTLRequest();
    };
    struct DeleteRequest
    {
        std::string path;
        int32_t version;
    };
    struct GetChildrenRequest
    {
        std::string path;
        int32_t watch;
    };
    struct GetAllChildrenNumberRequest
    {
        std::string path;
    };
    struct GetChildren2Request
    {
        std::string path;
        int32_t watch;
    };
    struct CheckVersionRequest
    {
        std::string path;
        int32_t version;
    };
    struct GetMaxChildrenRequest
    {
        std::string path;
    };
    struct GetMaxChildrenResponse
    {
        int32_t max;
    };
    struct SetMaxChildrenRequest
    {
        std::string path;
        int32_t max;
    };
    struct SyncRequest
    {
        std::string path;
    };
    struct SyncResponse
    {
        std::string path;
    };
    struct GetACLRequest
    {
        std::string path;
    };
    struct SetACLRequest
    {
        std::string path;
        ACLVec acl;
        int32_t version;
    };
    struct SetACLResponse
    {
        struct Stat stat;
    };
    struct AddWatchRequest
    {
        std::string path;
        int32_t mode;
    };
    struct WatcherEvent
    {
        int32_t type;
        int32_t state;
        std::string path;
        void clear();
    };
    struct ErrorResponse
    {
        int32_t err;
    };
    struct CreateResponse
    {
        std::string path;
    };
    struct Create2Response
    {
        std::string path;
        Stat stat;
    };
    struct ExistsRequest
    {
        std::string path;
        int32_t watch;
    };
    struct ExistsResponse
    {
        Stat stat;
    };
    struct GetDataResponse
    {
        std::string data;
        Stat stat;
    };
    struct GetChildrenResponse
    {
        StringVec children;
    };
    struct GetAllChildrenNumberResponse
    {
        int32_t totalNumber;
    };
    struct GetChildren2Response
    {
        StringVec children;
        Stat stat;
    };
    struct GetACLResponse
    {
        ACLVec acl;
        Stat stat;
        void clear();
    };
    struct CheckWatchesRequest
    {
        std::string path;
        int32_t type;
    };

    struct RemoveWatchesRequest
    {
        std::string path;
        int32_t type;
        ~RemoveWatchesRequest() = default;
    };
    struct GetEphemeralsRequest
    {
        std::string prefixPath;
    };
    struct GetEphemeralsResponse
    {
        StringVec ephemerals;
    };
    struct ClientInfoVec : std::vector<ClientInfo>
    {
    };

    struct WhoAmIResponse
    {
        ClientInfoVec clientInfo;
        ~WhoAmIResponse();
    };
    struct LearnerInfo
    {
        int64_t serverid;
        int32_t protocolVersion;
        int64_t configVersion;
    };
    struct IdVec : std::vector<Id>
    {
    };

    struct QuorumPacket
    {
        int32_t type;
        int64_t zxid;
        std::string data;
        IdVec authinfo;
        void clear();
    };
    struct QuorumAuthPacket
    {
        int64_t magic;
        int32_t status;
        std::string token;
    };
    struct FileHeader
    {
        int32_t magic;
        int32_t version;
        int64_t dbid;
    };
    struct TxnDigest
    {
        int32_t version;
        int64_t treeDigest;
    };
    struct TxnHeader
    {
        int64_t clientId;
        int32_t cxid;
        int64_t zxid;
        int64_t time;
        int32_t type;
    };
    struct CreateTxnV0
    {
        std::string path;
        std::string data;
        ACLVec acl;
        int32_t ephemeral;
    };
    struct CreateTxn
    {
        std::string path;
        std::string data;
        ACLVec acl;
        int32_t ephemeral;
        int32_t parentCVersion;
    };
    struct CreateTTLTxn
    {
        std::string path;
        std::string data;
        ACLVec acl;
        int32_t parentCVersion;
        int64_t ttl;
    };
    struct CreateContainerTxn
    {
        std::string path;
        std::string data;
        ACLVec acl;
        int32_t parentCVersion;
    };
    struct DeleteTxn
    {
        std::string path;
    };
    struct SetDataTxn
    {
        std::string path;
        std::string data;
        int32_t version;
    };
    struct CheckVersionTxn
    {
        std::string path;
        int32_t version;
    };
    struct SetACLTxn
    {
        std::string path;
        ACLVec acl;
        int32_t version;
    };
    struct SetMaxChildrenTxn
    {
        std::string path;
        int32_t max;
    };
    struct CreateSessionTxn
    {
        int32_t timeOut;
    };
    struct CloseSessionTxn
    {
        StringVec paths2Delete;
    };
    struct ErrorTxn
    {
        int32_t err;
    };
    struct Txn
    {
        int32_t type;
        std::string data;
    };
    struct Txn_vector
    {
        int32_t count;
        Txn *data;
    };
    struct MultiTxn
    {
        Txn_vector txns;
    };

    struct iarchive
    {
        std::string_view m_view;
        ReplyHeader m_header;
        iarchive() = default;
        iarchive(const char *data, size_t size);
        iarchive(const std::string &data);

        int deserialize_bool(const char *name, int32_t *);
        int deserialize_int(const char *name, int32_t *);
        int deserialize_long(const char *name, int64_t *);

        int deserialize_buffer(const char *name, std::string &);
        int deserialize_string(const char *name, std::string &s);

        int deserialize_Id(const char *tag, Id *v);
        int deserialize_ACL(const char *tag, ACL *v);
        int deserialize_Stat(const char *tag, Stat *v);
        int deserialize_StatPersisted(const char *tag, StatPersisted *v);
        int deserialize_ClientInfo(const char *tag, ClientInfo *v);
        int deserialize_ConnectRequest(const char *tag, ConnectRequest *v);
        int deserialize_ConnectResponse(const char *tag, ConnectResponse *v);
        int deserialize_String_vector(const char *tag, StringVec *v);
        int deserialize_SetWatches(const char *tag, SetWatches *v);
        int deserialize_SetWatches2(const char *tag, SetWatches2 *v);
        int deserialize_RequestHeader(const char *tag, RequestHeader *v);
        int deserialize_MultiHeader(const char *tag, MultiHeader *v);
        int deserialize_AuthPacket(const char *tag, AuthPacket *v);
        int deserialize_ReplyHeader(const char *tag, ReplyHeader *v);
        int deserialize_GetDataRequest(const char *tag, GetDataRequest *v);
        int deserialize_SetDataRequest(const char *tag, SetDataRequest *v);
        int deserialize_ReconfigRequest(const char *tag, ReconfigRequest *v);
        int deserialize_SetDataResponse(const char *tag, SetDataResponse *v);
        int deserialize_GetSASLRequest(const char *tag, GetSASLRequest *v);
        int deserialize_SetSASLRequest(const char *tag, SetSASLRequest *v);
        int deserialize_SetSASLResponse(const char *tag, SetSASLResponse *v);
        int deserialize_ACLVec(const char *tag, ACLVec *v);
        int deserialize_CreateRequest(const char *tag, CreateRequest *v);
        int deserialize_CreateTTLRequest(const char *tag, CreateTTLRequest *v);
        int deserialize_DeleteRequest(const char *tag, DeleteRequest *v);
        int deserialize_GetChildrenRequest(const char *tag, GetChildrenRequest *v);
        int deserialize_GetAllChildrenNumberRequest(const char *tag, GetAllChildrenNumberRequest *v);
        int deserialize_GetChildren2Request(const char *tag, GetChildren2Request *v);
        int deserialize_CheckVersionRequest(const char *tag, CheckVersionRequest *v);
        int deserialize_GetMaxChildrenRequest(const char *tag, GetMaxChildrenRequest *v);
        int deserialize_GetMaxChildrenResponse(const char *tag, GetMaxChildrenResponse *v);
        int deserialize_SetMaxChildrenRequest(const char *tag, SetMaxChildrenRequest *v);
        int deserialize_SyncRequest(const char *tag, SyncRequest *v);
        int deserialize_SyncResponse(const char *tag, SyncResponse *v);
        int deserialize_GetACLRequest(const char *tag, GetACLRequest *v);
        int deserialize_SetACLRequest(const char *tag, SetACLRequest *v);
        int deserialize_SetACLResponse(const char *tag, SetACLResponse *v);
        int deserialize_AddWatchRequest(const char *tag, AddWatchRequest *v);
        int deserialize_WatcherEvent(const char *tag, WatcherEvent *v);
        int deserialize_ErrorResponse(const char *tag, ErrorResponse *v);
        int deserialize_CreateResponse(const char *tag, CreateResponse *v);
        int deserialize_Create2Response(const char *tag, Create2Response *v);
        int deserialize_ExistsRequest(const char *tag, ExistsRequest *v);
        int deserialize_ExistsResponse(const char *tag, ExistsResponse *v);
        int deserialize_GetDataResponse(const char *tag, GetDataResponse *v);
        int deserialize_GetChildrenResponse(const char *tag, GetChildrenResponse *v);
        int deserialize_GetAllChildrenNumberResponse(const char *tag, GetAllChildrenNumberResponse *v);
        int deserialize_GetChildren2Response(const char *tag, GetChildren2Response *v);
        int deserialize_GetACLResponse(const char *tag, GetACLResponse *v);
        int deserialize_CheckWatchesRequest(const char *tag, CheckWatchesRequest *v);
        int deserialize_RemoveWatchesRequest(const char *tag, RemoveWatchesRequest *v);
        int deserialize_GetEphemeralsRequest(const char *tag, GetEphemeralsRequest *v);
        int deserialize_GetEphemeralsResponse(const char *tag, GetEphemeralsResponse *v);
        int deserialize_ClientInfo_vector(const char *tag, ClientInfoVec *v);
        int deserialize_WhoAmIResponse(const char *tag, WhoAmIResponse *v);
        int deserialize_LearnerInfo(const char *tag, LearnerInfo *v);
        int deserialize_Id_vector(const char *tag, IdVec *v);
        int deserialize_QuorumPacket(const char *tag, QuorumPacket *v);
        int deserialize_QuorumAuthPacket(const char *tag, QuorumAuthPacket *v);
        int deserialize_FileHeader(const char *tag, FileHeader *v);
        int deserialize_TxnDigest(const char *tag, TxnDigest *v);
        int deserialize_TxnHeader(const char *tag, TxnHeader *v);
        int deserialize_CreateTxnV0(const char *tag, CreateTxnV0 *v);
        int deserialize_CreateTxn(const char *tag, CreateTxn *v);
        int deserialize_CreateTTLTxn(const char *tag, CreateTTLTxn *v);
        int deserialize_CreateContainerTxn(const char *tag, CreateContainerTxn *v);
        int deserialize_DeleteTxn(const char *tag, DeleteTxn *v);
        int deserialize_SetDataTxn(const char *tag, SetDataTxn *v);
        int deserialize_CheckVersionTxn(const char *tag, CheckVersionTxn *v);
        int deserialize_SetACLTxn(const char *tag, SetACLTxn *v);
        int deserialize_SetMaxChildrenTxn(const char *tag, SetMaxChildrenTxn *v);
        int deserialize_CreateSessionTxn(const char *tag, CreateSessionTxn *v);
        int deserialize_CloseSessionTxn(const char *tag, CloseSessionTxn *v);
        int deserialize_ErrorTxn(const char *tag, ErrorTxn *v);
        int deserialize_Txn(const char *tag, Txn *v);
        int deserialize_Txn_vector(const char *tag, Txn_vector *v);
        int deserialize_MultiTxn(const char *tag, MultiTxn *v);
    };
    struct oarchive
    {
        oarchive();
        ~oarchive();

        const char *get_buffer() const;
        int get_buffer_len() const;

        int start_vector(const char *tag, int32_t *count);

        int serialize_bool(const char *name, int32_t);
        int serialize_int(const char *name, int32_t);
        int serialize_log(const char *name, int64_t);
        int serialize_buffer(const char *name, const std::string &);
        int serialize_string(const char *name, const std::string &);

        std::string m_buff;

        int serialize_Id(const char *tag, Id *v);
        int serialize_ACL(const char *tag, ACL *v);
        int serialize_Stat(const char *tag, Stat *v);
        int serialize_StatPersisted(const char *tag, StatPersisted *v);
        int serialize_ClientInfo(const char *tag, ClientInfo *v);
        int serialize_ConnectRequest(const char *tag, ConnectRequest *v);
        int serialize_ConnectResponse(const char *tag, ConnectResponse *v);
        int serialize_StringVec(const char *tag, StringVec *v);
        int serialize_SetWatches(const char *tag, SetWatches *v);
        int serialize_SetWatches2(const char *tag, SetWatches2 *v);
        int serialize_RequestHeader(const char *tag, RequestHeader *v);
        int serialize_MultiHeader(const char *tag, MultiHeader *v);
        int serialize_AuthPacket(const char *tag, AuthPacket *v);
        int serialize_ReplyHeader(const char *tag, ReplyHeader *v);
        int serialize_GetDataRequest(const char *tag, GetDataRequest *v);
        int serialize_SetDataRequest(const char *tag, SetDataRequest *v);
        int serialize_ReconfigRequest(const char *tag, ReconfigRequest *v);
        int serialize_SetDataResponse(const char *tag, SetDataResponse *v);
        int serialize_GetSASLRequest(const char *tag, GetSASLRequest *v);
        int serialize_SetSASLRequest(const char *tag, SetSASLRequest *v);
        int serialize_SetSASLResponse(const char *tag, SetSASLResponse *v);
        int serialize_ACL_vector(const char *tag, ACLVec *v);
        int serialize_CreateRequest(const char *tag, CreateRequest *v);
        int serialize_CreateTTLRequest(const char *tag, CreateTTLRequest *v);
        int serialize_DeleteRequest(const char *tag, DeleteRequest *v);
        int serialize_GetChildrenRequest(const char *tag, GetChildrenRequest *v);
        int serialize_GetAllChildrenNumberRequest(const char *tag, GetAllChildrenNumberRequest *v);
        int serialize_GetChildren2Request(const char *tag, GetChildren2Request *v);
        int serialize_CheckVersionRequest(const char *tag, CheckVersionRequest *v);
        int serialize_GetMaxChildrenRequest(const char *tag, GetMaxChildrenRequest *v);
        int serialize_GetMaxChildrenResponse(const char *tag, GetMaxChildrenResponse *v);
        int serialize_SetMaxChildrenRequest(const char *tag, SetMaxChildrenRequest *v);
        int serialize_SyncRequest(const char *tag, SyncRequest *v);
        int serialize_SyncResponse(const char *tag, SyncResponse *v);
        int serialize_GetACLRequest(const char *tag, GetACLRequest *v);
        int serialize_SetACLRequest(const char *tag, SetACLRequest *v);
        int serialize_SetACLResponse(const char *tag, SetACLResponse *v);
        int serialize_AddWatchRequest(const char *tag, AddWatchRequest *v);
        int serialize_WatcherEvent(const char *tag, WatcherEvent *v);
        int serialize_ErrorResponse(const char *tag, ErrorResponse *v);
        int serialize_CreateResponse(const char *tag, CreateResponse *v);
        int serialize_Create2Response(const char *tag, Create2Response *v);
        int serialize_ExistsRequest(const char *tag, ExistsRequest *v);
        int serialize_ExistsResponse(const char *tag, ExistsResponse *v);
        int serialize_GetDataResponse(const char *tag, GetDataResponse *v);
        int serialize_GetChildrenResponse(const char *tag, GetChildrenResponse *v);
        int serialize_GetAllChildrenNumberResponse(const char *tag, GetAllChildrenNumberResponse *v);
        int serialize_GetChildren2Response(const char *tag, GetChildren2Response *v);
        int serialize_GetACLResponse(const char *tag, GetACLResponse *v);
        int serialize_CheckWatchesRequest(const char *tag, CheckWatchesRequest *v);
        int serialize_RemoveWatchesRequest(const char *tag, RemoveWatchesRequest *v);
        int serialize_GetEphemeralsRequest(const char *tag, GetEphemeralsRequest *v);
        int serialize_GetEphemeralsResponse(const char *tag, GetEphemeralsResponse *v);
        int serialize_ClientInfo_vector(const char *tag, ClientInfoVec *v);
        int serialize_WhoAmIResponse(const char *tag, WhoAmIResponse *v);
        int serialize_LearnerInfo(const char *tag, LearnerInfo *v);
        int serialize_Id_vector(const char *tag, IdVec *v);
        int serialize_QuorumPacket(const char *tag, QuorumPacket *v);
        int serialize_QuorumAuthPacket(const char *tag, QuorumAuthPacket *v);
        int serialize_FileHeader(const char *tag, FileHeader *v);
        int serialize_TxnDigest(const char *tag, TxnDigest *v);
        int serialize_TxnHeader(const char *tag, TxnHeader *v);
        int serialize_CreateTxnV0(const char *tag, CreateTxnV0 *v);
        int serialize_CreateTxn(const char *tag, CreateTxn *v);
        int serialize_CreateTTLTxn(const char *tag, CreateTTLTxn *v);
        int serialize_CreateContainerTxn(const char *tag, CreateContainerTxn *v);
        int serialize_DeleteTxn(const char *tag, DeleteTxn *v);
        int serialize_SetDataTxn(const char *tag, SetDataTxn *v);
        int serialize_CheckVersionTxn(const char *tag, CheckVersionTxn *v);
        int serialize_SetACLTxn(const char *tag, SetACLTxn *v);
        int serialize_SetMaxChildrenTxn(const char *tag, SetMaxChildrenTxn *v);
        int serialize_CreateSessionTxn(const char *tag, CreateSessionTxn *v);
        int serialize_CloseSessionTxn(const char *tag, CloseSessionTxn *v);
        int serialize_ErrorTxn(const char *tag, ErrorTxn *v);
        int serialize_Txn(const char *tag, Txn *v);
        int serialize_Txn_vector(const char *tag, Txn_vector *v);
        int serialize_MultiTxn(const char *tag, MultiTxn *v);
    };

    std::unique_ptr<oarchive> create_buffer_oarchive(void);
    std::unique_ptr<iarchive> create_buffer_iarchive(const char *buffer, int len);

}