#pragma once
#include <string>
#include <map>
#include <cstdint>

// Abstract Base Class
class Cache {
protected:
    std::size_t capacity;
    std::size_t size;
    // Stats counters
    uint64_t hits = 0;
    uint64_t misses = 0;

public:
    explicit Cache(std::size_t cap) : capacity(cap), size(0) {}
    
    // Virtual Destructor is CRITICAL for inheritance
    virtual ~Cache() = default;

    // The Core API: Returns true if hit, false if miss
    virtual bool access(std::string key) = 0;

    // Getters for analysis
    double get_hit_rate() const {
        uint64_t total = hits + misses;
        return total == 0 ? 0.0 : (double)hits / total * 100.0;
    }
    
    uint64_t get_hits() const { return hits; }
    uint64_t get_misses() const { return misses; }
};
