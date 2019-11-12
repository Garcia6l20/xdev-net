#pragma once

#include <stdexcept>

namespace xdev::net {
struct error: std::runtime_error {
    using std::runtime_error::runtime_error;
    error(int err);
};
}  // namspace xdev::net