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
			m_taskchain->erase_task(this);
			coev::resume<1>(_taskchain, []() {});
		}
	}
	taskevent::~taskevent()
	{
		__resume();
	}
}