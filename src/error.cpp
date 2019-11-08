#include <net/error.hpp>

#include <string.h>

using namespace net;

static std::string get_strerror(int err) {
    return strerror(err);
//    char str[256] = {0};
//    strerror_r(err, str, sizeof(str));
//    return str;
}

error::error(int err):
    std::runtime_error(get_strerror(err))
{

}
