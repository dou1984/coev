
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <utils/htonll.h>
#include "proto.h"

namespace coev
{

    int oarchive::serialize_int(const char *tag, int32_t d)
    {
        int32_t i = htonl(d);
        m_buff.append((char *)&i, sizeof(i));
        return 0;
    }

    int oarchive::serialize_log(const char *tag, int64_t d)
    {
        const int64_t i = htonll(d);
        m_buff.append((char *)&i, sizeof(i));
        return 0;
    }
    int oarchive::start_vector(const char *tag, int32_t *count)
    {
        return serialize_int(tag, *count);
    }

    int oarchive::serialize_bool(const char *name, int32_t d)
    {
        bool i = d != 0;
        m_buff.append((char *)&i, sizeof(bool));
        return 0;
    }
    int32_t negone = -1;
    int oarchive::serialize_buffer(const char *name, const std::string &s)
    {
        return serialize_string(name, s);
    }
    int oarchive::serialize_string(const char *name, const std::string &s)
    {
        int rc = 0;
        if (s.empty())
        {
            return serialize_int("len", negone);
        }
        int32_t len = s.size();
        rc = serialize_int("len", len);
        if (rc < 0)
        {
            return rc;
        }

        m_buff.append(s.data(), s.size());
        return 0;
    }

    int iarchive::deserialize_int(const char *tag, int32_t *count)
    {
        if (m_view.size() < sizeof(*count))
        {
            return -E2BIG;
        }

        *count = ntohl(*(int32_t *)m_view.data());
        m_view = m_view.substr(sizeof(*count));
        return 0;
    }

    int iarchive::deserialize_long(const char *tag, int64_t *count)
    {
        if (m_view.size() < sizeof(*count))
        {
            return -E2BIG;
        }
        *count = ntohl(*(int64_t *)m_view.data());
        m_view = m_view.substr(sizeof(*count));
        return 0;
    }
    int iarchive::deserialize_bool(const char *name, int32_t *v)
    {
        if (m_view.size() < 1)
        {
            return -E2BIG;
        }
        *v = *(bool *)m_view.data();
        m_view = m_view.substr(1);
        return 0;
    }

    int iarchive::deserialize_buffer(const char *name, std::string &str)
    {
        return deserialize_string(name, str);
    }

    int iarchive::deserialize_string(const char *name, std::string &str)
    {
        int32_t len;
        int rc = deserialize_int("len", &len);
        if (rc < 0)
        {
            return rc;
        }
        if (len < 0)
        {
            return -EINVAL;
        }

        if ((int32_t)m_view.size() < len)
        {
            return -E2BIG;
        }

        str.assign(m_view.data(), len);
        m_view = m_view.substr(len);
        return 0;
    }

    std::unique_ptr<iarchive> create_buffer_iarchive(const char *buffer, int len)
    {
        auto ia = std::make_unique<iarchive>(buffer, len);
        return ia;
    }

    std::unique_ptr<oarchive> create_buffer_oarchive()
    {
        return std::make_unique<oarchive>();
    }

    oarchive::oarchive()
    {
        m_buff.reserve(128);
    }
    oarchive::~oarchive()
    {
    }
    const char *oarchive::get_buffer() const
    {
        return m_buff.data();
    }
    int oarchive::get_buffer_len() const
    {
        return m_buff.size();
    }

    int oarchive::serialize_Id(const char *tag, Id *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("scheme", v->scheme);
        rc = rc ? rc : serialize_string("id", v->id);
        return rc;
    }
    int iarchive::deserialize_Id(const char *tag, Id *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("scheme", v->scheme);
        rc = rc ? rc : deserialize_string("id", v->id);
        return rc;
    }
    Id::~Id()
    {
        scheme.clear();
        id.clear();
    }

    int oarchive::serialize_ACL(const char *tag, ACL *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("perms", v->perms);
        rc = rc ? rc : serialize_Id("id", &v->id);
        return rc;
    }
    int iarchive::deserialize_ACL(const char *tag, ACL *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("perms", &v->perms);
        rc = rc ? rc : deserialize_Id("id", &v->id);
        return rc;
    }
    ACL::~ACL()
    {
    }

    int oarchive::serialize_Stat(const char *tag, Stat *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log("czxid", v->czxid);
        rc = rc ? rc : serialize_log("mzxid", v->mzxid);
        rc = rc ? rc : serialize_log("ctime", v->ctime);
        rc = rc ? rc : serialize_log("mtime", v->mtime);
        rc = rc ? rc : serialize_int("version", v->version);
        rc = rc ? rc : serialize_int("cversion", v->cversion);
        rc = rc ? rc : serialize_int("aversion", v->aversion);
        rc = rc ? rc : serialize_log("ephemeralOwner", v->ephemeralOwner);
        rc = rc ? rc : serialize_int("dataLength", v->dataLength);
        rc = rc ? rc : serialize_int("numChildren", v->numChildren);
        rc = rc ? rc : serialize_log("pzxid", v->pzxid);
        return rc;
    }
    int iarchive::deserialize_Stat(const char *tag, Stat *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long("czxid", &v->czxid);
        rc = rc ? rc : deserialize_long("mzxid", &v->mzxid);
        rc = rc ? rc : deserialize_long("ctime", &v->ctime);
        rc = rc ? rc : deserialize_long("mtime", &v->mtime);
        rc = rc ? rc : deserialize_int("version", &v->version);
        rc = rc ? rc : deserialize_int("cversion", &v->cversion);
        rc = rc ? rc : deserialize_int("aversion", &v->aversion);
        rc = rc ? rc : deserialize_long("ephemeralOwner", &v->ephemeralOwner);
        rc = rc ? rc : deserialize_int("dataLength", &v->dataLength);
        rc = rc ? rc : deserialize_int("numChildren", &v->numChildren);
        rc = rc ? rc : deserialize_long("pzxid", &v->pzxid);
        return rc;
    }

