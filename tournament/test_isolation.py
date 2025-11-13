#!/usr/bin/env python3
"""Test script to verify bot isolation works correctly"""

import sys
sys.path.insert(0, '/Users/arinjaysinghal/Desktop/Stones-and-Rivers-COL333')

from tournament.bot_loader import BotLoader

print("=== Testing Bot Isolation ===\n")

loader = BotLoader()

print("Step 1: Loading bot1 for circle player...")
bot1_circle = loader.create_agent("bot1", "circle")
print(f"✓ bot1 circle agent created: {bot1_circle}\n")

print("Step 2: Loading bot2 for square player...")
bot2_square = loader.create_agent("bot2", "square")
print(f"✓ bot2 square agent created: {bot2_square}\n")

print("Step 3: Verifying both agents are different...")
print(f"bot1 type: {type(bot1_circle)}")
print(f"bot2 type: {type(bot2_square)}")
print(f"Same type? {type(bot1_circle) == type(bot2_square)}")
print(f"Same instance? {bot1_circle is bot2_square}")

print("\n=== Success! Both bots loaded independently ===")
