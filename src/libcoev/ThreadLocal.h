#pragma once

namespace coev
{
	template <class _Type>
	struct ThreadLocal final
	{
		static _Type &instance()
		{
			static thread_local _Type obj;
			return obj;
		}
	};
}
