#pragma once
#include "request.h"
#include "key_mapper.h"
#include <vector>
#include <string>

// Reads an access trace from disk and produces a fully normalized
// vector<Request> using the provided KeyMapper.
//
// Input format (one per line):
//   R <key>   — read access
//   W <key>   — write access
//
// All string keys are converted to integer IDs before returning.
// The full trace is loaded into memory so that:
//   - All policies see the same in-memory representation
//   - Belady can preprocess future accesses without special casing
class TraceLoader {
public:
    static std::vector<Request> load(const std::string& filepath, KeyMapper& mapper);
};
