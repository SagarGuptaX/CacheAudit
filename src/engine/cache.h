#pragma once
#include <cstdint>
#include <cstddef>

// Abstract base class for all cache eviction policies.
//
// Design rule: this class knows nothing about strings or traces.
// It operates on integer IDs only. String handling belongs upstream
// in the trace loader and key mapper.
class Cache {
protected:
    std::size_t capacity;
    uint64_t hits   = 0;
    uint64_t misses = 0;

public:
    explicit Cache(std::size_t cap) : capacity(cap) {}
    virtual ~Cache() = default;

    // Core interface. Returns true on hit, false on miss.
    virtual bool access(int id) = 0;

    // --- Metrics accessors ---
    double   get_hit_rate()  const {
        uint64_t total = hits + misses;
        return total == 0 ? 0.0 : static_cast<double>(hits) / total;
    }
    uint64_t get_hits()      const { return hits; }
    uint64_t get_misses()    const { return misses; }
    std::size_t get_capacity() const { return capacity; }
};
