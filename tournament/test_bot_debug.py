#!/usr/bin/env python3
"""Test that bot1 and bot2 produce different debug output"""

import sys
sys.path.insert(0, '/Users/arinjaysinghal/Desktop/Stones-and-Rivers-COL333')
sys.path.insert(0, '/Users/arinjaysinghal/Desktop/Stones-and-Rivers-COL333/client_server')

from client_server.agent import get_agent
from client_server.gameEngine import default_start_board, score_cols_for

print("="*60)
print("Testing bot1 vs bot2 debug output")
print("="*60)

# Create a simple board
rows, cols = 13, 12
board = default_start_board(rows, cols)
score_cols = score_cols_for(cols)

# Load both bots
print("\n1. Loading bot1 for circle...")
bot1 = get_agent('circle', 'bot1')

print("\n2. Loading bot2 for square...")
bot2 = get_agent('square', 'bot2')

print("\n" + "="*60)
print("Making moves - watch for debug output!")
print("="*60)

print("\n3. Bot1 (circle) making move...")
print("-" * 60)
move1 = bot1.choose(board, rows, cols, score_cols, 100.0, 100.0)
print(f"Bot1 chose: {move1}")

print("\n4. Bot2 (square) making move...")
print("-" * 60)
move2 = bot2.choose(board, rows, cols, score_cols, 100.0, 100.0)
print(f"Bot2 chose: {move2}")

print("\n" + "="*60)
print("✓ Test complete!")
print("="*60)
print("\nExpected output:")
print("  - Bot1 should show: [BOT1 - DEPTH=1]")
print("  - Bot2 should show: [BOT2 - DEPTH=2]")
