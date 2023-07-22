#pragma once
#include <memory>
#include "awaiter.h"
#include "event.h"

namespace coev
{
	awaiter sleep_for(long);
	awaiter usleep_for(long);

}