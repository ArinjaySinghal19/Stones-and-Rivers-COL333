#!/usr/bin/env python3
"""
Test to demonstrate the difference between mock fitness and real game evaluation

This script shows:
1. Mock fitness completes in milliseconds (FAKE)
2. Real game fitness takes seconds (REAL)
"""

import time
import sys
from pathlib import Path

# Add parent dir to path
sys.path.insert(0, str(Path(__file__).parent))

from train_weights import WeightVector


def test_mock_fitness():
    """Test mock fitness (fake - very fast)"""
    print("="*70)
    print("TEST 1: MOCK FITNESS (Currently in train_weights.py)")
    print("="*70)
    print("\nThis simulates 100 'games' using mock fitness...\n")
    
    wv = WeightVector()
    
    start = time.time()
    num_evaluations = 100
    
    for i in range(num_evaluations):
        # Mock fitness: just a mathematical formula
        import numpy as np
        import random
        
        w = wv.weights
        max_weight = np.max(np.abs(w))
        mean_weight = np.mean(np.abs(w))
        balance_score = mean_weight / (max_weight + 1e-10)
        noise = random.gauss(0, 0.1)
        fitness = balance_score + noise
    
    elapsed = time.time() - start
    
    print(f"Completed {num_evaluations} evaluations in {elapsed:.4f} seconds")
    print(f"Average time per evaluation: {elapsed/num_evaluations*1000:.2f} ms")
    print()
    print("⚠️  WARNING: These are NOT real games!")
    print("   This is just a mathematical formula - no game simulation.")
    print("   370 'games' in < 1 second is IMPOSSIBLE with real games.")
    print()
    print("="*70)
    print()
    
    return elapsed


def test_real_game_timing():
    """Show how long real games would take"""
    print("="*70)
    print("TEST 2: REAL GAME TIMING (Expected)")
    print("="*70)
    print("\nWith REAL game evaluation, here's what to expect:\n")
    
    games_per_eval = 10
    avg_game_time = 2.0  # seconds (conservative estimate)
    total_time = games_per_eval * avg_game_time
    
    print(f"Games per evaluation: {games_per_eval}")
    print(f"Avg time per game:    ~{avg_game_time:.1f} seconds")
    print(f"Total time per eval:  ~{total_time:.1f} seconds")
    print()
    print(f"For 100 evaluations:  ~{total_time * 100 / 60:.1f} minutes")
    print()
    print("✓ This is REALISTIC for actual game simulation.")
    print("  Each game involves:")
    print("  - Setting up the board")
    print("  - Running minimax search (with pruning)")
    print("  - Making 20-50+ moves")
    print("  - Evaluating heuristics many times")
    print()
    print("="*70)
    print()


def show_speed_comparison():
    """Show the speed comparison"""
    print("="*70)
    print("SPEED COMPARISON SUMMARY")
    print("="*70)
    print()
    
    mock_time = test_mock_fitness()
    
    print()
    test_real_game_timing()
    
    print("="*70)
    print("CONCLUSION")
    print("="*70)
    print()
    print("If your training completes 100+ games in < 1 second:")
    print("  → You are using MOCK FITNESS (not real games)")
    print()
    print("If your training takes minutes for 10 games:")
    print("  → You are using REAL GAME EVALUATION")
    print()
    print("Current train_weights.py uses MOCK FITNESS.")
    print("To fix: integrate simple_game_evaluator.py")
    print()
    print("="*70)


if __name__ == "__main__":
    show_speed_comparison()
