#pragma once
#include "../engine/cache.h"
#include "../engine/request.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <limits>

// Belady's Optimal Algorithm — Belady, 1966.
//
// Evicts the item whose next use is furthest in the future.
// If an item will never be used again, it is the ideal eviction target.
//
// This is the theoretical upper bound for any online policy.
// No online policy can beat Belady on the same trace.
// Its purpose here is to show the gap between real policies and optimal.
//
// Why offline only:
//   Belady requires knowing the full future of the trace before simulation
//   begins. No online policy has that information.
//
// Implementation:
//   Precompute: for each id, a sorted list of all access positions.
//   At eviction time: for each cached item, find its next access after
//   the current position using binary search (O(log n)).
//   Evict the item with the largest next-access index (or INF if never).
class Belady : public Cache {
private:
    std::unordered_set<int> cache_set;

    // access_times[id] = sorted list of positions where id appears in trace
    std::unordered_map<int, std::vector<int>> access_times;

    int current_pos = 0;

    // Returns the next access time of id after current_pos.
    // Returns INT_MAX if id is never accessed again (ideal eviction target).
    int next_access(int id) const {
        auto it = access_times.find(id);
        if (it == access_times.end()) return std::numeric_limits<int>::max();

        const auto& times = it->second;
        // First position strictly after current_pos
        auto pos = std::upper_bound(times.begin(), times.end(), current_pos);
        if (pos == times.end()) return std::numeric_limits<int>::max();
        return *pos;
    }

public:
    // Belady requires the full trace at construction time for preprocessing.
    Belady(std::size_t cap, const std::vector<Request>& trace) : Cache(cap) {
        for (int i = 0; i < static_cast<int>(trace.size()); i++) {
            access_times[trace[i].id].push_back(i);
            // Vectors are built in order, so they are already sorted.
        }
    }

    bool access(int id) override {
        if (cache_set.count(id)) {
            hits++;
            current_pos++;
            return true;
        }

        misses++;

        if (cache_set.size() >= capacity) {
            // Find cached item with the furthest next use
            int victim   = -1;
            int furthest = -1;

            for (int item : cache_set) {
                int nxt = next_access(item);
                if (nxt > furthest) {
                    furthest = nxt;
                    victim   = item;
                }
            }

            cache_set.erase(victim);
        }

        cache_set.insert(id);
        current_pos++;
        return false;
    }
};
