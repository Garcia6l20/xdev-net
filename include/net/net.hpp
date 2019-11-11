#pragma once

#ifndef _WIN32
#include <sys/socket.h>
#else
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

#if !defined(__cpp_concepts)
# ifdef _WIN32
# pragma message ("Constrains are not enabled in pre cxx20")
# else
# warning "Constrains are not enabled in pre cxx20"
# endif
#endif

namespace net {

	void initialize();
	void cleanup();

enum family {
    none = 0,
    inet = AF_INET,
};

}  // namespace net
