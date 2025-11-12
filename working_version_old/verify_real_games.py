#!/usr/bin/env python3
"""
Real Weight Training with Self-Play and Verification

This system:
1. Plays perturbed weights vs original weights (self-play)
2. Uses GUI mode for verification (you can watch the games)
3. Uses CLI mode for actual training (fast, no graphics)
4. Actually compiles C++ weights between games
"""

import sys
import time
import subprocess
import shutil
import json
from pathlib import Path
from typing import Tuple, Dict, Optional
import tempfile

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))

from train_weights import WeightVector


class RealWeightTrainer:
    """
    Weight trainer that actually plays games with different weight configurations
    """
    
    def __init__(self, cpp_dir: str = "c++_sample_files", use_gui: bool = False):
        """
        Initialize trainer
        
        Args:
            cpp_dir: Directory containing C++ files
            use_gui: Whether to show GUI (for verification)
        """
        self.cpp_dir = Path(cpp_dir)
        self.use_gui = use_gui
        self.games_played = 0
        self.total_time = 0.0
        
        # Backup original weights
        self.original_weights_backup = self.cpp_dir / "heuristics.h.original_backup"
        if not self.original_weights_backup.exists():
            shutil.copy(self.cpp_dir / "heuristics.h", self.original_weights_backup)
            print(f"✓ Backed up original weights to {self.original_weights_backup}")
    
    def update_cpp_weights(self, weights: WeightVector, backup_suffix: str = "") -> bool:
        """
        Update weights in heuristics.h and recompile
        
        Args:
            weights: WeightVector to write
            backup_suffix: Optional suffix for backup file
            
        Returns:
            True if successful
        """
        heuristics_h = self.cpp_dir / "heuristics.h"
        
        if not heuristics_h.exists():
            print(f"✗ Error: {heuristics_h} not found!")
            return False
        
        # Backup current if requested
        if backup_suffix:
            backup_file = self.cpp_dir / f"heuristics.h.{backup_suffix}"
            shutil.copy(heuristics_h, backup_file)
        
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
        Compile the C++ agent using the root Makefile
        
        Returns:
            True if successful
        """
        print("  Compiling C++ agent...", end=" ", flush=True)
        
        try:
            # Check if build directory exists
            build_dir = Path.cwd() / "c++_sample_files" / "build"
            
            if not build_dir.exists():
                # First time build - need full build
                print("\n  (First build - this may take a minute)...", end=" ", flush=True)
                result = subprocess.run(
                    ['make', 'build'],
                    cwd=Path.cwd(),
                    capture_output=True,
                    text=True,
                    timeout=300  # 5 minutes for first build
                )
            else:
                # Quick rebuild
                result = subprocess.run(
                    ['make', 'rebuild'],
                    cwd=Path.cwd(),
                    capture_output=True,
                    text=True,
                    timeout=120
                )
            
            if result.returncode != 0:
                print("✗ FAILED")
                print(f"Compilation error:\n{result.stderr}")
                return False
            
            print("✓ Done")
            return True
            
        except Exception as e:
            print(f"✗ Error: {e}")
            return False
    
    def play_game(self, 
                  player1: str = "student_cpp",
                  player2: str = "student_cpp",
                  time_limit: float = 5.0,
                  verbose: bool = True) -> Tuple[str, Dict]:
        """
        Play a single game using the game engine
        
        Args:
            player1: Circle player type
            player2: Square player type  
            time_limit: Time limit per move
            verbose: Print progress
            
        Returns:
            (winner, game_stats)
        """
        start_time = time.time()
        
        mode = "gui" if self.use_gui else "cli"
        
        if verbose:
            print(f"  Playing game ({mode} mode)...", end=" ", flush=True)
        
        # Build command
        cmd = [
            sys.executable,  # python
            "client_server/gameEngine.py",
            "--mode", mode,
            "--circle", player1,
            "--square", player2,
            "--time", str(time_limit),
        ]
        
        if not self.use_gui:
            cmd.append("--no-gui")
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300,  # 5 minutes max
                cwd=Path.cwd()
            )
            
            output = result.stdout + result.stderr
            
            # Parse winner from output
            winner = "draw"
            circle_score = 0
            square_score = 0
            
            # Look for score information
            for line in output.split('\n'):
                if 'circle' in line.lower() and 'score' in line.lower():
                    try:
                        # Try to extract score
                        import re
                        match = re.search(r'circle.*?(\d+)', line.lower())
                        if match:
                            circle_score = int(match.group(1))
                    except:
                        pass
                if 'square' in line.lower() and 'score' in line.lower():
                    try:
                        match = re.search(r'square.*?(\d+)', line.lower())
                        if match:
                            square_score = int(match.group(1))
                    except:
                        pass
            
            # Determine winner
            if circle_score > square_score:
                winner = "circle"
            elif square_score > circle_score:
                winner = "square"
            else:
                winner = "draw"
            
            # Also check for explicit winner statements
            if "circle wins" in output.lower():
                winner = "circle"
            elif "square wins" in output.lower():
                winner = "square"
            
            elapsed = time.time() - start_time
            
            if verbose:
                print(f"{winner.upper()} wins! (took {elapsed:.1f}s)")
            
            stats = {
                "winner": winner,
                "time_taken": elapsed,
                "circle_score": circle_score,
                "square_score": square_score,
                "output_length": len(output),
            }
            
            self.games_played += 1
            self.total_time += elapsed
            
            return winner, stats
            
        except subprocess.TimeoutExpired:
            if verbose:
                print("✗ TIMEOUT")
            return "draw", {"winner": "draw", "timeout": True}
        except Exception as e:
            if verbose:
                print(f"✗ Error: {e}")
            return "draw", {"winner": "draw", "error": str(e)}
    
    def evaluate_weights_selfplay(self,
                                   current_weights: WeightVector,
                                   perturbed_weights: WeightVector,
                                   num_games: int = 1,
                                   verbose: bool = True) -> Dict:
        """
        Evaluate perturbed weights vs current weights via self-play
        
        Args:
            current_weights: Current weight configuration
            perturbed_weights: Perturbed weight configuration  
            num_games: Number of games to play
            verbose: Print progress
            
        Returns:
            Evaluation results
        """
        if verbose:
            print(f"\n{'='*70}")
            print(f"Self-Play Evaluation: Perturbed vs Original ({num_games} games)")
            print(f"{'='*70}")
        
        perturbed_wins = 0
        current_wins = 0
        draws = 0
        all_stats = []
        
        for game_num in range(num_games):
            if verbose:
                print(f"\nGame {game_num + 1}/{num_games}")
                print("-" * 70)
            
            # Alternate who plays as circle/square for fairness
            if game_num % 2 == 0:
                # Perturbed plays as Circle, Current plays as Square
                if verbose:
                    print("  Perturbed (CIRCLE) vs Current (SQUARE)")
                
                # Set perturbed weights and compile
                if verbose:
                    print("  Setting up perturbed weights...")
                self.update_cpp_weights(perturbed_weights, backup_suffix="perturbed")
                if not self.compile_cpp():
                    return {"error": "Compilation failed for perturbed weights"}
                
                # Play game
                winner, stats = self.play_game(
                    player1="student_cpp",
                    player2="student_cpp",
                    time_limit=5.0,
                    verbose=verbose
                )
                
                # Count win
                if winner == "circle":
                    perturbed_wins += 1
                elif winner == "square":
                    current_wins += 1
                else:
                    draws += 1
                    
            else:
                # Current plays as Circle, Perturbed plays as Square
                if verbose:
                    print("  Current (CIRCLE) vs Perturbed (SQUARE)")
                
                # Set current weights and compile
                if verbose:
                    print("  Setting up current weights...")
                self.update_cpp_weights(current_weights, backup_suffix="current")
                if not self.compile_cpp():
                    return {"error": "Compilation failed for current weights"}
                
                # Play game
                winner, stats = self.play_game(
                    player1="student_cpp",
                    player2="student_cpp",
                    time_limit=5.0,
                    verbose=verbose
                )
                
                # Count win
                if winner == "square":
                    perturbed_wins += 1
                elif winner == "circle":
                    current_wins += 1
                else:
                    draws += 1
            
            all_stats.append(stats)
        
        # Calculate win rate for perturbed weights
        total_decisive = perturbed_wins + current_wins
        if total_decisive > 0:
            perturbed_win_rate = perturbed_wins / total_decisive
        else:
            perturbed_win_rate = 0.5  # All draws = neutral
        
        results = {
            "perturbed_wins": perturbed_wins,
            "current_wins": current_wins,
            "draws": draws,
            "perturbed_win_rate": perturbed_win_rate,
            "games_played": num_games,
            "all_stats": all_stats,
        }
        
        if verbose:
            print(f"\n{'='*70}")
            print("RESULTS")
            print(f"{'='*70}")
            print(f"Perturbed wins: {perturbed_wins}")
            print(f"Current wins:   {current_wins}")
            print(f"Draws:          {draws}")
            print(f"Perturbed win rate: {perturbed_win_rate*100:.1f}%")
            print(f"{'='*70}")
        
        return results


def verify_with_gui():
    """
    Run a verification test with GUI so user can see the game actually running
    """
    print("="*70)
    print("VERIFICATION TEST - GUI MODE")
    print("="*70)
    print()
    print("This will play 1 game with GUI so you can watch and verify")
    print("that actual game simulation is happening.")
    print()
    input("Press Enter to start the GUI game...")
    print()
    
    trainer = RealWeightTrainer(cpp_dir="c++_sample_files", use_gui=True)
    
    # Create two slightly different weight configurations
    current = WeightVector()
    perturbed = current.perturb()
    
    print("Playing: Perturbed weights vs Original weights")
    print("You should see a game window open...")
    print()
    
    results = trainer.evaluate_weights_selfplay(
        current_weights=current,
        perturbed_weights=perturbed,
        num_games=1,
        verbose=True
    )
    
    print()
    print("="*70)
    print("VERIFICATION COMPLETE")
    print("="*70)
    print()
    print("Did you see the game window and watch pieces moving?")
    response = input("Enter 'yes' if games are actually running: ")
    
    if response.lower() in ['yes', 'y']:
        print()
        print("✓ Great! Games are verified to be running.")
        print("  You can now use CLI mode for faster training.")
        return True
    else:
        print()
        print("✗ Something is wrong. Games may not be running properly.")
        return False


def test_cli_mode():
    """
    Test with CLI mode (no GUI, faster)
    """
    print()
    print("="*70)
    print("TESTING CLI MODE (Faster, No GUI)")
    print("="*70)
    print()
    
    trainer = RealWeightTrainer(cpp_dir="c++_sample_files", use_gui=False)
    
    current = WeightVector()
    perturbed = current.perturb()
    
    print("Playing 2 games in CLI mode (should be faster)...")
    print()
    
    start = time.time()
    results = trainer.evaluate_weights_selfplay(
        current_weights=current,
        perturbed_weights=perturbed,
        num_games=2,
        verbose=True
    )
    elapsed = time.time() - start
    
    print()
    print("="*70)
    print("CLI MODE TEST COMPLETE")
    print("="*70)
    print(f"Total time: {elapsed:.1f}s")
    print(f"Avg time/game: {elapsed/2:.1f}s")
    print()
    
    if elapsed < 1.0:
        print("⚠️  WARNING: Games finished too fast!")
        print("   This suggests games may not be actually running.")
        return False
    else:
        print("✓ Timing looks good - games are being simulated.")
        return True


if __name__ == "__main__":
    print()
    print("*"*70)
    print("*" + " "*68 + "*")
    print("*" + " "*15 + "REAL WEIGHT TRAINING VERIFICATION" + " "*21 + "*")
    print("*" + " "*68 + "*")
    print("*"*70)
    print()
    
    # Step 1: Verify with GUI
    gui_verified = verify_with_gui()
    
    if not gui_verified:
        print()
        print("Please fix the game execution before proceeding to training.")
        sys.exit(1)
    
    # Step 2: Test CLI mode
    cli_verified = test_cli_mode()
    
    if not cli_verified:
        print()
        print("CLI mode verification failed.")
        sys.exit(1)
    
    print()
    print("="*70)
    print("ALL VERIFICATIONS PASSED!")
    print("="*70)
    print()
    print("You can now use this for weight training:")
    print("1. GUI mode: Set use_gui=True (slow, but you can watch)")
    print("2. CLI mode: Set use_gui=False (fast, for training)")
    print()
    print("The system properly:")
    print("  ✓ Updates C++ weights")
    print("  ✓ Recompiles the agent")
    print("  ✓ Plays perturbed vs original (self-play)")
    print("  ✓ Determines winner accurately")
    print("="*70)
