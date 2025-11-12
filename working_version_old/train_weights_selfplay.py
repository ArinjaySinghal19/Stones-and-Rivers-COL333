#!/usr/bin/env python3
"""
Hill Climbing Weight Training with Real Self-Play

This script performs hill climbing optimization where:
- Perturbed weights play against original weights (self-play)
- If perturbed weights win, they become the new current weights
- Uses actual game simulation with C++ compilation
- Can use GUI for verification or CLI for fast training
"""

import sys
import time
import json
from pathlib import Path
import argparse

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))

from train_weights import WeightVector
from verify_real_games import RealWeightTrainer


class HillClimbingTrainer:
    """
    Hill climbing trainer using real self-play evaluation
    """
    
    def __init__(self,
                 cpp_dir: str = "c++_sample_files",
                 games_per_eval: int = 4,
                 max_iterations: int = 50,
                 use_gui: bool = False):
        """
        Initialize trainer
        
        Args:
            cpp_dir: Directory with C++ files
            games_per_eval: Games to play per evaluation (should be even)
            max_iterations: Maximum iterations
            use_gui: Whether to show GUI (for verification)
        """
        self.cpp_dir = Path(cpp_dir)
        self.games_per_eval = games_per_eval if games_per_eval % 2 == 0 else games_per_eval + 1
        self.max_iterations = max_iterations
        self.use_gui = use_gui
        
        self.trainer = RealWeightTrainer(cpp_dir=cpp_dir, use_gui=use_gui)
        self.best_weights = WeightVector()
        self.best_fitness = 0.5  # Start at 50% (neutral)
        self.history = []
    
    def train(self):
        """
        Run hill climbing training
        """
        print("="*70)
        print("HILL CLIMBING WITH REAL SELF-PLAY")
        print("="*70)
        print(f"Max iterations:      {self.max_iterations}")
        print(f"Games per eval:      {self.games_per_eval}")
        print(f"Mode:                {'GUI (slow)' if self.use_gui else 'CLI (fast)'}")
        print(f"Perturbation:        5-10%")
        print("="*70)
        print()
        print("Strategy: Perturbed weights vs Current weights")
        print("Accept:   If perturbed wins >= 50% of games")
        print()
        print("✓ Using REAL game simulation")
        print("✓ C++ weights updated and recompiled each evaluation")
        print("✓ Actual self-play determines fitness")
        print()
        print("="*70)
        print()
        
        # Start with default weights
        current_weights = self.best_weights.copy()
        current_fitness = self.best_fitness
        
        improvements = 0
        no_improvement_count = 0
        
        for iteration in range(self.max_iterations):
            print(f"\n{'='*70}")
            print(f"ITERATION {iteration + 1}/{self.max_iterations}")
            print(f"{'='*70}")
            print(f"Current fitness: {current_fitness*100:.1f}% win rate")
            print()
            
            # Generate perturbed weights
            perturbed_weights = current_weights.perturb()
            
            print("Generated perturbed weights (5-10% change)")
            print()
            
            # Evaluate via self-play
            results = self.trainer.evaluate_weights_selfplay(
                current_weights=current_weights,
                perturbed_weights=perturbed_weights,
                num_games=self.games_per_eval,
                verbose=True
            )
            
            if "error" in results:
                print(f"\n✗ Error during evaluation: {results['error']}")
                print("Skipping this iteration...")
                continue
            
            perturbed_fitness = results["perturbed_win_rate"]
            
            # Hill climbing: accept if perturbed >= 50% (wins at least half the time)
            print()
            print("-"*70)
            if perturbed_fitness >= 0.5:
                print(f"✓ IMPROVEMENT: {perturbed_fitness*100:.1f}% win rate >= 50%")
                print("  Accepting perturbed weights as new current")
                
                current_weights = perturbed_weights
                current_fitness = perturbed_fitness
                improvements += 1
                no_improvement_count = 0
                
                # Update best if this is better
                if current_fitness > self.best_fitness:
                    self.best_weights = current_weights.copy()
                    self.best_fitness = current_fitness
                    print(f"  ★ NEW BEST: {self.best_fitness*100:.1f}% win rate")
            else:
                print(f"✗ NO IMPROVEMENT: {perturbed_fitness*100:.1f}% win rate < 50%")
                print("  Keeping current weights")
                no_improvement_count += 1
            print("-"*70)
            
            # Record history
            self.history.append({
                "iteration": iteration + 1,
                "perturbed_fitness": perturbed_fitness,
                "current_fitness": current_fitness,
                "best_fitness": self.best_fitness,
                "perturbed_wins": results["perturbed_wins"],
                "current_wins": results["current_wins"],
                "draws": results["draws"],
                "accepted": perturbed_fitness >= 0.5,
            })
            
            # Early stopping
            if no_improvement_count >= 10:
                print(f"\n{'='*70}")
                print(f"EARLY STOPPING: No improvement for {no_improvement_count} iterations")
                print(f"{'='*70}")
                break
        
        # Training complete
        print()
        print("="*70)
        print("TRAINING COMPLETE")
        print("="*70)
        print(f"Total iterations:    {iteration + 1}")
        print(f"Improvements:        {improvements}")
        print(f"Best fitness:        {self.best_fitness*100:.1f}% win rate")
        print(f"Games played:        {self.trainer.games_played}")
        print(f"Total time:          {self.trainer.total_time/60:.1f} minutes")
        print("="*70)
        print()
        
        # Save results
        self.save_results()
        
        # Restore best weights
        print("Restoring best weights to heuristics.h...")
        self.trainer.update_cpp_weights(self.best_weights)
        self.trainer.compile_cpp()
        print("✓ Done")
        print()
        
        return self.best_weights, self.best_fitness
    
    def save_results(self):
        """Save training results"""
        results_dir = self.cpp_dir / "weight_training_results"
        results_dir.mkdir(exist_ok=True)
        
        # Save best weights
        with open(results_dir / "best_weights_selfplay.json", 'w') as f:
            json.dump({
                'fitness': self.best_fitness,
                'win_rate_percent': self.best_fitness * 100,
                'weights': self.best_weights.to_dict(),
                'weights_normalized': self.best_weights.weights.tolist(),
                'games_played': self.trainer.games_played,
                'total_time_minutes': self.trainer.total_time / 60,
            }, f, indent=2)
        
        # Save history
        with open(results_dir / "training_history_selfplay.json", 'w') as f:
            json.dump(self.history, f, indent=2)
        
        # Save C++ format
        with open(results_dir / "best_weights_selfplay.cpp", 'w') as f:
            f.write("// Best weights found by hill climbing with self-play\n")
            f.write(f"// Win rate: {self.best_fitness*100:.1f}%\n")
            f.write(f"// Games played: {self.trainer.games_played}\n\n")
            f.write("struct Weights {\n")
            f.write(self.best_weights.to_cpp_format())
            f.write("\n};\n")
        
        print(f"✓ Results saved to {results_dir}")
        print(f"  - best_weights_selfplay.json")
        print(f"  - best_weights_selfplay.cpp")
        print(f"  - training_history_selfplay.json")


