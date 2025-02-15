#include <cosys/cosys.h>
#include "Nghttpserver.h"

namespace coev::http2
{

    ssize_t Nghttprequest::__send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data)
    {
        return 0;
    }
    ssize_t Nghttprequest::__recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data)
    {
        return 0;
    }
    ssize_t Nghttprequest::__on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        return 0;
    }
    ssize_t Nghttprequest::__on_begin_frame_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        return 0;
    }
    Nghttpserver::Nghttpserver(const char *host, int port)
    {
    }

    Nghttpserver::~Nghttpserver()
    {
    }
    void Nghttpserver::stand()
    {

        nghttp2_bufs bufs;
        size_t framelen;
        nghttp2_frame frame;
        size_t i;
        nghttp2_outbound_item *item;
        nghttp2_nv *nva;
        size_t nvlen;
        nghttp2_hd_deflater m_deflater;
        int rv;
        nghttp2_mem *mem;

        mem = nghttp2_mem_default();
        frame_pack_bufs_init(&bufs);

        nghttp2_session_callbacks_new(&m_callbacks);
        nghttp2_session_callbacks_set_send_callback(m_callbacks, &Nghttprequest::__send_callback);
        nghttp2_session_callbacks_set_recv_callback(m_callbacks, &Nghttprequest::__recv_callback);
        nghttp2_session_callbacks_set_on_frame_recv_callback(m_callbacks, &Nghttprequest::__on_frame_recv_callback);
        nghttp2_session_callbacks_set_on_begin_frame_callback(m_callbacks, &Nghttprequest::__on_begin_frame_callback);

        nghttp2_session_server_new(&m_session, &m_callbacks, this);
        nghttp2_hd_deflate_init(m_deflater, mem);

        nvlen = ARRLEN(reqnv);
        nghttp2_nv_array_copy(&nva, reqnv, nvlen, mem);
        nghttp2_frame_headers_init(&frame.headers, NGHTTP2_FLAG_END_HEADERS, 1,
                                   NGHTTP2_HCAT_HEADERS, NULL, nva, nvlen);
        rv = nghttp2_frame_pack_headers(&bufs, &frame.headers, &m_deflater);

        framelen = nghttp2_bufs_len(&bufs);

        /* Send 1 byte per each read */
        for (i = 0; i < framelen; ++i)
        {
            df.feedseq[i] = 1;
        }

        nghttp2_frame_headers_free(&frame.headers, mem);

        user_data.frame_recv_cb_called = 0;
        user_data.begin_frame_cb_called = 0;

        while (df.seqidx < framelen)
        {
            assert_int(0, ==, nghttp2_session_recv(m_session));
        }
        assert_int(1, ==, user_data.frame_recv_cb_called);
        assert_int(1, ==, user_data.begin_frame_cb_called);

        nghttp2_bufs_reset(&bufs);

        /* Receive PRIORITY */
        nghttp2_frame_priority_init(&frame.priority, 5, &pri_spec_default);

        nghttp2_frame_pack_priority(&bufs, &frame.priority);

        nghttp2_frame_priority_free(&frame.priority);

        scripted_data_feed_init2(&df, &bufs);

        user_data.frame_recv_cb_called = 0;
        user_data.begin_frame_cb_called = 0;

        nghttp2_bufs_reset(&bufs);

        nghttp2_hd_deflate_free(&deflater);
        nghttp2_session_del(m_session);

        /* Some tests for frame too large */
        nghttp2_session_server_new(&m_session, &m_callbacks, &user_data);

        /* Receive PING with too large payload */
        nghttp2_frame_ping_init(&frame.ping, NGHTTP2_FLAG_NONE, NULL);

        nghttp2_frame_pack_ping(&bufs, &frame.ping);

        /* Add extra 16 bytes */
        nghttp2_bufs_seek_last_present(&bufs);
        assert(nghttp2_buf_len(&bufs.cur->buf) >= 16);

        bufs.cur->buf.last += 16;
        nghttp2_put_uint32be(
            bufs.cur->buf.pos,
            (uint32_t)(((frame.hd.length + 16) << 8) + bufs.cur->buf.pos[3]));

        nghttp2_frame_ping_free(&frame.ping);

        scripted_data_feed_init2(&df, &bufs);
        user_data.frame_recv_cb_called = 0;
        user_data.begin_frame_cb_called = 0;

        item = nghttp2_session_get_next_ob_item(m_session);

        nghttp2_bufs_free(&bufs);
        nghttp2_session_del(m_session);
    }

}