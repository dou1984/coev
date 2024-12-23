/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Rediscli.h"
#include "../cosys/cosys.h"

namespace coev
{
	const char STRING_NIL[] = "nil";
	const char STRING_CLOSED[] = "closed";
	const char STRING_TIMEOUT[] = "timeout";
	void Rediscli::__callback(redisAsyncContext *, void *__reply, void *__data)
	{
		auto _this = (Rediscli *)(__data);
		assert(_this != nullptr);
		_this->__oncallback((redisReply *)(__reply));
	}
	void Rediscli::cb_connect(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		auto _this = (Rediscli *)(w->data);
		assert(_this != nullptr);
		_this->m_listener.resume();
	}
	void Rediscli::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		auto _this = (Rediscli *)(w->data);
		assert(_this != nullptr);
		_this->__onsend();
	}
	void Rediscli::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		auto _this = (Rediscli *)(w->data);
		assert(_this != nullptr);
		_this->__onrecv();
	}
	void Rediscli::__oncallback(redisReply *_reply)
	{
		if (_reply != nullptr)
		{
			LOG_CORE("type:%d elements:%ld\n", _reply->type, _reply->elements);
			m_reply = _reply;
			m_listener.resume();
			m_reply = nullptr;
		}
	}
	std::string Rediscli::reply_string()
	{
		assert(m_reply);
		switch (m_reply->type)
		{
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_ERROR:
			return m_reply->str;
		}
		return reply();
	}
	int Rediscli::reply_integer()
	{
		assert(m_reply);
		switch (m_reply->type)
		{
		case REDIS_REPLY_INTEGER:
		case REDIS_REPLY_ARRAY:
		case REDIS_REPLY_ERROR:
			return m_reply->integer;
		default:
			assert(false);
		}
		return 0;
	}
	std::string Rediscli::reply()
	{
		assert(m_reply);
		switch (m_reply->type)
		{
		case REDIS_REPLY_INTEGER:
			return std::to_string(m_reply->integer);
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_ERROR:
			return m_reply->str;
		case REDIS_REPLY_NIL:
			return STRING_NIL;
		default:
			assert(false);
		}
		return STRING_CLOSED;
	}
	bool Rediscli::error() const
	{
		return m_context == nullptr || m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR;
	}
	int Rediscli::__connect()
	{
		assert(m_context == nullptr);
		m_context = redisAsyncConnect(m_ip.c_str(), m_port);
		if (m_context->c.err != 0)
		{
			LOG_ERR("redisAsyncConnect error %s\n", m_context->c.errstr);
		}
		__attach(m_context);
		LOG_CORE("__attach %d\n", fd());
		__connect_insert();
		redisAsyncSetConnectCallback(m_context, &Rediscli::__connected);
		redisAsyncSetDisconnectCallback(m_context, &Rediscli::__disconnected);
		return 0;
	}
	int Rediscli::__connect_insert()
	{
		m_read.data = this;
		ev_io_init(&m_read, &Rediscli::cb_connect, fd(), EV_READ | EV_WRITE);
		ev_io_start(cosys::at(m_tid), &m_read);
		return 0;
	}
	int Rediscli::__connect_remove()
	{
		ev_io_stop(cosys::at(m_tid), &m_read);
		return 0;
	}
	int Rediscli::__process_insert()
	{
		assert(fd() != INVALID);
		m_read.data = this;
		ev_io_init(&m_read, &Rediscli::cb_read, fd(), EV_READ);
		ev_io_start(cosys::at(m_tid), &m_read);
		m_write.data = this;
		ev_io_init(&m_write, &Rediscli::cb_write, fd(), EV_WRITE);
		return 0;
	}
	int Rediscli::__process_remove()
	{
		if (m_context)
		{
			ev_io_stop(cosys::at(m_tid), &m_read);
			ev_io_stop(cosys::at(m_tid), &m_write);
			m_context = nullptr;
		}
		return 0;
	}
	void Rediscli::__clearup(void *privdata)
	{
		auto _this = (Rediscli *)(privdata);
		_this->__process_remove();
	}
	void Rediscli::__connected(const redisAsyncContext *ac, int status)
	{
		auto _this = __get(ac);
		if (status == REDIS_OK)
		{
			LOG_DBG("connected fd:%d\n", _this->fd());
		}
		else
		{
			LOG_CORE("connected error fd:%d\n", _this->fd());
			_this->__process_remove();
			_this->m_listener.resume();
		}
	}
	void Rediscli::__disconnected(const redisAsyncContext *ac, int status)
	{
		auto _this = __get(ac);
		if (status == REDIS_OK)
		{
			LOG_CORE("disconnect fd:%d\n", _this->fd());
		}
		else
		{
			LOG_CORE("disconnect fd:%d %d %s\n", _this->fd(), ac->err, ac->errstr);
			_this->__process_remove();
			_this->m_listener.resume();
		}
	}
	void Rediscli::__onsend()
	{
		assert(m_context);
		redisAsyncHandleWrite(m_context);
	}
	void Rediscli::__onrecv()
	{
		assert(m_context);
		redisAsyncHandleRead(m_context);
	}
	int Rediscli::__attach(redisAsyncContext *c)
	{
		c->ev.addRead = &Rediscli::__addread;
		c->ev.delRead = &Rediscli::__delread;
		c->ev.addWrite = &Rediscli::__addwrite;
		c->ev.delWrite = &Rediscli::__delwrite;
		c->ev.cleanup = &Rediscli::__clearup;
		c->ev.data = this;
		return 0;
	}
	void Rediscli::__addread(void *privdata)
	{
		auto _this = (Rediscli *)(privdata);
		ev_io_start(cosys::at(_this->m_tid), &_this->m_read);
	}
	void Rediscli::__delread(void *privdata)
	{
		auto _this = (Rediscli *)(privdata);
		ev_io_stop(cosys::at(_this->m_tid), &_this->m_read);
	}
	void Rediscli::__addwrite(void *privdata)
	{
		auto _this = (Rediscli *)(privdata);
		ev_io_start(cosys::at(_this->m_tid), &_this->m_write);
	}
	void Rediscli::__delwrite(void *privdata)
	{
		auto _this = (Rediscli *)(privdata);
		ev_io_stop(cosys::at(_this->m_tid), &_this->m_write);
	}
	int Rediscli::fd()
	{
		return m_context != nullptr ? m_context->c.fd : INVALID;
	}
	Rediscli::Rediscli(const char *ip, int port, const char *auth)
	{
		m_tid = gtid();
		m_ip = ip;
		m_port = port;
		m_auth = auth;
	}
	awaitable<int> Rediscli::connect()
	{
		__connect();
		co_await m_listener.suspend();
		__connect_remove();
		__process_insert();
		co_return 0;
	}

	awaitable<int> Rediscli::query(const char *message)
	{
		LOG_CORE("send %s\n", message);
		redisAsyncCommand(m_context, Rediscli::__callback, this, message);
		co_await m_listener.suspend();
		co_return (m_context) ? 0 : INVALID;
	}
	awaitable<int> Rediscli::query(const std::string &message)
	{
		return query(message.c_str());
	}
}