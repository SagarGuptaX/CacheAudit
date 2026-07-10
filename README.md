# CacheAudit

A trace-driven cache simulation framework written in C++17 — built to study how eviction policy choice affects hit rate, and to show exactly when and why one policy outperforms another under different workload shapes.

CacheAudit runs controlled experiments on access traces, compares five eviction policies across four synthetic workloads, and exports results to CSV for analysis. It is not a production caching library. It is a system for understanding the tradeoffs that production caching libraries hide.

---

## What it demonstrates

Cache replacement policy is one of the oldest and most studied problems in systems — it appears in CPU caches, OS page replacement, database buffer pools, and CDN edge caches. CacheAudit is built around five canonical policies, each implemented cleanly with the data structures that make them correct and efficient:

- **FIFO** — insertion-order eviction using a `deque` + `unordered_set`. Simple baseline with predictable behavior and a well-known failure mode on repeated scans.
- **LRU** — recency-based eviction using a doubly-linked list and an iterator map for O(1) move-to-front via `std::list::splice`. The standard reference policy in systems literature.
- **LFU** — frequency-based eviction using a `std::set` sorted by `(frequency ASC, recency ASC)`. Tie-breaking is LRU-order within the same frequency bucket. Update requires remove → modify → reinsert since `std::set` elements are immutable.
- **ARC** — Adaptive Replacement Cache (Megiddo & Modha, FAST 2003). Maintains four lists — T1, T2, B1, B2 — and adapts the balance between recency and frequency at runtime based on ghost-list hits. No manual tuning required.
- **Bélády's Optimal** — offline upper bound. Preprocesses the full trace to build per-item future-access lists, then uses `std::upper_bound` at each eviction to find the item needed furthest in the future. Exists to show the gap between real policies and theoretical optimal.

The framework is designed so that timing differences between policies reflect eviction logic, not lookup overhead. All policies use O(1) hash-based lookups; string keys are normalized to integers once at load time so the hot path is integer-only.

---

## Architecture

```
Trace file  (R/W key)
     │
 TraceLoader
     │
  KeyMapper          ← string → int, assigned once at load time
     │
 vector<Request>     ← integer IDs only from here down
     │
   Runner            ← simulation loop + chrono timing
     │
 Cache (abstract)    ← access(int id) → bool
 /   |   |   |   \
FIFO LRU LFU ARC Belady
     │
  Metrics            ← hit_rate, hits, misses, runtime_ms
     │
stdout + CSV row
```

The `Cache` abstract base class defines a single virtual method `access(int id) → bool`. Every policy implements exactly this interface. The `Runner` feeds the trace into the cache and measures wall-clock time without knowing which policy it is running. This separation means adding a new policy requires no changes to the runner, loader, or output layer.

**Design rule: string handling ends at the KeyMapper.** Every policy operates on integers. This avoids repeated string hashing in the hot path and keeps the cache layer clean.

---

## Project structure

```
src/
  engine/
    request.h          — Request{Op, id} — the atomic unit of simulation
    key_mapper.h       — string → int normalization, assigned once
    trace_loader.h/cpp — reads trace file, invokes KeyMapper
    cache.h            — abstract base class: access(int id), hit/miss counters
    metrics.h          — Metrics struct: hit_rate, misses, runtime_ms
    runner.h           — simulation loop + chrono timing
  policies/
    fifo.h             — deque + unordered_set
    lru.h              — std::list + iterator map, O(1) splice
    lfu.h              — std::set frequency tree + lookup map
    arc.h              — four-list adaptive policy, ghost-hit adaptation
    belady.h           — offline optimal, future-access preprocessing
  main.cpp             — CLI, policy factory, CSV append

scripts/
  gen_traces.py        — generates all four synthetic traces
  run_experiments.sh   — batch runner: 5 policies × 4 traces × 3 sizes = 60 runs

traces/
  synthetic/           — loop, scan, skewed, hot_cold
```

---

## Build

Requires a C++17 compiler and CMake 3.10+. Tested on Linux (GCC 13).

```bash
cmake -B build
cmake --build build
```

Or directly with g++:

```bash
g++ -std=c++17 -Wall -Wextra -O2 -I src \
    src/main.cpp src/engine/trace_loader.cpp \
    -o cache_audit
```

The binary is placed at the project root as `./cache_audit`.

---

## Usage

```bash
# Generate traces
python3 scripts/gen_traces.py

# Single run
./cache_audit traces/synthetic/loop.txt lru 40

# With CSV output
./cache_audit traces/synthetic/hot_cold.txt arc 10 --out results/out.csv

# All 60 experiments
bash scripts/run_experiments.sh
```

**Policies:** `fifo` `lru` `lfu` `arc` `belady`

