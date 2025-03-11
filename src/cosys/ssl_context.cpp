#include <coev/coev.h>
#include "ssl_context.h"

namespace coev
{
    enum E_METHOD
    {
        TLS_SERVER = 0,
        TLS_CLIENT = 1,
    };
    struct __ssl_manager
    {

        __ssl_manager(int method)
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
        ~__ssl_manager()
        {
            if (m_context)
            {
                SSL_CTX_free(m_context);
            }
        }
        SSL_CTX *get()
        {
            return m_context;
        }
        void load_certificated(const char *cert_file, const char *key_file)
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

        SSL_CTX *m_context = nullptr;
    };
    static __ssl_manager g_srv_mgr(TLS_SERVER);
    static __ssl_manager g_cli_mgr(TLS_CLIENT);

    ssl_context::ssl_context()
    {
        m_ssl = SSL_new(g_cli_mgr.get());
        if (m_ssl == nullptr)
        {
            LOG_ERR("SSL_new failed %p\n", m_ssl);
            throw std::runtime_error("SSL_new failed");
        }
    }
    ssl_context::ssl_context(int fd) : io_context(fd)
    {
        m_ssl = SSL_new(g_srv_mgr.get());
        if (m_ssl == nullptr)
        {
            LOG_ERR("SSL_new failed %p\n", m_ssl);
            throw std::runtime_error("SSL_new failed");
        }
        int err = SSL_set_fd(m_ssl, m_fd);
        if (err != 1)
        {
            LOG_ERR("SSL_set_fd failed %d\n", err);
            __async_finally();
            throw std::runtime_error("SSL_set_mode failed");
        }
        SSL_set_accept_state(m_ssl);
    }
    ssl_context::~ssl_context()
    {
        __async_finally();
    }

    void ssl_context::__async_finally()
    {
        if (m_ssl)
        {
            SSL_free(m_ssl);
            m_ssl = nullptr;
        }
    }

    awaitable<int> ssl_context::connect(const char *host, int port)
    {
        if (m_ssl == nullptr)
        {
            throw std::runtime_error("connect error m_ssl is nullptr");
        }
        int err = co_await io_context::connect(host, port);
        if (err == INVALID)
        {
            LOG_ERR("connect failed %d\n", err);
        __error__:
            __async_finally();
            co_return INVALID;
        }
        err = SSL_set_fd(m_ssl, m_fd);
        if (err != 1)
        {
            LOG_ERR("SSL_set_fd failed %d\n", err);
            goto __error__;
        }
        SSL_set_connect_state(m_ssl);
        err = co_await do_handshake();
        if (err == INVALID)
        {
            LOG_ERR("handshake failed %d\n", err);
            goto __error__;
        }
        co_return m_fd;
    }
    awaitable<int> ssl_context::send(const char *buf, int len)
    {
        while (__valid())
        {
            if (int err = SSL_write(m_ssl, buf, len); err == INVALID)
            {
                if (err = SSL_get_error(m_ssl, err); err != SSL_ERROR_WANT_WRITE)
                {
                    errno = -err;
                    co_return INVALID;
                }
                ev_io_start(m_loop, &m_write);
                co_await m_write_waiter.suspend();
            }
            else
            {
                co_return err;
            }
        }
        co_return INVALID;
    }
    awaitable<int> ssl_context::recv(char *buf, int size)
    {
        while (__valid())
        {
            co_await m_read_waiter.suspend();
            if (auto err = SSL_read(m_ssl, buf, size); err == INVALID)
            {
                if (err = SSL_get_error(m_ssl, err); err != SSL_ERROR_WANT_READ)
                {
                    co_return INVALID;
                }
            }
            else
            {
                co_return err;
            }
        }
        co_return INVALID;
    }
    awaitable<int> ssl_context::do_handshake()
    {
        int err = 0;
        while ((err = SSL_do_handshake(m_ssl)) != 1)
        {
            LOG_CORE("SSL_do_handshake %d\n", err);
            err = SSL_get_error(m_ssl, err);
            if (err == SSL_ERROR_WANT_READ)
            {
                co_await m_read_waiter.suspend();
            }
            else if (err == SSL_ERROR_WANT_WRITE)
            {
                co_await m_write_waiter.suspend();
            }
            else if (err == SSL_ERROR_NONE)
            {
                if (isInprocess())
                {
                    ev_io_start(m_loop, &m_write);
                }
                co_await wait_for_any(
                    [this]() -> awaitable<void>
                    { co_await m_read_waiter.suspend(); }(),
                    [this]() -> awaitable<void>
                    { co_await m_write_waiter.suspend(); }());
            }
            else
            {
                LOG_ERR("do_handshake failed %d errno:%d %s\n", err, errno, strerror(errno));
                errno = -err;
                co_return INVALID;
            }
        }
        co_return 0;
    }
    void ssl_context::load_certificated(const char *cert_file, const char *key_file)
    {
        g_srv_mgr.load_certificated(cert_file, key_file);
    }
}