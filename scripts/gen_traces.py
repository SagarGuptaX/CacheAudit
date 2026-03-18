import os

# Get absolute path to project root
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
TRACES_DIR = os.path.join(PROJECT_ROOT, "traces", "synthetic")
# Ensure traces directory exists
os.makedirs("../traces/synthetic", exist_ok=True)

def generate_loop(filename, n_items, n_cycles):
    with open(filename, "w") as f:
        for _ in range(n_cycles):
            for i in range(n_items):
                f.write(f"R {i}\n")

def generate_scan(filename, n_items):
    with open(filename, "w") as f:
        for i in range(n_items):
            f.write(f"R {i}\n")

def generate_skewed(filename, n_items, n_cycles):
    """
    Item 0 is accessed 5 times for every 1 time we cycle through 1..N
    Pattern: 0, 0, 0, 0, 0, 1, 2, 3 ... N
    """
    with open(filename, "w") as f:
        # Boost '0' frequency initially to make it a VIP
        for _ in range(10):
            f.write("R 0\n")
            
        # Now loop through others, but revisit 0 occasionally
        for _ in range(n_cycles):
            for i in range(1, n_items):
                f.write(f"R {i}\n")
            # Revisit 0 (LRU might have evicted it by now!)
            f.write("R 0\n")

generate_loop(os.path.join(TRACES_DIR, "loop.txt"), 50, 20)
generate_scan(os.path.join(TRACES_DIR, "scan.txt"), 1000)
generate_skewed(os.path.join(TRACES_DIR, "skewed.txt"), n_items=50, n_cycles=20)
print(f"Generated: {os.path.abspath(os.path.join(TRACES_DIR, 'loop.txt'))}")
print(f"Generated: {os.path.abspath(os.path.join(TRACES_DIR, 'scan.txt'))}")
print(f"Generated: {os.path.abspath(os.path.join(TRACES_DIR, 'skewed.txt'))}")
