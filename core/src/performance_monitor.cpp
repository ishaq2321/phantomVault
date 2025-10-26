/**
 * PhantomVault Performance Monitor Implementation
 * 
 * Monitors system performance and provides adaptive optimization.
 */

#include "performance_monitor.hpp"
#include "memory_manager.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <sys/times.h>
#include <sys/sysinfo.h>
#include <fstream>
#elif PLATFORM_WINDOWS
#include <windows.h>
#include <psapi.h>
#include <powrprof.h>
#elif PLATFORM_MACOS
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <IOKit/ps/IOPowerSources.h>
#endif

namespace phantomvault {

class PerformanceMonitor::Implementation {
public:
    Implementation()
        : running_(false)
        , performance_mode_(PerformanceMode::BALANCED)
        , adaptive_tuning_enabled_(true)
        , memory_limit_kb_(8192) // 8MB default
        , cpu_limit_percent_(5.0) // 5% CPU max
        , monitoring_thread_()
        , last_cpu_times_{}
        , start_time_(std::chrono::steady_clock::now())
    {}
    
    ~Implementation() {
        stop();
    }
    
    bool initialize() {
        try {
            // Initialize platform-specific monitoring
            #ifdef PLATFORM_LINUX
            if (!initializeLinux()) {
                return false;
            }
            #elif PLATFORM_WINDOWS
            if (!initializeWindows()) {
                return false;
            }
            #elif PLATFORM_MACOS
            if (!initializeMacOS()) {
                return false;
            }
            #endif
            
            std::cout << "[PerformanceMonitor] Initialized successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "[PerformanceMonitor] Initialization failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool start() {
        if (running_) {
            return true;
        }
        
        running_ = true;
        monitoring_thread_ = std::thread(&Implementation::monitoringLoop, this);
        
        std::cout << "[PerformanceMonitor] Started monitoring" << std::endl;
        return true;
    }
    
    void stop() {
        if (!running_) {
            return;
        }
        
        running_ = false;
        
        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }
        
        std::cout << "[PerformanceMonitor] Stopped monitoring" << std::endl;
    }
    
    bool isRunning() const {
        return running_;
    }
    
    PerformanceMetrics getMetrics() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        
        PerformanceMetrics metrics = current_metrics_;
        
        // Update uptime
        auto now = std::chrono::steady_clock::now();
        metrics.uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time_);
        
        // Get memory usage from MemoryManager
        auto memStats = MemoryManager::getInstance().getStats();
        metrics.memoryUsage = memStats.currentUsage / 1024; // Convert to KB
        metrics.peakMemoryUsage = memStats.peakUsage / 1024;
        
        return metrics;
    }
    
