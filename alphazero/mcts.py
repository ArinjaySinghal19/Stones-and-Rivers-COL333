"""
Monte Carlo Tree Search implementation for Stones and Rivers AlphaZero
"""

import numpy as np
import torch
import math


class Node:
    """MCTS Node for Stones and Rivers game tree."""

    def __init__(self, game, args, state, parent=None, action_taken=None,
                 prior=0, visit_count=0):
        self.game = game
        self.args = args
        self.state = state
        self.parent = parent
        self.action_taken = action_taken
        self.prior = prior

        self.children = []
        self.visit_count = visit_count
        self.value_sum = 0

    def is_fully_expanded(self):
        return len(self.children) > 0

    def select(self):
        """Select best child using UCB formula."""
        best_child = None
        best_ucb = -np.inf

        for child in self.children:
            ucb = self.get_ucb(child)
            if ucb > best_ucb:
                best_child = child
                best_ucb = ucb

        return best_child

    def get_ucb(self, child):
        """
        Calculate Upper Confidence Bound for child.
        Q-value + exploration term
        """
        if child.visit_count == 0:
            q_value = 0
        else:
            # Normalize Q-value to [0, 1]
            q_value = 1 - ((child.value_sum / child.visit_count) + 1) / 2

        # UCB formula with prior
        exploration_term = (
            self.args['C'] *
            (math.sqrt(self.visit_count) / (child.visit_count + 1)) *
            child.prior
        )

        return q_value + exploration_term

    def expand(self, policy):
        """Expand node by creating children for all valid actions."""
        for action, prob in enumerate(policy):
            if prob > 0:
                child_state = self.state.copy()
                child_state = self.game.get_next_state(child_state, action, 1)
                child_state = self.game.change_perspective(child_state, player=-1)

                child = Node(self.game, self.args, child_state, self, action, prob)
                self.children.append(child)

    def backpropagate(self, value):
        """Backpropagate value up the tree."""
        self.value_sum += value
        self.visit_count += 1

        value = self.game.get_opponent_value(value)
        if self.parent is not None:
            self.parent.backpropagate(value)


class MCTS:
    """Monte Carlo Tree Search with neural network guidance."""

    def __init__(self, game, args, model):
        self.game = game
        self.args = args
        self.model = model

    @torch.no_grad()
    def search(self, state):
        """
        Perform MCTS search from given state.

        Returns:
            action_probs: Policy distribution over actions
        """
        root = Node(self.game, self.args, state, visit_count=1)

        # Get initial policy and value from network
        policy, _ = self.model(
            torch.tensor(self.game.get_encoded_state(state),
                        device=self.model.device).unsqueeze(0)
        )
        policy = torch.softmax(policy, axis=1).squeeze(0).cpu().numpy()

        # Add Dirichlet noise for exploration (only at root)
        policy = (
            (1 - self.args['dirichlet_epsilon']) * policy +
            self.args['dirichlet_epsilon'] *
            np.random.dirichlet([self.args['dirichlet_alpha']] * self.game.action_size)
        )

        # Mask invalid moves
        valid_moves = self.game.get_valid_moves(state)
        policy *= valid_moves
        policy_sum = np.sum(policy)
        if policy_sum > 0:
            policy /= policy_sum
        else:
            # All actions invalid - uniform over valid moves
            policy = valid_moves / np.sum(valid_moves) if np.sum(valid_moves) > 0 else valid_moves

        root.expand(policy)

        # Perform MCTS searches
        for search in range(self.args['num_searches']):
            node = root

            # Selection: traverse tree using UCB
            while node.is_fully_expanded():
                node = node.select()

            # Check terminal
            value, is_terminal = self.game.get_value_and_terminated(
                node.state, node.action_taken
            )
            value = self.game.get_opponent_value(value)

            if not is_terminal:
                # Expansion and evaluation
                policy, value = self.model(
                    torch.tensor(self.game.get_encoded_state(node.state),
                                device=self.model.device).unsqueeze(0)
                )
                policy = torch.softmax(policy, axis=1).squeeze(0).cpu().numpy()

                valid_moves = self.game.get_valid_moves(node.state)
                policy *= valid_moves
                policy_sum = np.sum(policy)
                if policy_sum > 0:
                    policy /= policy_sum
                else:
                    policy = valid_moves / np.sum(valid_moves) if np.sum(valid_moves) > 0 else valid_moves

                value = value.item()

                node.expand(policy)

            # Backpropagation
            node.backpropagate(value)

        # Return visit count distribution as policy
        action_probs = np.zeros(self.game.action_size)
        for child in root.children:
            action_probs[child.action_taken] = child.visit_count

        if np.sum(action_probs) > 0:
            action_probs /= np.sum(action_probs)

        return action_probs


