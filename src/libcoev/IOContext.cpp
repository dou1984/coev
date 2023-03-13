#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Loop.h"
#include "IOContext.h"

namespace coev
{
	void IOContext::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (IOContext *)w->data;
		assert(_this != NULL);
		assert(*_this);
		_this->EVRecv::resume_ex();
	}
	void IOContext::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (IOContext *)w->data;
		assert(_this != NULL);
		assert(*_this);
		_this->EVSend::resume_ex();
	}
	int IOContext::close()
	{
		if (m_fd != INVALID)
		{
			TRACE();
			ev_io_stop(Loop::at(m_tag), &m_Read);
			ev_io_stop(Loop::at(m_tag), &m_Write);
			::close(m_fd);
			m_fd = INVALID;
			WHILE(EVRecv::resume_ex());
			WHILE(EVSend::resume_ex());
		}
		return 0;
	}
	IOContext::operator bool() const
	{
		return m_fd != INVALID;
	}
	IOContext::IOContext(int fd) : m_fd(fd)
	{
		m_tag = Loop::tag();
		__init();
	}
	int IOContext::__init()
	{
		if (m_fd != INVALID)
		{
			m_Read.data = this;
			ev_io_init(&m_Read, IOContext::cb_read, m_fd, EV_READ);
			ev_io_start(Loop::at(m_tag), &m_Read);

			m_Write.data = this;
			ev_io_init(&m_Write, IOContext::cb_write, m_fd, EV_WRITE);
		}
		return 0;
	}
	IOContext::~IOContext()
	{
		assert(EVRecv::empty());
		assert(EVSend::empty());
		if (m_fd != INVALID)
		{
			ev_io_stop(Loop::at(m_tag), &m_Read);
			ev_io_stop(Loop::at(m_tag), &m_Write);
			::close(m_fd);
			m_fd = INVALID;
		}
	}
}