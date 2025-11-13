#!/usr/bin/env python3
"""
Single Game Runner for Tournament System

This script runs a single game with proper bot isolation using separate processes.
It's designed to be called by the tournament manager for each match.
"""

import subprocess
import sys
import json
from pathlib import Path
from typing import Dict, Any, Optional


def run_single_game(
    circle_bot: str,
    square_bot: str,
    board_size: str = "small",
    time_per_player: float = 2.0,
    output_json: Optional[str] = None
) -> Dict[str, Any]:
    """
    Run a single game between two bots.
    
    Args:
        circle_bot: Name of the bot playing as circle (e.g., "bot1")
        square_bot: Name of the bot playing as square (e.g., "bot2")
        board_size: "small", "medium", or "large"
        time_per_player: Time limit in minutes per player
        output_json: Optional path to save game result as JSON
    
    Returns:
        Dictionary with game results:
        {
            "winner": "circle" | "square" | None (draw),
            "circle_bot": str,
            "square_bot": str,
            "circle_score": float,
            "square_score": float,
            "circle_time_left": float,
            "square_time_left": float,
            "board_size": str,
            "total_moves": int
        }
    """
    
    # Path to gameEngine.py
    project_root = Path(__file__).parent.parent
    game_engine = project_root / "client_server" / "gameEngine.py"
    
    if not game_engine.exists():
        raise FileNotFoundError(f"gameEngine.py not found at {game_engine}")
    
    # Build command
    cmd = [
        sys.executable,  # Python interpreter
        str(game_engine),
        "--mode", "aivai",
        "--circle", circle_bot,
        "--square", square_bot,
        "--nogui",
        "--no-pause",  # Skip interactive pauses for automated execution
        "--board-size", board_size,
        "--time", str(time_per_player)
    ]
    
    print(f"\n{'='*70}")
    print(f"🎮 Running Game: {circle_bot} (circle) vs {square_bot} (square)")
    print(f"   Board: {board_size}, Time: {time_per_player}min")
    print(f"{'='*70}\n")
    
    # Run the game in a subprocess for complete isolation
    # Each subprocess will load its own .so files independently
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=time_per_player * 60 * 2 + 60,  # Game timeout with buffer
            cwd=str(project_root / "client_server")
        )
        
        output = result.stdout
        errors = result.stderr
        
        # Print output for debugging
        print("STDOUT:")
        print(output)
        if errors:
            print("\nSTDERR:")
            print(errors)
        
        # Parse the output to extract results
        game_result = parse_game_output(output, circle_bot, square_bot, board_size)
        
        # Save to JSON if requested
        if output_json:
            with open(output_json, 'w') as f:
                json.dump(game_result, f, indent=2)
            print(f"\n✓ Results saved to {output_json}")
        
        return game_result
        
    except subprocess.TimeoutExpired:
        print(f"\n⚠️  Game timed out!")
        return {
            "winner": None,
            "circle_bot": circle_bot,
            "square_bot": square_bot,
            "circle_score": 0.0,
            "square_score": 0.0,
            "error": "timeout",
            "board_size": board_size
        }
    except Exception as e:
        print(f"\n❌ Error running game: {e}")
        return {
            "winner": None,
            "circle_bot": circle_bot,
            "square_bot": square_bot,
            "error": str(e),
            "board_size": board_size
        }


def parse_game_output(output: str, circle_bot: str, square_bot: str, board_size: str) -> Dict[str, Any]:
    """
    Parse gameEngine output to extract game results.
    
    Looks for patterns like:
    - "WINNER: CIRCLE"
    - "Final Scores -> Circle: 85.0 | Square: 15.0"
    - "Remaining time — Circle: 00:45 | Square: 01:23"
    """
    result = {
        "circle_bot": circle_bot,
        "square_bot": square_bot,
        "board_size": board_size,
        "winner": None,
        "circle_score": 0.0,
        "square_score": 0.0,
        "circle_time_left": 0.0,
        "square_time_left": 0.0,
        "total_moves": 0
    }
    
    lines = output.split('\n')
    
    for line in lines:
        # Check for winner
        if "WINNER:" in line.upper():
            if "CIRCLE" in line.upper():
                result["winner"] = "circle"
            elif "SQUARE" in line.upper():
                result["winner"] = "square"
        
        # Check for draw
        if "draw" in line.lower() or "stalemate" in line.lower():
            result["winner"] = None
        
        # Parse final scores
        if "Final Scores" in line or "final scores" in line:
            # Format: "Final Scores -> Circle: 85.0 | Square: 15.0"
            try:
                parts = line.split("->")[1] if "->" in line else line
                if "Circle:" in parts and "Square:" in parts:
                    circle_part = parts.split("Circle:")[1].split("|")[0].strip()
                    square_part = parts.split("Square:")[1].strip()
                    result["circle_score"] = float(circle_part)
                    result["square_score"] = float(square_part)
            except (IndexError, ValueError) as e:
                print(f"Warning: Could not parse scores from: {line}")
        
        # Count turns
        if "Turn" in line and ":" in line:
            try:
                turn_num = int(line.split("Turn")[1].split(":")[0].strip())
                result["total_moves"] = max(result["total_moves"], turn_num)
            except (IndexError, ValueError):
                pass
    
    return result


def main():
    """CLI for running single games"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Run a single game for tournament")
    parser.add_argument("circle_bot", help="Bot name for circle player (e.g., bot1)")
    parser.add_argument("square_bot", help="Bot name for square player (e.g., bot2)")
    parser.add_argument("--board-size", choices=["small", "medium", "large"], 
                       default="small", help="Board size")
    parser.add_argument("--time", type=float, default=2.0, 
                       help="Time per player in minutes")
    parser.add_argument("--output", help="Save results to JSON file")
    
    args = parser.parse_args()
    
    result = run_single_game(
        args.circle_bot,
        args.square_bot,
        args.board_size,
        args.time,
        args.output
    )
    
    print(f"\n{'='*70}")
    print("📊 GAME RESULT:")
    print(f"   Winner: {result['winner'] or 'DRAW'}")
    print(f"   Circle ({result['circle_bot']}): {result['circle_score']:.1f}")
    print(f"   Square ({result['square_bot']}): {result['square_score']:.1f}")
    print(f"   Total Moves: {result['total_moves']}")
    print(f"{'='*70}\n")


if __name__ == "__main__":
    main()
