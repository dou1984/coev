/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <ev.h>
#include "cosys.h"
#include "log.h"
#include "co_deliver.h"
#include "io_terminal.h"

namespace coev
{

    ev_signal m_sigint;
    ev_signal m_sigterm;

    void cb_signal(struct ev_loop *loop, ev_signal *w, int revents)
    {
        LOG_CORE("Received signal %d, breaking loop", w->signum);
        ev_signal_stop(loop, &m_sigint);
        ev_signal_stop(loop, &m_sigterm);
        ev_break(loop, EVBREAK_ALL);
    }
    void intercept_singal()
    {
        auto loop = cosys::data();
        ev_signal_init(&m_sigint, cb_signal, SIGINT);
        ev_signal_init(&m_sigterm, cb_signal, SIGTERM);
        ev_signal_start(loop, &m_sigint);
        ev_signal_start(loop, &m_sigterm);
    }

    void ingore_signal(int sign)
    {
        struct sigaction s = {};
        s.sa_handler = SIG_IGN;
        s.sa_flags = 0;
        sigaction(sign, &s, NULL);
    }
}