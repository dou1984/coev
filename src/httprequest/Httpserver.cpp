#include "Httpserver.h"

namespace coev
{
	awaiter recv_request(iocontext &io, Httprequest &req)
	{
		while (io)
		{
			char buffer[0x1000];
			auto r = co_await io.recv(buffer, sizeof(buffer));
			if (r == INVALID)
			{
				LOG_DBG("close %d\n", errno)
				co_await io.close();
				co_return r;
			}
			req.parse(buffer, r);
		}
		co_return 0;
	}
	awaiter Httpserver::dispatch(const ipaddress &addr, iocontext &io)
	{
		Httprequest req;
		co_return co_await wait_for_any(
			timeout(addr, io),
			wait_for_all(recv_request(io, req), router(io, req)));
	}
	awaiter Httpserver::router(iocontext &io, Httprequest &req)
	{
		co_await wait_for<EVRecv>(req);
		LOG_CORE("router begin url:%s\n", req.m_url.c_str());
		if (auto it = m_router.find(req.m_url); it != m_router.end())
		{
			co_await it->second(io, req);
		}
		LOG_CORE("router end url:%s\n", req.m_url.c_str());
		co_return co_await io.close();
	}
	awaiter Httpserver::timeout(const ipaddress &addr, iocontext &io)
	{
		co_await sleep_for(m_timeout);
		co_await io.send("timeout", 8);
		co_return co_await io.close();
	}
	Httpserver::Httpserver(const char *ip, int port, int count)
	{
		m_pool.start(ip, port);
		routine::instance().add(
			count,
			[this]() -> awaiter
			{
				co_return co_await m_pool.get().accept(
					[this](const ipaddress &addr, iocontext &io) -> awaiter
					{ co_return co_await dispatch(addr, io); });
			});
	}
	void Httpserver::add_router(const std::string &api, const fnrouter &f)
	{
		m_router.emplace(api, f);
	}
}