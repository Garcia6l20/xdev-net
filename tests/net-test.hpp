#pragma once

#include <gtest/gtest.h>

#include <xdev/net.hpp>

namespace testing {
struct NetTest : testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
    }
};
}
