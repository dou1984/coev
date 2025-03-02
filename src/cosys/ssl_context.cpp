#include <coev/coev.h>
#include "ssl_context.h"

namespace coev
{
    struct __ssl_configure
    {
        __ssl_configure()
        {
            SSL_load_error_strings();
            OpenSSL_add_ssl_algorithms();

            m_method = TLS_client_method();
            if (m_method == nullptr)
            {
                LOG_ERR("TLS_client_method failed %p\n", m_method);
                exit(INVALID);
            }
            m_context = SSL_CTX_new(m_method);
            if (m_context == nullptr)
            {
                LOG_ERR("SSL_CTX_new failed %p\n", m_context);
                exit(INVALID);
            }
        }
        ~__ssl_configure()
        {
            if (m_context)
            {
                SSL_CTX_free(m_context);
            }
        }
        operator SSL_CTX *()
        {
            return m_context;
        }
        void load_certificated(const char *cert_file, const char *key_file)
        {
            int r = 0;
            // 设置证书链
            if ((r = SSL_CTX_use_certificate_chain_file(m_context, cert_file)) <= 0)
            {
                LOG_ERR("SSL_CTX_use_certificate_chain_file %s failed %ld\n", cert_file, ERR_get_error());
                exit(1);
            }
            // 设置私钥
            if ((r = SSL_CTX_use_PrivateKey_file(m_context, key_file, SSL_FILETYPE_PEM)) <= 0)
            {
                LOG_ERR("SSL_CTX_use_PrivateKey_file %s failed %ld\n", key_file, ERR_get_error());
                exit(1);
            }
        }
        const SSL_METHOD *m_method = nullptr;
        SSL_CTX *m_context = nullptr;
    };
    static __ssl_configure g_configure;

    ssl_context::ssl_context() : io_context(INVALID)
    {
        m_ssl = SSL_new(g_configure);
        if (m_ssl == nullptr)
        {
            LOG_ERR("SSL_new failed %p\n", m_ssl);
        }
    }
    ssl_context::ssl_context(int fd) : io_context(fd)
    {
        m_ssl = SSL_new(g_configure);
        if (m_ssl == nullptr)
        {
            LOG_ERR("SSL_new failed %p\n", m_ssl);
        }
        int err = SSL_set_fd(m_ssl, m_fd);
        if (err != 1)
        {
            LOG_ERR("SSL_set_fd failed %d\n", err);
        }
    }

    ssl_context::~ssl_context()
    {
        if (m_ssl)
        {
            SSL_free(m_ssl);
        }
    }

    awaitable<int> ssl_context::send(const char *buf, int len)
    {
        while (__valid())
        {
            auto r = SSL_write(m_ssl, buf, len);
            if (r == INVALID && isInprocess())
            {
                ev_io_start(m_loop, &m_write);
                co_await m_write_waiter.suspend();
                if (m_write_waiter.empty())
                {
                    ev_io_stop(m_loop, &m_write);
                }
            }
            else
            {
                co_return r;
            }
        }
        co_return INVALID;
    }
    awaitable<int> ssl_context::recv(char *buf, int size)
    {
        while (__valid())
        {
            co_await m_read_waiter.suspend();
            auto r = SSL_read(m_ssl, buf, size);
            if (r == INVALID && isInprocess())
            {
                continue;
            }
            co_return r;
        }
        co_return INVALID;
    }
    void ssl_context::load_certificated(const char *cert_file, const char *key_file)
    {
        g_configure.load_certificated(cert_file, key_file);
    }
}