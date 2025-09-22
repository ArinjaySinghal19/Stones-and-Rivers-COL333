#!/bin/bash

# Stones and Rivers C++ Compilation Script
# This script automates the complete setup and compilation process for the C++ agent

set -e  # Exit on any error

echo "🎮 Stones and Rivers C++ Compilation Script 🎮"
echo "================================================"

# Get script directory (works even if script is called from elsewhere)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPP_DIR="$SCRIPT_DIR/c++_sample_files"
CLIENT_DIR="$SCRIPT_DIR/client_server"
BUILD_DIR="$CPP_DIR/build"

echo "📁 Project directory: $SCRIPT_DIR"
echo "🔧 C++ source directory: $CPP_DIR"
echo "🎯 Client directory: $CLIENT_DIR"

# Check if we're in the right directory
if [ ! -f "$SCRIPT_DIR/README.md" ] || [ ! -d "$CPP_DIR" ]; then
    echo "❌ Error: Please run this script from the project root directory"
    echo "   Expected to find README.md and c++_sample_files/ directory"
    exit 1
fi

# Step 1: Set up Python environment
echo ""
echo "🐍 Step 1: Setting up Python environment..."
if [ ! -d "$SCRIPT_DIR/.venv" ]; then
    echo "   Creating virtual environment..."
    python3 -m venv .venv
fi

echo "   Activating virtual environment..."
source .venv/bin/activate

echo "   Installing dependencies..."
pip install --quiet --upgrade pip
pip install --quiet pygame numpy scipy pybind11

echo "   ✅ Python environment ready"

# Step 2: Clean and prepare build directory
echo ""
echo "🧹 Step 2: Preparing build directory..."
if [ -d "$BUILD_DIR" ]; then
    echo "   Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

echo "   Creating fresh build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "   ✅ Build directory ready"

# Step 3: Configure CMake
echo ""
echo "⚙️  Step 3: Configuring CMake..."
PYBIND_DIR=$(.venv/bin/python -c "import pybind11; print(pybind11.get_cmake_dir())" 2>/dev/null || $SCRIPT_DIR/.venv/bin/python -m pybind11 --cmakedir)

echo "   Using pybind11 from: $PYBIND_DIR"
echo "   Configuring with CMake..."

cmake .. \
    -Dpybind11_DIR="$PYBIND_DIR" \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_BUILD_TYPE=Release

echo "   ✅ CMake configuration complete"

# Step 4: Compile C++ code
echo ""
echo "🔨 Step 4: Compiling C++ code..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Verify compilation
if [ ! -f "student_agent_module"*.so ]; then
    echo "❌ Error: Compilation failed - no .so file found"
    exit 1
fi

echo "   ✅ C++ compilation successful"

# Step 5: Copy files to client directory
echo ""
echo "📦 Step 5: Setting up client directory..."
cd "$CLIENT_DIR"

echo "   Copying compiled module..."
cp -r "$BUILD_DIR" .

echo "   Copying C++ agent wrapper..."
cp "$CPP_DIR/student_agent_cpp.py" .

echo "   ✅ Client directory setup complete"

# Step 6: Test the setup
echo ""
echo "🧪 Step 6: Testing the setup..."
cd "$CLIENT_DIR"

echo "   Testing C++ module import..."
if "$SCRIPT_DIR/.venv/bin/python" -c "import build.student_agent_module as student_agent; print('✅ C++ module imported successfully!')" 2>/dev/null; then
    echo "   ✅ C++ module import test passed"
else
    echo "   ❌ C++ module import test failed"
    exit 1
fi

echo "   Testing agent creation..."
if "$SCRIPT_DIR/.venv/bin/python" -c "from student_agent_cpp import StudentAgent; agent = StudentAgent('circle'); print('✅ C++ agent created successfully!')" 2>/dev/null; then
    echo "   ✅ Agent creation test passed"
else
    echo "   ❌ Agent creation test failed"
    exit 1
fi

# Final success message
echo ""
echo "🎉 SUCCESS! C++ agent compilation and setup complete!"
echo "================================================"
echo ""
echo "📋 Quick Usage Guide:"
echo "   • Activate environment: source .venv/bin/activate"
echo "   • Test with random vs C++: .venv/bin/python gameEngine.py --mode aivai --circle random --square student_cpp --nogui"
echo "   • Test with GUI: .venv/bin/python gameEngine.py --mode aivai --circle random --square student_cpp"
echo ""
echo "📁 Files created/updated:"
echo "   • $BUILD_DIR/student_agent_module*.so (compiled C++ module)"
echo "   • $CLIENT_DIR/build/ (copied build directory)"
echo "   • $CLIENT_DIR/student_agent_cpp.py (Python wrapper)"
echo ""
echo "🔄 To recompile after C++ changes:"
echo "   • Run this script again: ./compile_cpp.sh"
echo "   • Or just: cd c++_sample_files/build && make"
echo ""
echo "✨ Happy coding!"