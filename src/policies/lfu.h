#pragma once
#include "../engine/cache.h"
#include <set>
#include <unordered_map>
#include <cstdint>

// Least Frequently Used eviction, with LRU tie-breaking.
//
// Data structures:
//   set<Node>                             — sorted by (freq ASC, last_access ASC)
//                                           begin() is always the eviction victim
//   unordered_map<int, set::iterator>     — O(1) lookup to any node
//
// Update challenge: std::set elements are immutable once inserted.
// Solution: remove → update → reinsert (O(log n) but correct).
//
// Tie-breaking: two items with equal frequency → evict the one
// accessed least recently. This makes LFU behave like LRU within
// the same frequency bucket, which is standard practice.
class LFU : public Cache {
private:
    uint64_t timer = 0;

    struct Node {
        int      key;
        int      freq;
        uint64_t last_access;

        bool operator<(const Node& other) const {
            if (freq != other.freq) return freq < other.freq;
            return last_access < other.last_access;
        }
    };

    std::set<Node> freq_set;
    std::unordered_map<int, std::set<Node>::iterator> lookup;

public:
    explicit LFU(std::size_t cap) : Cache(cap) {}

    bool access(int id) override {
        timer++;

        auto it = lookup.find(id);
        if (it != lookup.end()) {
            hits++;
            // Remove → update → reinsert
            Node updated  = *it->second;
            freq_set.erase(it->second);
            updated.freq++;
            updated.last_access = timer;
            auto result   = freq_set.insert(updated);
            lookup[id]    = result.first;
            return true;
        }

        misses++;

        if (lookup.size() >= capacity) {
            auto victim = freq_set.begin(); // lowest freq, oldest
            lookup.erase(victim->key);
            freq_set.erase(victim);
        }

        Node n{id, 1, timer};
        auto result = freq_set.insert(n);
        lookup[id]  = result.first;
        return false;
    }
};