    CPUStats getCPUStats() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        return current_metrics_.cpuStats;
    }
    
    BatteryInfo getBatteryInfo() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        return current_metrics_.batteryInfo;
    }
    
    void setPerformanceMode(PerformanceMode mode) {
        performance_mode_ = mode;
        applyPerformanceMode();
        std::cout << "[PerformanceMonitor] Performance mode set to " << 
                     static_cast<int>(mode) << std::endl;
    }
    
    PerformanceMode getPerformanceMode() const {
        return performance_mode_;
    }
    
    void enableAdaptiveTuning(bool enabled) {
        adaptive_tuning_enabled_ = enabled;
        std::cout << "[PerformanceMonitor] Adaptive tuning " << 
                     (enabled ? "enabled" : "disabled") << std::endl;
    }
    
    bool isAdaptiveTuningEnabled() const {
        return adaptive_tuning_enabled_;
    }
    
    void setPerformanceCallback(PerformanceCallback callback) {
        performance_callback_ = callback;
    }
    
    void setBatteryCallback(BatteryCallback callback) {
        battery_callback_ = callback;
    }
    
    void setMemoryLimit(size_t limitKB) {
        memory_limit_kb_ = limitKB;
        MemoryManager::getInstance().setMemoryLimit(limitKB * 1024);
    }
    
    void setCPULimit(double maxCPUPercent) {
        cpu_limit_percent_ = maxCPUPercent;
    }
    
    bool isResourceLimitExceeded() const {
        auto metrics = getMetrics();
        
        // Check memory limit
        if (metrics.memoryUsage > memory_limit_kb_) {
            return true;
        }
        
        // Check CPU limit
        if (metrics.cpuStats.totalTime > cpu_limit_percent_) {
            return true;
        }
        
        return false;
    }
    
    void onSystemSuspend() {
        std::cout << "[PerformanceMonitor] System suspending - reducing activity" << std::endl;
        setPerformanceMode(PerformanceMode::POWER_SAVER);
    }
    
    void onSystemResume() {
        std::cout << "[PerformanceMonitor] System resumed - restoring activity" << std::endl;
        if (adaptive_tuning_enabled_) {
            setPerformanceMode(PerformanceMode::ADAPTIVE);
        }
    }
    
    void onBatteryLow() {
        std::cout << "[PerformanceMonitor] Battery low - switching to power saver" << std::endl;
        setPerformanceMode(PerformanceMode::POWER_SAVER);
    }
    
    void onACPowerChanged(bool onAC) {
        std::cout << "[PerformanceMonitor] AC power " << (onAC ? "connected" : "disconnected") << std::endl;
        
        if (adaptive_tuning_enabled_) {
            if (onAC) {
                setPerformanceMode(PerformanceMode::BALANCED);
            } else {
                setPerformanceMode(PerformanceMode::POWER_SAVER);
            }
        }
    }

