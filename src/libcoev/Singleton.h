#pragma once

namespace coev
{
	template <class _Type>
	struct Singleton final
	{
		static _Type &instance()
		{
			static _Type obj;
			return obj;
		}
	};
}