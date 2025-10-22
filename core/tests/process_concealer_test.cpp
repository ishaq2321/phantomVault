#include <gtest/gtest.h>
#include "phantom_vault/process_concealer.hpp"
#include <unistd.h>
#include <fstream>
#include <thread>
#include <chrono>

using namespace phantom_vault;
using namespace std::chrono_literals;

class ProcessConcealerTest : public ::testing::Test {
protected:
    std::string readProcName() {
        std::ifstream comm("/proc/self/comm");
        std::string name;
        std::getline(comm, name);
        return name;
    }

    bool isProcessVisible() {
        std::string proc_path = "/proc/" + std::to_string(getpid());
        return access(proc_path.c_str(), R_OK) == 0;
    }
};

TEST_F(ProcessConcealerTest, InitializeTest) {
    ProcessConcealer concealer;
    EXPECT_TRUE(concealer.initialize());
    EXPECT_FALSE(concealer.isHidden());
    EXPECT_FALSE(concealer.getOriginalProcessName().empty());
}

TEST_F(ProcessConcealerTest, ProcessNameTest) {
    ProcessConcealer concealer;
    ASSERT_TRUE(concealer.initialize());

    std::string original_name = concealer.getOriginalProcessName();
    std::string new_name = "test_process";

    EXPECT_TRUE(concealer.setProcessName(new_name));
    EXPECT_EQ(concealer.getCurrentProcessName(), new_name);
    EXPECT_EQ(readProcName(), new_name);

    // Test name truncation (Linux limit is 16 chars)
    std::string long_name = "very_long_process_name";
    EXPECT_TRUE(concealer.setProcessName(long_name));
    EXPECT_EQ(concealer.getCurrentProcessName().length(), 15);

    // Restore original name
    EXPECT_TRUE(concealer.setProcessName(original_name));
    EXPECT_EQ(concealer.getCurrentProcessName(), original_name);
}

TEST_F(ProcessConcealerTest, HideShowTest) {
    ProcessConcealer concealer;
    ASSERT_TRUE(concealer.initialize());

    // Note: Some hide/show operations might require root privileges
    // So we'll focus on testing the API behavior and name disguise method

    std::string original_name = concealer.getOriginalProcessName();
    EXPECT_TRUE(concealer.hideProcess());
    EXPECT_TRUE(concealer.isHidden());
    EXPECT_NE(concealer.getCurrentProcessName(), original_name);

    EXPECT_TRUE(concealer.showProcess());
    EXPECT_FALSE(concealer.isHidden());
    EXPECT_EQ(concealer.getCurrentProcessName(), original_name);
}

TEST_F(ProcessConcealerTest, ErrorHandlingTest) {
    ProcessConcealer concealer;

    // Test operations before initialization
    EXPECT_FALSE(concealer.setProcessName("test"));
    EXPECT_FALSE(concealer.hideProcess());
    EXPECT_FALSE(concealer.showProcess());
    EXPECT_FALSE(concealer.getLastError().empty());

    // Initialize and test invalid operations
    ASSERT_TRUE(concealer.initialize());
    EXPECT_FALSE(concealer.setProcessName(""));  // Empty name
    EXPECT_FALSE(concealer.getLastError().empty());
}

TEST_F(ProcessConcealerTest, MultipleInstanceTest) {
    ProcessConcealer concealer1;
    ProcessConcealer concealer2;

    ASSERT_TRUE(concealer1.initialize());
    ASSERT_TRUE(concealer2.initialize());

    // Both instances should see the same original process name
    EXPECT_EQ(concealer1.getOriginalProcessName(), concealer2.getOriginalProcessName());

    // Both instances should be able to change the process name independently
    EXPECT_TRUE(concealer1.setProcessName("test_proc1"));
    EXPECT_EQ(concealer1.getCurrentProcessName(), "test_proc1");
    EXPECT_EQ(concealer2.getCurrentProcessName(), concealer2.getOriginalProcessName());

    // Both instances should be able to hide/show independently
    EXPECT_TRUE(concealer1.hideProcess());
    EXPECT_TRUE(concealer1.isHidden());
    EXPECT_FALSE(concealer2.isHidden());

    // Restore original name
    EXPECT_TRUE(concealer1.setProcessName(concealer1.getOriginalProcessName()));
} 