#!/usr/bin/env python3
"""Simple test to check if incremental hashing is working correctly."""

import sys
sys.path.insert(0, 'build')
import student_agent_module as agent_module

# Create a simple game state
state = agent_module.GameState()
print("Initial state created")

# Get a legal move and make it
moves = state.get_legal_moves()
if moves:
    print(f"Found {len(moves)} legal moves")
    move = moves[0]
    print(f"Making move: {move.action} from ({move.from_pos[0]},{move.from_pos[1]}) to ({move.to[0]},{move.to[1]})")
    
    # Make and undo the move
    undo_info = state.make_move(move)
    print("Move made successfully")
    state.undo_move(move, undo_info)
    print("Move undone successfully")
    
print("\nTest completed - check stderr for any hash mismatch warnings")
