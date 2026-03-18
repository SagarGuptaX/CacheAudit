#pragma once
#include <vector>
#include <cstdint>
#include <iostream>

template <typename T>
class SlabAllocator {
private:
    std::vector<T> pool;
    std::vector<std::size_t> free_indices; // Stack of available slots

public:
    // Pre-allocate everything at startup
    explicit SlabAllocator(std::size_t max_nodes) {
        pool.resize(max_nodes);
        
        // Push all indices into the free stack (backwards, so 0 is popped first)
        for (std::size_t i = max_nodes; i > 0; --i) {
            free_indices.push_back(i - 1);
        }
        std::cout << "[SlabAllocator] Reserved " << max_nodes << " nodes.\n";
    }

    // O(1) Allocation without hitting the OS
    T* allocate() {
        if (free_indices.empty()) {
            return nullptr; // Strict constraint: No dynamic expansion!
        }
        std::size_t idx = free_indices.back();
        free_indices.pop_back();
        return &pool[idx];
    }

    // O(1) Deallocation (just put the index back)
    void deallocate(T* ptr) {
        if (ptr == nullptr) return;
        
        // Clever pointer math to find the index
        std::size_t idx = ptr - pool.data();
        free_indices.push_back(idx);
    }
};