---

## Workloads and what they expose

Each trace is designed to stress a specific policy property. The results below are from the batch runner across three cache sizes (10, 20, 40).

### loop — working set threshold

30 unique items cycled 20 times. Below the working set size, all online policies thrash equally. Above it, all converge near 95%. Bélády achieves non-zero hit rates even below threshold because it knows which item will be needed soonest.

```
size=10 → all online: 0%    Bélády: 30%
size=20 → all online: 0%    Bélády: 63%
size=40 → all:  ~95%
```

**What this shows:** the working set threshold is a hard ceiling for all online policies. When the cache is smaller than the working set, eviction policy choice is irrelevant — you will always thrash.

### scan — all-miss baseline

1000 unique items, each accessed exactly once. Hit rate is 0% for all policies by definition. Runtime differences here reflect per-policy overhead, not quality. Use this to reason about the cost of each data structure under zero-hit conditions.

### skewed — hot key under cold flood

Item 0 is warmed with 10 accesses, then revisited at the end of each of 20 cycles through items 1–29.

```
size=10 → FIFO/LRU: 1.5%    LFU/ARC: 4.75%    Bélády: 31%
size=20 → FIFO/LRU: 1.5%    LFU/ARC: 4.75%    Bélády: 64%
```

LFU and ARC protect item 0 because its accumulated frequency dominates cold items. LRU evicts it during the cold flood because it becomes the oldest entry. This is the canonical recency-vs-frequency tradeoff.

### hot_cold — frequency advantage under sustained cold pressure

5 hot items warmed 15 times each, then 30 cycles of 20 unique cold items followed by all 5 hot items. Cold items are unique per cycle — they are never revisited, so they cannot build ghost history in ARC.

```
size=10 → FIFO/LRU: 8.5%    LFU/ARC/Bélády: 26.7%
size=20 → FIFO/LRU: 8.5%    LFU/ARC/Bélády: 26.7%
size=40 → FIFO: 17.6%    LRU/LFU/ARC/Bélády: 26.7%
```

26.7% is the theoretical ceiling for this trace — the ratio of warmup hits plus cycle-end hot hits to total requests. LFU and ARC hit the ceiling at all tested sizes. LRU and FIFO miss the hot items after cold floods. The gap is stable across sizes because the ceiling is determined by trace structure, not cache size.

**Why unique cold items matter (important design detail):** if cold items repeated across cycles, they would accumulate B1 ghost history in ARC. Ghost hits in B1 drive `p` upward until T2 — which holds the hot items — starts losing entries to B2. This produces a non-monotone result: larger cache, worse performance. Unique cold items prevent this entirely. `p` stays at 0, T2 is never disrupted, and ARC correctly protects the hot items throughout.

---

## Output

**Stdout** — one summary per run:

```
Trace:       traces/synthetic/hot_cold.txt
Requests:    825
Unique keys: 25
Policy:      lfu
Cache size:  10

--- Results ---
Hit Rate:   26.67%
Hits:       220
Misses:     605
Runtime:    0 ms
```

**CSV** — one row per run, header written once:

```
policy,trace,cache_size,hit_rate,hits,misses,runtime_ms
lfu,hot_cold,10,0.266667,220,605,0
arc,hot_cold,10,0.266667,220,605,0
belady,hot_cold,10,0.423030,349,476,0
```

---

## Key design decisions

**Why integer IDs throughout?** String hashing on every access would pollute timing results with allocator and hash cost. Normalizing once at load time means the hot path is integer comparison only. Timing differences between policies reflect their eviction logic, not their lookup overhead.

**Why load the full trace into memory?** Bélády requires future knowledge by definition. Loading everything upfront gives all policies the same execution model — there is no special code path for offline vs. streaming policies. The simulation loop is identical for all five.

**Why `std::set` for LFU instead of the O(1) frequency-list approach?** The `std::set` approach is O(log n) per operation but is significantly simpler to reason about and audit. The O(1) approach requires per-frequency doubly-linked lists with a `min_freq` pointer and is substantially harder to get right. For the workload sizes studied here the difference is not measurable. The O(1) variant is a natural extension if needed.

**Why hash maps everywhere?** All policies use hash-based lookups. Real production caches do too. If lookup complexity differed across policies, lookup cost would dominate timing results and obscure the eviction logic comparison.

---

<!---## Related

[NetServe](https://github.com/SagarGuptaX/NetServe) — a concurrent HTTP server written in C++ that exposes CacheAudit as a network service. NetServe wraps this binary: it accepts HTTP requests, shells out to `cache_audit`, and returns JSON. Together they form a pipeline that demonstrates both systems programming and backend service design.--->
