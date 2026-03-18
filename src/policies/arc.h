#pragma once
#include "../engine/cache.h"
#include <list>
#include <unordered_map>
#include <string>
#include <algorithm>

class ARC : public Cache {
private:
    std::size_t p; // Target size for T1

    std::list<std::string> t1; // Recent
    std::list<std::string> t2; // Frequent
    std::list<std::string> b1; // Ghost Recent
    std::list<std::string> b2; // Ghost Frequent

    enum ListType { T1, T2, B1, B2 };
    
    struct Entry {
        std::list<std::string>::iterator iter;
        ListType type;
    };
    
    std::unordered_map<std::string, Entry> map;

    // The core eviction logic from the IBM Paper.
    // This moves items from Real lists (T) to Ghost lists (B) to make space.
    void replace(bool hit_in_b2) {
        // Condition: If T1 is not empty AND 
        // (T1 exceeds target size 'p' OR (hit was in B2 and T1 is exactly 'p'))
        if (!t1.empty() && (t1.size() > p || (hit_in_b2 && t1.size() == p))) {
            // Evict LRU from T1 to MRU of B1
            std::string lru = t1.back();
            t1.pop_back();
            b1.push_front(lru);
            map[lru] = {b1.begin(), B1};
        } else {
            // Evict LRU from T2 to MRU of B2
            std::string lru = t2.back();
            t2.pop_back();
            b2.push_front(lru);
            map[lru] = {b2.begin(), B2};
        }
    }

public:
    explicit ARC(std::size_t cap) : Cache(cap), p(0) {}

    bool access(std::string key) override {
        // CASE 1: Real Hit (Item is in T1 or T2)
        if (map.count(key) && (map[key].type == T1 || map[key].type == T2)) {
            hits++;
            Entry e = map[key];
            
            // Remove from current list
            if (e.type == T1) t1.erase(e.iter);
            else t2.erase(e.iter);

            // Move to MRU of T2 (It is now "Frequent")
            t2.push_front(key);
            map[key] = {t2.begin(), T2};
            return true;
        }

        misses++;

        // CASE 2: Ghost Hit in B1 (Recency was important!)
        if (map.count(key) && map[key].type == B1) {
            // Adapt: Increase target size of T1
            std::size_t delta = (b1.size() >= b2.size()) ? 1 : b2.size() / b1.size();
            p = std::min(capacity, p + delta);

            // Make space, then move item to T2
            replace(false);
            b1.erase(map[key].iter);
            t2.push_front(key);
            map[key] = {t2.begin(), T2};
            return false;
        }

        // CASE 3: Ghost Hit in B2 (Frequency was important!)
        if (map.count(key) && map[key].type == B2) {
            // Adapt: Decrease target size of T1 (Favoring T2)
            std::size_t delta = (b2.size() >= b1.size()) ? 1 : b1.size() / b2.size();
            p = (p > delta) ? p - delta : 0; // Prevent underflow

            // Make space, then move item to T2
            replace(true);
            b2.erase(map[key].iter);
            t2.push_front(key);
            map[key] = {t2.begin(), T2};
            return false;
        }

        // CASE 4: Complete Miss (Not in any list)
        
        // Sub-case A: T1 + B1 is full
        if (t1.size() + b1.size() == capacity) {
            if (t1.size() < capacity) {
                // Discard LRU of B1
                map.erase(b1.back());
                b1.pop_back();
                replace(false);
            } else {
                // T1 itself is full. Discard LRU of T1.
                map.erase(t1.back());
                t1.pop_back();
            }
        } 
        // Sub-case B: T1 + B1 is not full, but total cache is full
        else if (t1.size() + b1.size() < capacity && 
                 t1.size() + t2.size() + b1.size() + b2.size() >= capacity) {
            
            if (t1.size() + t2.size() + b1.size() + b2.size() == 2 * capacity) {
                // Discard LRU of B2
                map.erase(b2.back());
                b2.pop_back();
            }
            replace(false);
        }

        // Finally, add the completely new item to MRU of T1
        t1.push_front(key);
        map[key] = {t1.begin(), T1};

        return false;
    }
};