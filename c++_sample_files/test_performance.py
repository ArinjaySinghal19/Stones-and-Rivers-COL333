#!/usr/bin/env python3
"""
Performance comparison test for incremental hashing
"""
import sys
import time
sys.path.insert(0, 'build')
import student_agent_module as sam

def create_realistic_board():
    """Create a more realistic game board"""
    board = []
    for y in range(16):
        row = []
        for x in range(12):
            row.append({})
        board.append(row)
    
    # Add circle pieces
    for x in range(3, 9):
        if x % 2 == 0:
            board[2][x] = {"owner": "circle", "side": "stone"}
        else:
            board[2][x] = {"owner": "circle", "side": "river", "orientation": "horizontal"}
    
    # Add square pieces
    for x in range(3, 9):
        if x % 2 == 1:
            board[13][x] = {"owner": "square", "side": "stone"}
        else:
            board[13][x] = {"owner": "square", "side": "river", "orientation": "vertical"}
    
    return board

def test_performance():
    print("=" * 60)
    print("Performance Test for Incremental Hashing")
    print("=" * 60)
    
    board = create_realistic_board()
    agent = sam.StudentAgent("square")
    
    print("\n✓ Agent created")
    print("✓ Running 3 moves to test performance...")
    
    total_time = 0
    for i in range(3):
        start = time.time()
        move = agent.choose(board, 16, 12, [3, 4, 5, 6, 7, 8], 120.0, 120.0)
        elapsed = time.time() - start
        total_time += elapsed
        print(f"\nMove {i+1}: {elapsed:.3f}s - {move.action} from {move.from_pos} to {move.to_pos}")
        
        # Apply move to board
        if move.action == "move" and move.from_pos and move.to_pos:
            fx, fy = move.from_pos
            tx, ty = move.to_pos
            if board[fy][fx]:
                board[ty][tx] = board[fy][fx]
                board[fy][fx] = {}
    
    avg_time = total_time / 3
    print(f"\n{'=' * 60}")
    print(f"Average time per move: {avg_time:.3f}s")
    print(f"Total time for 3 moves: {total_time:.3f}s")
    print(f"{'=' * 60}")
    
    if avg_time > 3.0:
        print("\n⚠️  WARNING: Moves are taking longer than expected!")
        print("   Expected: ~1-2 seconds per move")
        print(f"   Actual: {avg_time:.3f} seconds per move")
    else:
        print("\n✅ Performance looks good!")

if __name__ == "__main__":
    test_performance()
