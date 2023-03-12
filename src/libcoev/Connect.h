#pragma once
#include <ev.h>
#include "Socket.h"
#include "Event.h"
#include "EventSet.h"
#include "Client.h"

namespace coev
{	
	struct Connect : Client
	{		
		Connect();
		virtual ~Connect();
		int connect(const char *ip, int port);	
		int connect_insert();
		int connect_remove();	
		static void cb_connect(struct ev_loop *loop, struct ev_io *w, int revents);
	};

}