    int oarchive::serialize_StatPersisted(const char *tag, StatPersisted *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log("czxid", v->czxid);
        rc = rc ? rc : serialize_log("mzxid", v->mzxid);
        rc = rc ? rc : serialize_log("ctime", v->ctime);
        rc = rc ? rc : serialize_log("mtime", v->mtime);
        rc = rc ? rc : serialize_int("version", v->version);
        rc = rc ? rc : serialize_int("cversion", v->cversion);
        rc = rc ? rc : serialize_int("aversion", v->aversion);
        rc = rc ? rc : serialize_log("ephemeralOwner", v->ephemeralOwner);
        rc = rc ? rc : serialize_log("pzxid", v->pzxid);
        return rc;
    }
    int iarchive::deserialize_StatPersisted(const char *tag, StatPersisted *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long("czxid", &v->czxid);
        rc = rc ? rc : deserialize_long("mzxid", &v->mzxid);
        rc = rc ? rc : deserialize_long("ctime", &v->ctime);
        rc = rc ? rc : deserialize_long("mtime", &v->mtime);
        rc = rc ? rc : deserialize_int("version", &v->version);
        rc = rc ? rc : deserialize_int("cversion", &v->cversion);
        rc = rc ? rc : deserialize_int("aversion", &v->aversion);
        rc = rc ? rc : deserialize_long("ephemeralOwner", &v->ephemeralOwner);
        rc = rc ? rc : deserialize_long("pzxid", &v->pzxid);
        return rc;
    }

    int oarchive::serialize_ClientInfo(const char *tag, ClientInfo *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("authScheme", v->authScheme);
        rc = rc ? rc : serialize_string("user", v->user);
        return rc;
    }
    int iarchive::deserialize_ClientInfo(const char *tag, ClientInfo *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("authScheme", v->authScheme);
        rc = rc ? rc : deserialize_string("user", v->user);
        return rc;
    }
    void deallocate_ClientInfo(ClientInfo *v)
    {
        v->authScheme.clear();
        v->user.clear();
    }
    int oarchive::serialize_ConnectRequest(const char *tag, ConnectRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("protocolVersion", v->protocolVersion);
        rc = rc ? rc : serialize_log("lastZxidSeen", v->lastZxidSeen);
        rc = rc ? rc : serialize_int("timeOut", v->timeout);
        rc = rc ? rc : serialize_log("sessionId", v->sessionId);
        rc = rc ? rc : serialize_buffer("passwd", v->passwd);
        rc = rc ? rc : serialize_bool("readOnly", v->readOnly);
        return rc;
    }
    int iarchive::deserialize_ConnectRequest(const char *tag, ConnectRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("protocolVersion", &v->protocolVersion);
        rc = rc ? rc : deserialize_long("lastZxidSeen", &v->lastZxidSeen);
        rc = rc ? rc : deserialize_int("timeOut", &v->timeout);
        rc = rc ? rc : deserialize_long("sessionId", &v->sessionId);
        rc = rc ? rc : deserialize_buffer("passwd", v->passwd);
        rc = rc ? rc : deserialize_bool("readOnly", &v->readOnly);
        return rc;
    }
    void deallocate_ConnectRequest(ConnectRequest *v)
    {
        v->passwd.clear();
    }
    int oarchive::serialize_ConnectResponse(const char *tag, ConnectResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("protocolVersion", v->protocolVersion);
        rc = rc ? rc : serialize_int("timeOut", v->timeout);
        rc = rc ? rc : serialize_log("sessionId", v->sessionId);
        rc = rc ? rc : serialize_buffer("passwd", v->passwd);
        rc = rc ? rc : serialize_bool("readOnly", v->readOnly);
        return rc;
    }
    int iarchive::deserialize_ConnectResponse(const char *tag, ConnectResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("protocolVersion", &v->protocolVersion);
        rc = rc ? rc : deserialize_int("timeOut", &v->timeout);
        rc = rc ? rc : deserialize_long("sessionId", &v->sessionId);
        rc = rc ? rc : deserialize_buffer("passwd", v->passwd);
        rc = rc ? rc : deserialize_bool("readOnly", &v->readOnly);
        return rc;
    }
    void deallocate_ConnectResponse(ConnectResponse *v)
    {
        v->passwd.clear();
    }

    StringVec::~StringVec()
    {
        clear();
    }

