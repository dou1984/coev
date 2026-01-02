/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <cassert>
#include <regex>
#include <string>
#include "ZooCli.h"
#include "proto.h"

namespace coev
{

    bool isValidPath(const std::string &path, int mode)
    {
        if (path.empty() || path[0] != '/')
            return false;
        if (path.size() == 1)
            return true;
        if (path.back() == '/' && !ZOOKEEPER_IS_SEQUENCE(mode))
            return false;

        static const std::regex path_regex(
            "^/"               // 以斜杠开头
            "(?!.*//)"         // 禁止连续斜杠
            "(?!.*/\\./)"      // 禁止 /./
            "(?!.*/\\.\\./)"   // 禁止 /../
            "[^\\x00-\\x1f]+$" // 禁止控制字符 (正确转义)
        );

        static const std::regex dot_regex(
            "(^|/)\\.($|/)|"   // 单独的 .
            "(^|/)\\.\\.($|/)" // 单独的 ..
        );

        try
        {
            return std::regex_match(path, path_regex) && !std::regex_search(path, dot_regex);
        }
        catch (...)
        {
        }
        return false;
    }
    int ZooCli::path_init(std::string &path_out, const std::string &path)
    {
        path_out = path_string(path);
        if (!isValidPath(path_out, 0))
        {
            return INVALID;
        }
        return 0;
    }
    awaitable<int> ZooCli::connect(const char *host, int port)
    {
        m_hostname = std::string(host) + ":" + std::to_string(port);

        // zh = zookeeper_init(m_hostname.c_str(), nullptr, 1000, nullptr, nullptr, 0);

        auto r = co_await base::connect(host, port);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        r = co_await prime_connection();
        if (r == INVALID)
        {
            co_return INVALID;
        }

        r = co_await prime_response();
        if (r == INVALID)
        {
            co_return INVALID;
        }
        m_client_id.client_id = m_primer_storage.sessionId;

        m_task << send_periodic_ping();
        m_task << process();
        co_return r;
    }
    awaitable<int> ZooCli::send(const std::string &request)
    {
        if (request.empty())
        {
            co_return 0;
        }
        int len = request.size();
        int hlen = htonl(len);
        auto r = co_await base::send((char *)(&hlen), sizeof(hlen));
        if (r == INVALID)
        {
            co_return INVALID;
        }

        r = co_await base::send(request.data(), request.size());
        if (r == INVALID)
        {
            co_return INVALID;
        }
        co_return r;
    }
    awaitable<int> ZooCli::recv(std::string &response)
    {
        int32_t len = 0;
        int r = co_await base::recv((char *)&len, sizeof(len));
        if (r == INVALID)
        {
            co_return INVALID;
        }
        if (r < (int)sizeof(int32_t))
        {
            co_return INVALID;
        }
        int32_t hlen = htonl(len);
        response.resize(hlen);
        int32_t offset = 0;
        while (offset < hlen)
        {
            r = co_await base::recv(response.data() + offset, response.size() - offset);
            if (r == INVALID)
            {
                co_return INVALID;
            }
            offset += r;
        }
        co_return r;
    }

    awaitable<int> ZooCli::prime_connection()
    {

        oarchive oa;
        oa.m_buff.reserve(HANDSHAKE_REQ_SIZE);

        int32_t protocolVersion = 0;
        int64_t sessionId = m_seen_rw_server_before ? m_client_id.client_id : 0;
        int32_t timeOut = m_recv_timeout;
        int64_t lastZxidSeen = m_last_zxid;
        std::string password(m_client_id.passwd, sizeof(m_client_id.passwd));
        int32_t readOnly = m_allow_read_only;
        oa.serialize_int(protocolVersion);
        oa.serialize_log(lastZxidSeen);
        oa.serialize_int(timeOut);
        oa.serialize_log(sessionId);
        oa.serialize_string(password);
        oa.serialize_bool(readOnly);
        assert(oa.m_buff.size() == HANDSHAKE_REQ_SIZE);

        co_await send(oa.m_buff);
    }
    awaitable<int> ZooCli::prime_response()
    {
        std::string msg;
        msg.reserve(HANDSHAKE_REQ_SIZE);
        auto r = co_await recv(msg);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        m_primer_storage.len = r;

        iarchive ia(msg.data(), r);
        ia.deserialize_int(&m_primer_storage.protocolVersion);
        ia.deserialize_int(&m_primer_storage.timeOut);
        ia.deserialize_long(&m_primer_storage.sessionId);
        std::string password;
        ia.deserialize_string(password);
        strncpy(m_primer_storage.passwd, password.c_str(), sizeof(m_primer_storage.passwd));
        m_primer_storage.passwd_len = std::min(password.size(), sizeof(m_primer_storage.passwd));
        int32_t readOnly;
        ia.deserialize_bool(&readOnly);
        m_primer_storage.readOnly = readOnly;

        assert(ia.m_view.size() == 0);
        co_return 0;
    }
    awaitable<int> ZooCli::send_periodic_ping()
    {
        do
        {
            co_await sleep_for(m_recv_timeout);
            auto r = co_await send_ping();
            if (r == INVALID)
            {
                co_return INVALID;
            }
        } while (true);
        co_return 0;
    }
    awaitable<int> ZooCli::send_ping()
    {
        oarchive oa;

        RequestHeader_ h = {PING_XID, ZOO_PING_OP};
        oa.RequestHeader(&h);
        m_last_ping = std::chrono::system_clock::now();

        auto r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        co_return 0;
    }
    std::string ZooCli::path_string(const std::string &client_path)
    {
        if (m_chroot.empty())
        {
            return client_path;
        }
        if (client_path.size() == 1)
        {
            return m_chroot;
        }
        return m_chroot + client_path;
    }

