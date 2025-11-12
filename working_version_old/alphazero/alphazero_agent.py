"""
AlphaZero Agent for integration with game engine
"""

import sys
import os
import torch
import numpy as np

# Add alphazero directory to path
alphazero_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, alphazero_dir)

from game_adapter import StonesAndRivers
from model import StonesRiversNet
from mcts import MCTS


class AlphaZeroAgent:
    """
    Agent that uses trained AlphaZero model to play Stones and Rivers.
    """

    def __init__(self, player, model_path, board_size='small',
                 num_searches=600, device='auto'):
        """
        Initialize AlphaZero agent.

        Args:
            player: 'circle' or 'square'
            model_path: Path to trained model checkpoint
            board_size: 'small', 'medium', or 'large'
            num_searches: Number of MCTS simulations per move
            device: 'auto', 'cpu', or 'cuda'
        """
        self.player = player
        self.opponent = 'square' if player == 'circle' else 'circle'

        # Setup device
        if device == 'auto':
            self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        else:
            self.device = torch.device(device)

        # Initialize game
        self.game = StonesAndRivers(board_size)

        # Load model
        checkpoint = torch.load(model_path, map_location=self.device)

        # Determine architecture from checkpoint
        args = checkpoint.get('args', {})
        
        # Try to get architecture from args first
        num_resblocks = args.get('num_resblocks')
        num_hidden = args.get('num_hidden')
        
        # If not in args, infer from model state dict
        if num_resblocks is None or num_hidden is None:
            state_dict = checkpoint['model_state_dict']
            
            # Infer num_hidden from startBlock weight shape
            if 'startBlock.0.weight' in state_dict:
                num_hidden = state_dict['startBlock.0.weight'].shape[0]
            else:
                num_hidden = 128  # default
            
            # Infer num_resblocks from backbone layers
            backbone_indices = [int(k.split('.')[1]) 
                              for k in state_dict.keys() 
                              if k.startswith('backBone.') and k.split('.')[1].isdigit()]
            if backbone_indices:
                num_resblocks = max(backbone_indices) + 1
            else:
                num_resblocks = 9  # default
        
        print(f"Model architecture: {num_resblocks} resblocks, {num_hidden} hidden channels")

        self.model = StonesRiversNet(self.game, num_resblocks, num_hidden, self.device)
        self.model.load_state_dict(checkpoint['model_state_dict'])
        self.model.eval()

        # Setup MCTS
        mcts_args = {
            'C': 2,
            'num_searches': num_searches,
            'dirichlet_epsilon': 0.0,  # No exploration during play
            'dirichlet_alpha': 0.3
        }

        self.mcts = MCTS(self.game, mcts_args, self.model)

        print(f"AlphaZero agent loaded from {model_path}")
        print(f"Playing as {player} on {board_size} board")
        print(f"Using device: {self.device}")

    def choose(self, board, rows, cols, score_cols, current_player_time, opponent_time):
        """
        Choose best move using MCTS.

        Args:
            board: Game board state (from engine)
            rows, cols: Board dimensions
            score_cols: Scoring columns
            current_player_time: Remaining time for current player
            opponent_time: Remaining time for opponent

        Returns:
            move dictionary or None
        """
        # Convert board to game state
        state = self.game._board_to_numpy(board)

        # Get canonical form (always from current player's perspective)
        player_value = 1 if self.player == 'circle' else -1
        canonical_state = self.game.change_perspective(state, player_value)

        # Run MCTS
        action_probs = self.mcts.search(canonical_state)

        # Choose best action (greedy, no temperature)
        valid_moves = self.game.get_valid_moves(canonical_state)
        action_probs *= valid_moves

        if np.sum(action_probs) == 0:
            # No valid moves
            return None

        # Select action with highest probability
        action = np.argmax(action_probs)

        # Convert action to move dictionary
        move = self.game._action_to_move(action, canonical_state)

        return move


def create_alphazero_student_agent(player, board_size='small'):
    """
    Factory function to create AlphaZero agent for student_agent.py

    Args:
        player: 'circle' or 'square'
        board_size: 'small', 'medium', or 'large'

    Returns:
        AlphaZeroAgent instance
    """
    # Determine model path based on board size
    model_dir = os.path.join(alphazero_dir, 'checkpoints')

    # Look for latest model
    model_files = [f for f in os.listdir(model_dir)
                   if f.startswith(f'model_') and f.endswith(f'_{board_size}.pt')]

    if not model_files:
        raise FileNotFoundError(
            f"No trained model found for {board_size} board in {model_dir}"
        )

    # Get latest iteration - sort all models numerically
    def get_model_number(filename):
        """Extract iteration number from model filename."""
        parts = filename.split('_')
        # Handle bootstrap_complete specially - treat as end of bootstrap phase
        if 'bootstrap_complete' in filename:
            return 999999  # High number but not the highest
        # Skip partial bootstrap files
        if 'bootstrap' in filename:
            return -1  # Put at beginning
        try:
            return int(parts[1]) + 1000000  # Self-play models are later than bootstrap
        except (ValueError, IndexError):
            return -1
    
    model_files.sort(key=get_model_number)
    latest_model = model_files[-1]
    model_path = os.path.join(model_dir, latest_model)
    
    # Print which model was selected
    model_num = get_model_number(latest_model)
    if 'bootstrap_complete' in latest_model:
        print(f"✓ Using bootstrap complete model: {latest_model}")
    elif model_num >= 1000000:
        actual_iter = model_num - 1000000
        print(f"✓ Using latest self-play model: {latest_model} (iteration {actual_iter})")
    else:
        print(f"✓ Using model: {latest_model}")

    return AlphaZeroAgent(player, model_path, board_size, num_searches=400)
