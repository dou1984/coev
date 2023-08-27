#pragma once
#include "../coev.h"
#include "loop.h"
namespace coev
{
	class waitgroup final : public waitgroupimpl<loop::resume>
	{
	};
}