#include <coev.h>

using namespace coev;

ServerPool pool;

Awaiter<int> dispatch(int fd)
{
	IOContext c(fd);
	while (c)
	{
		char buffer[0x1000];
		auto r = co_await recv(c, buffer, sizeof(buffer));
		if (r == INVALID)
		{
			close(c);
			co_return 0;
		}
		LOG_DBG("%s\n", buffer);
		r = co_await send(c, buffer, r);
		if (r == INVALID)
		{
			close(c);
			co_return 0;
		}
	}
	co_return INVALID;
}
Awaiter<int> co_server()
{
	Server& s = pool;
	while (s)
	{
		ipaddress addr;
		auto fd = co_await accept(s, addr);
		if (fd != INVALID)
		{
			dispatch(fd);
		}
	}
	co_return INVALID;
}

int main()
{
	ingore_signal(SIGPIPE);
	set_log_level(LOG_LEVEL_ERROR);

	pool.start("127.0.0.1", 9960);
	Routine r;
	r.add(co_server);
	r.add(co_server);
	r.add(co_server);
	r.add(co_server);
	coev::Loop::start();
	return 0;
}