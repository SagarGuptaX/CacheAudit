#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <iomanip>
#include <stdexcept>

#include "engine/request.h"
#include "engine/key_mapper.h"
#include "engine/trace_loader.h"
#include "engine/cache.h"
#include "engine/metrics.h"
#include "engine/runner.h"
#include "policies/fifo.h"
#include "policies/lru.h"
#include "policies/lfu.h"
#include "policies/arc.h"
#include "policies/belady.h"

// ---------------------------------------------------------------------------
// CSV output
// Appends one row to the output file.
// Writes the header if the file does not already exist.
// ---------------------------------------------------------------------------
static void append_csv(const Metrics& m, const std::string& filepath) {
    // Need a header if the file doesn't exist OR exists but is empty.
    // check.good() returns true on an empty file, so we must also check size.
    std::ifstream check(filepath, std::ios::ate);
    bool need_header = !check.good() || check.tellg() == 0;
    check.close();

    std::ofstream file(filepath, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open output file: " + filepath);
    }

    if (need_header) {
        file << "policy,trace,cache_size,hit_rate,hits,misses,runtime_ms\n";
    }

    file << std::fixed << std::setprecision(6)
         << m.policy      << ","
         << m.trace       << ","
         << m.cache_size  << ","
         << m.hit_rate    << ","
         << m.hits        << ","
         << m.misses      << ","
         << m.runtime_ms  << "\n";
}

// ---------------------------------------------------------------------------
// Usage
// ---------------------------------------------------------------------------
static void print_usage() {
    std::cerr << "\nUsage:\n";
    std::cerr << "  ./cache_audit <trace_file> <policy> <cache_size> [--out <file.csv>]\n\n";
    std::cerr << "Policies:  fifo  lru  lfu  arc  belady\n";
    std::cerr << "Example:\n";
    std::cerr << "  ./cache_audit traces/synthetic/loop.txt lru 40 --out results/out.csv\n\n";
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 4) {
        print_usage();
        return 1;
    }

    std::string trace_file = argv[1];
    std::string algo_name  = argv[2];
    std::size_t cache_size = 0;

    try {
        cache_size = std::stoul(argv[3]);
    } catch (...) {
        std::cerr << "Error: cache_size must be a positive integer.\n";
        return 1;
    }

    if (cache_size == 0) {
        std::cerr << "Error: cache_size must be greater than 0.\n";
        return 1;
    }

    // Parse optional --out argument
    std::string out_file;
    for (int i = 4; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--out") {
            out_file = argv[i + 1];
        }
    }

    // --- Load trace ---
    KeyMapper mapper;
    std::vector<Request> trace;
    try {
        trace = TraceLoader::load(trace_file, mapper);
    } catch (const std::exception& e) {
        std::cerr << "Error loading trace: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Trace:       " << trace_file << "\n";
    std::cout << "Requests:    " << trace.size() << "\n";
    std::cout << "Unique keys: " << mapper.unique_count() << "\n";
    std::cout << "Policy:      " << algo_name << "\n";
    std::cout << "Cache size:  " << cache_size << "\n\n";

    // --- Build policy ---
    // Belady is constructed differently: it needs the full trace upfront.
    // All other policies only need capacity.
    std::unique_ptr<Cache> cache;

    if      (algo_name == "fifo")   cache = std::make_unique<FIFO>(cache_size);
    else if (algo_name == "lru")    cache = std::make_unique<LRU>(cache_size);
    else if (algo_name == "lfu")    cache = std::make_unique<LFU>(cache_size);
    else if (algo_name == "arc")    cache = std::make_unique<ARC>(cache_size);
    else if (algo_name == "belady") cache = std::make_unique<Belady>(cache_size, trace);
    else {
        std::cerr << "Unknown policy: " << algo_name << "\n";
        print_usage();
        return 1;
    }

    // Extract trace name (basename without extension) for CSV labeling
    std::string trace_name = trace_file;
    auto slash = trace_name.rfind('/');
    if (slash != std::string::npos) trace_name = trace_name.substr(slash + 1);
    auto dot = trace_name.rfind('.');
    if (dot != std::string::npos) trace_name = trace_name.substr(0, dot);

    // --- Run ---
    Metrics m = Runner::run(cache.get(), trace, algo_name, trace_name);

    // --- Print summary ---
    std::cout << "--- Results ---\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Hit Rate:   " << m.hit_rate * 100.0 << "%\n";
    std::cout << "Hits:       " << m.hits    << "\n";
    std::cout << "Misses:     " << m.misses  << "\n";
    std::cout << "Runtime:    " << m.runtime_ms << " ms\n";

    // --- CSV output ---
    if (!out_file.empty()) {
        try {
            append_csv(m, out_file);
            std::cout << "\nAppended to " << out_file << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Warning: " << e.what() << "\n";
        }
    }

    return 0;
}
