#pragma once
#include <string>
#include <cstdint>
#include <cstddef>

// All output data produced by one simulation run.
// Passed from Runner to main for printing and CSV export.
struct Metrics {
    std::string  policy;
    std::string  trace;        // Trace filename (for CSV labeling)
    std::size_t  cache_size;
    double       hit_rate;     // [0.0, 1.0]
    uint64_t     hits;
    uint64_t     misses;
    long long    runtime_ms;
};
