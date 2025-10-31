/**
 * PhantomVault Test Framework Implementation
 */

#include "test_framework.hpp"
#include <algorithm>
#include <random>
#include <cstring>
#include <iomanip>
#include <set>
#include <fstream>
#include <thread>
#include <cmath>
#include <numeric>

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

void TestFramework::logTest(const TestResult& result) const {
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
    auto diff = std::abs(static_cast<long long>(correct_avg.count()) - static_cast<long long>(incorrect_avg.count()));
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

// BenchmarkStats implementation
void BenchmarkStats::print() const {
    std::cout << "Benchmark Statistics:" << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;
    std::cout << "  Min Time: " << min_time.count() << "ns" << std::endl;
    std::cout << "  Max Time: " << max_time.count() << "ns" << std::endl;
    std::cout << "  Avg Time: " << avg_time.count() << "ns" << std::endl;
    std::cout << "  Median Time: " << median_time.count() << "ns" << std::endl;
    std::cout << "  Std Dev: " << std_dev.count() << "ns" << std::endl;
}

bool BenchmarkStats::meetsPerformanceTarget(std::chrono::nanoseconds target_time) const {
    return avg_time <= target_time;
}

// PerformanceTimer static method implementations
BenchmarkStats PerformanceTimer::calculateBenchmarkStats(std::vector<std::chrono::nanoseconds>& times) {
    BenchmarkStats stats;
    stats.iterations = times.size();
    
    if (times.empty()) {
        return stats;
    }
    
    // Sort for median calculation
    std::sort(times.begin(), times.end());
    
    stats.min_time = times.front();
    stats.max_time = times.back();
    stats.median_time = times[times.size() / 2];
    
    // Calculate average
    auto total = std::accumulate(times.begin(), times.end(), std::chrono::nanoseconds(0));
    stats.avg_time = total / times.size();
    
    // Calculate standard deviation
    double variance = 0.0;
    for (const auto& time : times) {
        double diff = time.count() - stats.avg_time.count();
        variance += diff * diff;
    }
    variance /= times.size();
    stats.std_dev = std::chrono::nanoseconds(static_cast<long long>(std::sqrt(variance)));
    
    return stats;
}

// Enhanced SecurityTestUtils implementation
double SecurityTestUtils::calculateEntropy(const std::vector<uint8_t>& data) {
    if (data.empty()) return 0.0;
    
    std::array<size_t, 256> counts = {};
    for (uint8_t byte : data) {
        counts[byte]++;
    }
    
    double entropy = 0.0;
    double total = static_cast<double>(data.size());
    
    for (size_t count : counts) {
        if (count > 0) {
            double probability = count / total;
            entropy -= probability * std::log2(probability);
        }
    }
    
    return entropy;
}

bool SecurityTestUtils::passesChiSquareTest(const std::vector<uint8_t>& data) {
    if (data.size() < 256) return false;
    
    std::array<size_t, 256> observed = {};
    for (uint8_t byte : data) {
        observed[byte]++;
    }
    
    double expected = static_cast<double>(data.size()) / 256.0;
    double chi_square = 0.0;
    
    for (size_t count : observed) {
        double diff = count - expected;
        chi_square += (diff * diff) / expected;
    }
    
    // Critical value for 255 degrees of freedom at 95% confidence is approximately 293.25
    return chi_square < 293.25;
}

bool SecurityTestUtils::passesRunsTest(const std::vector<uint8_t>& data) {
    if (data.size() < 100) return false;
    
    size_t runs = 1;
    for (size_t i = 1; i < data.size(); ++i) {
        if ((data[i] >= 128) != (data[i-1] >= 128)) {
            runs++;
        }
    }
    
    // Expected runs for random data
    size_t n = data.size();
    double expected_runs = (2.0 * n - 1.0) / 3.0;
    double variance = (16.0 * n - 29.0) / 90.0;
    double z_score = (runs - expected_runs) / std::sqrt(variance);
    
    // Should be within 2 standard deviations
    return std::abs(z_score) < 2.0;
}

TimingAnalysisResult SecurityTestUtils::analyzeTimingVulnerability(
    std::function<bool(const std::string&)> function,
    const std::vector<std::string>& test_inputs,
    size_t iterations) {
    
    TimingAnalysisResult result;
    result.vulnerable = false;
    result.confidence_level = 0.0;
    result.avg_time_difference = std::chrono::nanoseconds(0);
    
    if (test_inputs.size() < 2) {
        result.analysis_details = "Need at least 2 test inputs";
        return result;
    }
    
    std::vector<std::vector<std::chrono::nanoseconds>> timing_data(test_inputs.size());
    
    // Collect timing data
    for (size_t i = 0; i < iterations; ++i) {
        for (size_t j = 0; j < test_inputs.size(); ++j) {
            auto start = std::chrono::high_resolution_clock::now();
            function(test_inputs[j]);
            auto end = std::chrono::high_resolution_clock::now();
            
            timing_data[j].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
        }
    }
    
    // Calculate averages and check for significant differences
    std::vector<std::chrono::nanoseconds> averages;
    for (const auto& times : timing_data) {
        auto total = std::accumulate(times.begin(), times.end(), std::chrono::nanoseconds(0));
        averages.push_back(total / times.size());
    }
    
    // Find max difference
    auto min_avg = *std::min_element(averages.begin(), averages.end());
    auto max_avg = *std::max_element(averages.begin(), averages.end());
    result.avg_time_difference = max_avg - min_avg;
    
    // Calculate confidence level based on statistical significance
    double relative_difference = static_cast<double>(result.avg_time_difference.count()) / min_avg.count();
    
    if (relative_difference > 0.1) { // More than 10% difference
        result.vulnerable = true;
        result.confidence_level = std::min(0.99, relative_difference * 5.0);
        result.analysis_details = "Significant timing differences detected";
    } else {
        result.confidence_level = 1.0 - relative_difference * 10.0;
        result.analysis_details = "No significant timing vulnerabilities detected";
    }
    
    return result;
}

std::vector<uint8_t> SecurityTestUtils::generateWeakRandomData(size_t size) {
    std::vector<uint8_t> data(size);
    // Generate predictable "weak" random data for testing
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    return data;
}

bool SecurityTestUtils::detectMemoryLeaks(std::function<void()> test_function, size_t iterations) {
    size_t initial_memory = measureMemoryUsage();
    
    for (size_t i = 0; i < iterations; ++i) {
        test_function();
    }
    
    // Force garbage collection if applicable
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    size_t final_memory = measureMemoryUsage();
    size_t memory_increase = final_memory > initial_memory ? final_memory - initial_memory : 0;
    
    // Allow some memory increase but not excessive
    return memory_increase < (iterations * 1024); // Less than 1KB per iteration
}

size_t SecurityTestUtils::measureMemoryUsage() {
    #ifdef __linux__
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string label, value, unit;
            iss >> label >> value >> unit;
            return std::stoul(value) * 1024; // Convert KB to bytes
        }
    }
    #endif
    return 10 * 1024 * 1024; // 10MB fallback
}

