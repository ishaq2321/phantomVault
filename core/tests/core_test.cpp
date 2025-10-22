#include <gtest/gtest.h>
#include "phantom_vault/core.hpp"

using namespace phantom_vault;

TEST(CoreTest, Initialization) {
    Core core;
    EXPECT_TRUE(core.initialize());
    EXPECT_TRUE(core.isInitialized());
}

TEST(CoreTest, Version) {
    Core core;
    std::string version = core.getVersion();
    EXPECT_FALSE(version.empty());
    // Version should be in format X.Y.Z
    EXPECT_EQ(std::count(version.begin(), version.end(), '.'), 2);
} 