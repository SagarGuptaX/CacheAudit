#pragma once
#include "../engine/cache.h"
#include <list>
#include <unordered_map>

// Least Recently Used eviction.
//
// Data structures:
//   list<int>                              — doubly linked, front = MRU, back = LRU
//   unordered_map<int, list::iterator>     — O(1) lookup to any node
//
// On hit:  splice node to front (O(1) with iterator)
// On miss: evict back, insert new node at front
//
// std::list::splice gives O(1) move-to-front without pointer surgery.
// The iterator in the map stays valid after splice — that is the key property.
class LRU : public Cache {
private:
    std::list<int> order; // front = MRU, back = LRU
    std::unordered_map<int, std::list<int>::iterator> map;

public:
    explicit LRU(std::size_t cap) : Cache(cap) {}

    bool access(int id) override {
        auto it = map.find(id);
        if (it != map.end()) {
            // Move to MRU position
            order.splice(order.begin(), order, it->second);
            hits++;
            return true;
        }

        misses++;

        if (map.size() >= capacity) {
            int lru = order.back();
            order.pop_back();
            map.erase(lru);
        }

        order.push_front(id);
        map[id] = order.begin();
        return false;
    }
};
