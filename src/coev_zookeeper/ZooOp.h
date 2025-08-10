#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "constant.h"
#include "proto.h"

namespace coev
{
    int get_create_op_type(int mode, int default_op);
    struct ZooOp
    {
        ZooOp() = default;
        ~ZooOp() = default;

        void zoo_create_op_init(const char *path, const std::string &value, ACLVec &acl, int mode, char *path_buffer, int path_buffer_len);
        void zoo_create2_op_init(const char *path, const std::string &value, ACLVec &acl, int mode, char *path_buffer, int path_buffer_len);
        void zoo_delete_op_init(const char *path, int version);
        void zoo_set_op_init(const char *path, const std::string &buffer, int version, Stat *stat);
        void zoo_check_op_init(const char *path, int version);

        int m_type;
        union
        {

            struct
            {
                const char *path;
                const char *data;
                int datalen;
                char *buf;
                int buflen;
                ACLVec *acl;
                int flags;
                int64_t ttl;
            } m_create_op;

            struct
            {
                const char *path;
                int version;
            } m_delete_op;

            struct
            {
                const char *path;
                const char *data;
                int datalen;
                int version;
                Stat *stat;
            } m_set_op;

            struct
            {
                const char *path;
                int version;
            } m_check_op;
        };
    };

    struct zoo_op_result_t
    {
        int err;
        char *value;
        int valuelen;
        struct Stat *stat;
    };
}