    int oarchive::serialize_StringVec(const char *tag, StringVec *v)
    {
        int32_t count = v->size();
        int rc = 0;
        rc = start_vector(tag, &count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : serialize_string("data", v->at(i));
        }
        return rc;
    }
    int iarchive::deserialize_String_vector(const char *tag, StringVec *v)
    {
        int rc = 0;
        int32_t count = 0;
        rc = deserialize_int(tag, &count);
        v->resize(count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : deserialize_string("value", v->at(i));
        }
        return rc;
    }
    int oarchive::serialize_SetWatches(const char *tag, SetWatches *v)
    {
        int rc = 0;

        rc = rc ? rc : serialize_log("relativeZxid", v->relativeZxid);
        rc = rc ? rc : serialize_StringVec("dataWatches", &v->dataWatches);
        rc = rc ? rc : serialize_StringVec("existWatches", &v->existWatches);
        rc = rc ? rc : serialize_StringVec("childWatches", &v->childWatches);

        return rc;
    }
    int iarchive::deserialize_SetWatches(const char *tag, SetWatches *v)
    {
        int rc = 0;

        rc = rc ? rc : deserialize_long("relativeZxid", &v->relativeZxid);
        rc = rc ? rc : deserialize_String_vector("dataWatches", &v->dataWatches);
        rc = rc ? rc : deserialize_String_vector("existWatches", &v->existWatches);
        rc = rc ? rc : deserialize_String_vector("childWatches", &v->childWatches);

        return rc;
    }
    void deallocate_SetWatches(SetWatches *v)
    {
        v->dataWatches.clear();
        v->existWatches.clear();
        v->childWatches.clear();
    }
    int oarchive::serialize_SetWatches2(const char *tag, SetWatches2 *v)
    {
        int rc = 0;

        rc = rc ? rc : serialize_log("relativeZxid", v->relativeZxid);
        rc = rc ? rc : serialize_StringVec("dataWatches", &v->dataWatches);
        rc = rc ? rc : serialize_StringVec("existWatches", &v->existWatches);
        rc = rc ? rc : serialize_StringVec("childWatches", &v->childWatches);
        rc = rc ? rc : serialize_StringVec("persistentWatches", &v->persistentWatches);
        rc = rc ? rc : serialize_StringVec("persistentRecursiveWatches", &v->persistentRecursiveWatches);

        return rc;
    }
    int iarchive::deserialize_SetWatches2(const char *tag, SetWatches2 *v)
    {
        int rc = 0;

        rc = rc ? rc : deserialize_long("relativeZxid", &v->relativeZxid);
        rc = rc ? rc : deserialize_String_vector("dataWatches", &v->dataWatches);
        rc = rc ? rc : deserialize_String_vector("existWatches", &v->existWatches);
        rc = rc ? rc : deserialize_String_vector("childWatches", &v->childWatches);
        rc = rc ? rc : deserialize_String_vector("persistentWatches", &v->persistentWatches);
        rc = rc ? rc : deserialize_String_vector("persistentRecursiveWatches", &v->persistentRecursiveWatches);

        return rc;
    }
    void deallocate_SetWatches2(SetWatches2 *v)
    {
        v->dataWatches.clear();
        v->existWatches.clear();
        v->childWatches.clear();
        v->persistentWatches.clear();
        v->persistentRecursiveWatches.clear();
    }
    int oarchive::serialize_RequestHeader(const char *tag, RequestHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("xid", v->xid);
        rc = rc ? rc : serialize_int("type", v->type);
        return rc;
    }
    int iarchive::deserialize_RequestHeader(const char *tag, RequestHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("xid", &v->xid);
        rc = rc ? rc : deserialize_int("type", &v->type);
        return rc;
    }

