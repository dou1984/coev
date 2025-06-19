#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace coev::ssl
{
    class manager
    {
    public:
        enum E_METHOD
        {
            TLS_SERVER = 0,
            TLS_CLIENT = 1,
        };
        manager(int method);
        ~manager();

        SSL_CTX *get() { return m_context; }
        void use_certificate_file(const char *cert_file);
        void use_PrivateKey_file( const char *key_file);
        void check_private_key(const char *key_file);
    private:
        SSL_CTX *m_context = nullptr;
    };

}