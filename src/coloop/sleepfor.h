#pragma once
#include <memory>
#include "../coev.h"

namespace coev
{
	awaiter sleep_for(long);
	awaiter usleep_for(long);

}