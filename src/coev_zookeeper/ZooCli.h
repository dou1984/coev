#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <coev/coev.h>
#include <coev_ssl/client.h>
#include "constant.h"
#include "proto.h"
#include "Zoo.h"
#include "ZooOp.h"

namespace coev
{
    class ZooCli : public ssl::client
    {
    public:
        using base = ssl::client;
        using ssl::client::client;
        using io_context::operator bool;

        awaitable<int> connect(const char *host, int port);

        awaitable<int> zoo_exists(const char *path, Stat &data);
        awaitable<int> zoo_get(const char *path, const std::string &data);
        awaitable<int> zoo_get_config(std::string &data);
        awaitable<int> zoo_reconfig(const char *path, const std::string &joining, const std::string leaving, const std::string members, int64_t version);
        awaitable<int> zoo_set(const char *path, const std::string buffer, int version);
        awaitable<int> zoo_get_children(const char *path, std::string &data);
        awaitable<int> zoo_get_children2(const char *path, std::vector<std::string> &data);
        awaitable<int> zoo_async(const char *path, std::string &data);
        awaitable<int> zoo_get_acl(const char *path, Stat &stat);
        awaitable<int> zoo_set_acl(const char *path, int version, ACLVec &acl, Stat &stat);
        awaitable<int> zoo_multi(int count, const ZooOp *ops);
        awaitable<int> zoo_delete(const char *path, int version);
        awaitable<int> zoo_create2(const char *path, const std::string &value, const ACLVec &acl, int mode);
        awaitable<int> zoo_create2_ttl(const char *path, const std::string &value, const ACLVec &acl_entries, int mode, int64_t ttl);
        awaitable<int> zoo_set_watches();
        awaitable<int> zoo_wait_watcher();
        awaitable<int> close();

    private:
        awaitable<int> send_auth_info(auth_info *auth);
        awaitable<int> send(const std::string &request);
        awaitable<int> recv(std::string &response);
        awaitable<int> prime_connection();
        awaitable<int> prime_response();
        awaitable<int> send_periodic_ping();
        awaitable<int> send_ping();
        awaitable<int> process();
        awaitable<int> deserialize_response(const ReplyHeader &hdr, iarchive &ia);
        awaitable<int> completion(iarchive &ia);

        std::string path_string(const std::string &client_path);
        int path_init(std::string &path_out, const std::string &path);
        int collect_watchers(int type, const std::string &path);

        co_task m_task;
        clientid_t m_client_id;
        long long m_last_zxid;
        prime_struct m_primer_storage;
        std::chrono::system_clock::time_point m_last_ping;

        std::string m_hostname;
        std::string m_chroot;

        int m_recv_timeout = 30000;
        bool m_allow_read_only;
        bool m_seen_rw_server_before;

        std::list<std::string> m_completion_list;
        async m_completion;
        async m_watcher;

        // zk_watchers m_active_node_watchers;
        // zk_watchers m_active_exist_watchers;
        // zk_watchers m_active_child_watchers;
    };
}