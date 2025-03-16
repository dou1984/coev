#pragma once

namespace coev
{
    struct is_destroying
	{
		int m_status = 0;
		operator bool() const
		{
			return m_status;
		}
		void lock()
		{
			m_status = 1;
		}
		void unlock()
		{
			m_status = 0;
		}
	};
}