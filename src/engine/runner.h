#pragma once
#include "cache.h"
#include "request.h"
#include "metrics.h"
#include <vector>
#include <string>
#include <chrono>

// Drives the simulation loop.
// Feeds the trace into the cache policy, measures wall-clock time,
// and returns a populated Metrics struct.
//
// Runner does not know which policy it is running.
// That is intentional: policy selection happens in main.
class Runner {
public:
    static Metrics run(Cache* cache,
                       const std::vector<Request>& trace,
                       const std::string& policy_name,
                       const std::string& trace_name) {

        auto start = std::chrono::high_resolution_clock::now();

        for (const auto& req : trace) {
            cache->access(req.id);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        Metrics m;
        m.policy      = policy_name;
        m.trace       = trace_name;
        m.cache_size  = cache->get_capacity();
        m.hit_rate    = cache->get_hit_rate();
        m.hits        = cache->get_hits();
        m.misses      = cache->get_misses();
        m.runtime_ms  = elapsed;

        return m;
    }
};
