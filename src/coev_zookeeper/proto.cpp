
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <utils/htonll.h>
#include "proto.h"

namespace coev
{

    int oarchive::serialize_int(int32_t d)
    {
        int32_t i = htonl(d);
        m_buff.append((char *)&i, sizeof(i));
        return 0;
    }

    int oarchive::serialize_log(int64_t d)
    {
        const int64_t i = htonll(d);
        m_buff.append((char *)&i, sizeof(i));
        return 0;
    }

    int oarchive::serialize_bool(int32_t d)
    {
        bool i = d != 0;
        m_buff.append((char *)&i, sizeof(bool));
        return 0;
    }
    int32_t negone = -1;
    int oarchive::serialize_buffer(const std::string &s)
    {
        return serialize_string(s);
    }
    int oarchive::serialize_string(const std::string &s)
    {
        int rc = 0;
        if (s.empty())
        {
            return serialize_int(negone);
        }
        int32_t len = s.size();
        rc = serialize_int(len);
        if (rc < 0)
        {
            return rc;
        }

        m_buff.append(s.data(), s.size());
        return 0;
    }

    int iarchive::deserialize_int(int32_t *count)
    {
        if (m_view.size() < sizeof(*count))
        {
            return -E2BIG;
        }

        *count = ntohl(*(int32_t *)m_view.data());
        m_view = m_view.substr(sizeof(*count));
        return 0;
    }

    int iarchive::deserialize_long(int64_t *count)
    {
        if (m_view.size() < sizeof(*count))
        {
            return -E2BIG;
        }
        *count = ntohl(*(int64_t *)m_view.data());
        m_view = m_view.substr(sizeof(*count));
        return 0;
    }
    int iarchive::deserialize_bool(int32_t *v)
    {
        if (m_view.size() < 1)
        {
            return -E2BIG;
        }
        *v = *(bool *)m_view.data();
        m_view = m_view.substr(1);
        return 0;
    }

    int iarchive::deserialize_buffer(std::string &str)
    {
        return deserialize_string(str);
    }