private:
    std::atomic<bool> running_;
    PerformanceMode performance_mode_;
    std::atomic<bool> adaptive_tuning_enabled_;
    std::atomic<size_t> memory_limit_kb_;
    std::atomic<double> cpu_limit_percent_;
    
    std::thread monitoring_thread_;
    mutable std::mutex metrics_mutex_;
    PerformanceMetrics current_metrics_;
    
    PerformanceCallback performance_callback_;
    BatteryCallback battery_callback_;
    
    // Platform-specific data
    #ifdef PLATFORM_LINUX
    struct {
        long user_time = 0;
        long system_time = 0;
        long idle_time = 0;
        long total_time = 0;
    } last_cpu_times_;
    #endif
    
    std::chrono::steady_clock::time_point start_time_;
    
    void monitoringLoop() {
        while (running_) {
            try {
                updateMetrics();
                
                if (adaptive_tuning_enabled_) {
                    performAdaptiveTuning();
                }
                
                // Notify callbacks
                if (performance_callback_) {
                    performance_callback_(getMetrics());
                }
                
                // Sleep based on performance mode
                auto sleepDuration = getSleepDuration();
                std::this_thread::sleep_for(sleepDuration);
                
            } catch (const std::exception& e) {
                std::cerr << "[PerformanceMonitor] Monitoring error: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    void updateMetrics() {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        
        // Update CPU stats
        current_metrics_.cpuStats = measureCPUUsage();
        
        // Update battery info
        current_metrics_.batteryInfo = getBatteryStatus();
        
        // Calculate CPU efficiency (lower is better)
        current_metrics_.cpuEfficiency = calculateCPUEfficiency();
    }
    
    void performAdaptiveTuning() {
        auto metrics = getMetrics();
        
        // Adapt based on battery status
        if (metrics.batteryInfo.isOnBattery && metrics.batteryInfo.batteryLevel < 20) {
            if (performance_mode_ != PerformanceMode::POWER_SAVER) {
                setPerformanceMode(PerformanceMode::POWER_SAVER);
            }
        }
        
        // Adapt based on CPU usage
        if (metrics.cpuStats.totalTime > cpu_limit_percent_ * 1.5) {
            // Reduce activity if CPU usage is too high
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Adapt based on memory usage
        if (metrics.memoryUsage > memory_limit_kb_ * 0.8) {
            // Trigger memory cleanup
            MemoryManager::getInstance().compactPools();
        }
    }
    
    std::chrono::milliseconds getSleepDuration() const {
        switch (performance_mode_) {
            case PerformanceMode::HIGH_PERFORMANCE:
                return std::chrono::milliseconds(100);
            case PerformanceMode::BALANCED:
                return std::chrono::milliseconds(500);
            case PerformanceMode::POWER_SAVER:
                return std::chrono::milliseconds(2000);
            case PerformanceMode::ADAPTIVE:
                return std::chrono::milliseconds(1000);
            default:
                return std::chrono::milliseconds(1000);
        }
    }
    
    void applyPerformanceMode() {
        // Adjust monitoring frequency and resource limits based on mode
        switch (performance_mode_) {
            case PerformanceMode::HIGH_PERFORMANCE:
                setCPULimit(10.0); // Allow higher CPU usage
                break;
            case PerformanceMode::BALANCED:
                setCPULimit(5.0);  // Balanced CPU usage
                break;
            case PerformanceMode::POWER_SAVER:
                setCPULimit(2.0);  // Minimize CPU usage
                break;
            case PerformanceMode::ADAPTIVE:
                setCPULimit(5.0);  // Start balanced, adapt as needed
                break;
        }
    }
    
    double calculateCPUEfficiency() const {
        // Simple efficiency metric: operations per CPU cycle
        // Lower values indicate better efficiency
        auto metrics = getMetrics();
        if (metrics.cpuStats.totalTime > 0) {
            uint64_t totalOps = metrics.keyboardEvents + metrics.httpRequests + 
                               metrics.fileOperations + metrics.encryptionOperations;
            return metrics.cpuStats.totalTime / std::max(totalOps, static_cast<uint64_t>(1));
        }
        return 1.0;
    }
    
    // Platform-specific implementations
    #ifdef PLATFORM_LINUX
    bool initializeLinux() {
        // Check if we can read /proc/stat and /proc/meminfo
        std::ifstream stat("/proc/stat");
        std::ifstream meminfo("/proc/meminfo");
        return stat.is_open() && meminfo.is_open();
    }
    
    CPUStats measureCPUUsage() {
        CPUStats stats;
        
        std::ifstream stat("/proc/stat");
        if (!stat.is_open()) {
            return stats;
        }
        
        std::string line;
        std::getline(stat, line);
        
        // Parse CPU times from /proc/stat
        long user, nice, system, idle, iowait, irq, softirq, steal;
        sscanf(line.c_str(), "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
        
        long total = user + nice + system + idle + iowait + irq + softirq + steal;
        
        if (last_cpu_times_.total_time > 0) {
            long total_diff = total - last_cpu_times_.total_time;
            long idle_diff = idle - last_cpu_times_.idle_time;
            long user_diff = user - last_cpu_times_.user_time;
            long system_diff = system - last_cpu_times_.system_time;
            
            if (total_diff > 0) {
                stats.idleTime = (100.0 * idle_diff) / total_diff;
                stats.userTime = (100.0 * user_diff) / total_diff;
                stats.systemTime = (100.0 * system_diff) / total_diff;
                stats.totalTime = 100.0 - stats.idleTime;
            }
        }
        
        // Update last times
        last_cpu_times_.user_time = user;
        last_cpu_times_.system_time = system;
        last_cpu_times_.idle_time = idle;
        last_cpu_times_.total_time = total;
        
        return stats;
    }
    
    BatteryInfo getBatteryStatus() {
        BatteryInfo info;
        
        // Try to read battery information from /sys/class/power_supply/
        std::ifstream ac_online("/sys/class/power_supply/ADP1/online");
        if (ac_online.is_open()) {
            int online;
            ac_online >> online;
            info.isOnBattery = (online == 0);
            info.isCharging = (online == 1);
        }
        
        std::ifstream bat_capacity("/sys/class/power_supply/BAT0/capacity");
        if (bat_capacity.is_open()) {
            bat_capacity >> info.batteryLevel;
        }
        
        return info;
    }
    #endif
    
    #ifdef PLATFORM_WINDOWS
    bool initializeWindows() {
        // Initialize Windows performance counters
        return true;
    }
    
    CPUStats measureCPUUsage() {
        CPUStats stats;
        // Windows-specific CPU measurement implementation
        return stats;
    }
    
    BatteryInfo getBatteryStatus() {
        BatteryInfo info;
        // Windows-specific battery status implementation
        return info;
    }
    #endif
    
    #ifdef PLATFORM_MACOS
    bool initializeMacOS() {
        // Initialize macOS performance monitoring
        return true;
    }
    
    CPUStats measureCPUUsage() {
        CPUStats stats;
        // macOS-specific CPU measurement implementation
        return stats;
    }
    
    BatteryInfo getBatteryStatus() {
        BatteryInfo info;
        // macOS-specific battery status implementation
        return info;
    }
    #endif
};

// PerformanceMonitor public interface
PerformanceMonitor::PerformanceMonitor() : pimpl(std::make_unique<Implementation>()) {}
PerformanceMonitor::~PerformanceMonitor() = default;

bool PerformanceMonitor::initialize() { return pimpl->initialize(); }
bool PerformanceMonitor::start() { return pimpl->start(); }
void PerformanceMonitor::stop() { pimpl->stop(); }
bool PerformanceMonitor::isRunning() const { return pimpl->isRunning(); }

PerformanceMetrics PerformanceMonitor::getMetrics() const { return pimpl->getMetrics(); }
CPUStats PerformanceMonitor::getCPUStats() const { return pimpl->getCPUStats(); }
BatteryInfo PerformanceMonitor::getBatteryInfo() const { return pimpl->getBatteryInfo(); }

void PerformanceMonitor::setPerformanceMode(PerformanceMode mode) { pimpl->setPerformanceMode(mode); }
PerformanceMode PerformanceMonitor::getPerformanceMode() const { return pimpl->getPerformanceMode(); }

void PerformanceMonitor::enableAdaptiveTuning(bool enabled) { pimpl->enableAdaptiveTuning(enabled); }
bool PerformanceMonitor::isAdaptiveTuningEnabled() const { return pimpl->isAdaptiveTuningEnabled(); }

void PerformanceMonitor::setPerformanceCallback(PerformanceCallback callback) { pimpl->setPerformanceCallback(callback); }
void PerformanceMonitor::setBatteryCallback(BatteryCallback callback) { pimpl->setBatteryCallback(callback); }

void PerformanceMonitor::setMemoryLimit(size_t limitKB) { pimpl->setMemoryLimit(limitKB); }
void PerformanceMonitor::setCPULimit(double maxCPUPercent) { pimpl->setCPULimit(maxCPUPercent); }
bool PerformanceMonitor::isResourceLimitExceeded() const { return pimpl->isResourceLimitExceeded(); }

void PerformanceMonitor::onSystemSuspend() { pimpl->onSystemSuspend(); }
void PerformanceMonitor::onSystemResume() { pimpl->onSystemResume(); }
void PerformanceMonitor::onBatteryLow() { pimpl->onBatteryLow(); }
void PerformanceMonitor::onACPowerChanged(bool onAC) { pimpl->onACPowerChanged(onAC); }

void PerformanceMonitor::incrementKeyboardEvents() { ++performance_counters_.keyboardEvents; }
void PerformanceMonitor::incrementHttpRequests() { ++performance_counters_.httpRequests; }
void PerformanceMonitor::incrementFileOperations() { ++performance_counters_.fileOperations; }
void PerformanceMonitor::incrementEncryptionOperations() { ++performance_counters_.encryptionOperations; }

} // namespace phantomvault