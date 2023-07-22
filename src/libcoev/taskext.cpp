#include "taskext.h"
#include "task.h"

namespace coev
{
	void taskext::__resume()
	{
		if (m_taskchain)
		{
			LOG_CORE("m_taskchain %p\n", m_taskchain);
			auto _taskchain = m_taskchain;
			m_taskchain = nullptr;
			_taskchain->EVTask::erase(this);
			_taskchain->EVEvent::resume();
		}
	}
	taskext::~taskext()
	{
		__resume();
	}
}