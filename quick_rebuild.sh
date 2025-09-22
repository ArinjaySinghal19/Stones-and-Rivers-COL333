#!/bin/bash

# Quick rebuild script for C++ changes
# Use this when you only need to recompile after making changes to student_agent.cpp

set -e

echo "🔨 Quick C++ Rebuild"
echo "==================="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/c++_sample_files/build"
CLIENT_DIR="$SCRIPT_DIR/client_server"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "❌ Build directory not found. Please run ./compile_cpp.sh first for initial setup."
    exit 1
fi

echo "🔧 Rebuilding C++ module..."
cd "$BUILD_DIR"
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "📦 Updating client directory..."
cd "$CLIENT_DIR"
cp -r "$BUILD_DIR" .

echo "🧪 Testing rebuilt module..."
if "$SCRIPT_DIR/.venv/bin/python" -c "import build.student_agent_module; print('✅ Rebuild successful!')" 2>/dev/null; then
    echo "✅ Quick rebuild complete!"
else
    echo "❌ Module test failed after rebuild"
    exit 1
fi

echo ""
echo "🎯 Ready to test your changes!"
echo "   Run: .venv/bin/python gameEngine.py --mode aivai --circle random --square student_cpp --nogui"