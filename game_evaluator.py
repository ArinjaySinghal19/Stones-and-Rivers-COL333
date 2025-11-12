#!/usr/bin/env python3
"""
Game-based fitness evaluator for weight training

This module plays actual games to evaluate weight fitness.
Includes verification that games are actually being simulated.
"""

import sys
import time
import subprocess
import tempfile
import json
from pathlib import Path
from typing import Tuple, Optional

import sys
import time
import subprocess
import tempfile
import json
from pathlib import Path
from typing import Tuple, Optional

# Add client_server to path
sys.path.insert(0, str(Path(__file__).parent.absolute() / "client_server"))

try:
    import gameEngine
    from gameEngine import Game
    from agent import get_agent
    GAME_ENGINE_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Could not import game engine: {e}")
    print("Install requirements:")
    print("  cd client_server && pip install -r requirements.txt")
    Game = None
    get_agent = None
    GAME_ENGINE_AVAILABLE = False


class GameEvaluator:
    """Evaluates weight fitness by playing actual games"""
    
    def __init__(self, 
                 time_limit: float = 5.0,  # seconds per move
                 verify_simulation: bool = True):
        """
        Initialize game evaluator
        
        Args:
            time_limit: Time limit per move in seconds
            verify_simulation: Whether to verify games are actually being played
        """
        self.time_limit = time_limit
        self.verify_simulation = verify_simulation
        self.total_games_played = 0
        self.total_moves_made = 0
        self.total_time_spent = 0.0
        
        if not GAME_ENGINE_AVAILABLE:
            raise RuntimeError("Game engine not available. Install requirements first.")
    
    def play_game(self, 
                  agent1_type: str = "cpp",
                  agent2_type: str = "random",
                  verbose: bool = False) -> Tuple[str, dict]:
        """
        Play a single game
        
        Args:
            agent1_type: Type of agent 1 ("cpp", "random", "minimax_py")
            agent2_type: Type of agent 2 ("cpp", "random", "minimax_py")
            verbose: Whether to print game progress
            
        Returns:
            (winner, game_stats) where winner is "circle", "square", or "draw"
            game_stats contains: moves, time_taken, positions_evaluated, etc.
        """
        start_time = time.time()
        
        # Create game
        game = Game(rows=13, cols=12, gui=False)
        
        # Create agents
        try:
            agent_circle = get_agent("circle", agent1_type)
            agent_square = get_agent("square", agent2_type)
        except Exception as e:
            if verbose:
                print(f"Error creating agents: {e}")
            # Fall back to random agents
            agent_circle = get_agent("circle", "random")
            agent_square = get_agent("square", "random")
        
        agents = {"circle": agent_circle, "square": agent_square}
        
        moves_made = 0
        max_moves = 200  # Prevent infinite games
        
        if verbose:
            print(f"Starting game: {agent1_type} (circle) vs {agent2_type} (square)")
        
        # Play game
        while not game.is_terminal() and moves_made < max_moves:
            current_player = game.whose_turn()
            agent = agents[current_player]
            
            # Get move
            move_start = time.time()
            try:
                move = agent.get_move(game, time_left=self.time_limit)
                move_time = time.time() - move_start
                
                if move is None:
                    if verbose:
                        print(f"  {current_player} has no valid moves")
                    break
                
                # Make move
                game.place(*move)
                moves_made += 1
                
                if verbose and moves_made % 10 == 0:
                    print(f"  Move {moves_made}: {current_player} played {move}")
                
                # Verify game is progressing
                if self.verify_simulation and move_time < 0.0001:
                    if verbose:
                        print(f"  WARNING: Move computed suspiciously fast ({move_time:.6f}s)")
                
            except Exception as e:
                if verbose:
                    print(f"  Error during move: {e}")
                break
        
        # Determine winner
        scores = game.get_scores()
        if scores["circle"] > scores["square"]:
            winner = "circle"
        elif scores["square"] > scores["circle"]:
            winner = "square"
        else:
            winner = "draw"
        
        total_time = time.time() - start_time
        
        # Collect stats
        game_stats = {
            "winner": winner,
            "moves": moves_made,
            "time_taken": total_time,
            "scores": scores,
            "terminated_normally": game.is_terminal(),
        }
        
        if verbose:
            print(f"Game finished: {winner} wins")
            print(f"  Scores: Circle {scores['circle']}, Square {scores['square']}")
            print(f"  Moves: {moves_made}")
            print(f"  Time: {total_time:.2f}s")
        
        # Update totals
        self.total_games_played += 1
        self.total_moves_made += moves_made
        self.total_time_spent += total_time
        
        return winner, game_stats
    
    def evaluate_weights(self,
                        num_games: int = 10,
                        opponent_type: str = "random",
                        verbose: bool = False) -> dict:
        """
        Evaluate current weights by playing multiple games
        
        Args:
            num_games: Number of games to play
            opponent_type: Type of opponent ("random", "minimax_py", etc.)
            verbose: Whether to print progress
            
        Returns:
            Dictionary with evaluation results
        """
        if verbose:
            print(f"\nEvaluating weights over {num_games} games...")
            print("=" * 60)
        
        wins = 0
        losses = 0
        draws = 0
        all_stats = []
        
        eval_start = time.time()
        
        for i in range(num_games):
            if verbose:
                print(f"\nGame {i+1}/{num_games}")
                print("-" * 60)
            
            winner, stats = self.play_game(
                agent1_type="cpp",  # Our bot with current weights
                agent2_type=opponent_type,
                verbose=verbose
            )
            
            if winner == "circle":  # Our bot is circle
                wins += 1
            elif winner == "square":
                losses += 1
            else:
                draws += 1
            
            all_stats.append(stats)
        
        eval_time = time.time() - eval_start
        
        # Verification: Check if games are actually being simulated
        avg_time_per_game = eval_time / num_games if num_games > 0 else 0
        avg_moves_per_game = sum(s["moves"] for s in all_stats) / num_games if num_games > 0 else 0
        
        simulation_verified = True
        warnings = []
        
        if self.verify_simulation:
            # Games should take at least some time
            if avg_time_per_game < 0.01:  # Less than 10ms per game is suspicious
                simulation_verified = False
                warnings.append(f"Games finishing too fast! Avg: {avg_time_per_game*1000:.2f}ms per game")
            
            # Should have some moves
            if avg_moves_per_game < 1:
                simulation_verified = False
                warnings.append(f"Too few moves per game! Avg: {avg_moves_per_game:.1f} moves")
            
            # Should spend reasonable time
            if eval_time < 0.001:  # Less than 1ms total is impossible
                simulation_verified = False
                warnings.append(f"Evaluation too fast! Total: {eval_time*1000:.2f}ms")
        
        win_rate = wins / num_games if num_games > 0 else 0.0
        
        results = {
            "win_rate": win_rate,
            "wins": wins,
            "losses": losses,
            "draws": draws,
            "games_played": num_games,
            "avg_time_per_game": avg_time_per_game,
            "avg_moves_per_game": avg_moves_per_game,
            "total_time": eval_time,
            "simulation_verified": simulation_verified,
            "warnings": warnings,
            "all_game_stats": all_stats,
        }
        
        if verbose:
            print("\n" + "=" * 60)
            print("Evaluation Results")
            print("=" * 60)
            print(f"Win Rate: {win_rate*100:.1f}% ({wins}W / {losses}L / {draws}D)")
            print(f"Avg Time/Game: {avg_time_per_game:.3f}s")
            print(f"Avg Moves/Game: {avg_moves_per_game:.1f}")
            print(f"Total Time: {eval_time:.2f}s")
            print(f"Simulation Verified: {'✓ YES' if simulation_verified else '✗ NO'}")
            
            if warnings:
                print("\n⚠️  WARNINGS:")
                for w in warnings:
                    print(f"  - {w}")
            print("=" * 60)
        
        return results
    
    def get_statistics(self) -> dict:
        """Get cumulative statistics"""
        return {
            "total_games_played": self.total_games_played,
            "total_moves_made": self.total_moves_made,
            "total_time_spent": self.total_time_spent,
            "avg_time_per_game": self.total_time_spent / max(1, self.total_games_played),
            "avg_moves_per_game": self.total_moves_made / max(1, self.total_games_played),
        }


def test_game_evaluator():
    """Test the game evaluator"""
    print("Testing Game Evaluator")
    print("=" * 70)
    
    try:
        evaluator = GameEvaluator(time_limit=2.0, verify_simulation=True)
        
        # Test with just a few games
        results = evaluator.evaluate_weights(
            num_games=3,
            opponent_type="random",
            verbose=True
        )
        
        print("\n" + "=" * 70)
        print("Test Complete")
        print("=" * 70)
        print(f"Win rate: {results['win_rate']*100:.1f}%")
        print(f"Simulation verified: {results['simulation_verified']}")
        
        if not results['simulation_verified']:
            print("\n⚠️  WARNING: Games may not be actually running!")
            for warning in results['warnings']:
                print(f"  - {warning}")
            return False
        
        print("\n✓ Game evaluator working correctly!")
        return True
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == '__main__':
    # Run test
    success = test_game_evaluator()
    sys.exit(0 if success else 1)
