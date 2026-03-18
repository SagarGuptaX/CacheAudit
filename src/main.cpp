#include "engine/allocator.h" // Add this include

// ... inside main(), temporarily add this block:
struct DummyNode {
    int value;
    DummyNode* next;
};

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <chrono> // For timing
#include "policies/fifo.h"
#include "policies/lru.h"
#include "policies/lfu.h"
#include "policies/arc.h"

// Parse file and run simulation
void run_simulation(Cache* cache, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error opening " << filepath << std::endl;
        return;
    }

    std::string line;
    std::string operation;
    std::string key;

    // Start Timer
    auto start_time = std::chrono::high_resolution_clock::now();

    while (file >> operation >> key) {
        // Format is: R <key> or W <key>
        // For now, we treat Read/Write the same for eviction
        cache->access(key);
    }

    // Stop Timer
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Report
    std::cout << "--- Results ---" << std::endl;
    std::cout << "Hit Rate: " << cache->get_hit_rate() << "%" << std::endl;
    std::cout << "Hits: " << cache->get_hits() << ", Misses: " << cache->get_misses() << std::endl;
    std::cout << "Time: " << duration << " ms" << std::endl;
}

int main(int argc, char* argv[]) {
// slab alloctaor temporary test
std::cout << "Testing Allocator...\n";
SlabAllocator<DummyNode> my_allocator(10); // Pool of 10 nodes

DummyNode* n1 = my_allocator.allocate();
if (n1) n1->value = 42;

DummyNode* n2 = my_allocator.allocate();
if (n2) n2->value = 99;

my_allocator.deallocate(n1); // Give it back
// ... end of temporary test block
    // We now expect 3 arguments: Program Name, Trace File, Algorithm
    if (argc < 4) {
        std::cerr << "Usage: ./cache_audit <trace_file> <algorithm> <cache_size>" << std::endl;
        return 1;
    }

    std::string trace_file = argv[1];
    std::string algo_name = argv[2];
    std::size_t cache_size = std::stoi(argv[3]); // Parse size from arg

    std::unique_ptr<Cache> cache;
    // The "Factory" Logic
    if (algo_name == "fifo") {
        cache = std::make_unique<FIFO>(cache_size);
    } 
    else if (algo_name == "lru") {
        cache = std::make_unique<LRU>(cache_size);
    } 
    else if (algo_name == "lfu") {
        cache = std::make_unique<LFU>(cache_size);
    }
    else if (algo_name == "arc") {
        cache = std::make_unique<ARC>(cache_size);
    }
    else {
        std::cerr << "Unknown algorithm: " << algo_name << std::endl;
        return 1;
    }

    std::cout << "Running " << algo_name << " Simulation (Cap: " << cache_size << ")..." << std::endl;
    run_simulation(cache.get(), trace_file);

    return 0;
}
