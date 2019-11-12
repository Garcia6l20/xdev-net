#pragma once

#include <gtest/gtest.h>

#include <net/net.hpp>

namespace testing {
struct NetTest : testing::Test {
protected:
    void SetUp() override {
        xdev::net::initialize();
    }
    void TearDown() override {
        xdev::net::cleanup();
    }
};
}
