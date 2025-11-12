#!/usr/bin/env python3
"""
Demo script for weight training system

This demonstrates the weight training functionality with a small number of iterations.
"""

import sys
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).parent))
from train_weights import WeightVector, WeightTrainer


def demo_weight_operations():
    """Demonstrate basic weight vector operations"""
    print("=" * 70)
    print("Demo: Weight Vector Operations")
    print("=" * 70)
    print()
    
    # Create default weight vector
    print("1. Creating default weight vector...")
    wv = WeightVector()
    print(f"   Norm: {np.linalg.norm(wv.weights):.10f} (should be 1.0)")
    print(f"   Weights (normalized): {wv.weights}")
    print()
    
    # Show denormalized weights
    print("2. Denormalized weights (scaled for readability):")
    weights_dict = wv.to_dict()
    for name, value in weights_dict.items():
        print(f"   {name:40s}: {value:8.2f}")
    print()
    
    # Test perturbation
    print("3. Creating perturbations (5-10% range)...")
    for i in range(3):
        perturbed = wv.perturb()
        diff = perturbed.weights - wv.weights
        diff_magnitude = np.linalg.norm(diff)
        norm = np.linalg.norm(perturbed.weights)
        
        print(f"   Perturbation {i+1}:")
        print(f"     Difference magnitude: {diff_magnitude:.6f}")
        print(f"     New norm: {norm:.10f} (should be 1.0)")
        print(f"     Max weight change: {np.max(np.abs(diff)):.6f}")
        print()
    
    # Show C++ format
    print("4. C++ format output:")
    print(wv.to_cpp_format())
    print()


def demo_normalization():
    """Demonstrate normalization property"""
    print("=" * 70)
    print("Demo: Normalization Property")
    print("=" * 70)
    print()
    
    # Start with unnormalized weights
    unnormalized = np.array([10.0, 40.0, 10.0, 50.0, 150.0, 10.0, 
                            20.0, 40.0, 150.0, 10.0, 10.0, 50.0])
    
    print(f"Original weights: {unnormalized}")
    print(f"Original norm: {np.linalg.norm(unnormalized):.6f}")
    print()
    
    wv = WeightVector(unnormalized, normalize=True)
    print(f"Normalized weights: {wv.weights}")
    print(f"Normalized norm: {np.linalg.norm(wv.weights):.10f}")
    print()
    
    # Apply multiple perturbations
    print("Applying 10 perturbations in sequence...")
    for i in range(10):
        wv = wv.perturb()
        norm = np.linalg.norm(wv.weights)
        print(f"  After perturbation {i+1}: norm = {norm:.10f}")
    print()


def demo_hill_climbing():
    """Demonstrate hill climbing with a simple fitness function"""
    print("=" * 70)
    print("Demo: Hill Climbing (Simple Fitness)")
    print("=" * 70)
    print()
    
    # Simple fitness: prefer weights where sum of squares is minimized
    # (i.e., more uniform distribution)
    def simple_fitness(weights_vec):
        """Fitness based on uniformity of weights"""
        w = weights_vec.weights
        # Prefer more uniform distribution
        variance = np.var(w)
        return -variance  # Higher is better, so negative variance
    
    # Initialize
    current = WeightVector()
    current_fitness = simple_fitness(current)
    
    print(f"Initial fitness: {current_fitness:.6f}")
    print(f"Initial variance: {np.var(current.weights):.6f}")
    print()
    
    improvements = 0
    for iteration in range(20):
        # Generate neighbor
        neighbor = current.perturb()
        neighbor_fitness = simple_fitness(neighbor)
        
        # Accept if better
        if neighbor_fitness > current_fitness:
            print(f"Iteration {iteration+1}: ✓ Improvement "
                  f"{current_fitness:.6f} -> {neighbor_fitness:.6f}")
            current = neighbor
            current_fitness = neighbor_fitness
            improvements += 1
        else:
            print(f"Iteration {iteration+1}: ✗ No improvement "
                  f"{neighbor_fitness:.6f} <= {current_fitness:.6f}")
    
    print()
    print(f"Total improvements: {improvements}/20")
    print(f"Final fitness: {current_fitness:.6f}")
    print(f"Final variance: {np.var(current.weights):.6f}")
    print()


def main():
    """Run all demos"""
    print("\n")
    print("*" * 70)
    print("*" + " " * 68 + "*")
    print("*" + " " * 20 + "WEIGHT TRAINING DEMO" + " " * 28 + "*")
    print("*" + " " * 68 + "*")
    print("*" * 70)
    print("\n")
    
    demo_weight_operations()
    print("\n")
    
    demo_normalization()
    print("\n")
    
    demo_hill_climbing()
    print("\n")
    
    print("=" * 70)
    print("Demo Complete!")
    print("=" * 70)
    print()
    print("Next steps:")
    print("1. Run tests: python test_weight_training.py")
    print("2. Train weights: python train_weights.py --iterations 50")
    print("3. Check results: c++_sample_files/weight_training_results/")
    print()


if __name__ == '__main__':
    main()
