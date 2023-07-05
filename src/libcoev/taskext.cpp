#include "taskext.h"
#include "task.h"

namespace coev
{
	void taskext::__resume()
	{
		if (m_taskchain)
		{
			m_taskchain->EVTask::erase(this);
			m_taskchain->EVEvent::resume_ex();
			m_taskchain = nullptr;
		}
	}
	taskext::~taskext()
	{
		__resume();
	}
}