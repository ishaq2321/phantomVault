/**
 * PhantomVault Comprehensive Test Framework
 * 
 * Provides testing infrastructure for unit tests, integration tests,
 * security tests, and performance tests.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <iostream>
#include <sstream>
#include <memory>
#include <map>

namespace phantomvault {
namespace testing {

/**
 * @brief Test result status
 */
enum class TestStatus {
    PASSED,
    FAILED,
    SKIPPED,
    ERROR
};

/**
 * @brief Individual test result
 */
struct TestResult {
    std::string test_name;
    std::string test_category;
    TestStatus status;
    std::string message;
    std::chrono::milliseconds duration;
    std::string error_details;
    
    TestResult() : status(TestStatus::FAILED), duration(0) {}
};

/**
 * @brief Test suite statistics
 */
struct TestSuiteStats {
    size_t total_tests = 0;
    size_t passed_tests = 0;
    size_t failed_tests = 0;
    size_t skipped_tests = 0;
    size_t error_tests = 0;
    std::chrono::milliseconds total_duration{0};
    
    double pass_rate() const {
        return total_tests > 0 ? (double)passed_tests / total_tests * 100.0 : 0.0;
    }
};

/**
 * @brief Test function signature
 */
using TestFunction = std::function<void()>;

/**
 * @brief Test assertion exception
 */
class TestAssertionException : public std::exception {
public:
    explicit TestAssertionException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
    
private:
    std::string message_;
};

/**
 * @brief Main test framework class
 */
class TestFramework {
public:
    TestFramework();
    ~TestFramework();
    
    // Test registration
    void registerTest(const std::string& category, const std::string& name, TestFunction test);
    
    // Test execution
    bool runAllTests();
    bool runCategory(const std::string& category);
    bool runTest(const std::string& category, const std::string& name);
    
    // Results
    const std::vector<TestResult>& getResults() const { return results_; }
    TestSuiteStats getStats() const;
    void printResults() const;
    void printSummary() const;
    
    // Configuration
    void setVerbose(bool verbose) { verbose_ = verbose; }
    void setStopOnFailure(bool stop) { stop_on_failure_ = stop; }
    
private:
    struct TestInfo {
        std::string category;
        std::string name;
        TestFunction function;
    };
    
    std::vector<TestInfo> tests_;
    std::vector<TestResult> results_;
    bool verbose_;
    bool stop_on_failure_;
    
    TestResult runSingleTest(const TestInfo& test);
    void logTest(const TestResult& result);
};

/**
 * @brief Test assertion macros
 */
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__; \
            throw phantomvault::testing::TestAssertionException(ss.str()); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    ASSERT_TRUE(!(condition))

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected " << (expected) << " but got " << (actual) \
               << " at " << __FILE__ << ":" << __LINE__; \
            throw phantomvault::testing::TestAssertionException(ss.str()); \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected " << (expected) << " to not equal " << (actual) \
               << " at " << __FILE__ << ":" << __LINE__; \
            throw phantomvault::testing::TestAssertionException(ss.str()); \
        } \
    } while(0)

#define ASSERT_THROW(expression, exception_type) \
    do { \
        bool threw = false; \
        try { \
            (expression); \
        } catch (const exception_type&) { \
            threw = true; \
        } catch (...) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #expression << " threw wrong exception type at " \
               << __FILE__ << ":" << __LINE__; \
            throw phantomvault::testing::TestAssertionException(ss.str()); \
        } \
        if (!threw) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #expression << " did not throw " << #exception_type \
               << " at " << __FILE__ << ":" << __LINE__; \
            throw phantomvault::testing::TestAssertionException(ss.str()); \
        } \
    } while(0)

#define ASSERT_NO_THROW(expression) \
    do { \
        try { \
            (expression); \
        } catch (...) { \
            std::stringstream ss; \
            ss << "Assertion failed: " << #expression << " threw an exception at " \
               << __FILE__ << ":" << __LINE__; \
            throw phantomvault::testing::TestAssertionException(ss.str()); \
        } \
    } while(0)

/**
 * @brief Performance testing utilities
 */
class PerformanceTimer {
public:
    PerformanceTimer() : start_time_(std::chrono::high_resolution_clock::now()) {}
    
    std::chrono::milliseconds elapsed() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
    }
    
    void reset() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * @brief Security testing utilities
 */
class SecurityTestUtils {
public:
    // Generate test data
    static std::vector<uint8_t> generateRandomData(size_t size);
    static std::string generateRandomString(size_t length);
    
    // Cryptographic testing
    static bool isRandomDataUniform(const std::vector<uint8_t>& data);
    static bool hasProperEntropy(const std::vector<uint8_t>& data);
    
    // Timing attack resistance
    static bool isTimingAttackResistant(std::function<bool(const std::string&)> function,
                                       const std::string& correct_input,
                                       const std::string& incorrect_input,
                                       size_t iterations = 1000);
    
    // Memory security
    static bool isMemoryCleared(void* ptr, size_t size);
};

/**
 * @brief Test registration helper
 */
#define REGISTER_TEST(framework, category, name, function) \
    (framework).registerTest(category, name, [&]() { function(); })

} // namespace testing
} // namespace phantomvault