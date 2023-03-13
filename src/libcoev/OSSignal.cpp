/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <sys/signal.h>
#include <unistd.h>
#include "Loop.h"
#include "OSSignal.h"
#include "Event.h"

namespace coev
{
	void ingore_signal(int sign)
	{
		struct sigaction s = {};
		s.sa_handler = SIG_IGN;
		s.sa_flags = 0;
		sigaction(sign, &s, NULL);
	}

	void OSSignal::cb_signal(struct ev_loop *loop, struct ev_signal *w, int revents)
	{
		OSSignal *_this = (OSSignal *)w->data;
		assert(_this);
		_this->EVRecv::resume_ex();
	}
	OSSignal::OSSignal(uint32_t id) : m_id(id)
	{
		if (m_id >= SIGNALMAX)
		{
			throw("evSignal signalid is error");
		}
		m_Signal.data = this;
		ev_signal_init(&m_Signal, OSSignal::cb_signal, m_id);
		ev_signal_start(Loop::data(), &m_Signal);
		m_tag = Loop::tag();
	}
	OSSignal::~OSSignal()
	{
		ev_signal_stop(Loop::at(m_tag), &m_Signal);
	}
	int OSSignal::resume()
	{
		static auto pid = getpid();
		kill(pid, m_id);
		return 0;
	}
}