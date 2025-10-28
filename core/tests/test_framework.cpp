/**
 * PhantomVault Test Framework Implementation
 */

#include "test_framework.hpp"
#include <algorithm>
#include <random>
#include <cstring>
#include <iomanip>

namespace phantomvault {
namespace testing {

TestFramework::TestFramework() 
    : verbose_(false)
    , stop_on_failure_(false) {
}

TestFramework::~TestFramework() = default;

void TestFramework::registerTest(const std::string& category, const std::string& name, TestFunction test) {
    TestInfo info;
    info.category = category;
    info.name = name;
    info.function = test;
    tests_.push_back(info);
}

bool TestFramework::runAllTests() {
    results_.clear();
    
    std::cout << "\n=== PhantomVault Comprehensive Test Suite ===" << std::endl;
    std::cout << "Running " << tests_.size() << " tests..." << std::endl;
    
    for (const auto& test : tests_) {
        TestResult result = runSingleTest(test);
        results_.push_back(result);
        
        if (verbose_) {
            logTest(result);
        }
        
        if (stop_on_failure_ && result.status == TestStatus::FAILED) {
            std::cout << "Stopping on first failure." << std::endl;
            break;
        }
    }
    
    printSummary();
    
    auto stats = getStats();
    return stats.failed_tests == 0 && stats.error_tests == 0;
}

bool TestFramework::runCategory(const std::string& category) {
    results_.clear();
    
    std::cout << "\n=== Running " << category << " Tests ===" << std::endl;
    
    size_t category_tests = 0;
    for (const auto& test : tests_) {
        if (test.category == category) {
            TestResult result = runSingleTest(test);
            results_.push_back(result);
            category_tests++;
            
            if (verbose_) {
                logTest(result);
            }
            
            if (stop_on_failure_ && result.status == TestStatus::FAILED) {
                break;
            }
        }
    }
    
    std::cout << "Ran " << category_tests << " tests in category: " << category << std::endl;
    printSummary();
    
    auto stats = getStats();
    return stats.failed_tests == 0 && stats.error_tests == 0;
}

bool TestFramework::runTest(const std::string& category, const std::string& name) {
    results_.clear();
    
    for (const auto& test : tests_) {
        if (test.category == category && test.name == name) {
            TestResult result = runSingleTest(test);
            results_.push_back(result);
            logTest(result);
            return result.status == TestStatus::PASSED;
        }
    }
    
    std::cout << "Test not found: " << category << "::" << name << std::endl;
    return false;
}

TestResult TestFramework::runSingleTest(const TestInfo& test) {
    TestResult result;
    result.test_name = test.name;
    result.test_category = test.category;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        test.function();
        result.status = TestStatus::PASSED;
        result.message = "Test passed";
    } catch (const TestAssertionException& e) {
        result.status = TestStatus::FAILED;
        result.message = "Assertion failed";
        result.error_details = e.what();
    } catch (const std::exception& e) {
        result.status = TestStatus::ERROR;
        result.message = "Test error";
        result.error_details = e.what();
    } catch (...) {
        result.status = TestStatus::ERROR;
        result.message = "Unknown error";
        result.error_details = "Unknown exception thrown";
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

void TestFramework::logTest(const TestResult& result) {
    std::string status_str;
    switch (result.status) {
        case TestStatus::PASSED: status_str = "PASS"; break;
        case TestStatus::FAILED: status_str = "FAIL"; break;
        case TestStatus::SKIPPED: status_str = "SKIP"; break;
        case TestStatus::ERROR: status_str = "ERROR"; break;
    }
    
    std::cout << "[" << std::setw(5) << status_str << "] " 
              << result.test_category << "::" << result.test_name 
              << " (" << result.duration.count() << "ms)";
    
    if (result.status != TestStatus::PASSED) {
        std::cout << " - " << result.message;
        if (!result.error_details.empty()) {
            std::cout << ": " << result.error_details;
        }
    }
    
    std::cout << std::endl;
}

TestSuiteStats TestFramework::getStats() const {
    TestSuiteStats stats;
    
    for (const auto& result : results_) {
        stats.total_tests++;
        stats.total_duration += result.duration;
        
        switch (result.status) {
            case TestStatus::PASSED: stats.passed_tests++; break;
            case TestStatus::FAILED: stats.failed_tests++; break;
            case TestStatus::SKIPPED: stats.skipped_tests++; break;
            case TestStatus::ERROR: stats.error_tests++; break;
        }
    }
    
    return stats;
}

void TestFramework::printResults() const {
    std::cout << "\n=== Detailed Test Results ===" << std::endl;
    
    for (const auto& result : results_) {
        logTest(result);
    }
}

void TestFramework::printSummary() const {
    auto stats = getStats();
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total Tests: " << stats.total_tests << std::endl;
    std::cout << "Passed: " << stats.passed_tests << std::endl;
    std::cout << "Failed: " << stats.failed_tests << std::endl;
    std::cout << "Errors: " << stats.error_tests << std::endl;
    std::cout << "Skipped: " << stats.skipped_tests << std::endl;
    std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) << stats.pass_rate() << "%" << std::endl;
    std::cout << "Total Duration: " << stats.total_duration.count() << "ms" << std::endl;
    
    if (stats.failed_tests > 0 || stats.error_tests > 0) {
        std::cout << "\n❌ TEST SUITE FAILED" << std::endl;
    } else {
        std::cout << "\n✅ ALL TESTS PASSED" << std::endl;
    }
}

// SecurityTestUtils implementation
std::vector<uint8_t> SecurityTestUtils::generateRandomData(size_t size) {
    std::vector<uint8_t> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(dis(gen));
    }
    
