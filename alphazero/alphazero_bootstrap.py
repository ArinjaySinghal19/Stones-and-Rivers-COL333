"""
AlphaZero Bootstrap Training - Train against heuristic bot
This provides a better starting point than pure self-play from random initialization
"""

import numpy as np
import torch
import torch.nn.functional as F
import random
from tqdm import trange
from mcts import MCTS
import sys
import os

# Add client_server to path for game engine and agents
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'client_server'))

from gameEngine import (
    default_start_board, check_win, validate_and_apply_move,
    opponent as get_opponent_player, score_cols_for
)
from agent import get_agent


class BootstrapTraining:
    """
    Train AlphaZero by playing against a heuristic opponent.
    This accelerates learning compared to pure self-play.
    """

    def __init__(self, model, optimizer, game, args, heuristic_agent_name='student_cpp', use_gui=False):
        self.model = model
        self.optimizer = optimizer
        self.game = game
        self.args = args
        self.mcts = MCTS(game, args, model)
        self.heuristic_agent_name = heuristic_agent_name
        self.use_gui = use_gui

        print(f"Bootstrap training using {heuristic_agent_name} as opponent")
        
        if use_gui:
            try:
                import sys
                sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'client_server'))
                from gameEngine import board_to_ascii
                self.board_to_ascii = board_to_ascii
                print("Terminal GUI enabled for bootstrap training")
            except Exception as e:
                print(f"GUI not available: {e}")
                self.use_gui = False

    def play_vs_heuristic(self, alphazero_plays_first=True):
        """
        Play one game: AlphaZero vs Heuristic bot.

        Args:
            alphazero_plays_first: If True, AlphaZero plays as circle (first)

        Returns:
            memory: Training data from the game
        """
        memory = []
        board = default_start_board(self.game.row_count, self.game.column_count)
        score_cols = score_cols_for(self.game.column_count)

        # Setup players
        if alphazero_plays_first:
            alphazero_player = 'circle'
            heuristic_player = 'square'
            current_player = 'circle'
        else:
            alphazero_player = 'square'
            heuristic_player = 'circle'
            current_player = 'circle'

        # Create heuristic agent
        heuristic_agent = get_agent(heuristic_player, self.heuristic_agent_name)
        print(f"  Heuristic agent type: {type(heuristic_agent).__name__} ({self.heuristic_agent_name})")
        print(f"  AlphaZero: {alphazero_player}, Heuristic: {heuristic_player}")

        move_count = 0
        max_moves = 500
        winner = None  # Initialize winner variable
        
        # Print initial board if GUI enabled
        if self.use_gui:
            print("\n" + "="*60)
            print(f"Game Start: AlphaZero ({alphazero_player}) vs {self.heuristic_agent_name} ({heuristic_player})")
            print("="*60)
            print(self.board_to_ascii(board, self.game.row_count, self.game.column_count, score_cols))

        while move_count < max_moves:
            try:
                if current_player == alphazero_player:
                    # AlphaZero's turn
                    state = self.game._board_to_numpy(board)
                    player_value = 1 if alphazero_player == 'circle' else -1
                    neutral_state = self.game.change_perspective(state, player_value)

                    # Run MCTS
                    action_probs = self.mcts.search(neutral_state)

                    # Save to memory
                    memory.append((neutral_state, action_probs, alphazero_player))

                    # Sample action with temperature
                    temperature_action_probs = action_probs ** (1 / self.args['temperature'])
                    temperature_sum = np.sum(temperature_action_probs)

                    if temperature_sum > 0:
                        temperature_action_probs /= temperature_sum
                        action = np.random.choice(self.game.action_size, p=temperature_action_probs)
                    else:
                        # Fallback: use original state for valid moves
                        valid_moves = self.game.get_valid_moves(state)
                        valid_indices = np.where(valid_moves > 0)[0]
                        if len(valid_indices) > 0:
                            action = np.random.choice(valid_indices)
                        else:
                            # No valid moves - shouldn't happen
                            break

                    # Convert action to move (use neutral_state for this as it's perspective-correct)
                    move = self.game._action_to_move(action, neutral_state)

                else:
                    # Heuristic bot's turn
                    move = heuristic_agent.choose(
                        board, self.game.row_count, self.game.column_count,
                        score_cols, 120.0, 120.0
                    )

                    if move is None:
                        # Heuristic returned None - treat as AlphaZero win
                        winner = alphazero_player
                        break
                
                # Apply move
                success, _ = validate_and_apply_move(
                    board, move, current_player,
                    self.game.row_count, self.game.column_count, score_cols
                )
                
                # Print board update if GUI enabled
                if self.use_gui and success:
                    print(f"\n--- Move {move_count + 1}: {current_player} played ---")
                    print(self.board_to_ascii(board, self.game.row_count, self.game.column_count, score_cols))

                if not success:
                    # Invalid move - penalize current player
                    if current_player == alphazero_player:
                        # AlphaZero made invalid move - it loses
                        winner = heuristic_player
                        break
                    else:
                        # Heuristic made invalid move - skip turn
                        current_player = get_opponent_player(current_player)
                        move_count += 1
                        continue

                # Check win
                winner_check = check_win(board, self.game.row_count, self.game.column_count, score_cols)
                if winner_check is not None:
                    winner = winner_check
                    break

                # Next player
                current_player = get_opponent_player(current_player)
                move_count += 1
                
            except Exception as e:
                if self.use_gui:
                    print(f"\n[EXCEPTION] Error during move {move_count}:")
                    print(f"[EXCEPTION] {type(e).__name__}: {e}")
                    import traceback
                    traceback.print_exc()
                # Treat as draw and exit
                winner = None
                break

        else:
            # Max moves reached - draw
            winner = None

        # Print final result if GUI enabled
        if self.use_gui:
            print("\n" + "="*60)
            print(f"Game ended after {move_count} moves")
            print(f"Memory (training samples): {len(memory)}")
            if winner is None:
                print("Result: DRAW")
            elif winner == alphazero_player:
                print(f"Result: AlphaZero ({alphazero_player}) WON!")
            else:
                print(f"Result: {self.heuristic_agent_name} ({heuristic_player}) WON!")
            print("="*60 + "\n")

        # Process memory with outcomes
        return_memory = []
        for hist_neutral_state, hist_action_probs, hist_player in memory:
            if winner is None:
                # Draw
                outcome = 0
            elif winner == alphazero_player:
                # AlphaZero won
                outcome = 1 if hist_player == alphazero_player else -1
            else:
                # Heuristic won
                outcome = -1 if hist_player == alphazero_player else 1

            return_memory.append((
                self.game.get_encoded_state(hist_neutral_state),
                hist_action_probs,
                outcome
            ))

        return return_memory, winner

    def bootstrap_phase(self, num_games):
        """
        Play multiple games against heuristic for bootstrapping.

        Args:
            num_games: Number of games to play

        Returns:
            memory: Combined training data
            stats: Win/loss/draw statistics
        """
        memory = []
        stats = {'alphazero_wins': 0, 'heuristic_wins': 0, 'draws': 0}

        print(f"Playing {num_games} games against {self.heuristic_agent_name}...")

        self.model.eval()

        for game_idx in trange(num_games):
            # Alternate who goes first
            alphazero_first = (game_idx % 2 == 0)

            game_memory, winner = self.play_vs_heuristic(alphazero_first)
            memory.extend(game_memory)

            # Update stats
            alphazero_player = 'circle' if alphazero_first else 'square'
            if winner == alphazero_player:
                stats['alphazero_wins'] += 1
            elif winner is None:
                stats['draws'] += 1
            else:
                stats['heuristic_wins'] += 1

        win_rate = stats['alphazero_wins'] / num_games * 100
        print(f"\nBootstrap stats: AlphaZero {stats['alphazero_wins']}, "
              f"Heuristic {stats['heuristic_wins']}, "
              f"Draws {stats['draws']} (Win rate: {win_rate:.1f}%)")

        return memory, stats

    def train(self, memory):
        """Train model on collected memory."""
        if len(memory) == 0:
            return

        random.shuffle(memory)

        for batchIdx in range(0, len(memory), self.args['batch_size']):
            sample = memory[batchIdx:min(len(memory), batchIdx + self.args['batch_size'])]

            if len(sample) == 0:
                continue

            state, policy_targets, value_targets = zip(*sample)

            state = np.array(state)
            policy_targets = np.array(policy_targets)
            value_targets = np.array(value_targets).reshape(-1, 1)

            state = torch.tensor(state, dtype=torch.float32, device=self.model.device)
            policy_targets = torch.tensor(policy_targets, dtype=torch.float32, device=self.model.device)
            value_targets = torch.tensor(value_targets, dtype=torch.float32, device=self.model.device)

            out_policy, out_value = self.model(state)

            policy_loss = F.cross_entropy(out_policy, policy_targets)
            value_loss = F.mse_loss(out_value, value_targets)
            loss = policy_loss + value_loss

            self.optimizer.zero_grad()
            loss.backward()
            self.optimizer.step()

    def learn_with_bootstrap(self, num_bootstrap_iterations=10, num_bootstrap_games=100):
        """
        Main training loop with bootstrap phase.

        Args:
            num_bootstrap_iterations: Iterations of bootstrap training
            num_bootstrap_games: Games per bootstrap iteration
        """
        print("=" * 60)
        print("BOOTSTRAP TRAINING PHASE")
        print("=" * 60)

        for iteration in range(num_bootstrap_iterations):
            print(f"\nBootstrap Iteration {iteration + 1}/{num_bootstrap_iterations}")

            # Play against heuristic
            memory, stats = self.bootstrap_phase(num_bootstrap_games)

            print(f"Collected {len(memory)} training samples")

            # Train on bootstrap data
            self.model.train()
            print("Training on bootstrap data...")

            for epoch in trange(self.args['num_epochs']):
                self.train(memory)

            # Save checkpoint
            checkpoint = {
                'iteration': iteration,
                'bootstrap': True,
                'model_state_dict': self.model.state_dict(),
                'optimizer_state_dict': self.optimizer.state_dict(),
                'board_rows': self.game.row_count,
                'board_cols': self.game.column_count,
                'args': self.args,
                'stats': stats
            }
            save_path = f"model_bootstrap_{iteration}_{self.game}.pt"
            torch.save(checkpoint, save_path)
            print(f"Saved {save_path}")

        print("\n" + "=" * 60)
        print("BOOTSTRAP PHASE COMPLETE")
        print("=" * 60)
