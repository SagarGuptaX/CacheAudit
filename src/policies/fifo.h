#pragma once
#include "../engine/cache.h"
#include <deque>
#include <unordered_set>
#include <iostream>

class FIFO : public Cache {
private:
    std::deque<std::string> queue;        // Tracks order
    std::unordered_set<std::string> set;  // Fast lookups O(1)

public:
    explicit FIFO(std::size_t cap) : Cache(cap) {}

    bool access(std::string key) override {
        // 1. Check if in cache (Hit)
        if (set.find(key) != set.end()) {
            hits++;
            return true;
        }

        // 2. Cache Miss
        misses++;

        // 3. Eviction logic (if full)
        if (queue.size() >= capacity) {
            std::string victim = queue.front();
            queue.pop_front();
            set.erase(victim);
        }

        // 4. Insert new item
        queue.push_back(key);
        set.insert(key);
        return false;
    }
};
