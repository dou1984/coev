#include "Connect.h"
#include "Loop.h"

namespace coev
{
	void Connect::cb_connect(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
		{
			return;
		}
		Connect *_this = (Connect *)(w->data);
		assert(_this != nullptr);
		assert(*_this);
		_this->connect_remove();
		_this->EVRecv::resume_ex();
	}
	Connect::Connect()
	{
		m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (m_fd == INVALID)
		{
			return;
		}
		if (setNoBlock(m_fd, true) < 0)
		{
			::close(m_fd);
			return;
		}
		m_tag = Loop::tag();
		connect_insert();
	}
	Connect::~Connect()
	{
		assert(EVConnect::empty());
		close();
	}
	int Connect::connect_insert()
	{
		m_Read.data = this;
		ev_io_init(&m_Read, &Connect::cb_connect, m_fd, EV_READ | EV_WRITE);
		ev_io_start(Loop::at(m_tag), &m_Read);
		return 0;
	}
	int Connect::connect_remove()
	{
		ev_io_stop(Loop::at(m_tag), &m_Read);
		return 0;
	}
	int Connect::connect(const char *ip, int port)
	{
		if (connectTCP(m_fd, ip, port) < 0)
		{
			if (isInprocess())
			{
				return m_fd;
			}
		}
		return close();
	}
}