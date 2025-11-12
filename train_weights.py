#!/usr/bin/env python3
"""
Weight Training System using Hill Climbing with Random Perturbations

This script trains the heuristic weights for the Stones and Rivers bot using:
- Hill climbing local search
- Random perturbations (5-10% of weights)
- Unit norm constraint on weight vector
- Self-play evaluation for fitness

Usage:
    python train_weights.py --iterations 100 --games-per-eval 10
"""

import numpy as np
import argparse
import json
import os
import sys
import subprocess
import tempfile
from pathlib import Path
from typing import Dict, List, Tuple
import random

class WeightVector:
    """Represents a weight vector with unit norm constraint"""
    
    # Weight names in order (matching Weights struct in heuristics.h)
    WEIGHT_NAMES = [
        'vertical_push',
        'pieces_in_scoring_attack',
        'horizontal_attack_self',
        'inactive_self',
        'pieces_blocking_vertical_self',
        'horizontal_base_self',
        'horizontal_negative_self',
        'pieces_in_scoring_defense',
        'pieces_blocking_vertical_opp',
        'horizontal_base_opp',
        'horizontal_attack_opp',
        'inactive_opp',
    ]
    
    def __init__(self, weights: np.ndarray = None, normalize: bool = True):
        """
        Initialize weight vector
        
        Args:
            weights: numpy array of weights (default: default weights from heuristics.h)
            normalize: whether to normalize to unit norm
        """
        if weights is None:
            # Default weights from heuristics.h
            weights = np.array([
                10.0,   # vertical_push
                40.0,   # pieces_in_scoring_attack
                10.0,   # horizontal_attack_self
                50.0,   # inactive_self
                150.0,  # pieces_blocking_vertical_self
                10.0,   # horizontal_base_self
                20.0,   # horizontal_negative_self
                40.0,   # pieces_in_scoring_defense
                150.0,  # pieces_blocking_vertical_opp
                10.0,   # horizontal_base_opp
                10.0,   # horizontal_attack_opp
                50.0,   # inactive_opp
            ])
        
        self.weights = weights.copy()
        if normalize:
            self.normalize()
    
    def normalize(self):
        """Normalize weights to unit norm"""
        norm = np.linalg.norm(self.weights)
        if norm > 1e-10:  # Avoid division by zero
            self.weights = self.weights / norm
        else:
            raise ValueError("Weight vector has zero norm!")
    
    def copy(self) -> 'WeightVector':
        """Create a copy of this weight vector"""
        return WeightVector(self.weights.copy(), normalize=False)
    
    def perturb(self, perturbation_ratio: float = 0.075) -> 'WeightVector':
        """
        Create a perturbed version of this weight vector
        
        Args:
            perturbation_ratio: ratio of perturbation (default 7.5% for 5-10% range)
        
        Returns:
            New perturbed and normalized weight vector
        """
        # Random perturbation in range [5%, 10%]
        min_ratio = 0.05
        max_ratio = 0.10
        actual_ratio = random.uniform(min_ratio, max_ratio)
        
        # Create random perturbation vector
        perturbation = np.random.randn(len(self.weights))
        perturbation = perturbation / np.linalg.norm(perturbation)  # Normalize direction
        
        # Scale perturbation by the actual ratio
        perturbation = perturbation * actual_ratio * np.linalg.norm(self.weights)
        
        # Create new perturbed weights
        new_weights = self.weights + perturbation
        
        # Ensure no negative weights (use absolute value or clip to small positive)
        new_weights = np.abs(new_weights)
        
        return WeightVector(new_weights, normalize=True)
    
    def to_dict(self) -> Dict[str, float]:
        """Convert to dictionary mapping weight names to values"""
        # Denormalize for easier interpretation (scale back up)
        denorm_weights = self.weights * 100.0  # Scale factor for readability
        return {name: float(weight) for name, weight in zip(self.WEIGHT_NAMES, denorm_weights)}
    
    def to_cpp_format(self) -> str:
        """Generate C++ code snippet for Weights struct initialization"""
        denorm_weights = self.weights * 100.0  # Scale factor
        weights_dict = self.to_dict()
        
        cpp_lines = []
        for name in self.WEIGHT_NAMES:
            cpp_lines.append(f"        double {name} = {weights_dict[name]:.6f};")
        
        return "\n".join(cpp_lines)
    
    def __str__(self) -> str:
        return f"WeightVector(norm={np.linalg.norm(self.weights):.6f}, weights={self.weights})"