def main():
    parser = argparse.ArgumentParser(
        description='Train weights using hill climbing with real self-play'
    )
    parser.add_argument('--iterations', type=int, default=20,
                        help='Maximum number of iterations (default: 20)')
    parser.add_argument('--games-per-eval', type=int, default=4,
                        help='Games per evaluation, must be even (default: 4)')
    parser.add_argument('--gui', action='store_true',
                        help='Use GUI mode (slow, for verification)')
    parser.add_argument('--cpp-dir', type=str, default='c++_sample_files',
                        help='C++ directory (default: c++_sample_files)')
    
    args = parser.parse_args()
    
    # Verify setup
    if args.gui:
        print()
        print("="*70)
        print("GUI MODE ENABLED")
        print("="*70)
        print()
        print("You will see game windows for each game.")
        print("This is SLOW but lets you verify games are actually running.")
        print()
        print("For actual training, use without --gui flag (CLI mode).")
        print("="*70)
        print()
        input("Press Enter to continue...")
    
    # Create trainer
    trainer = HillClimbingTrainer(
        cpp_dir=args.cpp_dir,
        games_per_eval=args.games_per_eval,
        max_iterations=args.iterations,
        use_gui=args.gui
    )
    
    # Train
    best_weights, best_fitness = trainer.train()
    
    print()
    print("="*70)
    print("SUCCESS!")
    print("="*70)
    print()
    print(f"Best weights achieved {best_fitness*100:.1f}% win rate in self-play")
    print(f"Results saved to: {args.cpp_dir}/weight_training_results/")
    print()
    print("The best weights have been applied to heuristics.h")
    print("Your bot now uses the optimized weights!")
    print()
    print("="*70)


if __name__ == '__main__':
    main()
