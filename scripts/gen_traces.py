"""
gen_traces.py
Generates synthetic access traces for CacheAudit benchmarking.

Output: traces/synthetic/{loop,scan,skewed,hot_cold}.txt
Format: one access per line -> "R <key>"

Run from project root:
    python3 scripts/gen_traces.py
"""

import os

SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
TRACES_DIR   = os.path.join(PROJECT_ROOT, "traces", "synthetic")

os.makedirs(TRACES_DIR, exist_ok=True)


def generate_loop(path, n_items, n_cycles):
    """
    Repeatedly cycles through [0, n_items) for n_cycles passes.

    With n_items=30 and experiment sizes {10, 20, 40}:
      size=10 -> cache holds 1/3 of working set -> all online policies thrash
      size=20 -> cache holds 2/3 of working set -> all online policies thrash
      size=40 -> cache > working set -> all policies ~95% after first warm-up

    Key insight: below the working set threshold, all online policies fail
    equally badly. Belady achieves non-zero hit rates even below threshold
    by knowing which items will be needed soonest.
    """
    with open(path, "w") as f:
        for _ in range(n_cycles):
            for i in range(n_items):
                f.write(f"R {i}\n")


def generate_scan(path, n_items):
    """
    Accesses each item exactly once, sequentially, never repeated.
    Hit rate is 0% for all policies by definition.

    Purpose: all-miss baseline. Runtime differences here reflect per-policy
    overhead cost, not policy quality.
    """
    with open(path, "w") as f:
        for i in range(n_items):
            f.write(f"R {i}\n")


def generate_skewed(path, n_items, n_cycles):
    """
    Item 0 is a hot key accessed frequently throughout.
    All other items cycle through once per pass.

    Structure:
      - 10 warm-up accesses to item 0 (establishes high frequency)
      - n_cycles passes through [1, n_items), item 0 revisited at end of each

    FIFO/LRU: item 0 gets evicted during cold floods -> miss on revisit
    LFU:      high frequency protects item 0 throughout
    ARC:      ghost hits in B2 adapt p to protect item 0
    Belady:   never evicts item 0 when it knows it is needed soon
    """
    with open(path, "w") as f:
        for _ in range(10):
            f.write("R 0\n")
        for _ in range(n_cycles):
            for i in range(1, n_items):
                f.write(f"R {i}\n")
            f.write("R 0\n")


def generate_hot_cold(path, n_hot, n_cold_per_cycle, n_cycles, warmup_reps=15):
    """
    The canonical recency-vs-frequency stress test.

    Structure:
      Phase 1 - warmup: each hot item accessed warmup_reps times.
                Builds high frequency for hot items before any cold flood.
      Phase 2 - n_cycles of: (unique cold flood, hot item access)
                Each cycle uses a fresh batch of cold IDs never seen before.

    Why unique cold items per cycle (critical design decision):
      If cold items repeated across cycles, they would accumulate ghost
      history in B1 (ARC) or frequency counts (LFU). This would cause
      ARC's adaptive p parameter to overshoot via a cascade of B1 ghost
      hits, eventually evicting hot items from T2 into B2 and producing
      a non-monotone hit rate (larger cache = worse performance).
      Using unique cold items per cycle means cold items are never
      revisited, so no B1 ghost hits occur. p stays at 0, T2 remains
      clean, and ARC correctly protects the hot items throughout.

    With n_hot=5, n_cold_per_cycle=20, cache_size=10:
      Hot items (ids 0-4) accumulate freq=warmup_reps in LFU.
      Cold items each have freq=1 when first seen.

      LFU: hot items (freq >> 1) are never evicted. Cold items evict each
           other. Hot accesses at end of each cycle: all hits.

      LRU: after the cold flood (20 > 10 items), hot items are the oldest
           in cache (last accessed before the cold flood). They are the
           LRU candidates and get evicted. Hot accesses: all misses.

      FIFO: same failure mode as LRU.

      ARC (cap=10): warmup places hot items in T2 (freq > 1). p=0.
           Cold items fill T1. T1 evictions stay in T1 (prefer_t1 = T1>p=0).
           T2 (hot items) is never touched. Hot accesses: all hits.

      ARC (cap=20): T2 has 5 hot items. T1 absorbs up to 15 cold items.
           The remaining 5 cold items per cycle evict T1 LRU to B1.
           Cold items are unique -> those B1 entries are never re-accessed
           -> no B1 ghost hits -> p stays 0 -> T2 always protected.
           Hot accesses: all hits.

      Belady: knows cold items are accessed exactly once and never again.
              Evicts cold items first, always. Hot items perfectly protected.

    Result: LFU >= ARC >> LRU = FIFO at cache sizes below (n_hot + n_cold).
    At cache_size >= n_hot + n_cold_per_cycle, all policies converge.
    """
    with open(path, "w") as f:
        # Phase 1: warmup — establish frequency advantage for hot items
        for _ in range(warmup_reps):
            for i in range(n_hot):
                f.write(f"R {i}\n")

        # Phase 2: unique cold floods + hot accesses
        # Each cycle uses a fresh block of cold IDs: never seen before, never again.
        for c in range(n_cycles):
            cold_start = n_hot + c * n_cold_per_cycle
            cold_end   = cold_start + n_cold_per_cycle
            for i in range(cold_start, cold_end):
                f.write(f"R {i}\n")
            for i in range(n_hot):
                f.write(f"R {i}\n")


# ---------------------------------------------------------------------------
# Generate
# ---------------------------------------------------------------------------
loop_path     = os.path.join(TRACES_DIR, "loop.txt")
scan_path     = os.path.join(TRACES_DIR, "scan.txt")
skewed_path   = os.path.join(TRACES_DIR, "skewed.txt")
hot_cold_path = os.path.join(TRACES_DIR, "hot_cold.txt")

generate_loop(loop_path,         n_items=30,          n_cycles=20)
generate_scan(scan_path,         n_items=1000)
generate_skewed(skewed_path,     n_items=30,           n_cycles=20)
generate_hot_cold(hot_cold_path, n_hot=5, n_cold_per_cycle=20, n_cycles=30, warmup_reps=15)

print(f"Generated: {loop_path}")
print(f"Generated: {scan_path}")
print(f"Generated: {skewed_path}")
print(f"Generated: {hot_cold_path}")
