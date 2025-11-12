"""
AlphaZero Training Implementation for Stones and Rivers
Supports parallel self-play and training
"""

import numpy as np
import torch
import torch.nn.functional as F
import random
from tqdm import trange
from mcts import MCTS, MCTSParallel


class SelfPlayGame:
    """Container for a single self-play game."""

    def __init__(self, game):
        self.state = game.get_initial_state()
        self.memory = []
        self.root = None
        self.node = None


class AlphaZero:
    """Sequential AlphaZero implementation (for testing)."""

    def __init__(self, model, optimizer, game, args):
        self.model = model
        self.optimizer = optimizer
        self.game = game
        self.args = args
        self.mcts = MCTS(game, args, model)

    def selfPlay(self):
        """
        Play one game via self-play.

        Returns:
            memory: List of (state, policy, outcome) tuples
        """
        memory = []
        player = 1
        state = self.game.get_initial_state()

        move_count = 0
        max_moves = 500  # Prevent infinite games

        while move_count < max_moves:
            neutral_state = self.game.change_perspective(state, player)
            action_probs = self.mcts.search(neutral_state)

            memory.append((neutral_state, action_probs, player))

            # Sample action with temperature
            temperature_action_probs = action_probs ** (1 / self.args['temperature'])
            temperature_sum = np.sum(temperature_action_probs)
            if temperature_sum > 0:
                temperature_action_probs /= temperature_sum
                action = np.random.choice(self.game.action_size, p=temperature_action_probs)
            else:
                # Fallback: choose from valid moves
                valid_moves = self.game.get_valid_moves(neutral_state)
                valid_indices = np.where(valid_moves > 0)[0]
                if len(valid_indices) > 0:
                    action = np.random.choice(valid_indices)
                else:
                    # No valid moves - draw
                    break

            state = self.game.get_next_state(state, action, player)

            value, is_terminal = self.game.get_value_and_terminated(state, action)

            if is_terminal:
                return_memory = []
                for hist_neutral_state, hist_action_probs, hist_player in memory:
                    hist_outcome = value if hist_player == player else self.game.get_opponent_value(value)
                    return_memory.append((
                        self.game.get_encoded_state(hist_neutral_state),
                        hist_action_probs,
                        hist_outcome
                    ))
                return return_memory

            player = self.game.get_opponent(player)
            move_count += 1

        # Max moves reached - draw
        return_memory = []
        for hist_neutral_state, hist_action_probs, hist_player in memory:
            return_memory.append((
                self.game.get_encoded_state(hist_neutral_state),
                hist_action_probs,
                0  # Draw outcome
            ))
        return return_memory

    def train(self, memory):
        """
        Train the model on self-play memory.

        Args:
            memory: List of (encoded_state, policy, outcome) tuples
        """
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

    def learn(self):
        """Main training loop."""
        for iteration in range(self.args['num_iterations']):
            memory = []

            self.model.eval()
            print(f"\nIteration {iteration + 1}/{self.args['num_iterations']}")
            print("Self-play...")

            for selfPlay_iteration in trange(self.args['num_selfPlay_iterations']):
                memory += self.selfPlay()

            print(f"Collected {len(memory)} training samples")

            self.model.train()
            print("Training...")

            for epoch in trange(self.args['num_epochs']):
                self.train(memory)

            # Save checkpoint
            checkpoint = {
                'iteration': iteration,
                'model_state_dict': self.model.state_dict(),
                'optimizer_state_dict': self.optimizer.state_dict(),
                'board_rows': self.game.row_count,
                'board_cols': self.game.column_count,
                'args': self.args
            }
            torch.save(checkpoint, f"model_{iteration}_{self.game}.pt")
            print(f"Saved model_{iteration}_{self.game}.pt")


class AlphaZeroParallel:
    """Parallel AlphaZero for efficient GPU training."""

    def __init__(self, model, optimizer, game, args):
        self.model = model
        self.optimizer = optimizer
        self.game = game
        self.args = args
        self.mcts = MCTSParallel(game, args, model)

    def selfPlay(self):
        """
        Play multiple games in parallel.

        Returns:
            return_memory: Combined memory from all games
        """
        return_memory = []
        player = 1
        spGames = [SelfPlayGame(self.game) for _ in range(self.args['num_parallel_games'])]

        move_counts = [0 for _ in range(self.args['num_parallel_games'])]
        max_moves = 500

        while len(spGames) > 0:
            states = np.stack([spg.state for spg in spGames])
            neutral_states = self.game.change_perspective(states, player)

            self.mcts.search(neutral_states, spGames)

            for i in range(len(spGames) - 1, -1, -1):
                spg = spGames[i]

                action_probs = np.zeros(self.game.action_size)
                for child in spg.root.children:
                    action_probs[child.action_taken] = child.visit_count

                if np.sum(action_probs) > 0:
                    action_probs /= np.sum(action_probs)

                spg.memory.append((spg.root.state, action_probs, player))

                # Sample action with temperature
                temperature_action_probs = action_probs ** (1 / self.args['temperature'])
                temperature_sum = np.sum(temperature_action_probs)

                if temperature_sum > 0:
                    temperature_action_probs /= temperature_sum
                    action = np.random.choice(self.game.action_size, p=temperature_action_probs)
                else:
                    # Fallback
                    valid_moves = self.game.get_valid_moves(spg.state)
                    valid_indices = np.where(valid_moves > 0)[0]
                    if len(valid_indices) > 0:
                        action = np.random.choice(valid_indices)
                    else:
                        # No valid moves - end game as draw
                        for hist_neutral_state, hist_action_probs, hist_player in spg.memory:
                            return_memory.append((
                                self.game.get_encoded_state(hist_neutral_state),
                                hist_action_probs,
                                0  # Draw
                            ))
                        del spGames[i]
                        del move_counts[i]
                        continue

                spg.state = self.game.get_next_state(spg.state, action, player)
                move_counts[i] += 1

                value, is_terminal = self.game.get_value_and_terminated(spg.state, action)

                if is_terminal or move_counts[i] >= max_moves:
                    if move_counts[i] >= max_moves:
                        value = 0  # Draw

                    for hist_neutral_state, hist_action_probs, hist_player in spg.memory:
                        hist_outcome = value if hist_player == player else self.game.get_opponent_value(value)
                        return_memory.append((
                            self.game.get_encoded_state(hist_neutral_state),
                            hist_action_probs,
                            hist_outcome
                        ))
                    del spGames[i]
                    del move_counts[i]

            player = self.game.get_opponent(player)

        return return_memory

    def train(self, memory):
        """Train model on memory."""
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

    def learn(self):
        """Main parallel training loop."""
        for iteration in range(self.args['num_iterations']):
            memory = []

            self.model.eval()
            print(f"\nIteration {iteration + 1}/{self.args['num_iterations']}")
            print("Parallel self-play...")

            for selfPlay_iteration in trange(
                self.args['num_selfPlay_iterations'] // self.args['num_parallel_games']
            ):
                memory += self.selfPlay()

            print(f"Collected {len(memory)} training samples")

            self.model.train()
            print("Training...")

            for epoch in trange(self.args['num_epochs']):
                self.train(memory)

            # Save checkpoint
            checkpoint = {
                'iteration': iteration,
                'model_state_dict': self.model.state_dict(),
                'optimizer_state_dict': self.optimizer.state_dict(),
                'board_rows': self.game.row_count,
                'board_cols': self.game.column_count,
                'args': self.args
            }
            torch.save(checkpoint, f"model_{iteration}_{self.game}.pt")
            print(f"Saved model_{iteration}_{self.game}.pt")
