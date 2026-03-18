#pragma once
#include "../engine/cache.h"
#include <set>
#include <unordered_map>
#include <string>
#include <iostream>

class LFU : public Cache {
private:
    // We need a global counter to act as a "Timestamp" for tie-breaking
    uint64_t timer;

    struct Node {
        std::string key;
        mutable int frequency; // Mutable so we can modify it inside a set iterator (carefully!)
        uint64_t last_access_time;

        // Constructor
        Node(std::string k, uint64_t t) : key(k), frequency(1), last_access_time(t) {}

        // The Comparator: This defines how the std::set sorts items.
        // We want the "Smallest" item (victim) to be at the beginning.
        // Rule: Sort by Frequency ASC, then by Time ASC (LRU).
        bool operator<(const Node& other) const {
            if (frequency != other.frequency) {
                return frequency < other.frequency;
            }
            return last_access_time < other.last_access_time;
        }
    };

    // The O(log n) sorted structure. 
    // begin() will always be the victim (Lowest Freq, Oldest Time).
    std::set<Node> frequency_tree;

    // The O(1) lookup to find nodes quickly
    std::unordered_map<std::string, std::set<Node>::iterator> lookup;

public:
    explicit LFU(std::size_t cap) : Cache(cap), timer(0) {}

    bool access(std::string key) override {
        timer++; // Increment global time

        // 1. Check for Hit
        if (lookup.find(key) != lookup.end()) {
            // HIT LOGIC
            hits++;
            
            // We need to update the node's frequency and time.
            // Problem: You can't modify an element inside a std::set directly 
            // because it breaks the sort order.
            // Solution: Remove -> Update -> Re-insert.
            
            auto it = lookup[key];
            Node updated_node = *it; // Copy the data
            
            // Update stats
            updated_node.frequency++;
            updated_node.last_access_time = timer;

            // Remove old
            frequency_tree.erase(it);
            
            // Re-insert updated node and update lookup map
            auto result = frequency_tree.insert(updated_node);
            lookup[key] = result.first;
            
            return true;
        }

        // 2. Handle Miss
        misses++;

        // If full, Evict!
        if (lookup.size() >= capacity) {
            // The "begin()" of the set is guaranteed to be the victim
            // because of our custom operator<
            auto victim_it = frequency_tree.begin();
            
            lookup.erase(victim_it->key);
            frequency_tree.erase(victim_it);
        }

        // 3. Insert New Item
        Node new_node(key, timer);
        auto result = frequency_tree.insert(new_node);
        lookup[key] = result.first;

        return false;
    }
};
