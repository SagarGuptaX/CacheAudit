#pragma once

// The atomic unit of the simulation pipeline.
// After trace loading, everything downstream is integers only.
struct Request {
    enum class Op { READ, WRITE };
    Op op;
    int id; // Normalized integer ID, assigned by KeyMapper
};

