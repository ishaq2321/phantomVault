/**
 * PhantomVault Performance Monitor
 * 
 * Monitors system performance, CPU usage, and battery impact.
 * Provides adaptive performance tuning and resource management.
 */

#pragma once

#include <chrono>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include <vector>

namespace phantomvault {

/**
 * CPU usage statistics
 */
struct CPUStats {
    double userTime = 0.0;      // User CPU time percentage
    double systemTime = 0.0;    // System CPU time percentage
    double totalTime = 0.0;     // Total CPU time percentage
    double idleTime = 0.0;      // Idle time percentage
    std::chrono::milliseconds measurementPeriod{1000};
};

/**
 * Battery information
 */
struct BatteryInfo {
    bool isOnBattery = false;
    int batteryLevel = 100;     // 0-100 percentage
    bool isCharging = false;
    std::chrono::minutes estimatedLife{0};
};

/**
 * Performance metrics
 */
struct PerformanceMetrics {
    CPUStats cpuStats;
    BatteryInfo batteryInfo;
    size_t memoryUsage = 0;     // KB
    size_t peakMemoryUsage = 0; // KB
    double cpuEfficiency = 1.0; // 0.0-1.0, lower is more efficient
    std::chrono::milliseconds uptime{0};
    
    // Performance counters
    uint64_t keyboardEvents = 0;
    uint64_t httpRequests = 0;
    uint64_t fileOperations = 0;
    uint64_t encryptionOperations = 0;
};

/**
 * Performance optimization modes
 */
enum class PerformanceMode {
    HIGH_PERFORMANCE,   // Maximum responsiveness, higher resource usage
    BALANCED,          // Balance between performance and efficiency
    POWER_SAVER,       // Minimize battery usage, lower performance
    ADAPTIVE           // Automatically adjust based on system state
};

/**
 * Performance monitor and optimizer
 */
class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor();

    // Lifecycle
    bool initialize();
    bool start();
    void stop();
    bool isRunning() const;

    // Performance monitoring
    PerformanceMetrics getMetrics() const;
    CPUStats getCPUStats() const;
    BatteryInfo getBatteryInfo() const;
    
    // Performance optimization
    void setPerformanceMode(PerformanceMode mode);
    PerformanceMode getPerformanceMode() const;
    
    // Adaptive tuning
    void enableAdaptiveTuning(bool enabled);
    bool isAdaptiveTuningEnabled() const;
    
    // Event callbacks
    using PerformanceCallback = std::function<void(const PerformanceMetrics&)>;
    using BatteryCallback = std::function<void(const BatteryInfo&)>;
    
    void setPerformanceCallback(PerformanceCallback callback);
    void setBatteryCallback(BatteryCallback callback);
    
    // Performance counters
    void incrementKeyboardEvents();
    void incrementHttpRequests();
    void incrementFileOperations();
    void incrementEncryptionOperations();
    
    // Resource management
    void setMemoryLimit(size_t limitKB);
    void setCPULimit(double maxCPUPercent);
    bool isResourceLimitExceeded() const;
    
    // Power management
    void onSystemSuspend();
    void onSystemResume();
    void onBatteryLow();
    void onACPowerChanged(bool onAC);

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
    
    mutable PerformanceMetrics performance_counters_;
};

/**
 * Adaptive scheduler for background tasks
 */
class AdaptiveScheduler {
public:
    AdaptiveScheduler(PerformanceMonitor* monitor);
    ~AdaptiveScheduler();
    
    // Task scheduling
    using Task = std::function<void()>;
    using TaskId = uint64_t;
    
    TaskId scheduleTask(Task task, std::chrono::milliseconds interval, 
                       int priority = 0);
    TaskId scheduleDelayedTask(Task task, std::chrono::milliseconds delay,
                              int priority = 0);
    
    bool cancelTask(TaskId taskId);
    void pauseTask(TaskId taskId);
    void resumeTask(TaskId taskId);
    
    // Adaptive behavior
    void setAdaptiveMode(bool enabled);
    void setBatteryAwareScheduling(bool enabled);
    void setCPUAwareScheduling(bool enabled);
    
    // Statistics
    size_t getActiveTaskCount() const;
    size_t getCompletedTaskCount() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

/**
 * Resource usage limiter
 */
class ResourceLimiter {
public:
    ResourceLimiter();
    ~ResourceLimiter();
    
    // Memory limiting
    void setMemoryLimit(size_t limitKB);
    bool checkMemoryLimit() const;
    void enforceMemoryLimit();
    
    // CPU limiting
    void setCPULimit(double maxPercent);
    bool checkCPULimit() const;
    void enforceCPULimit();
    
    // I/O limiting
    void setIOLimit(size_t maxBytesPerSecond);
    bool checkIOLimit(size_t bytesUsed) const;
    void enforceIOLimit();
    
    // Network limiting
    void setNetworkLimit(size_t maxBytesPerSecond);
    bool checkNetworkLimit(size_t bytesUsed) const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault