#pragma once
#include <ev.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include "../coev.h"

namespace coev
{
	struct Redisconf
	{
		std::string m_ip = "127.0.0.1";
		std::string m_auth;
		int m_port = 6379;
	};
	struct Redisresult
	{
		int last_error = 0;
		int num_rows = 0;
		union
		{
			const char *str;
			const char *last_msg;
			struct redisReply **rows;
			int integer;
		};
	};
	class Rediscli : Redisconf, public EVConnect, public EVRecv
	{
	public:
		Rediscli(const char *ip, int port, const char *auth);
		Awaiter<int> connect();
		Awaiter<int> query(const char *, const std::function<void(Redisresult &)> &);
		operator bool() const { return m_context != nullptr; }

	private:
		int m_tag;
		ev_io m_Read;
		ev_io m_Write;
		redisAsyncContext *m_context = nullptr;
		Redisresult m_result;

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