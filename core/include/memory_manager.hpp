/**
 * PhantomVault Memory Manager
 * 
 * Efficient memory management with pools and smart allocation strategies.
 * Designed to minimize memory fragmentation and optimize performance.
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cstddef>

namespace phantomvault {

/**
 * Memory pool for fixed-size allocations
 */
template<size_t BlockSize, size_t PoolSize = 1024>
class MemoryPool {
public:
    MemoryPool();
    ~MemoryPool();

    void* allocate();
    void deallocate(void* ptr);
    
    size_t getUsedBlocks() const { return used_blocks_; }
    size_t getTotalBlocks() const { return PoolSize; }
    size_t getMemoryUsage() const { return PoolSize * BlockSize; }

private:
    alignas(std::max_align_t) char pool_[PoolSize * BlockSize];
    std::vector<void*> free_blocks_;
    std::atomic<size_t> used_blocks_;
    std::mutex mutex_;
    
    void initializeFreeList();
};

/**
 * Smart memory manager with multiple pools
 */
class MemoryManager {
public:
    static MemoryManager& getInstance();
    
    // Memory allocation with size-based pool selection
    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);
    
    // Memory statistics
    struct MemoryStats {
        size_t totalAllocated = 0;
        size_t totalDeallocated = 0;
        size_t currentUsage = 0;
        size_t peakUsage = 0;
        size_t poolUsage = 0;
        size_t systemUsage = 0;
    };
    
    MemoryStats getStats() const;
    void resetStats();
    
    // Memory optimization
    void compactPools();
    void setMemoryLimit(size_t limitBytes);
    bool isMemoryLimitExceeded() const;

private:
    MemoryManager();
    ~MemoryManager();
    
    // Different sized pools for common allocations
    std::unique_ptr<MemoryPool<32>> small_pool_;      // 32 bytes
    std::unique_ptr<MemoryPool<128>> medium_pool_;    // 128 bytes
    std::unique_ptr<MemoryPool<512>> large_pool_;     // 512 bytes
    std::unique_ptr<MemoryPool<2048>> xlarge_pool_;   // 2KB
    
    mutable std::mutex stats_mutex_;
    MemoryStats stats_;
    std::atomic<size_t> memory_limit_;
    
    // Track allocations for proper deallocation
    std::unordered_map<void*, size_t> allocation_sizes_;
    std::mutex allocation_mutex_;
    
    void* allocateFromPool(size_t size);
    void* allocateFromSystem(size_t size);
    void updateStats(size_t size, bool allocating);
};

/**
 * RAII memory guard for automatic cleanup
 */
class MemoryGuard {
public:
    MemoryGuard(void* ptr, size_t size) : ptr_(ptr), size_(size) {}
    ~MemoryGuard() {
        if (ptr_) {
            MemoryManager::getInstance().deallocate(ptr_, size_);
        }
    }
    
    void* get() const { return ptr_; }
    void release() { ptr_ = nullptr; }

private:
    void* ptr_;
    size_t size_;
};

/**
 * Custom allocator for STL containers
 */
template<typename T>
class PoolAllocator {
public:
    using value_type = T;
    
    PoolAllocator() = default;
    template<typename U> PoolAllocator(const PoolAllocator<U>&) {}
    
    T* allocate(size_t n) {
        return static_cast<T*>(MemoryManager::getInstance().allocate(n * sizeof(T)));
    }
    
    void deallocate(T* ptr, size_t n) {
        MemoryManager::getInstance().deallocate(ptr, n * sizeof(T));
    }
    
    template<typename U>
    bool operator==(const PoolAllocator<U>&) const { return true; }
    
    template<typename U>
    bool operator!=(const PoolAllocator<U>&) const { return false; }
};

// Optimized containers using pool allocator
template<typename T>
using PoolVector = std::vector<T, PoolAllocator<T>>;

template<typename K, typename V>
using PoolMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, 
                                   PoolAllocator<std::pair<const K, V>>>;

} // namespace phantomvault