    awaitable<int> ZooCli::process()
    {
        while (true)
        {
            std::string response;
            auto r = co_await recv(response);
            if (r == INVALID)
            {
                co_return INVALID;
            }
            assert(r == (int)response.size());
            iarchive ia(response.data(), response.size());
            ReplyHeader_ hdr;
            ia.ReplyHeader(&hdr);
            if (hdr.xid == PING_XID)
            {
                m_last_ping = std::chrono::system_clock::now();
            }
            else if (hdr.xid == SET_WATCHES_XID)
            {
            }
            else if (hdr.xid == WATCHER_EVENT_XID)
            {
                std::string path;
                WatcherEvent_ evt;
                ia.WatcherEvent(&evt);
                collect_watchers(evt.type, evt.path);
                m_watcher.resume_next_loop();
            }
            else
            {
                m_completion_list.emplace_back(std::move(response));
                m_completion.resume_next_loop();
            }
        }
        co_return 0;
    }
    awaitable<int> ZooCli::completion(iarchive &ia)
    {

        co_await m_completion.suspend();
        if (m_completion_list.empty())
        {
            co_return INVALID;
        }
        std::string response = std::move(m_completion_list.front());
        m_completion_list.pop_front();

        ia.m_view = std::string_view(response.data(), response.size());
        ia.ReplyHeader(&ia.m_header);

        if (ia.m_header.err)
        {
            co_return INVALID;
        }
        co_return 0;
    }

    awaitable<int> ZooCli::send_auth_info(auth_info *auth)
    {

        RequestHeader_ h = {AUTH_XID, ZOO_SETAUTH_OP};
        oarchive oa;

        AuthPacket_ req;
        req.type = 0;
        req.scheme = auth->m_scheme;
        req.auth = auth->m_auth;
        auto r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.AuthPacket(&req);

        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        iarchive ia;
        co_await completion(ia);

        AuthPacket_ resp;
        ia.AuthPacket(&resp);

        co_return 0;
    }
    awaitable<int> ZooCli::exists(const char *path, Stat_ &data)
    {

        RequestHeader_ h = {get_xid(), ZOO_EXISTS_OP};
        ExistsRequest_ req;
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        oarchive oa;
        r = r < 0 ? r : oa.RequestHeader(&h);
        r = r < 0 ? r : oa.ExistsRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        iarchive ia;
        r = co_await completion(ia);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        ExistsResponse_ res;
        r = ia.ExistsResponse(&res);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        data = res.stat;
        co_return 0;
    }
    awaitable<int> ZooCli::get(const char *path, const std::string &data)
    {

        RequestHeader_ h = {get_xid(), ZOO_GETDATA_OP};
        GetDataRequest_ req;
        req.watch = false;
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        oarchive oa;
        r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.GetDataRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        iarchive ia;
        co_await completion(ia);

        GetDataResponse_ res;
        ia.GetDataResponse(&res);

        co_return 0;
    }
    awaitable<int> ZooCli::get_config(std::string &data)
    {
        const std::string path = ZOO_CONFIG_NODE;
        const std::string server_path = ZOO_CONFIG_NODE;

        RequestHeader_ h = {get_xid(), ZOO_GETDATA_OP};

        oarchive oa;
        GetDataRequest_ req;
        req.path = server_path;
        req.watch = false;
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.GetDataRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        co_await completion(ia);

        GetDataResponse_ resp;
        ia.GetDataResponse(&resp);

        co_return 0;
    }

