/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include <cstdint>
#include <coev/coev.h>
#include "RedisCli.h"

namespace coev
{
	const char STRING_NIL[] = "nil";
	const char STRING_CLOSED[] = "closed";
	const char STRING_TIMEOUT[] = "timeout";
	void RedisCli::__callback(redisAsyncContext *, void *__reply, void *__data)
	{
		auto _this = (RedisCli *)(__data);
		assert(_this != nullptr);
		_this->__oncallback((redisReply *)(__reply));
	}
	void RedisCli::cb_connect(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		auto _this = (RedisCli *)(w->data);
		assert(_this != nullptr);
		_this->m_waiter.resume();
		local_resume();
	}
	void RedisCli::cb_write(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		auto _this = (RedisCli *)(w->data);
		assert(_this != nullptr);
		_this->__onsend();
		local_resume();
	}
	void RedisCli::cb_read(struct ev_loop *loop, struct ev_io *w, int revents)
	{
		if (EV_ERROR & revents)
			return;
		auto _this = (RedisCli *)(w->data);
		assert(_this != nullptr);
		_this->__onrecv();
		local_resume();
	}
	void RedisCli::__oncallback(redisReply *_reply)
	{
		if (_reply != nullptr)
		{
			LOG_CORE("type:%d elements:%ld\n", _reply->type, _reply->elements);
			assert(m_reply == nullptr);
			m_reply = _reply;
			m_waiter.resume();
			m_reply = nullptr;
		}
	}
	int RedisCli::result_integer()
	{
		assert(m_reply);
		switch (m_reply->type)
		{
		case REDIS_REPLY_INTEGER:
			return m_reply->integer;
		case REDIS_REPLY_ARRAY:
			return m_reply->elements;
		default:
			throw("No support redis type!\n");
		}
		return 0;
	}
	std::string RedisCli::result()
	{
		assert(m_reply);
		switch (m_reply->type)
		{
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_ERROR:
			return std::string(m_reply->str, m_reply->len);
		case REDIS_REPLY_INTEGER:
			return std::to_string(m_reply->integer);
		case REDIS_REPLY_ARRAY:
			return std::to_string(m_reply->elements);
		case REDIS_REPLY_NIL:
			return STRING_NIL;
		default:
			throw("No support redis type!\n");
		}
		return STRING_CLOSED;
	}
	RedisArray RedisCli::result_array()
	{
		if (m_reply->type == REDIS_REPLY_ARRAY)
		{
			return RedisArray(m_reply->element, m_reply->elements);
		}
		throw("No support redis type!\n");
	}

	bool RedisCli::error() const
	{
		return m_context == nullptr || m_reply == nullptr || m_reply->type == REDIS_REPLY_ERROR;
	}
	int RedisCli::__connect()
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
		redisAsyncSetConnectCallback(m_context, &RedisCli::__connected);
		redisAsyncSetDisconnectCallback(m_context, &RedisCli::__disconnected);
		return 0;
	}
	int RedisCli::__connect_insert()
	{
		m_read.data = this;
		ev_io_init(&m_read, &RedisCli::cb_connect, fd(), EV_READ | EV_WRITE);
		ev_io_start(m_loop, &m_read);
		return 0;
	}
	int RedisCli::__connect_remove()
	{
		ev_io_stop(m_loop, &m_read);
		return 0;
	}
	int RedisCli::__process_insert()
	{
		assert(fd() != INVALID);
		m_read.data = this;
		ev_io_init(&m_read, &RedisCli::cb_read, fd(), EV_READ);
		ev_io_start(m_loop, &m_read);
		m_write.data = this;
		ev_io_init(&m_write, &RedisCli::cb_write, fd(), EV_WRITE);
		return 0;
	}
	int RedisCli::__process_remove()
	{
		if (m_context)
		{
			ev_io_stop(m_loop, &m_read);
			ev_io_stop(m_loop, &m_write);
			m_context = nullptr;
		}
		return 0;
	}
	void RedisCli::__clearup(void *privdata)
	{
		auto _this = (RedisCli *)(privdata);
		_this->__process_remove();
	}
	void RedisCli::__connected(const redisAsyncContext *ac, int status)
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
			_this->m_waiter.resume();
		}
	}
	void RedisCli::__disconnected(const redisAsyncContext *ac, int status)
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
			_this->m_waiter.resume();
		}
	}
	void RedisCli::__onsend()
	{
		assert(m_context);
		redisAsyncHandleWrite(m_context);
	}
	void RedisCli::__onrecv()
	{
		assert(m_context);
		redisAsyncHandleRead(m_context);
	}
	int RedisCli::__attach(redisAsyncContext *c)
	{
		c->ev.addRead = &RedisCli::__addread;
		c->ev.delRead = &RedisCli::__delread;
		c->ev.addWrite = &RedisCli::__addwrite;
		c->ev.delWrite = &RedisCli::__delwrite;
		c->ev.cleanup = &RedisCli::__clearup;
		c->ev.data = this;
		return 0;
	}
	void RedisCli::__addread(void *privdata)
	{
		auto _this = (RedisCli *)(privdata);
		ev_io_start(_this->m_loop, &_this->m_read);
	}
	void RedisCli::__delread(void *privdata)
	{
		auto _this = (RedisCli *)(privdata);
		ev_io_stop(_this->m_loop, &_this->m_read);
	}
	void RedisCli::__addwrite(void *privdata)
	{
		auto _this = (RedisCli *)(privdata);
		ev_io_start(_this->m_loop, &_this->m_write);
	}
	void RedisCli::__delwrite(void *privdata)
	{
		auto _this = (RedisCli *)(privdata);
		ev_io_stop(_this->m_loop, &_this->m_write);
	}
	int RedisCli::fd()
	{
		return m_context != nullptr ? m_context->c.fd : INVALID;
	}
	RedisCli::RedisCli(const Redisconf &conf)
	{
		m_tid = gtid();
		m_ip = conf.m_ip;
		m_port = conf.m_port;
		m_auth = conf.m_auth;
		m_loop = cosys::data();
	}
	awaitable<int> RedisCli::connect()
	{
		__connect();
		co_await m_waiter.suspend();
		__connect_remove();
		__process_insert();
		co_return 0;
	}

	awaitable<int> RedisCli::query(const char *message)
	{
		LOG_CORE("send %s\n", message);
		redisAsyncCommand(m_context, RedisCli::__callback, this, message);
		co_await m_waiter.suspend();
		co_return (m_context) ? 0 : INVALID;
	}
	awaitable<int> RedisCli::query(const std::string &message)
	{
		return query(message.c_str());
	}

}