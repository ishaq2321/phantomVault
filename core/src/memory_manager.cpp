/**
 * PhantomVault Memory Manager Implementation
 * 
 * Efficient memory management with pools and smart allocation strategies.
 */

#include "memory_manager.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>

namespace phantomvault {

// MemoryPool implementation
template<size_t BlockSize, size_t PoolSize>
MemoryPool<BlockSize, PoolSize>::MemoryPool() : used_blocks_(0) {
    initializeFreeList();
}

template<size_t BlockSize, size_t PoolSize>
MemoryPool<BlockSize, PoolSize>::~MemoryPool() {
    // Memory is automatically freed when pool_ goes out of scope
}

template<size_t BlockSize, size_t PoolSize>
void MemoryPool<BlockSize, PoolSize>::initializeFreeList() {
    free_blocks_.reserve(PoolSize);
    for (size_t i = 0; i < PoolSize; ++i) {
        free_blocks_.push_back(pool_ + i * BlockSize);
    }
}

template<size_t BlockSize, size_t PoolSize>
void* MemoryPool<BlockSize, PoolSize>::allocate() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (free_blocks_.empty()) {
        return nullptr; // Pool exhausted
    }
    
    void* ptr = free_blocks_.back();
    free_blocks_.pop_back();
    ++used_blocks_;
    
    return ptr;
}

template<size_t BlockSize, size_t PoolSize>
void MemoryPool<BlockSize, PoolSize>::deallocate(void* ptr) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify pointer is within our pool
    if (ptr >= pool_ && ptr < pool_ + PoolSize * BlockSize) {
        free_blocks_.push_back(ptr);
        --used_blocks_;
    }
}

// Explicit template instantiations
template class MemoryPool<32, 1024>;
template class MemoryPool<128, 512>;
template class MemoryPool<512, 256>;
template class MemoryPool<2048, 128>;

// MemoryManager implementation
MemoryManager::MemoryManager() 
    : small_pool_(std::make_unique<MemoryPool<32>>())
    , medium_pool_(std::make_unique<MemoryPool<128>>())
    , large_pool_(std::make_unique<MemoryPool<512>>())
    , xlarge_pool_(std::make_unique<MemoryPool<2048>>())
    , memory_limit_(10 * 1024 * 1024) // 10MB default limit
{
    resetStats();
}

MemoryManager::~MemoryManager() = default;

MemoryManager& MemoryManager::getInstance() {
    static MemoryManager instance;
    return instance;
}

void* MemoryManager::allocate(size_t size) {
    if (size == 0) return nullptr;
    
    // Check memory limit
    if (isMemoryLimitExceeded()) {
        return nullptr;
    }
    
    void* ptr = nullptr;
    
    // Try pool allocation first
    ptr = allocateFromPool(size);
    
    // Fall back to system allocation if pools are exhausted
    if (!ptr) {
        ptr = allocateFromSystem(size);
    }
    
    if (ptr) {
        updateStats(size, true);
        
        // Track allocation size for proper deallocation
        std::lock_guard<std::mutex> lock(allocation_mutex_);
        allocation_sizes_[ptr] = size;
    }
    
    return ptr;
}

void MemoryManager::deallocate(void* ptr, size_t size) {
    if (!ptr) return;
    
    // Remove from tracking
    {
        std::lock_guard<std::mutex> lock(allocation_mutex_);
        auto it = allocation_sizes_.find(ptr);
        if (it != allocation_sizes_.end()) {
            size = it->second; // Use tracked size
            allocation_sizes_.erase(it);
        }
    }
    
    updateStats(size, false);
    
    // Try pool deallocation first
    bool deallocated = false;
    
    if (size <= 32) {
        small_pool_->deallocate(ptr);
        deallocated = true;
    } else if (size <= 128) {
        medium_pool_->deallocate(ptr);
        deallocated = true;
    } else if (size <= 512) {
        large_pool_->deallocate(ptr);
        deallocated = true;
    } else if (size <= 2048) {
        xlarge_pool_->deallocate(ptr);
        deallocated = true;
    }
    
    // Fall back to system deallocation
    if (!deallocated) {
        std::free(ptr);
    }
}

void* MemoryManager::allocateFromPool(size_t size) {
    if (size <= 32) {
        return small_pool_->allocate();
    } else if (size <= 128) {
        return medium_pool_->allocate();
    } else if (size <= 512) {
        return large_pool_->allocate();
    } else if (size <= 2048) {
        return xlarge_pool_->allocate();
    }
    
    return nullptr; // Size too large for pools
}

void* MemoryManager::allocateFromSystem(size_t size) {
    return std::malloc(size);
}

void MemoryManager::updateStats(size_t size, bool allocating) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (allocating) {
        stats_.totalAllocated += size;
        stats_.currentUsage += size;
        stats_.peakUsage = std::max(stats_.peakUsage, stats_.currentUsage);
    } else {
        stats_.totalDeallocated += size;
        stats_.currentUsage = (stats_.currentUsage >= size) ? 
                             stats_.currentUsage - size : 0;
    }
    
    // Update pool usage
    stats_.poolUsage = small_pool_->getUsedBlocks() * 32 +
                      medium_pool_->getUsedBlocks() * 128 +
                      large_pool_->getUsedBlocks() * 512 +
                      xlarge_pool_->getUsedBlocks() * 2048;
    
    stats_.systemUsage = stats_.currentUsage - stats_.poolUsage;
}

MemoryManager::MemoryStats MemoryManager::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void MemoryManager::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = MemoryStats{};
}

void MemoryManager::compactPools() {
    // Pools are pre-allocated, so compaction is not needed
    // This could be extended to support dynamic pool resizing
}

void MemoryManager::setMemoryLimit(size_t limitBytes) {
    memory_limit_ = limitBytes;
}

bool MemoryManager::isMemoryLimitExceeded() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_.currentUsage >= memory_limit_;
}

} // namespace phantomvault