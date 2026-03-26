#pragma once
#include "../engine/cache.h"
#include <list>
#include <unordered_map>
#include <algorithm>

// Adaptive Replacement Cache — Megiddo & Modha, FAST 2003.
//
// Maintains four lists:
//   T1 — recently accessed once   (real cache)
//   T2 — accessed more than once  (real cache)
//   B1 — ghost of recently evicted from T1
//   B2 — ghost of recently evicted from T2
//
// Parameter p: target size for T1. Adapts at runtime.
//   Ghost hit in B1 → recency was useful → increase p (grow T1)
//   Ghost hit in B2 → frequency was useful → decrease p (grow T2)
//
// This self-tuning is ARC's core advantage over LRU and LFU.
// It learns the working set characteristics from the trace itself.
class ARC : public Cache {
private:
    std::size_t p = 0;

    std::list<int> t1, t2, b1, b2;

    enum class List { T1, T2, B1, B2 };

    struct Entry {
        std::list<int>::iterator iter;
        List list;
    };

    std::unordered_map<int, Entry> map;

    // Evict one item from T1 or T2 into the corresponding ghost list.
    // hit_in_b2: true if the current miss was a ghost hit in B2.
    void replace(bool hit_in_b2) {
        bool prefer_t1 = !t1.empty() &&
                         (t1.size() > p || (hit_in_b2 && t1.size() == p));

        if (prefer_t1) {
            int lru = t1.back();
            t1.pop_back();
            b1.push_front(lru);
            map[lru] = {b1.begin(), List::B1};
        } else if (!t2.empty()) {
            int lru = t2.back();
            t2.pop_back();
            b2.push_front(lru);
            map[lru] = {b2.begin(), List::B2};
        }
    }

public:
    explicit ARC(std::size_t cap) : Cache(cap) {}

    bool access(int id) override {
        auto it = map.find(id);

        // Case 1: Real hit (T1 or T2)
        if (it != map.end() && (it->second.list == List::T1 || it->second.list == List::T2)) {
            hits++;
            if (it->second.list == List::T1) t1.erase(it->second.iter);
            else                              t2.erase(it->second.iter);
            t2.push_front(id);
            map[id] = {t2.begin(), List::T2};
            return true;
        }

        misses++;

        // Case 2: Ghost hit in B1 → recency matters, grow T1
        if (it != map.end() && it->second.list == List::B1) {
            std::size_t delta = (b1.size() >= b2.size()) ? 1 : b2.size() / b1.size();
            p = std::min(capacity, p + delta);
            replace(false);
            b1.erase(it->second.iter);
            t2.push_front(id);
            map[id] = {t2.begin(), List::T2};
            return false;
        }

        // Case 3: Ghost hit in B2 → frequency matters, grow T2
        if (it != map.end() && it->second.list == List::B2) {
            std::size_t delta = (b2.size() >= b1.size()) ? 1 : b1.size() / b2.size();
            p = (p > delta) ? p - delta : 0;
            replace(true);
            b2.erase(it->second.iter);
            t2.push_front(id);
            map[id] = {t2.begin(), List::T2};
            return false;
        }

        // Case 4: Complete miss — not in any list

        // Sub-case A: T1 + B1 is at capacity
        if (t1.size() + b1.size() == capacity) {
            if (t1.size() < capacity) {
                // Discard oldest ghost from B1
                map.erase(b1.back());
                b1.pop_back();
                replace(false);
            } else {
                // T1 alone is full — discard directly from T1
                map.erase(t1.back());
                t1.pop_back();
            }
        }
        // Sub-case B: total directory is at or beyond 2*capacity
        else if (t1.size() + b1.size() < capacity &&
                 t1.size() + t2.size() + b1.size() + b2.size() >= capacity) {
            if (t1.size() + t2.size() + b1.size() + b2.size() == 2 * capacity) {
                map.erase(b2.back());
                b2.pop_back();
            }
            replace(false);
        }

        // Insert new item into T1 (seen for first time)
        t1.push_front(id);
        map[id] = {t1.begin(), List::T1};
        return false;
    }
};
