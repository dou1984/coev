#pragma once
#include "../coev.h"

namespace coev
{
	template <class TYPE>
	class channel : public channelimpl<TYPE, loop::resume>
	{
	};
}