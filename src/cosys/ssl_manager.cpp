#include <coev/coev.h>
#include <cosys/cosys.h>
#include "ssl_manager.h"

namespace coev
{
    ssl_manager::ssl_manager(int method)
    {
        int err = SSL_load_error_strings();
        LOG_CORE("SSL_load_error_strings\n");
        if (!err)
        {
            throw std::runtime_error("SSL_load_error_strings failed");
        }
        err = SSL_library_init();
        if (!err)
        {
            throw std::runtime_error("SSL_library_init failed");
        }
        LOG_CORE("SSL_library_init\n");
        if (method == TLS_SERVER)
        {
            m_context = SSL_CTX_new(SSLv23_server_method());
        }
        else if (method == TLS_CLIENT)
        {
            m_context = SSL_CTX_new(SSLv23_client_method());
        }
        if (m_context == nullptr)
        {
            LOG_ERR("SSL_CTX_new failed %p\n", m_context);
            exit(INVALID);
        }
    }
    ssl_manager::~ssl_manager()
    {
        if (m_context)
        {
            SSL_CTX_free(m_context);
        }
    }
    SSL_CTX *ssl_manager::get()
    {
        return m_context;
    }
    void ssl_manager::load_certificated(const char *cert_file, const char *key_file)
    {
        if (m_context == nullptr)
        {
            LOG_ERR("SSL_CTX_new failed %p\n", m_context);
            exit(INVALID);
        }
        int r = 0;
        if ((r = SSL_CTX_use_certificate_file(m_context, cert_file, SSL_FILETYPE_PEM)) <= 0)
        {
            LOG_ERR("SSL_CTX_use_certificate_chain_file %s failed %ld\n", cert_file, ERR_get_error());
            exit(INVALID);
        }
        if ((r = SSL_CTX_use_PrivateKey_file(m_context, key_file, SSL_FILETYPE_PEM)) <= 0)
        {
            LOG_ERR("SSL_CTX_use_PrivateKey_file %s failed %ld\n", key_file, ERR_get_error());
            exit(INVALID);
        }
        if ((r = SSL_CTX_check_private_key(m_context)) <= 0)
        {
            LOG_ERR("SSL_CTX_check_private_key %ld\n", ERR_get_error());
            exit(INVALID);
        }
    }

}