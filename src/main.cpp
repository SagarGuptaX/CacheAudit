#include <iostream>
#include <fstream>
#include <memory>
#include <chrono> // For timing
#include "policies/fifo.h"

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
    if (argc < 2) {
        std::cerr << "Usage: ./cache_audit <trace_file>" << std::endl;
        return 1;
    }

    // Create a FIFO cache with size 2 (Tiny, for testing)
    // We use unique_ptr for automatic memory management (Modern C++)
    std::unique_ptr<Cache> cache = std::make_unique<FIFO>(2);

    std::cout << "Running FIFO Simulation..." << std::endl;
    run_simulation(cache.get(), argv[1]);

    return 0;
}