    awaitable<int> ZooCli::reconfig(const char *path, const std::string &joining, const std::string leaving, const std::string members, int64_t version)
    {

        RequestHeader_ h = {get_xid(), ZOO_RECONFIG_OP};
        ReconfigRequest_ req;
        req.joiningServers = joining;
        req.leavingServers = leaving;
        req.newMembers = members;
        req.curConfigId = version;

        oarchive oa;
        auto r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.ReconfigRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        co_await completion(ia);
        ReconfigRequest_ res;
        ia.ReconfigRequest(&res);
        co_return 0;
    }
    awaitable<int> ZooCli::set(const char *path, const std::string buffer, int version)
    {

        RequestHeader_ h = {get_xid(), ZOO_SETDATA_OP};
        SetDataRequest_ req;
        req.data = buffer;
        req.version = version;
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        oarchive oa;
        r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.SetDataRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        co_await completion(ia);
        GetDataResponse_ res;
        ia.GetDataResponse(&res);

        co_return 0;
    }

    awaitable<int> ZooCli::get_children(const char *path, std::string &data)
    {

        RequestHeader_ h = {get_xid(), ZOO_GETCHILDREN_OP};
        GetChildrenRequest_ req;
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        oarchive oa;
        r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.GetChildrenRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        r = co_await completion(ia);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        GetChildrenResponse_ res;
        r = ia.GetChildrenResponse(&res);

        co_return ZOK;
    }

