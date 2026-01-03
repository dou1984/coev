#include "curl_cli.h"

namespace coev
{
    class init_curl_context
    {
    public:
        init_curl_context()
        {
            curl_global_init(CURL_GLOBAL_ALL);
        }
        ~init_curl_context()
        {
            curl_global_cleanup();
        }
    };

    CurlCli::CurlCli()
    {
        m_multi = curl_multi_init();
        if (m_multi == nullptr)
        {
            LOG_ERR("curl_multi_init error \n");
        }
    }
    CurlCli::~CurlCli()
    {
        curl_multi_cleanup(m_multi);
    }

    static struct curl_context *create_curl_context(curl_socket_t sockfd, struct datauv *uv)
    {
        struct curl_context *context;

        context = (struct curl_context *)malloc(sizeof(*context));

        context->sockfd = sockfd;
        context->uv = uv;

        uv_poll_init_socket(uv->loop, &context->poll_handle, sockfd);
        context->poll_handle.data = context;

        return context;
    }

    void CurlCli::download(const char *url, int num)
    {
        char filename[50];
        FILE *file;
        CURL *curl;

        snprintf(filename, sizeof(filename), "%d.download", num);

        file = fopen(filename, "wb");
        if (!file)
        {
            fprintf(stderr, "Error opening %s\n", filename);
            return;
        }

        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_PRIVATE, file);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_multi_add_handle(m_multi, curl);
        fprintf(stderr, "Added download %s -> %s\n", url, filename);
    }

    static void curl_close_cb(uv_handle_t *handle)
    {
        struct curl_context *context = (struct curl_context *)handle->data;
        free(context);
    }

    static void check_multi_info(CurlCli *cli)
    {
        char *done_url;
        CURLMsg *message;
        int pending;
        CURL *curl;
        FILE *file;

        while ((message = curl_multi_info_read(cli->m_multi, &pending)))
        {
            switch (message->msg)
            {
            case CURLMSG_DONE:
                /* Do not use message data after calling curl_multi_remove_handle() and
                   curl_easy_cleanup(). As per curl_multi_info_read() docs:
                   "WARNING: The data the returned pointer points to does not survive
                   calling curl_multi_cleanup, curl_multi_remove_handle or
                   curl_easy_cleanup." */
                curl = message->easy_handle;

                curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &done_url);
                curl_easy_getinfo(curl, CURLINFO_PRIVATE, &file);
                printf("%s DONE\n", done_url);

                curl_multi_remove_handle(cli->multi, curl);
                curl_easy_cleanup(curl);
                if (file)
                {
                    fclose(file);
                }
                break;

            default:
                fprintf(stderr, "CURLMSG default\n");
                break;
            }
        }
    }

    static void on_uv_socket(uv_poll_t *req, int status, int events)
    {
        int running_handles;
        int flags = 0;
        struct curl_context *context = (struct curl_context *)req->data;
        (void)status;
        if (events & UV_READABLE)
            flags |= CURL_CSELECT_IN;
        if (events & UV_WRITABLE)
            flags |= CURL_CSELECT_OUT;

        curl_multi_socket_action(context->uv->multi, context->sockfd, flags,
                                 &running_handles);
        check_multi_info(context->uv);
    }

    static int cb_socket(CURL *curl, curl_socket_t s, int action, void *userp, void *socketp)
    {
        struct datauv *uv = (struct datauv *)userp;
        struct curl_context *curl_context;
        int events = 0;
        (void)curl;

        switch (action)
        {
        case CURL_POLL_IN:
        case CURL_POLL_OUT:
        case CURL_POLL_INOUT:
            curl_context = socketp ? (CurlCli *)socketp : CurlCli(s);

            curl_multi_assign(uv->multi, s, (void *)curl_context);

            if (action != CURL_POLL_IN)
                events |= UV_WRITABLE;
            if (action != CURL_POLL_OUT)
                events |= UV_READABLE;

            uv_poll_start(&curl_context->poll_handle, events, on_uv_socket);
            break;
        case CURL_POLL_REMOVE:
            if (socketp)
            {
                uv_poll_stop(&((struct curl_context *)socketp)->poll_handle);
                destroy_curl_context((struct curl_context *)socketp);
                curl_multi_assign(uv->multi, s, NULL);
            }
            break;
        default:
            abort();
        }

        return 0;
    }

}