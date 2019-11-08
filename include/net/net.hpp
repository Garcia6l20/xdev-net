#pragma once

#include <sys/socket.h>

#if !defined(__cpp_concepts)
#warning "Constrains are not enabled in pre cxx20"
#endif

namespace net {

enum family {
    none = 0,
    inet = AF_INET,
};

}  // namespace net
