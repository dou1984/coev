#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace coev::ssl
{
    class ssl_manager
    {
    public:
        enum E_METHOD
        {
            TLS_SERVER = 0,
            TLS_CLIENT = 1,
        };
        ssl_manager(int method);
        ~ssl_manager();

        SSL_CTX *get() { return m_context; }
        void load_certificated(const char *cert_file);
        void load_privatekey( const char *key_file);
        void check_privatekey(const char *key_file);
    private:
        SSL_CTX *m_context = nullptr;
    };

}