    int oarchive::serialize_MultiHeader(const char *tag, MultiHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("type", v->type);
        rc = rc ? rc : serialize_bool("done", v->done);
        rc = rc ? rc : serialize_int("err", v->err);
        return rc;
    }
    int iarchive::deserialize_MultiHeader(const char *tag, MultiHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("type", &v->type);
        rc = rc ? rc : deserialize_bool("done", &v->done);
        rc = rc ? rc : deserialize_int("err", &v->err);
        return rc;
    }
    void deallocate_MultiHeader(MultiHeader *v)
    {
    }
    int oarchive::serialize_AuthPacket(const char *tag, AuthPacket *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("type", v->type);
        rc = rc ? rc : serialize_string("scheme", v->scheme);
        rc = rc ? rc : serialize_buffer("auth", v->auth);
        return rc;
    }
    int iarchive::deserialize_AuthPacket(const char *tag, AuthPacket *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("type", &v->type);
        rc = rc ? rc : deserialize_string("scheme", v->scheme);
        rc = rc ? rc : deserialize_buffer("auth", v->auth);
        return rc;
    }
    void deallocate_AuthPacket(AuthPacket *v)
    {
        v->scheme.clear();
        v->auth.clear();
    }
    int oarchive::serialize_ReplyHeader(const char *tag, ReplyHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("xid", v->xid);
        rc = rc ? rc : serialize_log("zxid", v->zxid);
        rc = rc ? rc : serialize_int("err", v->err);
        return rc;
    }
    int iarchive::deserialize_ReplyHeader(const char *tag, ReplyHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("xid", &v->xid);
        rc = rc ? rc : deserialize_long("zxid", &v->zxid);
        rc = rc ? rc : deserialize_int("err", &v->err);
        return rc;
    }
    void deallocate_ReplyHeader(ReplyHeader *v)
    {
    }
    int oarchive::serialize_GetDataRequest(const char *tag, GetDataRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_bool("watch", v->watch);
        return rc;
    }
    int iarchive::deserialize_GetDataRequest(const char *tag, GetDataRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_bool("watch", &v->watch);
        return rc;
    }
    void deallocate_GetDataRequest(GetDataRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_SetDataRequest(const char *tag, SetDataRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_int("version", v->version);
        return rc;
    }
    int iarchive::deserialize_SetDataRequest(const char *tag, SetDataRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_int("version", &v->version);
        return rc;
    }
    void deallocate_SetDataRequest(SetDataRequest *v)
    {
        v->path.clear();
        v->data.clear();
    }
    int oarchive::serialize_ReconfigRequest(const char *tag, ReconfigRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("joiningServers", v->joiningServers);
        rc = rc ? rc : serialize_string("leavingServers", v->leavingServers);
        rc = rc ? rc : serialize_string("newMembers", v->newMembers);
        rc = rc ? rc : serialize_log("curConfigId", v->curConfigId);
        return rc;
    }
    int iarchive::deserialize_ReconfigRequest(const char *tag, ReconfigRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("joiningServers", v->joiningServers);
        rc = rc ? rc : deserialize_string("leavingServers", v->leavingServers);
        rc = rc ? rc : deserialize_string("newMembers", v->newMembers);
        rc = rc ? rc : deserialize_long("curConfigId", &v->curConfigId);
        return rc;
    }
    void deallocate_ReconfigRequest(ReconfigRequest *v)
    {
        v->joiningServers.clear();
        v->leavingServers.clear();
        v->newMembers.clear();
    }
    int oarchive::serialize_SetDataResponse(const char *tag, SetDataResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_Stat("stat", &v->stat);
        return rc;
    }
    int iarchive::deserialize_SetDataResponse(const char *tag, SetDataResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_Stat("stat", &v->stat);
        return rc;
    }
    void deallocate_SetDataResponse(SetDataResponse *v)
    {
    }
    int oarchive::serialize_GetSASLRequest(const char *tag, GetSASLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_buffer("token", v->token);
        return rc;
    }
    int iarchive::deserialize_GetSASLRequest(const char *tag, GetSASLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_buffer("token", v->token);
        return rc;
    }
    void deallocate_GetSASLRequest(GetSASLRequest *v)
    {
        v->token.clear();
    }
    int oarchive::serialize_SetSASLRequest(const char *tag, SetSASLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_buffer("token", v->token);
        return rc;
    }
    int iarchive::deserialize_SetSASLRequest(const char *tag, SetSASLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_buffer("token", v->token);
        return rc;
    }
    void deallocate_SetSASLRequest(SetSASLRequest *v)
    {
        v->token.clear();
    }
    int oarchive::serialize_SetSASLResponse(const char *tag, SetSASLResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_buffer("token", v->token);
        return rc;
    }
    int iarchive::deserialize_SetSASLResponse(const char *tag, SetSASLResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_buffer("token", v->token);
        return rc;
    }

    int oarchive::serialize_ACL_vector(const char *tag, ACLVec *v)
    {
        int32_t count = 0;
        int rc = 0;
        int32_t i;
        rc = start_vector(tag, &count);
        for (i = 0; i < count; i++)
        {
            rc = rc ? rc : serialize_ACL("data", &(*v)[i]);
        }
        return rc;
    }
    int iarchive::deserialize_ACLVec(const char *tag, ACLVec *v)
    {
        int rc = 0;
        int32_t i;
        int count = 0;
        rc = deserialize_int(tag, &count);
        v->resize(count);
        for (i = 0; i < count; i++)
        {
            rc = rc ? rc : deserialize_ACL("value", &(*v)[i]);
        }
        return rc;
    }
    int oarchive::serialize_CreateRequest(const char *tag, CreateRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_ACL_vector("acl", &v->acl);
        rc = rc ? rc : serialize_int("flags", v->flags);
        return rc;
    }
    int iarchive::deserialize_CreateRequest(const char *tag, CreateRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_ACLVec("acl", &v->acl);
        rc = rc ? rc : deserialize_int("flags", &v->flags);
        return rc;
    }
    void CreateRequest::clear()
    {
        path.clear();
        data.clear();
        acl.clear();
    }

    int oarchive::serialize_CreateTTLRequest(const char *tag, CreateTTLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_ACL_vector("acl", &v->acl);
        rc = rc ? rc : serialize_int("flags", v->flags);
        rc = rc ? rc : serialize_log("ttl", v->ttl);
        return rc;
    }
    int iarchive::deserialize_CreateTTLRequest(const char *tag, CreateTTLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_ACLVec("acl", &v->acl);
        rc = rc ? rc : deserialize_int("flags", &v->flags);
        rc = rc ? rc : deserialize_long("ttl", &v->ttl);
        return rc;
    }
    CreateTTLRequest::~CreateTTLRequest()
    {
        path.clear();
        data.clear();
        acl.clear();
    }

    int oarchive::serialize_DeleteRequest(const char *tag, DeleteRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_int("version", v->version);
        return rc;
    }
    int iarchive::deserialize_DeleteRequest(const char *tag, DeleteRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_int("version", &v->version);
        return rc;
    }
    void deallocate_DeleteRequest(DeleteRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_GetChildrenRequest(const char *tag, GetChildrenRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_bool("watch", v->watch);
        return rc;
    }

    iarchive::iarchive(const char *data, size_t size)
    {
        m_view = std::string_view(data, size);
    }
    iarchive::iarchive(const std::string &data)
    {
        m_view = std::string_view(data.data(), data.size());
    }
    int iarchive::deserialize_GetChildrenRequest(const char *tag, GetChildrenRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_bool("watch", &v->watch);
        return rc;
    }
    void deallocate_GetChildrenRequest(GetChildrenRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_GetAllChildrenNumberRequest(const char *tag, GetAllChildrenNumberRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        return rc;
    }
    int iarchive::deserialize_GetAllChildrenNumberRequest(const char *tag, GetAllChildrenNumberRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        return rc;
    }
    void deallocate_GetAllChildrenNumberRequest(GetAllChildrenNumberRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_GetChildren2Request(const char *tag, GetChildren2Request *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_bool("watch", v->watch);
        return rc;
    }
    int iarchive::deserialize_GetChildren2Request(const char *tag, GetChildren2Request *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_bool("watch", &v->watch);
        return rc;
    }
    void deallocate_GetChildren2Request(GetChildren2Request *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_CheckVersionRequest(const char *tag, CheckVersionRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_int("version", v->version);
        return rc;
    }
    int iarchive::deserialize_CheckVersionRequest(const char *tag, CheckVersionRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_int("version", &v->version);
        return rc;
    }
    void deallocate_CheckVersionRequest(CheckVersionRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_GetMaxChildrenRequest(const char *tag, GetMaxChildrenRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        return rc;
    }
    int iarchive::deserialize_GetMaxChildrenRequest(const char *tag, GetMaxChildrenRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        return rc;
    }
    void deallocate_GetMaxChildrenRequest(GetMaxChildrenRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_GetMaxChildrenResponse(const char *tag, GetMaxChildrenResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("max", v->max);
        return rc;
    }
    int iarchive::deserialize_GetMaxChildrenResponse(const char *tag, GetMaxChildrenResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("max", &v->max);
        return rc;
    }
    void deallocate_GetMaxChildrenResponse(GetMaxChildrenResponse *v)
    {
    }
    int oarchive::serialize_SetMaxChildrenRequest(const char *tag, SetMaxChildrenRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_int("max", v->max);
        return rc;
    }
    int iarchive::deserialize_SetMaxChildrenRequest(const char *tag, SetMaxChildrenRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_int("max", &v->max);
        return rc;
    }
    void deallocate_SetMaxChildrenRequest(SetMaxChildrenRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_SyncRequest(const char *tag, SyncRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        return rc;
    }
    int iarchive::deserialize_SyncRequest(const char *tag, SyncRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        return rc;
    }
    void deallocate_SyncRequest(SyncRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_SyncResponse(const char *tag, SyncResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        return rc;
    }
    int iarchive::deserialize_SyncResponse(const char *tag, SyncResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        return rc;
    }
    void deallocate_SyncResponse(SyncResponse *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_GetACLRequest(const char *tag, GetACLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        return rc;
    }
    int iarchive::deserialize_GetACLRequest(const char *tag, GetACLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        return rc;
    }
    void deallocate_GetACLRequest(GetACLRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_SetACLRequest(const char *tag, SetACLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_ACL_vector("acl", &v->acl);
        rc = rc ? rc : serialize_int("version", v->version);
        return rc;
    }
    int iarchive::deserialize_SetACLRequest(const char *tag, SetACLRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_ACLVec("acl", &v->acl);
        rc = rc ? rc : deserialize_int("version", &v->version);
        return rc;
    }
    void deallocate_SetACLRequest(SetACLRequest *v)
    {
        v->path.clear();
        v->acl.clear();
    }
    int oarchive::serialize_SetACLResponse(const char *tag, SetACLResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_Stat("stat", &v->stat);
        return rc;
    }
    int iarchive::deserialize_SetACLResponse(const char *tag, SetACLResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_Stat("stat", &v->stat);
        return rc;
    }
    void deallocate_SetACLResponse(SetACLResponse *v)
    {
    }
    int oarchive::serialize_AddWatchRequest(const char *tag, AddWatchRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_int("mode", v->mode);
        return rc;
    }
    int iarchive::deserialize_AddWatchRequest(const char *tag, AddWatchRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_int("mode", &v->mode);
        return rc;
    }
    void deallocate_AddWatchRequest(AddWatchRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_WatcherEvent(const char *tag, WatcherEvent *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("type", v->type);
        rc = rc ? rc : serialize_int("state", v->state);
        rc = rc ? rc : serialize_string("path", v->path);
        return rc;
    }
    int iarchive::deserialize_WatcherEvent(const char *tag, WatcherEvent *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("type", &v->type);
        rc = rc ? rc : deserialize_int("state", &v->state);
        rc = rc ? rc : deserialize_string("path", v->path);
        return rc;
    }
    void WatcherEvent::clear()
    {
        path.clear();
    }

    int oarchive::serialize_ErrorResponse(const char *tag, ErrorResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("err", v->err);
        return rc;
    }
    int iarchive::deserialize_ErrorResponse(const char *tag, ErrorResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("err", &v->err);
        return rc;
    }
    void deallocate_ErrorResponse(ErrorResponse *v)
    {
    }
    int oarchive::serialize_CreateResponse(const char *tag, CreateResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        return rc;
    }
    int iarchive::deserialize_CreateResponse(const char *tag, CreateResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        return rc;
    }
    void deallocate_CreateResponse(CreateResponse *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_Create2Response(const char *tag, Create2Response *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_Stat("stat", &v->stat);
        return rc;
    }
    int iarchive::deserialize_Create2Response(const char *tag, Create2Response *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_Stat("stat", &v->stat);
        return rc;
    }
    void deallocate_Create2Response(Create2Response *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_ExistsRequest(const char *tag, ExistsRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_bool("watch", v->watch);
        return rc;
    }
    int iarchive::deserialize_ExistsRequest(const char *tag, ExistsRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_bool("watch", &v->watch);
        return rc;
    }
    void deallocate_ExistsRequest(ExistsRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_ExistsResponse(const char *tag, ExistsResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_Stat("stat", &v->stat);
        return rc;
    }
    int iarchive::deserialize_ExistsResponse(const char *tag, ExistsResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_Stat("stat", &v->stat);
        return rc;
    }
    void deallocate_ExistsResponse(ExistsResponse *v)
    {
    }
    int oarchive::serialize_GetDataResponse(const char *tag, GetDataResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_Stat("stat", &v->stat);
        return rc;
    }
    int iarchive::deserialize_GetDataResponse(const char *tag, GetDataResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_Stat("stat", &v->stat);
        return rc;
    }

    int oarchive::serialize_GetChildrenResponse(const char *tag, GetChildrenResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_StringVec("children", &v->children);
        return rc;
    }
    int iarchive::deserialize_GetChildrenResponse(const char *tag, GetChildrenResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_String_vector("children", &v->children);
        return rc;
    }
    void deallocate_GetChildrenResponse(GetChildrenResponse *v)
    {
        v->children.clear();
    }
    int oarchive::serialize_GetAllChildrenNumberResponse(const char *tag, GetAllChildrenNumberResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("totalNumber", v->totalNumber);
        return rc;
    }
    int iarchive::deserialize_GetAllChildrenNumberResponse(const char *tag, GetAllChildrenNumberResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("totalNumber", &v->totalNumber);
        return rc;
    }
    void deallocate_GetAllChildrenNumberResponse(GetAllChildrenNumberResponse *v)
    {
    }
    int oarchive::serialize_GetChildren2Response(const char *tag, GetChildren2Response *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_StringVec("children", &v->children);
        rc = rc ? rc : serialize_Stat("stat", &v->stat);
        return rc;
    }
    int iarchive::deserialize_GetChildren2Response(const char *tag, GetChildren2Response *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_String_vector("children", &v->children);
        rc = rc ? rc : deserialize_Stat("stat", &v->stat);
        return rc;
    }
    void deallocate_GetChildren2Response(GetChildren2Response *v)
    {
        v->children.clear();
    }
    int oarchive::serialize_GetACLResponse(const char *tag, GetACLResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_ACL_vector("acl", &v->acl);
        rc = rc ? rc : serialize_Stat("stat", &v->stat);
        return rc;
    }
    int iarchive::deserialize_GetACLResponse(const char *tag, GetACLResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_ACLVec("acl", &v->acl);
        rc = rc ? rc : deserialize_Stat("stat", &v->stat);
        return rc;
    }

    void GetACLResponse::clear()
    {
        acl.clear();
    }

    int oarchive::serialize_CheckWatchesRequest(const char *tag, CheckWatchesRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_int("type", v->type);
        return rc;
    }
    int iarchive::deserialize_CheckWatchesRequest(const char *tag, CheckWatchesRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_int("type", &v->type);
        return rc;
    }
    void deallocate_CheckWatchesRequest(CheckWatchesRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_RemoveWatchesRequest(const char *tag, RemoveWatchesRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_int("type", v->type);
        return rc;
    }
    int iarchive::deserialize_RemoveWatchesRequest(const char *tag, RemoveWatchesRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_int("type", &v->type);
        return rc;
    }
    void deallocate_RemoveWatchesRequest(RemoveWatchesRequest *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_GetEphemeralsRequest(const char *tag, GetEphemeralsRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("prefixPath", v->prefixPath);
        return rc;
    }
    int iarchive::deserialize_GetEphemeralsRequest(const char *tag, GetEphemeralsRequest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("prefixPath", v->prefixPath);
        return rc;
    }
    void deallocate_GetEphemeralsRequest(GetEphemeralsRequest *v)
    {
        v->prefixPath.clear();
    }
    int oarchive::serialize_GetEphemeralsResponse(const char *tag, GetEphemeralsResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_StringVec("ephemerals", &v->ephemerals);
        return rc;
    }
    int iarchive::deserialize_GetEphemeralsResponse(const char *tag, GetEphemeralsResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_String_vector("ephemerals", &v->ephemerals);
        return rc;
    }
    void deallocate_GetEphemeralsResponse(GetEphemeralsResponse *v)
    {
        v->ephemerals.clear();
    }
    int allocate_ClientInfo_vector(ClientInfoVec *v, int32_t len)
    {

        v->resize(len);
        return 0;
    }
    int deallocate_ClientInfo_vector(ClientInfoVec *v)
    {
        v->clear();
        return 0;
    }
    int oarchive::serialize_ClientInfo_vector(const char *tag, ClientInfoVec *v)
    {
        int32_t count = v->size();
        int rc = start_vector(tag, &count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : serialize_ClientInfo("data", &(*v)[i]);
        }
        return rc;
    }
    int iarchive::deserialize_ClientInfo_vector(const char *tag, ClientInfoVec *v)
    {
        int32_t count = 0;
        auto rc = deserialize_int(tag, &count);
        v->resize(count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : deserialize_ClientInfo("value", &(*v)[i]);
        }
        return rc;
    }
    int oarchive::serialize_WhoAmIResponse(const char *tag, WhoAmIResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_ClientInfo_vector("clientInfo", &v->clientInfo);
        return rc;
    }
    int iarchive::deserialize_WhoAmIResponse(const char *tag, WhoAmIResponse *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_ClientInfo_vector("clientInfo", &v->clientInfo);
        return rc;
    }
    WhoAmIResponse::~WhoAmIResponse()
    {
        clientInfo.clear();
    }

    int oarchive::serialize_LearnerInfo(const char *tag, LearnerInfo *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log("serverid", v->serverid);
        rc = rc ? rc : serialize_int("protocolVersion", v->protocolVersion);
        rc = rc ? rc : serialize_log("configVersion", v->configVersion);
        return rc;
    }
    int iarchive::deserialize_LearnerInfo(const char *tag, LearnerInfo *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long("serverid", &v->serverid);
        rc = rc ? rc : deserialize_int("protocolVersion", &v->protocolVersion);
        rc = rc ? rc : deserialize_long("configVersion", &v->configVersion);
        return rc;
    }

    int allocate_Id_vector(IdVec *v, int32_t len)
    {
        v->reserve(len);
        return 0;
    }

    int oarchive::serialize_Id_vector(const char *tag, IdVec *v)
    {
        int32_t count = v->size();
        int rc = start_vector(tag, &count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : serialize_Id("data", &(*v)[i]);
        }
        return rc;
    }
    int iarchive::deserialize_Id_vector(const char *tag, IdVec *v)
    {
        int32_t count;
        int rc = deserialize_int(tag, &count);
        v->resize(count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : deserialize_Id("value", &(*v)[i]);
        }
        return rc;
    }
    int oarchive::serialize_QuorumPacket(const char *tag, QuorumPacket *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("type", v->type);
        rc = rc ? rc : serialize_log("zxid", v->zxid);
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_Id_vector("authinfo", &v->authinfo);
        return rc;
    }
    int iarchive::deserialize_QuorumPacket(const char *tag, QuorumPacket *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("type", &v->type);
        rc = rc ? rc : deserialize_long("zxid", &v->zxid);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_Id_vector("authinfo", &v->authinfo);
        return rc;
    }
    void QuorumPacket::clear()
    {
        data.clear();
        authinfo.clear();
    }
    int oarchive::serialize_QuorumAuthPacket(const char *tag, QuorumAuthPacket *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log("magic", v->magic);
        rc = rc ? rc : serialize_int("status", v->status);
        rc = rc ? rc : serialize_buffer("token", v->token);
        return rc;
    }
    int iarchive::deserialize_QuorumAuthPacket(const char *tag, QuorumAuthPacket *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long("magic", &v->magic);
        rc = rc ? rc : deserialize_int("status", &v->status);
        rc = rc ? rc : deserialize_buffer("token", v->token);
        return rc;
    }
    void deallocate_QuorumAuthPacket(QuorumAuthPacket *v)
    {
        v->token.clear();
    }
    int oarchive::serialize_FileHeader(const char *tag, FileHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("magic", v->magic);
        rc = rc ? rc : serialize_int("version", v->version);
        rc = rc ? rc : serialize_log("dbid", v->dbid);
        return rc;
    }
    int iarchive::deserialize_FileHeader(const char *tag, FileHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("magic", &v->magic);
        rc = rc ? rc : deserialize_int("version", &v->version);
        rc = rc ? rc : deserialize_long("dbid", &v->dbid);
        return rc;
    }
    void deallocate_FileHeader(FileHeader *v)
    {
    }
    int oarchive::serialize_TxnDigest(const char *tag, TxnDigest *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("version", v->version);
        rc = rc ? rc : serialize_log("treeDigest", v->treeDigest);
        return rc;
    }
    int iarchive::deserialize_TxnDigest(const char *tag, TxnDigest *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("version", &v->version);
        rc = rc ? rc : deserialize_long("treeDigest", &v->treeDigest);
        return rc;
    }
    void deallocate_TxnDigest(TxnDigest *v)
    {
    }
    int oarchive::serialize_TxnHeader(const char *tag, TxnHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log("clientId", v->clientId);
        rc = rc ? rc : serialize_int("cxid", v->cxid);
        rc = rc ? rc : serialize_log("zxid", v->zxid);
        rc = rc ? rc : serialize_log("time", v->time);
        rc = rc ? rc : serialize_int("type", v->type);
        return rc;
    }
    int iarchive::deserialize_TxnHeader(const char *tag, TxnHeader *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long("clientId", &v->clientId);
        rc = rc ? rc : deserialize_int("cxid", &v->cxid);
        rc = rc ? rc : deserialize_long("zxid", &v->zxid);
        rc = rc ? rc : deserialize_long("time", &v->time);
        rc = rc ? rc : deserialize_int("type", &v->type);
        return rc;
    }
    void deallocate_TxnHeader(TxnHeader *v)
    {
    }
    int oarchive::serialize_CreateTxnV0(const char *tag, CreateTxnV0 *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_ACL_vector("acl", &v->acl);
        rc = rc ? rc : serialize_bool("ephemeral", v->ephemeral);
        return rc;
    }
    int iarchive::deserialize_CreateTxnV0(const char *tag, CreateTxnV0 *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_ACLVec("acl", &v->acl);
        rc = rc ? rc : deserialize_bool("ephemeral", &v->ephemeral);
        return rc;
    }
    void deallocate_CreateTxnV0(CreateTxnV0 *v)
    {
        v->path.clear();
        v->data.clear();
        v->acl.clear();
    }
    int oarchive::serialize_CreateTxn(const char *tag, CreateTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_ACL_vector("acl", &v->acl);
        rc = rc ? rc : serialize_bool("ephemeral", v->ephemeral);
        rc = rc ? rc : serialize_int("parentCVersion", v->parentCVersion);
        return rc;
    }
    int iarchive::deserialize_CreateTxn(const char *tag, CreateTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_ACLVec("acl", &v->acl);
        rc = rc ? rc : deserialize_bool("ephemeral", &v->ephemeral);
        rc = rc ? rc : deserialize_int("parentCVersion", &v->parentCVersion);
        return rc;
    }
    void deallocate_CreateTxn(CreateTxn *v)
    {
        v->path.clear();
        v->data.clear();
        v->acl.clear();
    }
    int oarchive::serialize_CreateTTLTxn(const char *tag, CreateTTLTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_ACL_vector("acl", &v->acl);
        rc = rc ? rc : serialize_int("parentCVersion", v->parentCVersion);
        rc = rc ? rc : serialize_log("ttl", v->ttl);
        return rc;
    }
    int iarchive::deserialize_CreateTTLTxn(const char *tag, CreateTTLTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_ACLVec("acl", &v->acl);
        rc = rc ? rc : deserialize_int("parentCVersion", &v->parentCVersion);
        rc = rc ? rc : deserialize_long("ttl", &v->ttl);
        return rc;
    }
    void deallocate_CreateTTLTxn(CreateTTLTxn *v)
    {
        v->path.clear();
        v->data.clear();
        v->acl.clear();
    }
    int oarchive::serialize_CreateContainerTxn(const char *tag, CreateContainerTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_ACL_vector("acl", &v->acl);
        rc = rc ? rc : serialize_int("parentCVersion", v->parentCVersion);
        return rc;
    }
    int iarchive::deserialize_CreateContainerTxn(const char *tag, CreateContainerTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_ACLVec("acl", &v->acl);
        rc = rc ? rc : deserialize_int("parentCVersion", &v->parentCVersion);
        return rc;
    }
    void deallocate_CreateContainerTxn(CreateContainerTxn *v)
    {
        v->path.clear();
        v->data.clear();
        v->acl.clear();
    }
    int oarchive::serialize_DeleteTxn(const char *tag, DeleteTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        return rc;
    }
    int iarchive::deserialize_DeleteTxn(const char *tag, DeleteTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        return rc;
    }
    void deallocate_DeleteTxn(DeleteTxn *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_SetDataTxn(const char *tag, SetDataTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_buffer("data", v->data);
        rc = rc ? rc : serialize_int("version", v->version);
        return rc;
    }
    int iarchive::deserialize_SetDataTxn(const char *tag, SetDataTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        rc = rc ? rc : deserialize_int("version", &v->version);
        return rc;
    }
    void deallocate_SetDataTxn(SetDataTxn *v)
    {
        v->path.clear();
        v->data.clear();
    }
    int oarchive::serialize_CheckVersionTxn(const char *tag, CheckVersionTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_int("version", v->version);
        return rc;
    }
    int iarchive::deserialize_CheckVersionTxn(const char *tag, CheckVersionTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_int("version", &v->version);
        return rc;
    }
    void deallocate_CheckVersionTxn(CheckVersionTxn *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_SetACLTxn(const char *tag, SetACLTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_ACL_vector("acl", &v->acl);
        rc = rc ? rc : serialize_int("version", v->version);
        return rc;
    }
    int iarchive::deserialize_SetACLTxn(const char *tag, SetACLTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_ACLVec("acl", &v->acl);
        rc = rc ? rc : deserialize_int("version", &v->version);
        return rc;
    }
    void deallocate_SetACLTxn(SetACLTxn *v)
    {
        v->path.clear();
        v->acl.clear();
    }
    int oarchive::serialize_SetMaxChildrenTxn(const char *tag, SetMaxChildrenTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string("path", v->path);
        rc = rc ? rc : serialize_int("max", v->max);
        return rc;
    }
    int iarchive::deserialize_SetMaxChildrenTxn(const char *tag, SetMaxChildrenTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string("path", v->path);
        rc = rc ? rc : deserialize_int("max", &v->max);
        return rc;
    }
    void deallocate_SetMaxChildrenTxn(SetMaxChildrenTxn *v)
    {
        v->path.clear();
    }
    int oarchive::serialize_CreateSessionTxn(const char *tag, CreateSessionTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("timeOut", v->timeOut);
        return rc;
    }
    int iarchive::deserialize_CreateSessionTxn(const char *tag, CreateSessionTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("timeOut", &v->timeOut);
        return rc;
    }
    void deallocate_CreateSessionTxn(CreateSessionTxn *v)
    {
    }
    int oarchive::serialize_CloseSessionTxn(const char *tag, CloseSessionTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_StringVec("paths2Delete", &v->paths2Delete);
        return rc;
    }
    int iarchive::deserialize_CloseSessionTxn(const char *tag, CloseSessionTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_String_vector("paths2Delete", &v->paths2Delete);
        return rc;
    }
    void deallocate_CloseSessionTxn(CloseSessionTxn *v)
    {
        v->paths2Delete.clear();
    }
    int oarchive::serialize_ErrorTxn(const char *tag, ErrorTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("err", v->err);
        return rc;
    }
    int iarchive::deserialize_ErrorTxn(const char *tag, ErrorTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("err", &v->err);
        return rc;
    }
    void deallocate_ErrorTxn(ErrorTxn *v)
    {
    }
    int oarchive::serialize_Txn(const char *tag, Txn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int("type", v->type);
        rc = rc ? rc : serialize_buffer("data", v->data);
        return rc;
    }
    int iarchive::deserialize_Txn(const char *tag, Txn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int("type", &v->type);
        rc = rc ? rc : deserialize_buffer("data", v->data);
        return rc;
    }
    void deallocate_Txn(Txn *v)
    {
        v->data.clear();
    }
    int allocate_Txn_vector(Txn_vector *v, int32_t len)
    {
        if (!len)
        {
            v->count = 0;
            v->data = 0;
        }
        else
        {
            v->count = len;
            v->data = (Txn *)calloc(sizeof(*v->data), len);
        }
        return 0;
    }
    int deallocate_Txn_vector(Txn_vector *v)
    {
        if (v->data)
        {
            int32_t i;
            for (i = 0; i < v->count; i++)
            {
                deallocate_Txn(&v->data[i]);
            }
            free(v->data);
            v->data = 0;
        }
        return 0;
    }
    int oarchive::serialize_Txn_vector(const char *tag, Txn_vector *v)
    {
        int32_t count = v->count;
        int rc = 0;
        int32_t i;
        rc = start_vector(tag, &count);
        for (i = 0; i < v->count; i++)
        {
            rc = rc ? rc : serialize_Txn("data", &v->data[i]);
        }
        return rc;
    }
    int iarchive::deserialize_Txn_vector(const char *tag, Txn_vector *v)
    {
        int rc = 0;
        int32_t i;
        rc = deserialize_int(tag, &v->count);
        v->data = (Txn *)calloc(v->count, sizeof(*v->data));
        for (i = 0; i < v->count; i++)
        {
            rc = rc ? rc : deserialize_Txn("value", &v->data[i]);
        }
        return rc;
    }
    int oarchive::serialize_MultiTxn(const char *tag, MultiTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_Txn_vector("txns", &v->txns);
        return rc;
    }
    int iarchive::deserialize_MultiTxn(const char *tag, MultiTxn *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_Txn_vector("txns", &v->txns);
        return rc;
    }
    void deallocate_MultiTxn(MultiTxn *v)
    {
        deallocate_Txn_vector(&v->txns);
    }

}