#!/usr/bin/env python3
"""
Bot process - runs a single bot in isolation.
This ensures each bot's .so file loads in its own process space.
"""

import sys
import os
import json

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'client_server'))

from agent import load_tournament_bot

def main():
    if len(sys.argv) != 4:
        print("Usage: bot_process.py <bot_name> <player_side> <board_size>", file=sys.stderr)
        sys.exit(1)
    
    bot_name = sys.argv[1]
    player_side = sys.argv[2]
    board_size_str = sys.argv[3]
    
    # Convert board size string to dimensions
    board_sizes = {
        'small': (13, 12),
        'medium': (15, 14),
        'large': (17, 16)
    }
    
    if board_size_str not in board_sizes:
        print(f"Invalid board size: {board_size_str}", file=sys.stderr)
        sys.exit(1)
    
    rows, cols = board_sizes[board_size_str]
    
    # Load the bot
    print(f"[BOT_PROCESS] Loading {bot_name} for {player_side}", file=sys.stderr)
    bot = load_tournament_bot(bot_name)
    
    # Read game state from stdin, make move, write to stdout
    # This is a simple protocol: JSON lines
    for line in sys.stdin:
        try:
            request = json.loads(line)
            
            if request['type'] == 'move':
                game_state = request['game_state']
                time_limit = request['time_limit']
                
                # Make move
                move = bot.move(game_state, player_side, time_limit)
                
                # Send response
                response = {
                    'type': 'move_response',
                    'move': move
                }
                print(json.dumps(response), flush=True)
            
            elif request['type'] == 'quit':
                break
                
        except Exception as e:
            print(f"[BOT_PROCESS] Error: {e}", file=sys.stderr)
            response = {
                'type': 'error',
                'error': str(e)
            }
            print(json.dumps(response), flush=True)
    
    print(f"[BOT_PROCESS] {bot_name} shutting down", file=sys.stderr)

if __name__ == '__main__':
    main()
