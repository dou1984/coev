#include "Log.h"
#include "Timer.h"
#include "Loop.h"

namespace coev
{
	void Timer::cb_timer(struct ev_loop *loop, struct ev_timer *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		Timer *_this = (Timer *)(w->data);
		assert(_this != NULL);
		_this->EVTimer::resume_ex();
	}
	Timer::Timer(ev_tstamp itimer, ev_tstamp rtimer)
	{
		m_data.data = this;
		ev_timer_init(&m_data, Timer::cb_timer, itimer, rtimer);
		m_tag = Loop::tag();
	}
	Timer::~Timer()
	{
		if (ev_is_active(&m_data))
			ev_timer_stop(Loop::at(m_tag), &m_data);
		assert(EVTimer::empty());
	}
	int Timer::stop()
	{
		if (ev_is_active(&m_data))
			ev_timer_stop(Loop::at(m_tag), &m_data);
		return 0;
	}
	int Timer::active()
	{
		if (!ev_is_active(&m_data))
			ev_timer_start(Loop::at(m_tag), &m_data);
		return 0;
	}
	bool Timer::is_active()
	{
		return ev_is_active(&m_data);
	}
	ev_tstamp Timer::remaining()
	{
		return ev_timer_remaining(Loop::at(m_tag), &m_data);
	}
}