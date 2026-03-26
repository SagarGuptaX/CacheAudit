cmake_minimum_required(VERSION 3.10)
project(CacheAudit VERSION 2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# -O2: enables optimizations without hiding bugs (no -O3 for now)
add_compile_options(-Wall -Wextra -Wpedantic -O2)

set(SOURCES
    src/main.cpp
    src/engine/trace_loader.cpp
)

add_executable(cache_audit ${SOURCES})

# Output binary to project root so run_experiments.sh finds it at ./cache_audit
set_target_properties(cache_audit PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Let source files include each other as "engine/foo.h", "policies/foo.h"
target_include_directories(cache_audit PRIVATE src)
