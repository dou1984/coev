/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <ev.h>
#include <string.h>
#include <string>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <coev/coev.h>
#include "RedisArray.h"
namespace coev
{
	struct Redisconf
	{
		std::string m_ip = "127.0.0.1";
		std::string m_auth;
		int m_port = 6379;
	};

	class Rediscli : Redisconf
	{
	public:
		Rediscli(const Redisconf &);
		awaitable<int> connect();
		awaitable<int> query(const char *message);
		awaitable<int> query(const std::string &);
		std::string result();
		int result_integer();
		RedisArray result_array();
		bool error() const;

	private:
		int m_tid;
		struct ev_loop *m_loop;
		ev_io m_read;
		ev_io m_write;
		async m_waiter;

		redisAsyncContext *m_context = nullptr;
		redisReply *m_reply = nullptr;

		static void cb_connect(struct ev_loop *loop, struct ev_io *w, int revents);
		static void cb_write(struct ev_loop *loop, struct ev_io *w, int revents);
		static void cb_read(struct ev_loop *loop, struct ev_io *w, int revents);

		static void __addread(void *privdata);
		static void __delread(void *privdata);
		static void __addwrite(void *privdata);
		static void __delwrite(void *privdata);
		static void __clearup(void *privdata);
		static void __connected(const redisAsyncContext *ac, int status);
		static void __disconnected(const redisAsyncContext *ac, int status);
		static void __callback(redisAsyncContext *, void *__reply, void *__data);
		static auto __get(const redisAsyncContext *ac) { return (Rediscli *)ac->ev.data; }
		int __attach(redisAsyncContext *c);
		int __connect();
		int __connect_insert();
		int __connect_remove();
		int __process_insert();
		int __process_remove();
		void __oncallback(redisReply *_reply);
		void __onsend();
		void __onrecv();
		int fd();
	};

	
}