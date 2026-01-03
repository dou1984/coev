/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <sys/types.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace coev
{

    struct Id_
    {
        std::string scheme;
        std::string id;
        ~Id_();
    };

    struct ACL_
    {
        int32_t perms;
        Id_ id;
        ~ACL_();
    };

    struct Stat_
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

    struct StatPersisted_
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
    struct ClientInfo_
    {
        std::string authScheme;
        std::string user;
    };
    struct ConnectRequest_
    {
        int32_t protocolVersion;
        int64_t lastZxidSeen;
        int32_t timeout;
        int64_t sessionId;
        std::string passwd;
        int32_t readOnly;
    };
    struct ConnectResponse_
    {
        int32_t protocolVersion;
        int32_t timeout;
        int64_t sessionId;
        std::string passwd;
        int32_t readOnly;
    };
    struct StringVec_ : std::vector<std::string>
    {
        auto operator=(std::vector<std::string> &&o)
        {
            std::vector<std::string>::operator=(std::move(o));
            return *this;
        }
        ~StringVec_();
    };
    struct SetWatches_
    {
        int64_t relativeZxid;
        StringVec_ dataWatches;
        StringVec_ existWatches;
        StringVec_ childWatches;
    };
    struct SetWatches2_
    {
        int64_t relativeZxid;
        StringVec_ dataWatches;
        StringVec_ existWatches;
        StringVec_ childWatches;
        StringVec_ persistentWatches;
        StringVec_ persistentRecursiveWatches;
    };
    struct RequestHeader_
    {
        int32_t xid;
        int32_t type;
    };
    struct MultiHeader_
    {
        int32_t type;
        int32_t done;
        int32_t err;
    };
    struct AuthPacket_
    {
        int32_t type;
        std::string scheme;
        std::string auth;
    };
    struct ReplyHeader_
    {
        int32_t xid;
        int64_t zxid;
        int32_t err;
    };
    struct GetDataRequest_
    {
        std::string path;
        int32_t watch;
    };
    struct SetDataRequest_
    {
        std::string path;
        std::string data;
        int32_t version;
    };
    struct ReconfigRequest_
    {
        std::string joiningServers;
        std::string leavingServers;
        std::string newMembers;
        int64_t curConfigId;
    };
    struct SetDataResponse_
    {
        struct Stat_ stat;
    };
    struct GetSASLRequest_
    {
        std::string token;
    };
    struct SetSASLRequest_
    {
        std::string token;
    };
    struct SetSASLResponse_
    {
        std::string token;
    };
    struct ACLVec_ : std::vector<ACL_>
    {
    };

    struct CreateRequest_
    {
        std::string path;
        std::string data;
        ACLVec_ acl;
        int32_t flags;
        void clear();
    };
    struct CreateTTLRequest_
    {
        std::string path;
        std::string data;
        ACLVec_ acl;
        int32_t flags;
        int64_t ttl;
        ~CreateTTLRequest_();
    };
    struct DeleteRequest_
    {
        std::string path;
        int32_t version;
    };
    struct GetChildrenRequest_
    {
        std::string path;
        int32_t watch;
    };
    struct GetAllChildrenNumberRequest_
    {
        std::string path;
    };
    struct GetChildren2Request_
    {
        std::string path;
        int32_t watch;
    };
    struct CheckVersionRequest_
    {
        std::string path;
        int32_t version;
    };
    struct GetMaxChildrenRequest_
    {
        std::string path;
    };
    struct GetMaxChildrenResponse_
    {
        int32_t max;
    };
    struct SetMaxChildrenRequest_
    {
        std::string path;
        int32_t max;
    };
    struct SyncRequest_
    {
        std::string path;
    };
    struct SyncResponse_
    {
        std::string path;
    };
    struct GetACLRequest_
    {
        std::string path;
    };
    struct SetACLRequest_
    {
        std::string path;
        ACLVec_ acl;
        int32_t version;
    };
    struct SetACLResponse_
    {
        struct Stat_ stat;
    };
    struct AddWatchRequest_
    {
        std::string path;
        int32_t mode;
    };
    struct WatcherEvent_
    {
        int32_t type;
        int32_t state;
        std::string path;
        void clear();
    };
    struct ErrorResponse_
    {
        int32_t err;
    };
    struct CreateResponse_
    {
        std::string path;
    };
    struct Create2Response_
    {
        std::string path;
        Stat_ stat;
    };
    struct ExistsRequest_
    {
        std::string path;
        int32_t watch;
    };
    struct ExistsResponse_
    {
        Stat_ stat;
    };
    struct GetDataResponse_
    {
        std::string data;
        Stat_ stat;
    };
    struct GetChildrenResponse_
    {
        StringVec_ children;
    };
    struct GetAllChildrenNumberResponse_
    {
        int32_t totalNumber;
    };
    struct GetChildren2Response_
    {
        StringVec_ children;
        Stat_ stat;
    };
    struct GetACLResponse_
    {
        ACLVec_ acl;
        Stat_ stat;
        void clear();
    };
    struct CheckWatchesRequest_
    {
        std::string path;
        int32_t type;
    };

    struct RemoveWatchesRequest_
    {
        std::string path;
        int32_t type;
        ~RemoveWatchesRequest_() = default;
    };
    struct GetEphemeralsRequest_
    {
        std::string prefixPath;
    };
    struct GetEphemeralsResponse_
    {
        StringVec_ ephemerals;
    };
    struct ClientInfoVec_ : std::vector<ClientInfo_>
    {
    };

    struct WhoAmIResponse_
    {
        ClientInfoVec_ clientInfo;
        ~WhoAmIResponse_();
    };
    struct LearnerInfo_
    {
        int64_t serverid;
        int32_t protocolVersion;
        int64_t configVersion;
    };
    struct IdVec_ : std::vector<Id_>
    {
    };

    struct QuorumPacket_
    {
        int32_t type;
        int64_t zxid;
        std::string data;
        IdVec_ authinfo;
        void clear();
    };
    struct QuorumAuthPacket_
    {
        int64_t magic;
        int32_t status;
        std::string token;
    };
    struct FileHeader_
    {
        int32_t magic;
        int32_t version;
        int64_t dbid;
    };
    struct TxnDigest_
    {
        int32_t version;
        int64_t treeDigest;
    };
    struct TxnHeader_
    {
        int64_t clientId;
        int32_t cxid;
        int64_t zxid;
        int64_t time;
        int32_t type;
    };
    struct CreateTxnV0_
    {
        std::string path;
        std::string data;
        ACLVec_ acl;
        int32_t ephemeral;
    };
    struct CreateTxn_
    {
        std::string path;
        std::string data;
        ACLVec_ acl;
        int32_t ephemeral;
        int32_t parentCVersion;
    };
    struct CreateTTLTxn_
    {
        std::string path;
        std::string data;
        ACLVec_ acl;
        int32_t parentCVersion;
        int64_t ttl;
    };
    struct CreateContainerTxn_
    {
        std::string path;
        std::string data;
        ACLVec_ acl;
        int32_t parentCVersion;
    };
    struct DeleteTxn_
    {
        std::string path;
    };
    struct SetDataTxn_
    {
        std::string path;
        std::string data;
        int32_t version;
    };
    struct CheckVersionTxn_
    {
        std::string path;
        int32_t version;
    };
    struct SetACLTxn_
    {
        std::string path;
        ACLVec_ acl;
        int32_t version;
    };
    struct SetMaxChildrenTxn_
    {
        std::string path;
        int32_t max;
    };
    struct CreateSessionTxn_
    {
        int32_t timeOut;
    };
    struct CloseSessionTxn_
    {
        StringVec_ paths2Delete;
    };
    struct ErrorTxn_
    {
        int32_t err;
    };
    struct Txn_
    {
        int32_t type;
        std::string data;
    };
    struct TxnVec_ : std::vector<Txn_>
    {
    };
    struct MultiTxn_
    {
        TxnVec_ txns;
    };

    struct iarchive
    {
        std::string_view m_view;
        ReplyHeader_ m_header;
        iarchive() = default;
        iarchive(const char *data, size_t size);
        iarchive(const std::string &data);

        int deserialize_bool(int32_t *);
        int deserialize_int(int32_t *);
        int deserialize_long(int64_t *);

        int deserialize_buffer(std::string &);
        int deserialize_string(std::string &s);

        int Id(Id_ *v);
        int ACL(ACL_ *v);
        int Stat(Stat_ *v);
        int StatPersisted(StatPersisted_ *v);
        int ClientInfo(ClientInfo_ *v);
        int ConnectRequest(ConnectRequest_ *v);
        int ConnectResponse(ConnectResponse_ *v);
        int StringVec(StringVec_ *v);
        int SetWatches(SetWatches_ *v);
        int SetWatches2(SetWatches2_ *v);
        int RequestHeader(RequestHeader_ *v);
        int MultiHeader(MultiHeader_ *v);
        int AuthPacket(AuthPacket_ *v);
        int ReplyHeader(ReplyHeader_ *v);
        int GetDataRequest(GetDataRequest_ *v);
        int SetDataRequest(SetDataRequest_ *v);
        int ReconfigRequest(ReconfigRequest_ *v);
        int SetDataResponse(SetDataResponse_ *v);
        int GetSASLRequest(GetSASLRequest_ *v);
        int SetSASLRequest(SetSASLRequest_ *v);
        int SetSASLResponse(SetSASLResponse_ *v);
        int ACLVec(ACLVec_ *v);
        int CreateRequest(CreateRequest_ *v);
        int CreateTTLRequest(CreateTTLRequest_ *v);
        int DeleteRequest(DeleteRequest_ *v);
        int GetChildrenRequest(GetChildrenRequest_ *v);
        int GetAllChildrenNumberRequest(GetAllChildrenNumberRequest_ *v);
        int GetChildren2Request(GetChildren2Request_ *v);
        int CheckVersionRequest(CheckVersionRequest_ *v);
        int GetMaxChildrenRequest(GetMaxChildrenRequest_ *v);
        int GetMaxChildrenResponse(GetMaxChildrenResponse_ *v);
        int SetMaxChildrenRequest(SetMaxChildrenRequest_ *v);
        int SyncRequest(SyncRequest_ *v);
        int SyncResponse(SyncResponse_ *v);
        int GetACLRequest(GetACLRequest_ *v);
        int SetACLRequest(SetACLRequest_ *v);
        int SetACLResponse(SetACLResponse_ *v);
        int AddWatchRequest(AddWatchRequest_ *v);
        int WatcherEvent(WatcherEvent_ *v);
        int ErrorResponse(ErrorResponse_ *v);
        int CreateResponse(CreateResponse_ *v);
        int Create2Response(Create2Response_ *v);
        int ExistsRequest(ExistsRequest_ *v);
        int ExistsResponse(ExistsResponse_ *v);
        int GetDataResponse(GetDataResponse_ *v);
        int GetChildrenResponse(GetChildrenResponse_ *v);
        int GetAllChildrenNumberResponse(GetAllChildrenNumberResponse_ *v);
        int GetChildren2Response(GetChildren2Response_ *v);
        int GetACLResponse(GetACLResponse_ *v);
        int CheckWatchesRequest(CheckWatchesRequest_ *v);
        int RemoveWatchesRequest(RemoveWatchesRequest_ *v);
        int GetEphemeralsRequest(GetEphemeralsRequest_ *v);
        int GetEphemeralsResponse(GetEphemeralsResponse_ *v);
        int ClientInfoVec(ClientInfoVec_ *v);
        int WhoAmIResponse(WhoAmIResponse_ *v);
        int LearnerInfo(LearnerInfo_ *v);
        int IdVec(IdVec_ *v);
        int QuorumPacket(QuorumPacket_ *v);
        int QuorumAuthPacket(QuorumAuthPacket_ *v);
        int FileHeader(FileHeader_ *v);
        int TxnDigest(TxnDigest_ *v);
        int TxnHeader(TxnHeader_ *v);
        int CreateTxnV0(CreateTxnV0_ *v);
        int CreateTxn(CreateTxn_ *v);
        int CreateTTLTxn(CreateTTLTxn_ *v);
        int CreateContainerTxn(CreateContainerTxn_ *v);
        int DeleteTxn(DeleteTxn_ *v);
        int SetDataTxn(SetDataTxn_ *v);
        int CheckVersionTxn(CheckVersionTxn_ *v);
        int SetACLTxn(SetACLTxn_ *v);
        int SetMaxChildrenTxn(SetMaxChildrenTxn_ *v);
        int CreateSessionTxn(CreateSessionTxn_ *v);
        int CloseSessionTxn(CloseSessionTxn_ *v);
        int ErrorTxn(ErrorTxn_ *v);
        int Txn(Txn_ *v);
        int TxnVec(TxnVec_ *v);
        int MultiTxn(MultiTxn_ *v);
    };
    struct oarchive
    {
        oarchive();
        ~oarchive();

        const char *get_buffer() const;
        int get_buffer_len() const;

        int serialize_bool(int32_t);
        int serialize_int(int32_t);
        int serialize_log(int64_t);
        int serialize_buffer(const std::string &);
        int serialize_string(const std::string &);

        std::string m_buff;

        int Id(Id_ *v);
        int ACL(ACL_ *v);
        int Stat(Stat_ *v);
        int StatPersisted(StatPersisted_ *v);
        int ClientInfo(ClientInfo_ *v);
        int ConnectRequest(ConnectRequest_ *v);
        int ConnectResponse(ConnectResponse_ *v);
        int StringVec(StringVec_ *v);
        int SetWatches(SetWatches_ *v);
        int SetWatches2(SetWatches2_ *v);
        int RequestHeader(RequestHeader_ *v);
        int MultiHeader(MultiHeader_ *v);
        int AuthPacket(AuthPacket_ *v);
        int ReplyHeader(ReplyHeader_ *v);
        int GetDataRequest(GetDataRequest_ *v);
        int SetDataRequest(SetDataRequest_ *v);
        int ReconfigRequest(ReconfigRequest_ *v);
        int SetDataResponse(SetDataResponse_ *v);
        int GetSASLRequest(GetSASLRequest_ *v);
        int SetSASLRequest(SetSASLRequest_ *v);
        int SetSASLResponse(SetSASLResponse_ *v);
        int ACLVec(ACLVec_ *v);
        int CreateRequest(CreateRequest_ *v);
        int CreateTTLRequest(CreateTTLRequest_ *v);
        int DeleteRequest(DeleteRequest_ *v);
        int GetChildrenRequest(GetChildrenRequest_ *v);
        int GetAllChildrenNumberRequest(GetAllChildrenNumberRequest_ *v);
        int GetChildren2Request(GetChildren2Request_ *v);
        int CheckVersionRequest(CheckVersionRequest_ *v);
        int GetMaxChildrenRequest(GetMaxChildrenRequest_ *v);
        int GetMaxChildrenResponse(GetMaxChildrenResponse_ *v);
        int SetMaxChildrenRequest(SetMaxChildrenRequest_ *v);
        int SyncRequest(SyncRequest_ *v);
        int SyncResponse(SyncResponse_ *v);
        int GetACLRequest(GetACLRequest_ *v);
        int SetACLRequest(SetACLRequest_ *v);
        int SetACLResponse(SetACLResponse_ *v);
        int AddWatchRequest(AddWatchRequest_ *v);
        int WatcherEvent(WatcherEvent_ *v);
        int ErrorResponse(ErrorResponse_ *v);
        int CreateResponse(CreateResponse_ *v);
        int Create2Response(Create2Response_ *v);
        int ExistsRequest(ExistsRequest_ *v);
        int ExistsResponse(ExistsResponse_ *v);
        int GetDataResponse(GetDataResponse_ *v);
        int GetChildrenResponse(GetChildrenResponse_ *v);
        int GetAllChildrenNumberResponse(GetAllChildrenNumberResponse_ *v);
        int GetChildren2Response(GetChildren2Response_ *v);
        int GetACLResponse(GetACLResponse_ *v);
        int CheckWatchesRequest(CheckWatchesRequest_ *v);
        int RemoveWatchesRequest(RemoveWatchesRequest_ *v);
        int GetEphemeralsRequest(GetEphemeralsRequest_ *v);
        int GetEphemeralsResponse(GetEphemeralsResponse_ *v);
        int ClientInfoVec(ClientInfoVec_ *v);
        int WhoAmIResponse(WhoAmIResponse_ *v);
        int LearnerInfo(LearnerInfo_ *v);
        int IdVec(IdVec_ *v);
        int QuorumPacket(QuorumPacket_ *v);
        int QuorumAuthPacket(QuorumAuthPacket_ *v);
        int FileHeader(FileHeader_ *v);
        int TxnDigest(TxnDigest_ *v);
        int TxnHeader(TxnHeader_ *v);
        int CreateTxnV0(CreateTxnV0_ *v);
        int CreateTxn(CreateTxn_ *v);
        int CreateTTLTxn(CreateTTLTxn_ *v);
        int CreateContainerTxn(CreateContainerTxn_ *v);
        int DeleteTxn(DeleteTxn_ *v);
        int SetDataTxn(SetDataTxn_ *v);
        int CheckVersionTxn(CheckVersionTxn_ *v);
        int SetACLTxn(SetACLTxn_ *v);
        int SetMaxChildrenTxn(SetMaxChildrenTxn_ *v);
        int CreateSessionTxn(CreateSessionTxn_ *v);
        int CloseSessionTxn(CloseSessionTxn_ *v);
        int ErrorTxn(ErrorTxn_ *v);
        int Txn(Txn_ *v);
        int TxnVec(TxnVec_ *v);
        int MultiTxn(MultiTxn_ *v);
    };

    std::unique_ptr<oarchive> create_buffer_oarchive(void);
    std::unique_ptr<iarchive> create_buffer_iarchive(const char *buffer, int len);

}