    awaitable<int> ZooCli::get_children2(const char *path, std::vector<std::string> &data)
    {

        RequestHeader_ h = {get_xid(), ZOO_GETCHILDREN2_OP};
        GetChildren2Request_ req = {
            .watch = 0,
        };
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        oarchive oa;
        r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.GetChildren2Request(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        r = co_await completion(ia);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        GetChildren2Response_ res;
        r = ia.GetChildren2Response(&res);
        data = std::move(res.children);

        co_return 0;
    }

    awaitable<int> ZooCli::async_(const char *path, std::string &data)
    {
        SyncRequest_ req = {};
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        RequestHeader_ h = {get_xid(), ZOO_SYNC_OP};
        oarchive oa;
        r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.SyncRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        r = co_await completion(ia);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        SyncResponse_ resp;
        r = r < 0 ? r : ia.SyncResponse(&resp);
        data = std::move(resp.path);

        co_return 0;
    }
    awaitable<int> ZooCli::get_acl(const char *path, Stat_ &stat)
    {

        GetACLRequest_ req = {};
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        RequestHeader_ h = {get_xid(), ZOO_GETACL_OP};
        oarchive oa;
        r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.GetACLRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        r = co_await completion(ia);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        SetACLResponse_ resp;
        r = r < 0 ? r : ia.SetACLResponse(&resp);
        stat = resp.stat;
        co_return 0;
    }
    awaitable<int> ZooCli::set_acl(const char *path, int version, ACLVec_ &acl, Stat_ &stat)
    {

        SetACLRequest_ req = {
            .acl = acl,
            .version = version,
        };
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        RequestHeader_ h = {get_xid(), ZOO_SETACL_OP};
        oarchive oa;
        r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.SetACLRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        r = co_await completion(ia);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        SetACLResponse_ resp;
        r = r < 0 ? r : ia.SetACLResponse(&resp);
        stat = resp.stat;
        co_return 0;
    }
    awaitable<int> ZooCli::multi(int count, const ZooOp *ops)
    {
        RequestHeader_ h = {get_xid(), ZOO_MULTI_OP};
        MultiHeader_ mh = {-1, 1, -1};
        oarchive oa;

        int r = oa.RequestHeader(&h);

        int index = 0;
        for (index = 0; index < count; index++)
        {
            const ZooOp *op = ops + index;

            struct MultiHeader_ mh = {op->m_type, 0, -1};
            r = r < 0 ? r : oa.MultiHeader(&mh);

            switch (op->m_type)
            {
            case ZOO_CREATE_CONTAINER_OP:
            case ZOO_CREATE_OP:
            {
                CreateRequest_ req = {
                    .data = op->m_create_op.data,
                    .acl = *op->m_create_op.acl,
                    .flags = op->m_create_op.flags,
                };
                r = path_init(req.path, op->m_create_op.path);
                r = r < 0 ? r : oa.CreateRequest(&req);
                break;
            }

            case ZOO_DELETE_OP:
            {
                DeleteRequest_ req = {
                    .version = op->m_delete_op.version,
                };
                r = path_init(req.path, op->m_delete_op.path);
                r = r < 0 ? r : oa.DeleteRequest(&req);

                break;
            }

            case ZOO_SETDATA_OP:
            {
                SetDataRequest_ req = {
                    .data = op->m_set_op.data,
                    .version = op->m_set_op.version,
                };
                r = path_init(req.path, op->m_set_op.path);
                r = r < 0 ? r : oa.SetDataRequest(&req);
                break;
            }

            case ZOO_CHECK_OP:
            {
                CheckVersionRequest_ req = {};
                r = path_init(req.path, op->m_check_op.path);
                r = r < 0 ? r : oa.CheckVersionRequest(&req);
                break;
            }
            }
        }

        r = r < 0 ? r : oa.MultiHeader(&mh);
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        co_return 0;
    }
    awaitable<int> ZooCli::delete_(const char *path, int version)
    {

        RequestHeader_ h = {get_xid(), ZOO_DELETE_OP};
        DeleteRequest_ req;
        auto r = path_init(req.path, path);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        oarchive oa;
        r = oa.RequestHeader(&h);
        r = r < 0 ? r : oa.DeleteRequest(&req);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        r = co_await completion(ia);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        co_return 0;
    }
    awaitable<int> ZooCli::create2(const char *path, const std::string &value, const ACLVec_ &acl, int mode)
    {
        return create2_ttl(path, value, acl, mode, -1);
    }
    awaitable<int> ZooCli::create2_ttl(const char *path, const std::string &value, const ACLVec_ &acl_entries, int mode, int64_t ttl)
    {

        RequestHeader_ h = {get_xid(), get_create_op_type(mode, ZOO_CREATE2_OP)};
        if (ZOOKEEPER_IS_TTL(mode))
        {
            if (ttl <= 0 || ttl > ZOO_MAX_TTL)
            {
                co_return INVALID;
            }
            CreateTTLRequest_ req = {
                .data = value,
                .acl = acl_entries,
                .flags = mode,
                .ttl = ttl,
            };

            auto r = path_init(req.path, path);
            if (r == INVALID)
            {
                co_return INVALID;
            }

            oarchive oa;
            r = r < 0 ? r : oa.RequestHeader(&h);
            r = r < 0 ? r : oa.CreateTTLRequest(&req);

            r = co_await send(oa.m_buff);
            if (r == INVALID)
            {
                co_return INVALID;
            }
            iarchive ia;
            r = co_await completion(ia);
            if (r == INVALID)
            {
                co_return INVALID;
            }

            Create2Response_ res;
            ia.Create2Response(&res);
        }
        else
        {
            if (ttl >= 0)
            {
                co_return INVALID;
            }
            CreateRequest_ req = {
                .data = value,
                .acl = acl_entries,
                .flags = mode,
            };
            auto r = path_init(req.path, path);
            if (r == INVALID)
            {
                co_return INVALID;
            }
            oarchive oa;
            r = r < 0 ? r : oa.RequestHeader(&h);
            r = r < 0 ? r : oa.CreateRequest(&req);

            r = co_await send(oa.m_buff);
            if (r == INVALID)
            {
                co_return INVALID;
            }

            iarchive ia;
            r = co_await completion(ia);
            if (r == INVALID)
            {
                co_return INVALID;
            }
            Create2Response_ res;
            ia.Create2Response(&res);
        }
    }

    awaitable<int> ZooCli::set_watches()
    {
        auto r = 0;
        RequestHeader_ h = {SET_WATCHES_XID, ZOO_SETWATCHES_OP};
        SetWatches_ req;
        req.relativeZxid = m_last_zxid;

        oarchive oa;
        r = r < 0 ? r : oa.RequestHeader(&h);
        r = r < 0 ? r : oa.SetWatches(&req);
        r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        iarchive ia;
        r = co_await completion(ia);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        co_return 0;
    }

    int ZooCli::collect_watchers(int type, const std::string &path)
    {

        switch (type)
        {
        case CREATED_EVENT_DEF:
        case CHANGED_EVENT_DEF:
            break;
        case CHILD_EVENT_DEF:
            break;
        case DELETED_EVENT_DEF:
            break;
        }
        return 0;
    }

    awaitable<int> ZooCli::close()
    {
        RequestHeader_ h = {get_xid(), ZOO_CLOSE_OP};

        oarchive oa;
        oa.RequestHeader(&h);

        auto r = co_await send(oa.m_buff);
        if (r == INVALID)
        {
            co_return INVALID;
        }

        iarchive ia;
        r = co_await completion(ia);
        if (r == INVALID)
        {
            co_return INVALID;
        }
        co_return 0;
    }
    awaitable<int> ZooCli::wait_watches()
    {
        co_await m_watcher.suspend();
        co_return 0;
    }
}