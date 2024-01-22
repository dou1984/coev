/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sys/signal.h>
#include <unistd.h>
#include "../coev.h"
#include "loop.h"
#include "ossignal.h"

namespace coev
{
	void ossignal::cb_signal(struct ev_loop *loop, struct ev_signal *w, int revents)
	{
		ossignal *_this = (ossignal *)w->data;
		assert(_this);
		coev::resume(_this);
	}
	ossignal::ossignal(uint64_t id) : m_id(id)
	{
		if (m_id >= SIGNALMAX)
		{
			throw("evSignal signalid is error");
		}
		m_Signal.data = this;
		ev_signal_init(&m_Signal, ossignal::cb_signal, m_id);
		ev_signal_start(loop::data(), &m_Signal);
		m_tid = gtid();
	}
	ossignal::~ossignal()
	{
		ev_signal_stop(loop::at(m_tid), &m_Signal);
	}
	int ossignal::resume()
	{
		static auto pid = getpid();
		kill(pid, m_id);
		return 0;
	}
}