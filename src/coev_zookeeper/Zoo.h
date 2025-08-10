#pragma once
#include <cstring>
#include <cstdint>
namespace coev
{

    struct clientid_t
    {
        int64_t client_id;
        char passwd[16] = {0};
        const clientid_t &operator=(const clientid_t &o);
        void clear();
    };

    struct prime_struct
    {
        int32_t len;
        int32_t protocolVersion;
        int32_t timeOut;
        int64_t sessionId;
        int32_t passwd_len;
        char passwd[16];
        char readOnly;
    };

    struct auth_info
    {
        int m_state;
        std::string m_scheme;
        std::string m_auth;
        const char *m_data;
    };

    int32_t get_xid();

}