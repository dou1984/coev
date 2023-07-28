#pragma once
#include <unordered_map>
#include <functional>
#include "../coloop.h"
#include "Httpparser.h"

namespace coev
{
	class Httpserver final
	{
	public:
		using fnrouter = std::function<awaiter(iocontext &io, Httpparser &req)>;
		Httpserver(const char *, int, int);
		virtual ~Httpserver() = default;

		void add_router(const std::string &, const fnrouter &);

	private:
		tcp::serverpool m_pool;
		int m_timeout = 15;
		std::unordered_map<std::string, fnrouter> m_router;
		awaiter dispatch(const ipaddress &addr, iocontext &io);
		awaiter router(iocontext &c, Httpparser &req);
		awaiter timeout(const ipaddress &addr, iocontext &io);
	};

}