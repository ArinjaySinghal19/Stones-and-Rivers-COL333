#!/bin/bash

# Run AlphaZero Agent Script
# This script makes it easy to run your trained AlphaZero agent

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Determine which Python to use (use absolute paths)
if [ -f "$SCRIPT_DIR/.venv/bin/python" ]; then
    PYTHON="$SCRIPT_DIR/.venv/bin/python"
elif [ -f "$SCRIPT_DIR/venv/bin/python" ]; then
    PYTHON="$SCRIPT_DIR/venv/bin/python"
else
    PYTHON="python"
fi

echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}AlphaZero for Stones and Rivers${NC}"
echo -e "${BLUE}================================${NC}"
echo ""

# Check if we have trained models
if [ ! -d "alphazero/checkpoints" ] || [ -z "$(ls -A alphazero/checkpoints/*.pt 2>/dev/null)" ]; then
    echo -e "${RED}Error: No trained models found!${NC}"
    echo ""
    echo "Please train a model first:"
    echo "  cd alphazero"
    echo "  make train-small"
    echo ""
    exit 1
fi

# Count available models
NUM_MODELS=$(ls alphazero/checkpoints/*.pt 2>/dev/null | wc -l)
echo -e "${GREEN}Found $NUM_MODELS trained model(s)${NC}"
echo ""

# Determine board size
BOARD_SIZE=${BOARD_SIZE:-small}
echo "Board size: $BOARD_SIZE (set BOARD_SIZE env variable to change)"
echo ""

# Export board size for Python
export BOARD_SIZE

# Menu
echo "How do you want to run?"
echo "  1) AlphaZero (student) vs Random [GUI]"
echo "  2) AlphaZero (student) vs Random [No GUI - Faster]"
echo "  3) Human vs AlphaZero [GUI]"
echo "  4) AlphaZero vs AlphaZero [No GUI]"
echo "  5) Custom command"
echo ""
read -p "Choose [1-5]: " choice

case $choice in
    1)
        echo -e "${BLUE}Starting: AlphaZero vs Random (GUI mode)${NC}"
        cd client_server
        $PYTHON gameEngine.py --mode aivai --circle student --square random --board-size $BOARD_SIZE
        ;;
    2)
        echo -e "${BLUE}Starting: AlphaZero vs Random (No GUI - Terminal mode)${NC}"
        cd client_server
        $PYTHON gameEngine.py --mode aivai --circle student --square random --board-size $BOARD_SIZE --nogui
        ;;
    3)
        echo -e "${BLUE}Starting: Human vs AlphaZero (GUI mode)${NC}"
        echo "You play as Circle (red), AlphaZero plays as Square (blue)"
        cd client_server
        $PYTHON gameEngine.py --mode hvai --square student --board-size $BOARD_SIZE
        ;;
    4)
        echo -e "${BLUE}Starting: AlphaZero vs AlphaZero (No GUI)${NC}"
        cd client_server
        $PYTHON gameEngine.py --mode aivai --circle student --square student --board-size $BOARD_SIZE --nogui
        ;;
    5)
        echo "Enter your custom command (from project root):"
        read -p "> " custom_cmd
        eval $custom_cmd
        ;;
    *)
        echo -e "${RED}Invalid choice${NC}"
        exit 1
        ;;
esac
