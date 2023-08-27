#pragma once
#include "../coev.h"
#include "loop.h"

namespace coev
{
	class comutex final : public comuteximpl<loop::resume>
	{
	};

}