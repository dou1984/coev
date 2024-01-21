#include "taskevent.h"
#include "task.h"
#include "waitfor.h"
namespace coev
{
	void taskevent::__resume()
	{
		if (m_taskchain)
		{
			LOG_CORE("m_taskchain %p\n", m_taskchain);
			auto _taskchain = m_taskchain;
			m_taskchain = nullptr;
			_taskchain->EVTask::erase(this);
			coev::resume<EVEvent>(_taskchain);
		}
	}
	taskevent::~taskevent()
	{
		__resume();
	}
}