    int iarchive::deserialize_string(std::string &str)
    {
        int32_t len;
        int rc = deserialize_int(&len);
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

    int oarchive::Id(Id_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->scheme);
        rc = rc ? rc : serialize_string(v->id);
        return rc;
    }
    int iarchive::Id(Id_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->scheme);
        rc = rc ? rc : deserialize_string(v->id);
        return rc;
    }
    Id_::~Id_()
    {
        scheme.clear();
        id.clear();
    }

    int oarchive::ACL(ACL_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->perms);
        rc = rc ? rc : Id(&v->id);
        return rc;
    }
    int iarchive::ACL(ACL_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->perms);
        rc = rc ? rc : Id(&v->id);
        return rc;
    }
    ACL_::~ACL_()
    {
    }

    int oarchive::Stat(Stat_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log(v->czxid);
        rc = rc ? rc : serialize_log(v->mzxid);
        rc = rc ? rc : serialize_log(v->ctime);
        rc = rc ? rc : serialize_log(v->mtime);
        rc = rc ? rc : serialize_int(v->version);
        rc = rc ? rc : serialize_int(v->cversion);
        rc = rc ? rc : serialize_int(v->aversion);
        rc = rc ? rc : serialize_log(v->ephemeralOwner);
        rc = rc ? rc : serialize_int(v->dataLength);
        rc = rc ? rc : serialize_int(v->numChildren);
        rc = rc ? rc : serialize_log(v->pzxid);
        return rc;
    }
    int iarchive::Stat(Stat_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long(&v->czxid);
        rc = rc ? rc : deserialize_long(&v->mzxid);
        rc = rc ? rc : deserialize_long(&v->ctime);
        rc = rc ? rc : deserialize_long(&v->mtime);
        rc = rc ? rc : deserialize_int(&v->version);
        rc = rc ? rc : deserialize_int(&v->cversion);
        rc = rc ? rc : deserialize_int(&v->aversion);
        rc = rc ? rc : deserialize_long(&v->ephemeralOwner);
        rc = rc ? rc : deserialize_int(&v->dataLength);
        rc = rc ? rc : deserialize_int(&v->numChildren);
        rc = rc ? rc : deserialize_long(&v->pzxid);
        return rc;
    }

    int oarchive::StatPersisted(StatPersisted_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log(v->czxid);
        rc = rc ? rc : serialize_log(v->mzxid);
        rc = rc ? rc : serialize_log(v->ctime);
        rc = rc ? rc : serialize_log(v->mtime);
        rc = rc ? rc : serialize_int(v->version);
        rc = rc ? rc : serialize_int(v->cversion);
        rc = rc ? rc : serialize_int(v->aversion);
        rc = rc ? rc : serialize_log(v->ephemeralOwner);
        rc = rc ? rc : serialize_log(v->pzxid);
        return rc;
    }
    int iarchive::StatPersisted(StatPersisted_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long(&v->czxid);
        rc = rc ? rc : deserialize_long(&v->mzxid);
        rc = rc ? rc : deserialize_long(&v->ctime);
        rc = rc ? rc : deserialize_long(&v->mtime);
        rc = rc ? rc : deserialize_int(&v->version);
        rc = rc ? rc : deserialize_int(&v->cversion);
        rc = rc ? rc : deserialize_int(&v->aversion);
        rc = rc ? rc : deserialize_long(&v->ephemeralOwner);
        rc = rc ? rc : deserialize_long(&v->pzxid);
        return rc;
    }

    int oarchive::ClientInfo(ClientInfo_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->authScheme);
        rc = rc ? rc : serialize_string(v->user);
        return rc;
    }
    int iarchive::ClientInfo(ClientInfo_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->authScheme);
        rc = rc ? rc : deserialize_string(v->user);
        return rc;
    }
    void deallocate_ClientInfo(ClientInfo_ *v)
    {
        v->authScheme.clear();
        v->user.clear();
    }
    int oarchive::ConnectRequest(ConnectRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->protocolVersion);
        rc = rc ? rc : serialize_log(v->lastZxidSeen);
        rc = rc ? rc : serialize_int(v->timeout);
        rc = rc ? rc : serialize_log(v->sessionId);
        rc = rc ? rc : serialize_buffer(v->passwd);
        rc = rc ? rc : serialize_bool(v->readOnly);
        return rc;
    }
    int iarchive::ConnectRequest(ConnectRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->protocolVersion);
        rc = rc ? rc : deserialize_long(&v->lastZxidSeen);
        rc = rc ? rc : deserialize_int(&v->timeout);
        rc = rc ? rc : deserialize_long(&v->sessionId);
        rc = rc ? rc : deserialize_buffer(v->passwd);
        rc = rc ? rc : deserialize_bool(&v->readOnly);
        return rc;
    }
    void deallocate_ConnectRequest(ConnectRequest_ *v)
    {
        v->passwd.clear();
    }
    int oarchive::ConnectResponse(ConnectResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->protocolVersion);
        rc = rc ? rc : serialize_int(v->timeout);
        rc = rc ? rc : serialize_log(v->sessionId);
        rc = rc ? rc : serialize_buffer(v->passwd);
        rc = rc ? rc : serialize_bool(v->readOnly);
        return rc;
    }
    int iarchive::ConnectResponse(ConnectResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->protocolVersion);
        rc = rc ? rc : deserialize_int(&v->timeout);
        rc = rc ? rc : deserialize_long(&v->sessionId);
        rc = rc ? rc : deserialize_buffer(v->passwd);
        rc = rc ? rc : deserialize_bool(&v->readOnly);
        return rc;
    }
    void deallocate_ConnectResponse(ConnectResponse_ *v)
    {
        v->passwd.clear();
    }

    StringVec_::~StringVec_()
    {
        clear();
    }

    int oarchive::StringVec(StringVec_ *v)
    {
        int32_t count = v->size();
        int rc = 0;
        rc = serialize_int(count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : serialize_string(v->at(i));
        }
        return rc;
    }
    int iarchive::StringVec(StringVec_ *v)
    {
        int rc = 0;
        int32_t count = 0;
        rc = deserialize_int(&count);
        v->resize(count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : deserialize_string(v->at(i));
        }
        return rc;
    }
    int oarchive::SetWatches(SetWatches_ *v)
    {
        int rc = 0;

        rc = rc ? rc : serialize_log(v->relativeZxid);
        rc = rc ? rc : StringVec(&v->dataWatches);
        rc = rc ? rc : StringVec(&v->existWatches);
        rc = rc ? rc : StringVec(&v->childWatches);

        return rc;
    }
    int iarchive::SetWatches(SetWatches_ *v)
    {
        int rc = 0;

        rc = rc ? rc : deserialize_long(&v->relativeZxid);
        rc = rc ? rc : StringVec(&v->dataWatches);
        rc = rc ? rc : StringVec(&v->existWatches);
        rc = rc ? rc : StringVec(&v->childWatches);

        return rc;
    }
    void deallocate_SetWatches(SetWatches_ *v)
    {
        v->dataWatches.clear();
        v->existWatches.clear();
        v->childWatches.clear();
    }
    int oarchive::SetWatches2(SetWatches2_ *v)
    {
        int rc = 0;

        rc = rc ? rc : serialize_log(v->relativeZxid);
        rc = rc ? rc : StringVec(&v->dataWatches);
        rc = rc ? rc : StringVec(&v->existWatches);
        rc = rc ? rc : StringVec(&v->childWatches);
        rc = rc ? rc : StringVec(&v->persistentWatches);
        rc = rc ? rc : StringVec(&v->persistentRecursiveWatches);

        return rc;
    }
    int iarchive::SetWatches2(SetWatches2_ *v)
    {
        int rc = 0;

        rc = rc ? rc : deserialize_long(&v->relativeZxid);
        rc = rc ? rc : StringVec(&v->dataWatches);
        rc = rc ? rc : StringVec(&v->existWatches);
        rc = rc ? rc : StringVec(&v->childWatches);
        rc = rc ? rc : StringVec(&v->persistentWatches);
        rc = rc ? rc : StringVec(&v->persistentRecursiveWatches);

        return rc;
    }
    void deallocate_SetWatches2(SetWatches2_ *v)
    {
        v->dataWatches.clear();
        v->existWatches.clear();
        v->childWatches.clear();
        v->persistentWatches.clear();
        v->persistentRecursiveWatches.clear();
    }
    int oarchive::RequestHeader(RequestHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->xid);
        rc = rc ? rc : serialize_int(v->type);
        return rc;
    }
    int iarchive::RequestHeader(RequestHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->xid);
        rc = rc ? rc : deserialize_int(&v->type);
        return rc;
    }

    int oarchive::MultiHeader(MultiHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->type);
        rc = rc ? rc : serialize_bool(v->done);
        rc = rc ? rc : serialize_int(v->err);
        return rc;
    }
    int iarchive::MultiHeader(MultiHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->type);
        rc = rc ? rc : deserialize_bool(&v->done);
        rc = rc ? rc : deserialize_int(&v->err);
        return rc;
    }
    void deallocate_MultiHeader(MultiHeader_ *v)
    {
    }
    int oarchive::AuthPacket(AuthPacket_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->type);
        rc = rc ? rc : serialize_string(v->scheme);
        rc = rc ? rc : serialize_buffer(v->auth);
        return rc;
    }
    int iarchive::AuthPacket(AuthPacket_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->type);
        rc = rc ? rc : deserialize_string(v->scheme);
        rc = rc ? rc : deserialize_buffer(v->auth);
        return rc;
    }
    void deallocate_AuthPacket(AuthPacket_ *v)
    {
        v->scheme.clear();
        v->auth.clear();
    }
    int oarchive::ReplyHeader(ReplyHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->xid);
        rc = rc ? rc : serialize_log(v->zxid);
        rc = rc ? rc : serialize_int(v->err);
        return rc;
    }
    int iarchive::ReplyHeader(ReplyHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->xid);
        rc = rc ? rc : deserialize_long(&v->zxid);
        rc = rc ? rc : deserialize_int(&v->err);
        return rc;
    }
    void deallocate_ReplyHeader(ReplyHeader_ *v)
    {
    }
    int oarchive::GetDataRequest(GetDataRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_bool(v->watch);
        return rc;
    }
    int iarchive::GetDataRequest(GetDataRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_bool(&v->watch);
        return rc;
    }
    void deallocate_GetDataRequest(GetDataRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::SetDataRequest(SetDataRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : serialize_int(v->version);
        return rc;
    }
    int iarchive::SetDataRequest(SetDataRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : deserialize_int(&v->version);
        return rc;
    }
    void deallocate_SetDataRequest(SetDataRequest_ *v)
    {
        v->path.clear();
        v->data.clear();
    }
    int oarchive::ReconfigRequest(ReconfigRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->joiningServers);
        rc = rc ? rc : serialize_string(v->leavingServers);
        rc = rc ? rc : serialize_string(v->newMembers);
        rc = rc ? rc : serialize_log(v->curConfigId);
        return rc;
    }
    int iarchive::ReconfigRequest(ReconfigRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->joiningServers);
        rc = rc ? rc : deserialize_string(v->leavingServers);
        rc = rc ? rc : deserialize_string(v->newMembers);
        rc = rc ? rc : deserialize_long(&v->curConfigId);
        return rc;
    }
    void deallocate_ReconfigRequest(ReconfigRequest_ *v)
    {
        v->joiningServers.clear();
        v->leavingServers.clear();
        v->newMembers.clear();
    }
    int oarchive::SetDataResponse(SetDataResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    int iarchive::SetDataResponse(SetDataResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    void deallocate_SetDataResponse(SetDataResponse_ *v)
    {
    }
    int oarchive::GetSASLRequest(GetSASLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_buffer(v->token);
        return rc;
    }
    int iarchive::GetSASLRequest(GetSASLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_buffer(v->token);
        return rc;
    }
    void deallocate_GetSASLRequest(GetSASLRequest_ *v)
    {
        v->token.clear();
    }
    int oarchive::SetSASLRequest(SetSASLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_buffer(v->token);
        return rc;
    }
    int iarchive::SetSASLRequest(SetSASLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_buffer(v->token);
        return rc;
    }

    int oarchive::SetSASLResponse(SetSASLResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_buffer(v->token);
        return rc;
    }
    int iarchive::SetSASLResponse(SetSASLResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_buffer(v->token);
        return rc;
    }

    int oarchive::ACLVec(ACLVec_ *v)
    {
        int32_t count = 0;
        int rc = 0;
        int32_t i;
        rc = serialize_int(count);
        for (i = 0; i < count; i++)
        {
            rc = rc ? rc : ACL(&(*v)[i]);
        }
        return rc;
    }
    int iarchive::ACLVec(ACLVec_ *v)
    {
        int rc = 0;
        int32_t i;
        int count = 0;
        rc = deserialize_int(&count);
        v->resize(count);
        for (i = 0; i < count; i++)
        {
            rc = rc ? rc : ACL(&(*v)[i]);
        }
        return rc;
    }
    int oarchive::CreateRequest(CreateRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : serialize_int(v->flags);
        return rc;
    }
    int iarchive::CreateRequest(CreateRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : deserialize_int(&v->flags);
        return rc;
    }
    void CreateRequest_::clear()
    {
        path.clear();
        data.clear();
        acl.clear();
    }

    int oarchive::CreateTTLRequest(CreateTTLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : serialize_int(v->flags);
        rc = rc ? rc : serialize_log(v->ttl);
        return rc;
    }
    int iarchive::CreateTTLRequest(CreateTTLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : deserialize_int(&v->flags);
        rc = rc ? rc : deserialize_long(&v->ttl);
        return rc;
    }
    CreateTTLRequest_::~CreateTTLRequest_()
    {
        path.clear();
        data.clear();
        acl.clear();
    }

    int oarchive::DeleteRequest(DeleteRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_int(v->version);
        return rc;
    }
    int iarchive::DeleteRequest(DeleteRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_int(&v->version);
        return rc;
    }
    void deallocate_DeleteRequest(DeleteRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::GetChildrenRequest(GetChildrenRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_bool(v->watch);
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
    int iarchive::GetChildrenRequest(GetChildrenRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_bool(&v->watch);
        return rc;
    }
    void deallocate_GetChildrenRequest(GetChildrenRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::GetAllChildrenNumberRequest(GetAllChildrenNumberRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        return rc;
    }
    int iarchive::GetAllChildrenNumberRequest(GetAllChildrenNumberRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        return rc;
    }
    void deallocate_GetAllChildrenNumberRequest(GetAllChildrenNumberRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::GetChildren2Request(GetChildren2Request_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_bool(v->watch);
        return rc;
    }
    int iarchive::GetChildren2Request(GetChildren2Request_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_bool(&v->watch);
        return rc;
    }
    void deallocate_GetChildren2Request(GetChildren2Request_ *v)
    {
        v->path.clear();
    }
    int oarchive::CheckVersionRequest(CheckVersionRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_int(v->version);
        return rc;
    }
    int iarchive::CheckVersionRequest(CheckVersionRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_int(&v->version);
        return rc;
    }
    void deallocate_CheckVersionRequest(CheckVersionRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::GetMaxChildrenRequest(GetMaxChildrenRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        return rc;
    }
    int iarchive::GetMaxChildrenRequest(GetMaxChildrenRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        return rc;
    }
    void deallocate_GetMaxChildrenRequest(GetMaxChildrenRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::GetMaxChildrenResponse(GetMaxChildrenResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->max);
        return rc;
    }
    int iarchive::GetMaxChildrenResponse(GetMaxChildrenResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->max);
        return rc;
    }
    void deallocate_GetMaxChildrenResponse(GetMaxChildrenResponse_ *v)
    {
    }
    int oarchive::SetMaxChildrenRequest(SetMaxChildrenRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_int(v->max);
        return rc;
    }
    int iarchive::SetMaxChildrenRequest(SetMaxChildrenRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_int(&v->max);
        return rc;
    }
    void deallocate_SetMaxChildrenRequest(SetMaxChildrenRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::SyncRequest(SyncRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        return rc;
    }
    int iarchive::SyncRequest(SyncRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        return rc;
    }
    void deallocate_SyncRequest(SyncRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::SyncResponse(SyncResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        return rc;
    }
    int iarchive::SyncResponse(SyncResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        return rc;
    }
    void deallocate_SyncResponse(SyncResponse_ *v)
    {
        v->path.clear();
    }
    int oarchive::GetACLRequest(GetACLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        return rc;
    }
    int iarchive::GetACLRequest(GetACLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        return rc;
    }
    void deallocate_GetACLRequest(GetACLRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::SetACLRequest(SetACLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : serialize_int(v->version);
        return rc;
    }
    int iarchive::SetACLRequest(SetACLRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : deserialize_int(&v->version);
        return rc;
    }
    void deallocate_SetACLRequest(SetACLRequest_ *v)
    {
        v->path.clear();
        v->acl.clear();
    }
    int oarchive::SetACLResponse(SetACLResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    int iarchive::SetACLResponse(SetACLResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    void deallocate_SetACLResponse(SetACLResponse_ *v)
    {
    }
    int oarchive::AddWatchRequest(AddWatchRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_int(v->mode);
        return rc;
    }
    int iarchive::AddWatchRequest(AddWatchRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_int(&v->mode);
        return rc;
    }
    void deallocate_AddWatchRequest(AddWatchRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::WatcherEvent(WatcherEvent_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->type);
        rc = rc ? rc : serialize_int(v->state);
        rc = rc ? rc : serialize_string(v->path);
        return rc;
    }
    int iarchive::WatcherEvent(WatcherEvent_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->type);
        rc = rc ? rc : deserialize_int(&v->state);
        rc = rc ? rc : deserialize_string(v->path);
        return rc;
    }
    void WatcherEvent_::clear()
    {
        path.clear();
    }

    int oarchive::ErrorResponse(ErrorResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->err);
        return rc;
    }
    int iarchive::ErrorResponse(ErrorResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->err);
        return rc;
    }
    void deallocate_ErrorResponse(ErrorResponse_ *v)
    {
    }
    int oarchive::CreateResponse(CreateResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        return rc;
    }
    int iarchive::CreateResponse(CreateResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        return rc;
    }
    void deallocate_CreateResponse(CreateResponse_ *v)
    {
        v->path.clear();
    }
    int oarchive::Create2Response(Create2Response_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    int iarchive::Create2Response(Create2Response_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    void deallocate_Create2Response(Create2Response_ *v)
    {
        v->path.clear();
    }
    int oarchive::ExistsRequest(ExistsRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_bool(v->watch);
        return rc;
    }
    int iarchive::ExistsRequest(ExistsRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_bool(&v->watch);
        return rc;
    }
    void deallocate_ExistsRequest(ExistsRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::ExistsResponse(ExistsResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    int iarchive::ExistsResponse(ExistsResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    void deallocate_ExistsResponse(ExistsResponse_ *v)
    {
    }
    int oarchive::GetDataResponse(GetDataResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    int iarchive::GetDataResponse(GetDataResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }

    int oarchive::GetChildrenResponse(GetChildrenResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : StringVec(&v->children);
        return rc;
    }
    int iarchive::GetChildrenResponse(GetChildrenResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : StringVec(&v->children);
        return rc;
    }
    void deallocate_GetChildrenResponse(GetChildrenResponse_ *v)
    {
        v->children.clear();
    }
    int oarchive::GetAllChildrenNumberResponse(GetAllChildrenNumberResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->totalNumber);
        return rc;
    }
    int iarchive::GetAllChildrenNumberResponse(GetAllChildrenNumberResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->totalNumber);
        return rc;
    }
    void deallocate_GetAllChildrenNumberResponse(GetAllChildrenNumberResponse_ *v)
    {
    }
    int oarchive::GetChildren2Response(GetChildren2Response_ *v)
    {
        int rc = 0;
        rc = rc ? rc : StringVec(&v->children);
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    int iarchive::GetChildren2Response(GetChildren2Response_ *v)
    {
        int rc = 0;
        rc = rc ? rc : StringVec(&v->children);
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    void deallocate_GetChildren2Response(GetChildren2Response_ *v)
    {
        v->children.clear();
    }
    int oarchive::GetACLResponse(GetACLResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }
    int iarchive::GetACLResponse(GetACLResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : Stat(&v->stat);
        return rc;
    }

    void GetACLResponse_::clear()
    {
        acl.clear();
    }

    int oarchive::CheckWatchesRequest(CheckWatchesRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_int(v->type);
        return rc;
    }
    int iarchive::CheckWatchesRequest(CheckWatchesRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_int(&v->type);
        return rc;
    }
    void deallocate_CheckWatchesRequest(CheckWatchesRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::RemoveWatchesRequest(RemoveWatchesRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_int(v->type);
        return rc;
    }
    int iarchive::RemoveWatchesRequest(RemoveWatchesRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_int(&v->type);
        return rc;
    }
    void deallocate_RemoveWatchesRequest(RemoveWatchesRequest_ *v)
    {
        v->path.clear();
    }
    int oarchive::GetEphemeralsRequest(GetEphemeralsRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->prefixPath);
        return rc;
    }
    int iarchive::GetEphemeralsRequest(GetEphemeralsRequest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->prefixPath);
        return rc;
    }
    void deallocate_GetEphemeralsRequest(GetEphemeralsRequest_ *v)
    {
        v->prefixPath.clear();
    }
    int oarchive::GetEphemeralsResponse(GetEphemeralsResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : StringVec(&v->ephemerals);
        return rc;
    }
    int iarchive::GetEphemeralsResponse(GetEphemeralsResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : StringVec(&v->ephemerals);
        return rc;
    }
    void deallocate_GetEphemeralsResponse(GetEphemeralsResponse_ *v)
    {
        v->ephemerals.clear();
    }
    int allocate_ClientInfo_vector(ClientInfoVec_ *v, int32_t len)
    {

        v->resize(len);
        return 0;
    }
    int deallocate_ClientInfo_vector(ClientInfoVec_ *v)
    {
        v->clear();
        return 0;
    }
    int oarchive::ClientInfoVec(ClientInfoVec_ *v)
    {
        int32_t count = v->size();
        int rc = serialize_int(count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : ClientInfo(&(*v)[i]);
        }
        return rc;
    }
    int iarchive::ClientInfoVec(ClientInfoVec_ *v)
    {
        int32_t count = 0;
        auto rc = deserialize_int(&count);
        v->resize(count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : ClientInfo(&(*v)[i]);
        }
        return rc;
    }
    int oarchive::WhoAmIResponse(WhoAmIResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : ClientInfoVec(&v->clientInfo);
        return rc;
    }
    int iarchive::WhoAmIResponse(WhoAmIResponse_ *v)
    {
        int rc = 0;
        rc = rc ? rc : ClientInfoVec(&v->clientInfo);
        return rc;
    }
    WhoAmIResponse_::~WhoAmIResponse_()
    {
        clientInfo.clear();
    }

    int oarchive::LearnerInfo(LearnerInfo_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log(v->serverid);
        rc = rc ? rc : serialize_int(v->protocolVersion);
        rc = rc ? rc : serialize_log(v->configVersion);
        return rc;
    }
    int iarchive::LearnerInfo(LearnerInfo_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long(&v->serverid);
        rc = rc ? rc : deserialize_int(&v->protocolVersion);
        rc = rc ? rc : deserialize_long(&v->configVersion);
        return rc;
    }

    int allocate_Id_vector(IdVec_ *v, int32_t len)
    {
        v->reserve(len);
        return 0;
    }

    int oarchive::IdVec(IdVec_ *v)
    {
        int32_t count = v->size();
        int rc = serialize_int(count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : Id(&(*v)[i]);
        }
        return rc;
    }
    int iarchive::IdVec(IdVec_ *v)
    {
        int32_t count;
        int rc = deserialize_int(&count);
        v->resize(count);
        for (auto i = 0; i < (int32_t)v->size(); i++)
        {
            rc = rc ? rc : Id(&(*v)[i]);
        }
        return rc;
    }
    int oarchive::QuorumPacket(QuorumPacket_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->type);
        rc = rc ? rc : serialize_log(v->zxid);
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : IdVec(&v->authinfo);
        return rc;
    }
    int iarchive::QuorumPacket(QuorumPacket_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->type);
        rc = rc ? rc : deserialize_long(&v->zxid);
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : IdVec(&v->authinfo);
        return rc;
    }
    void QuorumPacket_::clear()
    {
        data.clear();
        authinfo.clear();
    }
    int oarchive::QuorumAuthPacket(QuorumAuthPacket_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log(v->magic);
        rc = rc ? rc : serialize_int(v->status);
        rc = rc ? rc : serialize_buffer(v->token);
        return rc;
    }
    int iarchive::QuorumAuthPacket(QuorumAuthPacket_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long(&v->magic);
        rc = rc ? rc : deserialize_int(&v->status);
        rc = rc ? rc : deserialize_buffer(v->token);
        return rc;
    }
    void deallocate_QuorumAuthPacket(QuorumAuthPacket_ *v)
    {
        v->token.clear();
    }
    int oarchive::FileHeader(FileHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->magic);
        rc = rc ? rc : serialize_int(v->version);
        rc = rc ? rc : serialize_log(v->dbid);
        return rc;
    }
    int iarchive::FileHeader(FileHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->magic);
        rc = rc ? rc : deserialize_int(&v->version);
        rc = rc ? rc : deserialize_long(&v->dbid);
        return rc;
    }
    void deallocate_FileHeader(FileHeader_ *v)
    {
    }
    int oarchive::TxnDigest(TxnDigest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->version);
        rc = rc ? rc : serialize_log(v->treeDigest);
        return rc;
    }
    int iarchive::TxnDigest(TxnDigest_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->version);
        rc = rc ? rc : deserialize_long(&v->treeDigest);
        return rc;
    }
    void deallocate_TxnDigest(TxnDigest_ *v)
    {
    }
    int oarchive::TxnHeader(TxnHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_log(v->clientId);
        rc = rc ? rc : serialize_int(v->cxid);
        rc = rc ? rc : serialize_log(v->zxid);
        rc = rc ? rc : serialize_log(v->time);
        rc = rc ? rc : serialize_int(v->type);
        return rc;
    }
    int iarchive::TxnHeader(TxnHeader_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_long(&v->clientId);
        rc = rc ? rc : deserialize_int(&v->cxid);
        rc = rc ? rc : deserialize_long(&v->zxid);
        rc = rc ? rc : deserialize_long(&v->time);
        rc = rc ? rc : deserialize_int(&v->type);
        return rc;
    }
    void deallocate_TxnHeader(TxnHeader_ *v)
    {
    }
    int oarchive::CreateTxnV0(CreateTxnV0_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : serialize_bool(v->ephemeral);
        return rc;
    }
    int iarchive::CreateTxnV0(CreateTxnV0_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : deserialize_bool(&v->ephemeral);
        return rc;
    }
    void deallocate_CreateTxnV0(CreateTxnV0_ *v)
    {
        v->path.clear();
        v->data.clear();
        v->acl.clear();
    }
    int oarchive::CreateTxn(CreateTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : serialize_bool(v->ephemeral);
        rc = rc ? rc : serialize_int(v->parentCVersion);
        return rc;
    }
    int iarchive::CreateTxn(CreateTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : deserialize_bool(&v->ephemeral);
        rc = rc ? rc : deserialize_int(&v->parentCVersion);
        return rc;
    }
    void deallocate_CreateTxn(CreateTxn_ *v)
    {
        v->path.clear();
        v->data.clear();
        v->acl.clear();
    }
    int oarchive::CreateTTLTxn(CreateTTLTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : serialize_int(v->parentCVersion);
        rc = rc ? rc : serialize_log(v->ttl);
        return rc;
    }
    int iarchive::CreateTTLTxn(CreateTTLTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : deserialize_int(&v->parentCVersion);
        rc = rc ? rc : deserialize_long(&v->ttl);
        return rc;
    }
    void deallocate_CreateTTLTxn(CreateTTLTxn_ *v)
    {
        v->path.clear();
        v->data.clear();
        v->acl.clear();
    }
    int oarchive::CreateContainerTxn(CreateContainerTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : serialize_int(v->parentCVersion);
        return rc;
    }
    int iarchive::CreateContainerTxn(CreateContainerTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : deserialize_int(&v->parentCVersion);
        return rc;
    }
    void deallocate_CreateContainerTxn(CreateContainerTxn_ *v)
    {
        v->path.clear();
        v->data.clear();
        v->acl.clear();
    }
    int oarchive::DeleteTxn(DeleteTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        return rc;
    }
    int iarchive::DeleteTxn(DeleteTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        return rc;
    }
    void deallocate_DeleteTxn(DeleteTxn_ *v)
    {
        v->path.clear();
    }
    int oarchive::SetDataTxn(SetDataTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_buffer(v->data);
        rc = rc ? rc : serialize_int(v->version);
        return rc;
    }
    int iarchive::SetDataTxn(SetDataTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_buffer(v->data);
        rc = rc ? rc : deserialize_int(&v->version);
        return rc;
    }
    void deallocate_SetDataTxn(SetDataTxn_ *v)
    {
        v->path.clear();
        v->data.clear();
    }
    int oarchive::CheckVersionTxn(CheckVersionTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_int(v->version);
        return rc;
    }
    int iarchive::CheckVersionTxn(CheckVersionTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_int(&v->version);
        return rc;
    }
    void deallocate_CheckVersionTxn(CheckVersionTxn_ *v)
    {
        v->path.clear();
    }
    int oarchive::SetACLTxn(SetACLTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : serialize_int(v->version);
        return rc;
    }
    int iarchive::SetACLTxn(SetACLTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : ACLVec(&v->acl);
        rc = rc ? rc : deserialize_int(&v->version);
        return rc;
    }
    void deallocate_SetACLTxn(SetACLTxn_ *v)
    {
        v->path.clear();
        v->acl.clear();
    }
    int oarchive::SetMaxChildrenTxn(SetMaxChildrenTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_string(v->path);
        rc = rc ? rc : serialize_int(v->max);
        return rc;
    }
    int iarchive::SetMaxChildrenTxn(SetMaxChildrenTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_string(v->path);
        rc = rc ? rc : deserialize_int(&v->max);
        return rc;
    }
    void deallocate_SetMaxChildrenTxn(SetMaxChildrenTxn_ *v)
    {
        v->path.clear();
    }
    int oarchive::CreateSessionTxn(CreateSessionTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->timeOut);
        return rc;
    }
    int iarchive::CreateSessionTxn(CreateSessionTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->timeOut);
        return rc;
    }
    void deallocate_CreateSessionTxn(CreateSessionTxn_ *v)
    {
    }
    int oarchive::CloseSessionTxn(CloseSessionTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : StringVec(&v->paths2Delete);
        return rc;
    }
    int iarchive::CloseSessionTxn(CloseSessionTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : StringVec(&v->paths2Delete);
        return rc;
    }
    void deallocate_CloseSessionTxn(CloseSessionTxn_ *v)
    {
        v->paths2Delete.clear();
    }
    int oarchive::ErrorTxn(ErrorTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->err);
        return rc;
    }
    int iarchive::ErrorTxn(ErrorTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->err);
        return rc;
    }
    void deallocate_ErrorTxn(ErrorTxn_ *v)
    {
    }
    int oarchive::Txn(Txn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : serialize_int(v->type);
        rc = rc ? rc : serialize_buffer(v->data);
        return rc;
    }
    int iarchive::Txn(Txn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : deserialize_int(&v->type);
        rc = rc ? rc : deserialize_buffer(v->data);
        return rc;
    }
    void deallocate_Txn(Txn_ *v)
    {
        v->data.clear();
    }

    int oarchive::TxnVec(TxnVec_ *v)
    {
        int32_t count = v->size();
        auto rc = serialize_int(count);
        for (auto i = 0; i < (int)v->size(); i++)
        {
            rc = rc ? rc : Txn(&(*v)[i]);
        }
        return rc;
    }
    int iarchive::TxnVec(TxnVec_ *v)
    {

        int32_t count = 0;
        auto rc = deserialize_int(&count);
        v->resize(count);

        for (auto i = 0; i < (int)v->size(); i++)
        {
            rc = rc ? rc : Txn(&(*v)[i]);
        }
        return rc;
    }
    int oarchive::MultiTxn(MultiTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : TxnVec(&v->txns);
        return rc;
    }
    int iarchive::MultiTxn(MultiTxn_ *v)
    {
        int rc = 0;
        rc = rc ? rc : TxnVec(&v->txns);
        return rc;
    }
    void deallocate_MultiTxn(MultiTxn_ *v)
    {
        v->txns.clear();
    }

}