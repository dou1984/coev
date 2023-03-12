#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Loop.h"
#include "Client.h"

namespace coev
{
	void Client::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (Client *)w->data;
		assert(_this != NULL);
		assert(*_this);
		_this->EVRecv::resume_ex();
	}
	void Client::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		auto _this = (Client *)w->data;
		assert(_this != NULL);
		assert(*_this);
		_this->EVSend::resume_ex();
	}
	int Client::close()
	{
		if (m_fd != INVALID)
		{
			TRACE();
			ev_io_stop(Loop::at(m_tag), &m_Read);
			ev_io_stop(Loop::at(m_tag), &m_Write);
			::close(m_fd);
			m_fd = INVALID;
			while (EVRecv::resume_ex())
			{
			}
			while (EVSend::resume_ex())
			{
			}
		}
		return 0;
	}
	Client::operator bool() const
	{
		return m_fd != INVALID;
	}
	Client::Client(int fd) : m_fd(fd)
	{
		m_tag = Loop::tag();
		__init();
	}
	int Client::__init()
	{
		if (m_fd != INVALID)
		{
			m_Read.data = this;
			ev_io_init(&m_Read, Client::cb_read, m_fd, EV_READ);
			ev_io_start(Loop::at(m_tag), &m_Read);

			m_Write.data = this;
			ev_io_init(&m_Write, Client::cb_write, m_fd, EV_WRITE);
		}
		return 0;
	}
	Client::~Client()
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