/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#include "Rediscli.h"

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
		_this->EVRecv::resume();
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
			LOG_DBG("type:%d elements:%ld\n", _reply->type, _reply->elements);
			switch (_reply->type)
			{
			case REDIS_REPLY_STRING:
				m_result.last_error = 0;
				m_result.num_rows = 0;
				m_result.str = _reply->str;
				break;
			case REDIS_REPLY_ARRAY:
				m_result.last_error = 0;
				m_result.num_rows = _reply->elements;
				m_result.rows = _reply->element;
				break;
			case REDIS_REPLY_INTEGER:
				m_result.last_error = 0;
				m_result.num_rows = 0;
				m_result.integer = _reply->integer;
				break;
			case REDIS_REPLY_NIL:
				m_result.last_error = INVALID;
				m_result.last_msg = STRING_NIL;
				break;
			case REDIS_REPLY_ERROR:
				m_result.last_error = INVALID;
				m_result.last_msg = _reply->str;
				break;
			case REDIS_REPLY_STATUS:
				m_result.last_error = INVALID;
				m_result.last_msg = _reply->str;
				break;
			default:
				assert(false);
				break;
			}
		}
		else
		{
			m_result.last_error = INVALID;
			m_result.last_msg = STRING_CLOSED;
		}
		EVRecv::resume();
	}
	int Rediscli::__connect()
	{
		assert(m_context == nullptr);
		m_context = redisAsyncConnect(m_ip.c_str(), m_port);
		__attach(m_context);
		LOG_CORE("__attach %d\n", fd());
		__connect_insert();
		redisAsyncSetConnectCallback(m_context, &Rediscli::__connected);
		redisAsyncSetDisconnectCallback(m_context, &Rediscli::__disconnected);
		return 0;
	}
	int Rediscli::__connect_insert()
	{
		m_Read.data = this;
		ev_io_init(&m_Read, &Rediscli::cb_connect, fd(), EV_READ | EV_WRITE);
		ev_io_start(loop::at(m_tag), &m_Read);
		return 0;
	}
	int Rediscli::__connect_remove()
	{
		ev_io_stop(loop::at(m_tag), &m_Read);
		return 0;
	}
	int Rediscli::__process_insert()
	{
		assert(fd() != INVALID);
		m_Read.data = this;
		ev_io_init(&m_Read, &Rediscli::cb_read, fd(), EV_READ);
		ev_io_start(loop::at(m_tag), &m_Read);
		m_Write.data = this;
		ev_io_init(&m_Write, &Rediscli::cb_write, fd(), EV_WRITE);
		return 0;
	}
	int Rediscli::__process_remove()
	{
		m_result.last_error = INVALID;
		m_result.last_msg = STRING_CLOSED;
		if (m_context)
		{
			ev_io_stop(loop::at(m_tag), &m_Read);
			ev_io_stop(loop::at(m_tag), &m_Write);
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
			LOG_DBG("connected error fd:%d\n", _this->fd());
			_this->__process_remove();
			_this->EVRecv::resume();
		}
	}
	void Rediscli::__disconnected(const redisAsyncContext *ac, int status)
	{
		auto _this = __get(ac);
		if (status == REDIS_OK)
		{
			LOG_DBG("disconnect fd:%d\n", _this->fd());
		}
		else
		{
			LOG_DBG("disconnect fd:%d\n", _this->fd());
			_this->__process_remove();
			_this->EVRecv::resume();
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
		ev_io_start(loop::at(_this->m_tag), &_this->m_Read);
	}
	void Rediscli::__delread(void *privdata)
	{
		auto _this = (Rediscli *)(privdata);
		ev_io_stop(loop::at(_this->m_tag), &_this->m_Read);
	}
	void Rediscli::__addwrite(void *privdata)
	{
		auto _this = (Rediscli *)(privdata);
		ev_io_start(loop::at(_this->m_tag), &_this->m_Write);
	}
	void Rediscli::__delwrite(void *privdata)
	{
		auto _this = (Rediscli *)(privdata);
		ev_io_stop(loop::at(_this->m_tag), &_this->m_Write);
	}
	int Rediscli::fd()
	{
		return m_context != nullptr ? m_context->c.fd : INVALID;
	}
	Rediscli::Rediscli(const char *ip, int port, const char *auth)
	{
		m_tag = loop::tag();
		m_ip = ip;
		m_port = port;
		m_auth = auth;
	}
	awaiter<int> Rediscli::connect()
	{
		__connect();
		co_await wait_for<EVRecv>(*this);
		__connect_remove();
		__process_insert();
		co_return 0;
	}
	awaiter<int> Rediscli::query(const char *message, const std::function<void(Redisresult &)> &callback)
	{
		LOG_DBG("query %s\n", message);
		redisAsyncCommand(m_context, Rediscli::__callback, this, message);
		co_await wait_for<EVRecv>(*this);
		if (m_context == nullptr)
			co_return INVALID;
		callback(m_result);
		co_return 0;
	}
}