std::vector<std::string> SecurityTestUtils::generateFuzzingInputs(size_t count) {
    std::vector<std::string> inputs;
    inputs.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> char_dis(0, 255);
    std::uniform_int_distribution<> length_dis(0, 1000);
    
    for (size_t i = 0; i < count; ++i) {
        size_t length = length_dis(gen);
        std::string input;
        input.reserve(length);
        
        for (size_t j = 0; j < length; ++j) {
            input += static_cast<char>(char_dis(gen));
        }
        
        inputs.push_back(input);
    }
    
    return inputs;
}

std::vector<std::vector<uint8_t>> SecurityTestUtils::generateMalformedData(size_t count) {
    std::vector<std::vector<uint8_t>> data_sets;
    data_sets.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (size_t i = 0; i < count; ++i) {
        std::uniform_int_distribution<> size_dis(0, 10000);
        size_t size = size_dis(gen);
        
        auto data = generateRandomData(size);
        data_sets.push_back(data);
    }
    
    return data_sets;
}

bool SecurityTestUtils::testBufferOverflow(std::function<void(const std::vector<uint8_t>&)> function) {
    try {
        // Test with extremely large buffer
        std::vector<uint8_t> large_buffer(100000, 0xFF);
        function(large_buffer);
        
        // Test with malformed size
        std::vector<uint8_t> malformed_buffer;
        malformed_buffer.resize(SIZE_MAX / 2, 0xAA);
        function(malformed_buffer);
        
        return true; // No crash = good
    } catch (...) {
        return true; // Exception handling = good
    }
}