class MCTSParallel:
    """Parallel MCTS for batch processing during self-play."""

    def __init__(self, game, args, model):
        self.game = game
        self.args = args
        self.model = model

    @torch.no_grad()
    def search(self, states, spGames):
        """
        Parallel MCTS search for multiple games.

        Args:
            states: Batch of states
            spGames: List of SelfPlayGame objects
        """
        # Get policies for all states in batch
        policy, _ = self.model(
            torch.tensor(self.game.get_encoded_state(states),
                        device=self.model.device)
        )
        policy = torch.softmax(policy, axis=1).cpu().numpy()

        # Add Dirichlet noise
        policy = (
            (1 - self.args['dirichlet_epsilon']) * policy +
            self.args['dirichlet_epsilon'] *
            np.random.dirichlet(
                [self.args['dirichlet_alpha']] * self.game.action_size,
                size=policy.shape[0]
            )
        )

        # Initialize roots for each game
        for i, spg in enumerate(spGames):
            spg_policy = policy[i]
            valid_moves = self.game.get_valid_moves(states[i])
            spg_policy *= valid_moves
            policy_sum = np.sum(spg_policy)
            if policy_sum > 0:
                spg_policy /= policy_sum
            else:
                spg_policy = valid_moves / np.sum(valid_moves) if np.sum(valid_moves) > 0 else valid_moves

            spg.root = Node(self.game, self.args, states[i], visit_count=1)
            spg.root.expand(spg_policy)

        # Perform parallel searches
        for search in range(self.args['num_searches']):
            for spg in spGames:
                spg.node = None
                node = spg.root

                # Selection
                while node.is_fully_expanded():
                    node = node.select()

                # Check terminal
                value, is_terminal = self.game.get_value_and_terminated(
                    node.state, node.action_taken
                )
                value = self.game.get_opponent_value(value)

                if is_terminal:
                    node.backpropagate(value)
                else:
                    spg.node = node

            # Batch evaluation of expandable nodes
            expandable_spGames = [
                idx for idx in range(len(spGames))
                if spGames[idx].node is not None
            ]

            if len(expandable_spGames) > 0:
                states_to_evaluate = np.stack([
                    spGames[idx].node.state
                    for idx in expandable_spGames
                ])

                policy, value = self.model(
                    torch.tensor(self.game.get_encoded_state(states_to_evaluate),
                                device=self.model.device)
                )
                policy = torch.softmax(policy, axis=1).cpu().numpy()
                value = value.cpu().numpy()

                # Expand and backpropagate
                for i, idx in enumerate(expandable_spGames):
                    node = spGames[idx].node
                    spg_policy, spg_value = policy[i], value[i]

                    valid_moves = self.game.get_valid_moves(node.state)
                    spg_policy *= valid_moves
                    policy_sum = np.sum(spg_policy)
                    if policy_sum > 0:
                        spg_policy /= policy_sum
                    else:
                        spg_policy = valid_moves / np.sum(valid_moves) if np.sum(valid_moves) > 0 else valid_moves

                    node.expand(spg_policy)
                    node.backpropagate(spg_value)
