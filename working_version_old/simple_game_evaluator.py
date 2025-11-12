#!/usr/bin/env python3
"""
Simple game-based fitness evaluation using actual bot matches

This module provides a REAL fitness evaluation by running actual games
and measuring win rates. Includes verification that games are being simulated.
"""

import subprocess
import time
import json
import tempfile
import os
from pathlib import Path
from typing import Tuple, Dict, List


class SimpleGameEvaluator:
    """Evaluates weight fitness by playing actual bot vs bot games"""
    
    def __init__(self):
        self.total_games = 0
        self.total_time = 0.0
        self.verification_enabled = True
    
    def play_single_game_via_cli(self, 
                                   player1: str = "cpp",
                                   player2: str = "random",
                                   time_limit: float = 5.0,
                                   verbose: bool = False) -> Tuple[str, Dict]:
        """
        Play a single game by invoking the CLI game engine
        
        Returns:
            (winner, stats) where winner is "circle", "square", or "draw"
        """
        start_time = time.time()
        
        # Run game via CLI
        cmd = [
            "python",
            "client_server/gameEngine.py",
            "--mode", "cli",
            "--circle", player1,
            "--square", player2,
            "--time", str(time_limit),
            "--no-gui"
        ]
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=time_limit * 100 + 30,  # Give plenty of time
                cwd=Path(__file__).parent
            )
            
            output = result.stdout + result.stderr
            
            # Parse winner from output
            winner = "draw"
            if "circle wins" in output.lower():
                winner = "circle"
            elif "square wins" in output.lower():
                winner = "square"
            
            elapsed = time.time() - start_time
            
            stats = {
                "winner": winner,
                "time_taken": elapsed,
                "output_lines": len(output.split('\n')),
            }
            
            if verbose:
                print(f"Game completed: {winner} wins (took {elapsed:.2f}s)")
            
            self.total_games += 1
            self.total_time += elapsed
            
            return winner, stats
            
        except subprocess.TimeoutExpired:
            if verbose:
                print("Game timed out!")
            return "draw", {"winner": "draw", "time_taken": time.time() - start_time, "timeout": True}
        except Exception as e:
            if verbose:
                print(f"Error playing game: {e}")
            return "draw", {"winner": "draw", "time_taken": time.time() - start_time, "error": str(e)}
    
    def evaluate_weights(self,
                        num_games: int = 10,
                        opponent: str = "random",
                        time_per_move: float = 2.0,
                        verbose: bool = False) -> Dict:
        """
        Evaluate fitness by playing multiple games
        
        Args:
            num_games: Number of games to play
            opponent: Opponent type ("random", "minimax_py", etc.)
            time_per_move: Time limit per move
            verbose: Print progress
            
        Returns:
            Evaluation results with verification
        """
        if verbose:
            print(f"\n{'='*70}")
            print(f"Evaluating weights over {num_games} games vs {opponent}")
            print(f"{'='*70}\n")
        
        wins = 0
        losses = 0
        draws = 0
        all_stats = []
        
        eval_start = time.time()
        
        for i in range(num_games):
            if verbose:
                print(f"Game {i+1}/{num_games}...", end=" ", flush=True)
            
            winner, stats = self.play_single_game_via_cli(
                player1="cpp",  # Our bot with current weights
                player2=opponent,
                time_limit=time_per_move,
                verbose=False
            )
            
            if winner == "circle":  # Our bot
                wins += 1
                if verbose:
                    print("WIN ✓")
            elif winner == "square":
                losses += 1
                if verbose:
                    print("LOSS ✗")
            else:
                draws += 1
                if verbose:
                    print("DRAW =")
            
            all_stats.append(stats)
        
        total_eval_time = time.time() - eval_start
        avg_time = total_eval_time / num_games if num_games > 0 else 0
        
        # VERIFICATION: Check if games are actually being simulated
        simulation_verified = True
        warnings = []
        
        if self.verification_enabled:
            # Games should take reasonable time
            if avg_time < 0.01:  # Less than 10ms per game
                simulation_verified = False
                warnings.append(
                    f"⚠️  Games finishing impossibly fast! "
                    f"Average: {avg_time*1000:.2f}ms per game"
                )
            
            # Total time should be substantial
            if total_eval_time < 0.001:  # Less than 1ms total
                simulation_verified = False
                warnings.append(
                    f"⚠️  Evaluation too fast! Total: {total_eval_time*1000:.2f}ms for {num_games} games"
                )
            
            # Check if output indicates actual games
            total_output_lines = sum(s.get("output_lines", 0) for s in all_stats)
            avg_output = total_output_lines / num_games if num_games > 0 else 0
            
            if avg_output < 5:  # Very little output suggests no actual game
                simulation_verified = False
                warnings.append(
                    f"⚠️  Very little game output! Average: {avg_output:.1f} lines per game"
                )
        
        win_rate = wins / num_games if num_games > 0 else 0.0
        
        results = {
            "win_rate": win_rate,
            "wins": wins,
            "losses": losses,
            "draws": draws,
            "games_played": num_games,
            "avg_time_per_game": avg_time,
            "total_time": total_eval_time,
            "simulation_verified": simulation_verified,
            "warnings": warnings,
            "all_stats": all_stats,
        }
        
        if verbose:
            print(f"\n{'='*70}")
            print("EVALUATION RESULTS")
            print(f"{'='*70}")
            print(f"Win Rate:        {win_rate*100:.1f}% ({wins}W / {losses}L / {draws}D)")
            print(f"Avg Time/Game:   {avg_time:.2f}s")
            print(f"Total Time:      {total_eval_time:.2f}s")
            print(f"Verified:        {'✓ YES' if simulation_verified else '✗ NO - MOCK FITNESS'}")
            
            if warnings:
                print(f"\n{'='*70}")
                print("WARNINGS - GAMES MAY NOT BE REAL:")
                print(f"{'='*70}")
                for w in warnings:
                    print(f"  {w}")
            
            print(f"{'='*70}\n")
        
        return results
    
    def get_statistics(self) -> Dict:
        """Get cumulative statistics"""
        return {
            "total_games": self.total_games,
            "total_time": self.total_time,
            "avg_time_per_game": self.total_time / max(1, self.total_games),
        }


def test_evaluator():
    """Test the evaluator with a small number of games"""
    print("="*70)
    print("TESTING GAME EVALUATOR")
    print("="*70)
    print("\nThis will play a few REAL games to verify the system works.")
    print("Expected: Each game should take 1-5+ seconds\n")
    
    evaluator = SimpleGameEvaluator()
    
    # Play just 2 games as a test
    results = evaluator.evaluate_weights(
        num_games=2,
        opponent="random",
        time_per_move=2.0,
        verbose=True
    )
    
    if not results["simulation_verified"]:
        print("\n" + "!"*70)
        print("ERROR: GAMES ARE NOT BEING SIMULATED!")
        print("!"*70)
        print("\nThe 'fitness' calculation is fake/mock.")
        print("You need to implement real game playing.")
        print("!"*70 + "\n")
        return False
    else:
        print("\n" + "="*70)
        print("✓ SUCCESS: Games are actually being simulated!")
        print("="*70)
        print(f"\nPlayed {results['games_played']} games")
        print(f"Total time: {results['total_time']:.2f}s")
        print(f"Avg time/game: {results['avg_time_per_game']:.2f}s")
        print(f"Win rate: {results['win_rate']*100:.1f}%")
        print("\nThis evaluator can be used for weight training.")
        print("="*70 + "\n")
        return True


if __name__ == "__main__":
    import sys
    success = test_evaluator()
    sys.exit(0 if success else 1)
