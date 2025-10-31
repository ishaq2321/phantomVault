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
#include <algorithm>
#include <set>
#include <fstream>
#include <thread>

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
    void logTest(const TestResult& result) const;
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
            ss << "Assertion failed: expected and actual values differ at " << __FILE__ << ":" << __LINE__; \
            throw phantomvault::testing::TestAssertionException(ss.str()); \
        } \
    } while(0)

#define ASSERT_VECTOR_EQ(expected, actual) \
    do { \
        if ((expected).size() != (actual).size() || !std::equal((expected).begin(), (expected).end(), (actual).begin())) { \
            std::stringstream ss; \
            ss << "Assertion failed: vectors differ (expected size: " << (expected).size() \
               << ", actual size: " << (actual).size() << ") at " << __FILE__ << ":" << __LINE__; \
            throw phantomvault::testing::TestAssertionException(ss.str()); \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            std::stringstream ss; \
            ss << "Assertion failed: expected values should not be equal at " << __FILE__ << ":" << __LINE__; \
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
 * @brief Benchmark statistics for performance analysis
 */
struct BenchmarkStats {
    std::chrono::nanoseconds min_time;
    std::chrono::nanoseconds max_time;
    std::chrono::nanoseconds avg_time;
    std::chrono::nanoseconds median_time;
    std::chrono::nanoseconds std_dev;
    size_t iterations;
    
    void print() const;
    bool meetsPerformanceTarget(std::chrono::nanoseconds target_time) const;
};

/**
 * @brief High-precision performance testing utilities with nanosecond accuracy
 */
class PerformanceTimer {
public:
    PerformanceTimer() : start_time_(std::chrono::high_resolution_clock::now()) {}
    
    std::chrono::milliseconds elapsed() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
    }
    
    std::chrono::nanoseconds elapsedNanos() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);
    }
    
    std::chrono::microseconds elapsedMicros() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    }
    
    void reset() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    // Benchmark a function with nanosecond precision
    template<typename Func>
    static std::chrono::nanoseconds benchmark(Func&& func, size_t iterations = 1) {
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < iterations; ++i) {
            func();
        }
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start) / iterations;
    }
    
    // Statistical benchmarking with multiple runs
    template<typename Func>
    static BenchmarkStats benchmarkStats(Func&& func, size_t iterations = 100) {
        std::vector<std::chrono::nanoseconds> times;
        times.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();
            times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
        }
        
        return calculateBenchmarkStats(times);
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    
public:
    static BenchmarkStats calculateBenchmarkStats(std::vector<std::chrono::nanoseconds>& times);
};

/**
 * @brief Timing analysis result for security testing
 */
struct TimingAnalysisResult {
    bool vulnerable;
    double confidence_level;
    std::chrono::nanoseconds avg_time_difference;
    std::string analysis_details;
};

/**
 * @brief Enhanced security testing utilities with penetration testing capabilities
 */
class SecurityTestUtils {
public:
    // Generate test data
    static std::vector<uint8_t> generateRandomData(size_t size);
    static std::string generateRandomString(size_t length);
    static std::vector<uint8_t> generateWeakRandomData(size_t size);
    
    // Cryptographic testing
    static bool isRandomDataUniform(const std::vector<uint8_t>& data);
    static bool hasProperEntropy(const std::vector<uint8_t>& data);
    static double calculateEntropy(const std::vector<uint8_t>& data);
    static bool passesChiSquareTest(const std::vector<uint8_t>& data);
    static bool passesRunsTest(const std::vector<uint8_t>& data);
    
    // Timing attack resistance
    static bool isTimingAttackResistant(std::function<bool(const std::string&)> function,
                                       const std::string& correct_input,
                                       const std::string& incorrect_input,
                                       size_t iterations = 1000);
    
    // Advanced timing analysis
    static TimingAnalysisResult analyzeTimingVulnerability(
        std::function<bool(const std::string&)> function,
        const std::vector<std::string>& test_inputs,
        size_t iterations = 10000);
    
    // Memory security
    static bool isMemoryCleared(void* ptr, size_t size);
    static bool detectMemoryLeaks(std::function<void()> test_function, size_t iterations = 100);
    static size_t measureMemoryUsage();
    
    // Penetration testing utilities
    static std::vector<std::string> generateFuzzingInputs(size_t count = 1000);
    static std::vector<std::vector<uint8_t>> generateMalformedData(size_t count = 100);
    static bool testBufferOverflow(std::function<void(const std::vector<uint8_t>&)> function);
    static bool testSQLInjection(std::function<bool(const std::string&)> function);
    static bool testPathTraversal(std::function<bool(const std::string&)> function);
    
    // Side-channel attack testing
    static bool testPowerAnalysisResistance(std::function<void()> crypto_function);
    static bool testCacheTimingAttacks(std::function<void(const std::string&)> function);
    
    // Cryptographic strength testing
    static bool testKeyStrength(const std::vector<uint8_t>& key);
    static bool testIVUniqueness(std::function<std::vector<uint8_t>()> iv_generator, size_t samples = 10000);
    static bool testSaltUniqueness(std::function<std::vector<uint8_t>()> salt_generator, size_t samples = 10000);
    
private:
};

/**
 * @brief Test registration helper
 */
#define REGISTER_TEST(framework, category, name, function) \
    (framework).registerTest(category, name, [&]() { function(); })

} // namespace testing
} // namespace phantomvault