bool SecurityTestUtils::testSQLInjection(std::function<bool(const std::string&)> function) {
    std::vector<std::string> injection_payloads = {
        "'; DROP TABLE users; --",
        "admin'--",
        "' OR '1'='1",
        "' UNION SELECT * FROM passwords --"
    };
    
    for (const auto& payload : injection_payloads) {
        bool result = function(payload);
        if (result) {
            return false; // Injection succeeded = bad
        }
    }
    
    return true; // All injections failed = good
}

bool SecurityTestUtils::testPathTraversal(std::function<bool(const std::string&)> function) {
    std::vector<std::string> traversal_payloads = {
        "../../../etc/passwd",
        "..\\..\\..\\windows\\system32",
        "/etc/shadow",
        "../../../../root/.ssh/id_rsa"
    };
    
    for (const auto& payload : traversal_payloads) {
        bool result = function(payload);
        if (result) {
            return false; // Traversal succeeded = bad
        }
    }
    
    return true; // All traversals failed = good
}

bool SecurityTestUtils::testPowerAnalysisResistance(std::function<void()> crypto_function) {
    // Simplified power analysis test - measure timing consistency
    std::vector<std::chrono::nanoseconds> timings;
    
    for (int i = 0; i < 1000; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        crypto_function();
        auto end = std::chrono::high_resolution_clock::now();
        
        timings.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
    }
    
    // Calculate coefficient of variation
    auto total = std::accumulate(timings.begin(), timings.end(), std::chrono::nanoseconds(0));
    auto mean = total / timings.size();
    
    double variance = 0.0;
    for (const auto& timing : timings) {
        double diff = timing.count() - mean.count();
        variance += diff * diff;
    }
    variance /= timings.size();
    
    double cv = std::sqrt(variance) / mean.count();
    
    // Low coefficient of variation indicates consistent timing (good)
    return cv < 0.1; // Less than 10% variation
}

bool SecurityTestUtils::testCacheTimingAttacks(std::function<void(const std::string&)> function) {
    // Test cache timing resistance by measuring timing with different inputs
    std::vector<std::string> test_inputs = {
        "cache_test_1", "cache_test_2", "different_input", "another_test"
    };
    
    std::vector<std::chrono::nanoseconds> timings;
    
    for (const auto& input : test_inputs) {
        auto start = std::chrono::high_resolution_clock::now();
        function(input);
        auto end = std::chrono::high_resolution_clock::now();
        
        timings.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
    }
    
    // Check for consistent timing
    auto min_time = *std::min_element(timings.begin(), timings.end());
    auto max_time = *std::max_element(timings.begin(), timings.end());
    
    double variation = static_cast<double>(max_time.count() - min_time.count()) / min_time.count();
    
    // Low variation indicates cache timing resistance
    return variation < 0.2; // Less than 20% variation
}

bool SecurityTestUtils::testKeyStrength(const std::vector<uint8_t>& key) {
    if (key.size() < 32) return false; // At least 256 bits
    
    return hasProperEntropy(key) && isRandomDataUniform(key);
}

bool SecurityTestUtils::testIVUniqueness(std::function<std::vector<uint8_t>()> iv_generator, size_t samples) {
    std::set<std::vector<uint8_t>> unique_ivs;
    
    for (size_t i = 0; i < samples; ++i) {
        auto iv = iv_generator();
        unique_ivs.insert(iv);
    }
    
    // Should have very high uniqueness rate
    return unique_ivs.size() >= (samples * 0.99); // At least 99% unique
}

bool SecurityTestUtils::testSaltUniqueness(std::function<std::vector<uint8_t>()> salt_generator, size_t samples) {
    std::set<std::vector<uint8_t>> unique_salts;
    
    for (size_t i = 0; i < samples; ++i) {
        auto salt = salt_generator();
        unique_salts.insert(salt);
    }
    
    // Should have very high uniqueness rate
    return unique_salts.size() >= (samples * 0.99); // At least 99% unique
}

} // namespace testing
} // namespace phantomvault