cmake_minimum_required(VERSION 3.10)
project(CacheAudit VERSION 1.0)

# Use C++17 (Standard for modern systems)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable strict warnings (Catch bugs early)
add_compile_options(-Wall -Wextra -Wpedantic)

# Define the source files (We will add more later)
set(SOURCES
    src/main.cpp
)

# Create the executable
add_executable(cache_audit ${SOURCES})
