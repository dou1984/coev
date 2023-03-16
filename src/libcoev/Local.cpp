#include <memory>
#include "Loop.h"
#include "ThreadLocal.h"
#include "Local.h"
#include "System.h"

namespace coev
{
	thread_local std::shared_ptr<Local> g_local = std::make_shared<Local>();
	LocalExt::LocalExt(std::shared_ptr<Local> &_) : m_current(_)
	{
		TRACE();
		assert(m_current);
		++m_current->m_ref;
	}
	LocalExt::~LocalExt()
	{
		TRACE();
		assert(m_current);
		if (--m_current->m_ref == 0)
			m_current->EVRecv::resume_ex();
	}
	std::unique_ptr<LocalExt> Local::ref()
	{
		return std::make_unique<LocalExt>(g_local);
	}
	Awaiter<int> wait_for_local()
	{
		auto _this = g_local;
		g_local = std::make_shared<Local>();
		co_await wait_for<EVRecv>(*_this);
		co_return 0;
	}
}
