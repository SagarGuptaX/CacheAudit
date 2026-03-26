#pragma once
#include "../engine/cache.h"
#include <deque>
#include <unordered_set>

// First-In First-Out eviction.
//
// Data structures:
//   deque<int>          — tracks insertion order (front = oldest)
//   unordered_set<int>  — O(1) presence check
//
// On hit:  nothing changes (FIFO does not reorder on access)
// On miss: evict front of deque, insert new item at back
//
// Weakness: thrashes on loops larger than cache size.
// A recently inserted item is protected; a hot item is not.
class FIFO : public Cache {
private:
    std::deque<int>        queue;
    std::unordered_set<int> set;

public:
    explicit FIFO(std::size_t cap) : Cache(cap) {}

    bool access(int id) override {
        if (set.count(id)) {
            hits++;
            return true;
        }

        misses++;

        if (queue.size() >= capacity) {
            set.erase(queue.front());
            queue.pop_front();
        }

        queue.push_back(id);
        set.insert(id);
        return false;
    }
};
