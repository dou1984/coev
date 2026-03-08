/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cosys.h"
#include "io_context.h"
#include "local_resume.h"

namespace coev
{
	void io_context::cb_read(struct ev_loop *loop, struct ev_io *w, int revents) noexcept
	{
		auto _this = (io_context *)w->data;
		assert(_this != NULL);
		_this->m_r_waiter.resume();
		local_resume();
	}
	void io_context::cb_write(struct ev_loop *loop, struct ev_io *w, int revents) noexcept
	{
		auto _this = (io_context *)w->data;
		assert(_this != NULL);
		_this->m_w_waiter.resume();
		local_resume();
		_this->__del_write();
	}
	int io_context::__close() noexcept
	{
		if (m_fd != INVALID)
		{
			__finally();
			auto _fd = std::exchange(m_fd, INVALID);
			::close(_fd);
			while (m_r_waiter.resume())
			{
			}
			while (m_w_waiter.resume())
			{
			}
		}
		return 0;
	}
	bool io_context::__valid() const noexcept
	{
		return m_fd != INVALID;
	}
	bool io_context::__invalid() const noexcept
	{
		return m_fd == INVALID;
	}
	io_context::operator bool() const
	{
		return m_fd != INVALID;
	}
	io_context::io_context() noexcept
	{
		m_tid = gtid();
		m_loop = cosys::data();
		__init_connect();
		m_type |= IO_CLI | IO_TCP;
	}
	io_context::io_context(int fd) noexcept : m_fd(fd)
	{
		m_tid = gtid();
		m_loop = cosys::data();
		__initial();
	}

	int io_context::__initial() noexcept
	{
		if (m_fd != INVALID)
		{
			setNoBlock(m_fd, true);

			m_read.data = this;
			ev_io_init(&m_read, io_context::cb_read, m_fd, EV_READ);
			ev_io_start(m_loop, &m_read);

			m_write.data = this;
			ev_io_init(&m_write, io_context::cb_write, m_fd, EV_WRITE);
			LOG_CORE("fd:%d", m_fd);
		}
		return 0;
	}
	int io_context::__finally() noexcept
	{
		if (m_fd != INVALID)
		{
			LOG_CORE("fd:%d", m_fd);
			if (ev_is_active(&m_read))
			{
				ev_io_stop(m_loop, &m_read);
			}
			if (ev_is_active(&m_write))
			{
				ev_io_stop(m_loop, &m_write);
			}
		}
		return 0;
	}
	int io_context::__del_write() noexcept
	{
		if (m_w_waiter.empty())
		{
			ev_io_stop(m_loop, &m_write);
		}
		return 0;
	}
	io_context::~io_context() noexcept
	{
		__close();
	}
	awaitable<int> io_context::send(const char *buffer, int size) noexcept
	{
		while (__valid())
		{
			int r = ::send(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				ev_io_start(m_loop, &m_write);
				co_await m_w_waiter.suspend();
			}
			else if (r == 0)
			{
				LOG_CORE("fd:%d closed", m_fd);
				co_return INVALID;
			}
			else
			{
				LOG_CORE("fd:%d send %d bytes", m_fd, r);
				co_return r;
			}
		}
		co_return INVALID;
	}
	awaitable<int> io_context::recv(char *buffer, int size) noexcept
	{
		while (__valid())
		{
			co_await m_r_waiter.suspend();
			int r = ::recv(m_fd, buffer, size, 0);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			else if (r == 0)
			{
				LOG_CORE("fd:%d closed", m_fd);
				co_return INVALID;
			}
			else
			{
				LOG_CORE("fd:%d recv %d bytes", m_fd, r);
				co_return r;
			}
		}
		co_return INVALID;
	}

	awaitable<int> io_context::recvfrom(char *buffer, int size, addrInfo &info) noexcept
	{
		while (__valid())
		{
			co_await m_r_waiter.suspend();
			sockaddr_in addr;
			socklen_t addrsize = sizeof(addr);
			int r = ::recvfrom(m_fd, buffer, size, 0, (struct sockaddr *)&addr, &addrsize);
			if (r == INVALID && isInprocess())
			{
				continue;
			}
			else if (r == 0)
			{
				LOG_CORE("fd:%d closed", m_fd);
				co_return INVALID;
			}
			parseAddr(addr, info);
			co_return r;
		}
		co_return INVALID;
	}
	awaitable<int> io_context::sendto(const char *buffer, int size, addrInfo &info) noexcept
	{
		while (__valid())
		{
			sockaddr_in addr;
			fillAddr(addr, info.ip, info.port);
			int r = ::sendto(m_fd, buffer, size, 0, (struct sockaddr *)&addr, sizeof(addr));
			if (r == INVALID && isInprocess())
			{
				ev_io_start(m_loop, &m_write);
				co_await m_w_waiter.suspend();
			}
			else if (r == 0)
			{
				LOG_CORE("sendto return m_fd:%d", m_fd);
				co_return INVALID;
			}
			co_return r;
		}
		co_return INVALID;
	}
	int io_context::close() noexcept
	{
		LOG_CORE("m_fd:%d", m_fd);
		__close();
		return 0;
	}

	void io_context::cb_connect(struct ev_loop *loop, struct ev_io *w, int revents) noexcept
	{
		if (EV_ERROR & revents)
		{
			return;
		}
		io_context *_this = (io_context *)(w->data);
		assert(_this != nullptr);
		_this->__del_connect();
		_this->m_r_waiter.resume();
		local_resume();
	}
	void io_context::__init_connect() noexcept
	{
		m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (m_fd == INVALID)
		{
			return;
		}
		if (setNoBlock(m_fd, true) < 0)
		{
			::close(m_fd);
			m_fd = INVALID;
			return;
		}
	}

	int io_context::__add_connect() noexcept
	{
		m_read.data = this;
		ev_io_init(&m_read, &io_context::cb_connect, m_fd, EV_READ | EV_WRITE);
		ev_io_start(m_loop, &m_read);
		m_write.data = this;
		ev_io_init(&m_write, &io_context::cb_connect, m_fd, EV_READ | EV_WRITE);
		return 0;
	}
	int io_context::__del_connect() noexcept
	{
		ev_io_stop(m_loop, &m_read);
		return 0;
	}

	int io_context::__connect(const char *ip, int port) noexcept
	{
		if (__connect(m_fd, ip, port) < 0)
		{
			if (isInprocess())
			{
				__add_connect();
				return m_fd;
			}
		}
		__close();
		return m_fd;
	}
	int io_context::__connect(int fd, const char *ip, int port) noexcept
	{
		sockaddr_in addr = {0};
		fillAddr(addr, ip, port);
		return ::connect(fd, (sockaddr *)&addr, sizeof(addr));
	}

	awaitable<int> io_context::connect(const char *ip, int port) noexcept
	{
		m_fd = __connect(ip, port);
		if (m_fd == INVALID)
		{
			co_return m_fd;
		}
		co_await m_r_waiter.suspend();
		auto err = getSocketError(m_fd);
		if (err != 0)
		{
			LOG_ERR("connect error: %s", strerror(err));
			__close();
			co_return INVALID;
		}
		__initial();
		co_return m_fd;
	}

}