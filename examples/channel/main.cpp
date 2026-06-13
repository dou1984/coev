/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <thread>
#include <atomic>
#include <coev/coev.h>

using namespace coev;

guard::co_channel<int> ch;

std::atomic<int> total = 0;
int total_max = 10000;
int thread_max = 4;

awaitable<void> task_01()
{
	LOG_DBG("task_01");
	int x = 0;
	for (int i = 0; i < total_max; i++)
	{
		x++;
		ch.set(x);
	}
	total += x;
	LOG_DBG("set total:%d", total.load());
	co_await sleep_for(1);
}

awaitable<void> task_02()
{
	LOG_DBG("task_02");
	int x = 0;
	for (int i = 0; i < total_max; i++)
	{
		x++;
		auto r = co_await ch.get(x);
		if (r == INVALID)
		{
			break;
		}
		// LOG_DBG("x=%d", x);
	}
	total += x;
	LOG_DBG("get total:%d", total.load());
}

struct Message
{
	int id;
	std::string name;
	std::string body;
};

co_channel<std::shared_ptr<Message>> sch;

awaitable<void> task_producer()
{
	for (int i = 0; i < total_max; i++)
	{
		auto msg = std::make_shared<Message>();
		msg->id = i;
		msg->name = "admin";
		msg->body = "hello world";
		sch.set(msg);
	}

	co_await sleep_for(1);
}

awaitable<void> task_consumer()
{
	for (int i = 0; i < total_max; i++)
	{
		std::shared_ptr<Message> msg;
		auto r = co_await sch.get(msg);
		if (r == INVALID)
		{
			break;
		}
		// LOG_DBG("id:%d, name:%s, body:%s", msg->id, msg->name.c_str(), msg->body.c_str());
	}
}
int main()
{
	set_log_level(LOG_LEVEL_CORE);
	runnable::instance()
		.start(thread_max, task_01)
		.start(thread_max, task_02)
		.start([]() -> awaitable<void>
			   { co_await wait_for_all(task_producer(), task_consumer()); })
		.end();
	return 0;
}