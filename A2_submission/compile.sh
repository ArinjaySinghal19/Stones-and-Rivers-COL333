#!/bin/bash

# C++ Agent Compilation Script
# Based on instructions from README.md

# Create build directory and navigate to it
mkdir -p build && cd build

# Configure with CMake
cmake .. -Dpybind11_DIR=$(python3 -m pybind11 --cmakedir) \
 -DCMAKE_C_COMPILER=gcc \
 -DCMAKE_CXX_COMPILER=g++

# Build the project
make

# Return to parent directory
cd ..