    return data;
}

std::string SecurityTestUtils::generateRandomString(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.reserve(length);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    for (size_t i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    
    return result;
}

bool SecurityTestUtils::isRandomDataUniform(const std::vector<uint8_t>& data) {
    if (data.size() < 256) return false;
    
    std::array<size_t, 256> counts = {};
    for (uint8_t byte : data) {
        counts[byte]++;
    }
    
    // Check if distribution is reasonably uniform
    size_t expected = data.size() / 256;
    size_t tolerance = expected / 4; // 25% tolerance
    
    for (size_t count : counts) {
        if (count < expected - tolerance || count > expected + tolerance) {
            return false;
        }
    }
    
    return true;
}

bool SecurityTestUtils::hasProperEntropy(const std::vector<uint8_t>& data) {
    if (data.size() < 100) return false;
    
    // Simple entropy check - count unique bytes
    std::set<uint8_t> unique_bytes(data.begin(), data.end());
    
    // Should have reasonable variety
    return unique_bytes.size() >= std::min(data.size() / 4, size_t(64));
}

bool SecurityTestUtils::isTimingAttackResistant(std::function<bool(const std::string&)> function,
                                               const std::string& correct_input,
                                               const std::string& incorrect_input,
                                               size_t iterations) {
    std::vector<std::chrono::nanoseconds> correct_times;
    std::vector<std::chrono::nanoseconds> incorrect_times;
    
    // Warm up
    for (size_t i = 0; i < 10; ++i) {
        function(correct_input);
        function(incorrect_input);
    }
    
    // Measure timing
    for (size_t i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        function(correct_input);
        auto end = std::chrono::high_resolution_clock::now();
        correct_times.push_back(end - start);
        
        start = std::chrono::high_resolution_clock::now();
        function(incorrect_input);
        end = std::chrono::high_resolution_clock::now();
        incorrect_times.push_back(end - start);
    }
    
    // Calculate averages
    auto correct_avg = std::accumulate(correct_times.begin(), correct_times.end(), 
                                     std::chrono::nanoseconds(0)) / correct_times.size();
    auto incorrect_avg = std::accumulate(incorrect_times.begin(), incorrect_times.end(), 
                                       std::chrono::nanoseconds(0)) / incorrect_times.size();
    
    // Check if timing difference is within acceptable range (less than 10% difference)
    auto diff = std::abs(correct_avg.count() - incorrect_avg.count());
    auto max_time = std::max(correct_avg.count(), incorrect_avg.count());
    
    return (double)diff / max_time < 0.1;
}

bool SecurityTestUtils::isMemoryCleared(void* ptr, size_t size) {
    uint8_t* bytes = static_cast<uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        if (bytes[i] != 0) {
            return false;
        }
    }
    return true;
}

} // namespace testing
} // namespace phantomvault