class WeightTrainer:
    """Hill climbing trainer for heuristic weights"""
    
    def __init__(self, 
                 cpp_dir: str,
                 games_per_eval: int = 10,
                 max_iterations: int = 100,
                 perturbation_ratio: float = 0.075):
        """
        Initialize the weight trainer
        
        Args:
            cpp_dir: Directory containing C++ files
            games_per_eval: Number of games to play for each evaluation
            max_iterations: Maximum number of hill climbing iterations
            perturbation_ratio: Ratio for weight perturbation
        """
        self.cpp_dir = Path(cpp_dir)
        self.games_per_eval = games_per_eval
        self.max_iterations = max_iterations
        self.perturbation_ratio = perturbation_ratio
        
        self.best_weights = WeightVector()
        self.best_fitness = None
        self.history = []
        
    def update_cpp_weights(self, weights: WeightVector) -> bool:
        """
        Update the weights in heuristics.h
        
        Args:
            weights: WeightVector to write to file
            
        Returns:
            True if successful
        """
        heuristics_h = self.cpp_dir / "heuristics.h"
        
        if not heuristics_h.exists():
            print(f"Error: {heuristics_h} not found!")
            return False
        
        # Read the file
        with open(heuristics_h, 'r') as f:
            content = f.read()
        
        # Generate new weights section
        weights_dict = weights.to_dict()
        
        # Build the replacement string
        new_weights_lines = []
        for name in WeightVector.WEIGHT_NAMES:
            new_weights_lines.append(f"        double {name} = {weights_dict[name]:.6f};")
        
        new_weights_section = "\n".join(new_weights_lines)
        
        # Replace the weights in the struct
        import re
        pattern = r'(struct Weights \{)(.*?)(^\s*\};)'
        
        def replacer(match):
            return f"{match.group(1)}\n{new_weights_section}\n    {match.group(3)}"
        
        new_content = re.sub(pattern, replacer, content, flags=re.DOTALL | re.MULTILINE)
        
        # Write back
        with open(heuristics_h, 'w') as f:
            f.write(new_content)
        
        return True
    
    def compile_cpp(self) -> bool:
        """
        Compile the C++ code
        
        Returns:
            True if compilation successful
        """
        try:
            result = subprocess.run(
                ['make', 'clean'],
                cwd=self.cpp_dir,
                capture_output=True,
                text=True,
                timeout=30
            )
            
            result = subprocess.run(
                ['make'],
                cwd=self.cpp_dir,
                capture_output=True,
                text=True,
                timeout=60
            )
            
            if result.returncode != 0:
                print(f"Compilation failed:\n{result.stderr}")
                return False
            
            return True
            
        except Exception as e:
            print(f"Compilation error: {e}")
            return False
    
    def evaluate_fitness(self, weights: WeightVector) -> float:
        """
        Evaluate fitness of a weight vector by playing games
        
        This is a placeholder - you'll need to implement game playing logic
        For now, we'll use a simple heuristic based on weight balance
        
        Args:
            weights: WeightVector to evaluate
            
        Returns:
            Fitness score (higher is better)
        """
        # ============================================================
        # WARNING: MOCK FITNESS - NOT REAL GAMES!
        # ============================================================
        # This is a PLACEHOLDER fitness function for testing the hill
        # climbing algorithm. It does NOT play actual games!
        #
        # To use REAL game evaluation, see simple_game_evaluator.py
        # and integrate it here.
        # ============================================================
        
        # Mock fitness calculation (FAST but NOT REAL)
        w = weights.weights
        
        # Check if weights are too concentrated in one dimension
        max_weight = np.max(np.abs(w))
        mean_weight = np.mean(np.abs(w))
        
        # Fitness is higher when weights are more balanced
        balance_score = mean_weight / (max_weight + 1e-10)
        
        # Add some randomness to simulate game variance
        noise = random.gauss(0, 0.1)
        
        fitness = balance_score + noise
        
        print(f"  Mock Fitness: {fitness:.4f} ⚠️  NOT REAL GAMES!")
        
        return fitness
    
    def train(self):
        """Run hill climbing training"""
        print("=" * 60)
        print("Weight Training using Hill Climbing")
        print("=" * 60)
        print(f"Max iterations: {self.max_iterations}")
        print(f"Games per evaluation: {self.games_per_eval}")
        print(f"Perturbation range: 5-10%")
        print("=" * 60)
        print()
        print("!" * 60)
        print("WARNING: USING MOCK FITNESS - NOT REAL GAMES!")
        print("!" * 60)
        print("The current fitness function does NOT play actual games.")
        print("It uses a simple mathematical formula for testing purposes.")
        print()
        print("To use REAL game-based evaluation:")
        print("  1. See: simple_game_evaluator.py")
        print("  2. Integrate it into evaluate_fitness() method")
        print("  3. Each evaluation will then play actual games")
        print("!" * 60)
        print()
        
        # Initialize with current weights
        current_weights = self.best_weights.copy()
        current_fitness = self.evaluate_fitness(current_weights)
        self.best_fitness = current_fitness
        
        print(f"Initial fitness: {current_fitness:.4f}")
        print(f"Initial weights: {current_weights.to_dict()}")
        print()
        
        improvements = 0
        no_improvement_count = 0
        
        for iteration in range(self.max_iterations):
            print(f"Iteration {iteration + 1}/{self.max_iterations}")
            print("-" * 60)
            
            # Generate neighbor by perturbation
            neighbor_weights = current_weights.perturb(self.perturbation_ratio)
            
            print(f"Testing perturbed weights...")
            neighbor_fitness = self.evaluate_fitness(neighbor_weights)
            
            # Hill climbing: accept if better
            if neighbor_fitness > current_fitness:
                print(f"✓ Improvement found! {current_fitness:.4f} -> {neighbor_fitness:.4f}")
                current_weights = neighbor_weights
                current_fitness = neighbor_fitness
                improvements += 1
                no_improvement_count = 0
                
                # Update best if this is the best so far
                if current_fitness > self.best_fitness:
                    self.best_weights = current_weights.copy()
                    self.best_fitness = current_fitness
                    print(f"  New best fitness: {self.best_fitness:.4f}")
            else:
                print(f"✗ No improvement: {neighbor_fitness:.4f} <= {current_fitness:.4f}")
                no_improvement_count += 1
            
            # Record history
            self.history.append({
                'iteration': iteration + 1,
                'fitness': current_fitness,
                'best_fitness': self.best_fitness,
                'weights': current_weights.to_dict(),
            })
            
            print()
            
            # Early stopping if no improvement for too long
            if no_improvement_count >= 20:
                print(f"Early stopping: no improvement for {no_improvement_count} iterations")
                break
        
        print("=" * 60)
        print("Training Complete!")
        print("=" * 60)
        print(f"Total improvements: {improvements}/{iteration + 1}")
        print(f"Best fitness: {self.best_fitness:.4f}")
        print(f"Best weights: {self.best_weights.to_dict()}")
        print()
        
        # Save results
        self.save_results()
        
        return self.best_weights, self.best_fitness
    
    def save_results(self):
        """Save training results to files"""
        results_dir = self.cpp_dir / "weight_training_results"
        results_dir.mkdir(exist_ok=True)
        
        # Save best weights
        with open(results_dir / "best_weights.json", 'w') as f:
            json.dump({
                'fitness': self.best_fitness,
                'weights': self.best_weights.to_dict(),
                'weights_normalized': self.best_weights.weights.tolist(),
                'norm': float(np.linalg.norm(self.best_weights.weights)),
            }, f, indent=2)
        
        # Save training history
        with open(results_dir / "training_history.json", 'w') as f:
            json.dump(self.history, f, indent=2)
        
        # Save C++ format
        with open(results_dir / "best_weights.cpp", 'w') as f:
            f.write("// Best weights found by hill climbing\n")
            f.write("// Fitness: {:.4f}\n\n".format(self.best_fitness))
            f.write("struct Weights {\n")
            f.write(self.best_weights.to_cpp_format())
            f.write("\n};\n")
        
        print(f"Results saved to {results_dir}")


def main():
    parser = argparse.ArgumentParser(description='Train heuristic weights using hill climbing')
    parser.add_argument('--cpp-dir', type=str, default='c++_sample_files',
                        help='Directory containing C++ files')
    parser.add_argument('--iterations', type=int, default=100,
                        help='Maximum number of iterations')
    parser.add_argument('--games-per-eval', type=int, default=10,
                        help='Number of games to play per evaluation')
    parser.add_argument('--perturbation', type=float, default=0.075,
                        help='Perturbation ratio (default: 0.075 for 5-10%% range)')
    
    args = parser.parse_args()
    
    # Create trainer
    trainer = WeightTrainer(
        cpp_dir=args.cpp_dir,
        games_per_eval=args.games_per_eval,
        max_iterations=args.iterations,
        perturbation_ratio=args.perturbation
    )
    
    # Run training
    best_weights, best_fitness = trainer.train()
    
    print("\nTraining completed successfully!")
    print(f"Apply the best weights from: {args.cpp_dir}/weight_training_results/best_weights.cpp")


if __name__ == '__main__':
    main()
