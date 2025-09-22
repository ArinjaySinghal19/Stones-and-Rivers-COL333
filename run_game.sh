#!/bin/bash

# Convenient run script for the Stones and Rivers game
# Automatically activates the virtual environment and runs the game

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT_DIR="$SCRIPT_DIR/client_server"

# Check if virtual environment exists
if [ ! -d "$SCRIPT_DIR/.venv" ]; then
    echo "❌ Virtual environment not found. Please run ./compile_cpp.sh first."
    exit 1
fi

# Activate virtual environment
source "$SCRIPT_DIR/.venv/bin/activate"

# Change to client directory
cd "$CLIENT_DIR"

echo "🎮 Starting Stones and Rivers Game..."
echo "======================================"

# Default to C++ agent vs random with no GUI
MODE=${1:-"aivai"}
CIRCLE=${2:-"random"}
SQUARE=${3:-"student_cpp"}
GUI_FLAG="--gui"

# Check if --nogui should be added (if not explicitly disabled)
if [[ "$*" != *"--nogui"* ]]; then
    GUI_FLAG="--nogui"
fi

echo "Mode: $MODE | Circle: $CIRCLE | Square: $SQUARE"
echo "Command: python gameEngine.py --mode $MODE --circle $CIRCLE --square $SQUARE $GUI_FLAG"
echo ""

# Run the game
python gameEngine.py --mode "$MODE" --circle "$CIRCLE" --square "$SQUARE" $GUI_FLAG "$@"