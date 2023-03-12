#pragma once
#include <coroutine>
#include <iostream>
#include <string.h>
#include "Log.h"

namespace coev
{
	struct Promise
	{
		Promise() = default;
		~Promise() = default;
		void unhandled_exception();
		std::suspend_never initial_suspend();
		std::suspend_never final_suspend() noexcept;
	};
}
