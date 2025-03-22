#pragma once

namespace coev
{
	struct is_destroying
	{
		int m_status = 0;
		operator bool() const;
		void lock();
		void unlock();
	};
}