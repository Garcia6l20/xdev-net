#pragma once

#include <gtest/gtest.h>

#include <net/net.hpp>

namespace testing {
struct NetTest : testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
    }
};
}
