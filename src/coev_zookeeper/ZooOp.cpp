/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include "ZooOp.h"

#define ZOOKEEPER_IS_TTL(mode)            \
    ((mode) == ZOO_PERSISTENT_WITH_TTL || \
     (mode) == ZOO_PERSISTENT_SEQUENTIAL_WITH_TTL)

namespace coev
{
    int get_create_op_type(int mode, int default_op)
    {
        if (mode == ZOO_CONTAINER)
        {
            return ZOO_CREATE_CONTAINER_OP;
        }
        else if (ZOOKEEPER_IS_TTL(mode))
        {
            return ZOO_CREATE_TTL_OP;
        }
        else
        {
            return default_op;
        }
    }
    void ZooOp::zoo_create_op_init(const char *path, const std::string &value, ACLVec_ &acl, int mode, char *path_buffer, int path_buffer_len)
    {
        m_type = get_create_op_type(mode, ZOO_CREATE_OP);
        m_create_op.path = path;
        m_create_op.data = value.data();
        m_create_op.datalen = value.size();
        m_create_op.acl = &acl;
        m_create_op.flags = mode;
        m_create_op.ttl = 0;
        m_create_op.buf = path_buffer;
        m_create_op.buflen = path_buffer_len;
    }

    void ZooOp::zoo_create2_op_init(const char *path, const std::string &value, ACLVec_ &acl, int mode, char *path_buffer, int path_buffer_len)
    {
        m_type = get_create_op_type(mode, ZOO_CREATE2_OP);
        m_create_op.path = path;
        m_create_op.data = value.data();
        m_create_op.datalen = value.size();
        m_create_op.acl = &acl;
        m_create_op.flags = mode;
        m_create_op.buf = path_buffer;
        m_create_op.buflen = path_buffer_len;
    }

    void ZooOp::zoo_delete_op_init(const char *path, int version)
    {
        m_type = ZOO_DELETE_OP;
        m_delete_op.path = path;
        m_delete_op.version = version;
    }

    void ZooOp::zoo_set_op_init(const char *path, const std::string &buffer, int version, Stat_ *stat)
    {
        m_type = ZOO_SETDATA_OP;
        m_set_op.path = path;
        m_set_op.data = buffer.data();
        m_set_op.datalen = buffer.size();
        m_set_op.version = version;
        m_set_op.stat = stat;
    }

    void ZooOp::zoo_check_op_init(const char *path, int version)
    {
        m_type = ZOO_CHECK_OP;
        m_check_op.path = path;
        m_check_op.version = version;
    }
}