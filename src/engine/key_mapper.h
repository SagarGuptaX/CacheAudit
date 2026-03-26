#pragma once
#include <string>
#include <unordered_map>

// Converts raw string keys from the trace into compact integer IDs.
// Called once during trace loading. Everything downstream uses ints.
//
// Why a separate component:
//   - Separation of concerns: trace I/O vs cache logic
//   - Reusable if multiple traces share the same key space
//   - Clean to explain in interviews
class KeyMapper {
private:
    std::unordered_map<std::string, int> map;
    int next_id = 0;

public:
    // Returns existing ID if key is known, otherwise assigns a new one.
    int normalize(const std::string& key) {
        auto it = map.find(key);
        if (it != map.end()) return it->second;
        return map[key] = next_id++;
    }

    // Total number of unique keys seen so far.
    int unique_count() const { return next_id; }
};
