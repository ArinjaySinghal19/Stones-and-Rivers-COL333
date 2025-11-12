"""
Game Adapter for Stones and Rivers
Adapts the game engine to work with AlphaZero implementation
"""

import numpy as np
import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'client_server'))

from gameEngine import (
    Piece, default_start_board, score_cols_for,
    top_score_row, bottom_score_row, check_win,
    validate_and_apply_move, generate_all_moves,
    in_bounds, opponent as get_opponent_player,
    get_win_count
)
import copy
import json


class StonesAndRivers:
    """
    Game adapter for AlphaZero training on Stones and Rivers.
    Supports multiple board sizes: small (13x12), medium (15x14), large (17x16)
    """

    def __init__(self, board_size='small'):
        """
        Initialize game with specified board size.

        Args:
            board_size: 'small' (13x12), 'medium' (15x14), or 'large' (17x16)
        """
        self.board_size = board_size

        if board_size == 'small':
            self.row_count, self.column_count = 13, 12
            self.pieces_per_player = 12
        elif board_size == 'medium':
            self.row_count, self.column_count = 15, 14
            self.pieces_per_player = 14
        elif board_size == 'large':
            self.row_count, self.column_count = 17, 16
            self.pieces_per_player = 16
        else:
            raise ValueError(f"Unknown board size: {board_size}")

        self.score_cols = score_cols_for(self.column_count)
        self.win_count = get_win_count(self.column_count)

        # Cache for move generation
        self._move_cache = {}

    def __repr__(self):
        return f"StonesAndRivers_{self.board_size}"

    def get_initial_state(self):
        """Return initial board state as numpy array."""
        board = default_start_board(self.row_count, self.column_count)
        return self._board_to_numpy(board)

    def _board_to_numpy(self, board):
        """Convert board list to numpy representation."""
        state = np.zeros((self.row_count, self.column_count, 4), dtype=np.int8)

        for y in range(self.row_count):
            for x in range(self.column_count):
                piece = board[y][x]
                if piece is not None:
                    # Channel 0: circle stones
                    # Channel 1: circle rivers (horizontal=1, vertical=2)
                    # Channel 2: square stones
                    # Channel 3: square rivers (horizontal=1, vertical=2)
                    if piece.owner == 'circle':
                        if piece.side == 'stone':
                            state[y, x, 0] = 1
                        else:  # river
                            state[y, x, 1] = 1 if piece.orientation == 'horizontal' else 2
                    else:  # square
                        if piece.side == 'stone':
                            state[y, x, 2] = 1
                        else:  # river
                            state[y, x, 3] = 1 if piece.orientation == 'horizontal' else 2

        return state

    def _numpy_to_board(self, state):
        """Convert numpy state back to board list."""
        from gameEngine import empty_board

        board = empty_board(self.row_count, self.column_count)

        for y in range(self.row_count):
            for x in range(self.column_count):
                if state[y, x, 0] == 1:
                    board[y][x] = Piece('circle', 'stone')
                elif state[y, x, 1] > 0:
                    ori = 'horizontal' if state[y, x, 1] == 1 else 'vertical'
                    board[y][x] = Piece('circle', 'river', ori)
                elif state[y, x, 2] == 1:
                    board[y][x] = Piece('square', 'stone')
                elif state[y, x, 3] > 0:
                    ori = 'horizontal' if state[y, x, 3] == 1 else 'vertical'
                    board[y][x] = Piece('square', 'river', ori)

        return board

    def get_valid_moves(self, state):
        """
        Get valid moves as a binary mask.

        Returns:
            numpy array of shape (action_size,) where 1 indicates valid move
        """
        board = self._numpy_to_board(state)
        player = self._get_current_player_from_state(state)

        moves = generate_all_moves(board, player, self.row_count,
                                   self.column_count, self.score_cols)

        valid_mask = np.zeros(self.action_size, dtype=np.uint8)

        for move in moves:
            action_idx = self._move_to_action(move)
            if action_idx is not None and action_idx < self.action_size:
                valid_mask[action_idx] = 1

        return valid_mask

    @property
    def action_size(self):
        """
        Total number of possible actions.

        Actions encoded as:
        - Move/Push: from_pos * (rows*cols) + to_pos
        - Flip: (rows*cols)² + piece_pos * 3 + flip_type (0=h, 1=v, 2=river->stone)
        - Rotate: (rows*cols)² + (rows*cols)*3 + piece_pos
        """
        num_positions = self.row_count * self.column_count

        # Movement: each position to any position (includes invalid moves, filtered at runtime)
        move_actions = num_positions * num_positions

        # Flips: each position * 3 types (horizontal, vertical, river->stone)
        flip_actions = num_positions * 3

        # Rotates: each position
        rotate_actions = num_positions

        return move_actions + flip_actions + rotate_actions

    def _move_to_action(self, move):
        """Convert move dictionary to action index.
        
        Note: gameEngine uses [row, col] format for positions.
        """
        action = move.get('action')
        from_pos = move.get('from', [0, 0])
        # gameEngine uses [row, col], so from_pos[0] is row (y), from_pos[1] is col (x)
        fy, fx = from_pos[0], from_pos[1]
        from_idx = fy * self.column_count + fx

        num_positions = self.row_count * self.column_count

        if action == 'move' or action == 'push':
            to_pos = move.get('to', [0, 0])
            # gameEngine uses [row, col]
            ty, tx = to_pos[0], to_pos[1]
            to_idx = ty * self.column_count + tx

            # Encode as: from_idx * num_positions + to_idx
            return from_idx * num_positions + to_idx

        elif action == 'flip':
            ori = move.get('orientation')
            base = num_positions * num_positions

            if ori == 'horizontal':
                return base + from_idx * 3 + 0
            elif ori == 'vertical':
                return base + from_idx * 3 + 1
            else:  # river to stone
                return base + from_idx * 3 + 2

        elif action == 'rotate':
            base = num_positions * num_positions + num_positions * 3
            return base + from_idx

        return None

    def _action_to_move(self, action, state):
        """Convert action index back to move dictionary.
        
        Returns moves in [row, col] format to match gameEngine.
        """
        num_positions = self.row_count * self.column_count
        move_actions = num_positions * num_positions
        flip_actions = num_positions * 3

        if action < move_actions:
            # Move or push action
            from_idx = action // num_positions
            to_idx = action % num_positions

            fx = from_idx % self.column_count
            fy = from_idx // self.column_count
            tx = to_idx % self.column_count
            ty = to_idx // self.column_count

            # Determine if it's a push by checking if destination is occupied
            board = self._numpy_to_board(state)
            if board[ty][tx] is not None:
                # It's a push - need to determine pushed_to
                dx, dy = tx - fx, ty - fy
                # Return [row, col] format
                return {
                    'action': 'push',
                    'from': [fy, fx],
                    'to': [ty, tx],
                    'pushed_to': [ty + dy, tx + dx]
                }
            else:
                # Return [row, col] format
                return {
                    'action': 'move',
                    'from': [fy, fx],
                    'to': [ty, tx]
                }

        elif action < move_actions + flip_actions:
            # Flip action
            action_offset = action - move_actions
            from_idx = action_offset // 3
            flip_type = action_offset % 3

            fx = from_idx % self.column_count
            fy = from_idx // self.column_count

            # Return [row, col] format
            if flip_type == 0:
                return {'action': 'flip', 'from': [fy, fx], 'orientation': 'horizontal'}
            elif flip_type == 1:
                return {'action': 'flip', 'from': [fy, fx], 'orientation': 'vertical'}
            else:
                return {'action': 'flip', 'from': [fy, fx]}

        else:
            # Rotate action
            action_offset = action - move_actions - flip_actions
            from_idx = action_offset

            fx = from_idx % self.column_count
            fy = from_idx // self.column_count

            # Return [row, col] format
            return {'action': 'rotate', 'from': [fy, fx]}

    def get_next_state(self, state, action, player):
        """
        Apply action and return new state.

        Args:
            state: Current state
            action: Action index
            player: 1 for circle, -1 for square

        Returns:
            New state after applying action
        """
        board = self._numpy_to_board(state)
        player_str = 'circle' if player == 1 else 'square'

        move = self._action_to_move(action, state)

        # Validate and apply move
        board_copy = copy.deepcopy(board)
        success, _ = validate_and_apply_move(board_copy, move, player_str,
                                             self.row_count, self.column_count,
                                             self.score_cols)

        if success:
            return self._board_to_numpy(board_copy)
        else:
            # Invalid move - return unchanged state
            return state.copy()

    def get_value_and_terminated(self, state, action):
        """
        Check if game is over and return value.

        Returns:
            (value, is_terminated) where value is 1 for win, 0 for draw/ongoing
        """
        board = self._numpy_to_board(state)
        winner = check_win(board, self.row_count, self.column_count, self.score_cols)

        if winner is not None:
            # Game won by current player
            return 1, True

        # Check if board is full (draw) - simplified check
        total_pieces = np.sum(state[:, :, :] > 0)
        max_pieces = self.pieces_per_player * 2

        # If we've exceeded move limit, it's a draw
        if total_pieces >= max_pieces:
            return 0, True

        return 0, False

    def _get_current_player_from_state(self, state):
        """Infer current player from state (for internal use)."""
        # Count pieces - circle goes first
        circle_pieces = np.sum(state[:, :, 0] > 0) + np.sum(state[:, :, 1] > 0)
        square_pieces = np.sum(state[:, :, 2] > 0) + np.sum(state[:, :, 3] > 0)

        return 'circle' if circle_pieces <= square_pieces else 'square'

    def get_opponent(self, player):
        """Get opponent player."""
        return -player

    def get_opponent_value(self, value):
        """Get value from opponent's perspective."""
        return -value

    def change_perspective(self, state, player):
        """
        Change state perspective to current player.

        For Stones and Rivers:
        - If player is circle (1), return state as-is
        - If player is square (-1), swap circle and square channels
        """
        if player == 1:
            return state
        else:
            # Swap circle and square channels
            new_state = state.copy()
            new_state[:, :, [0, 1, 2, 3]] = state[:, :, [2, 3, 0, 1]]
            return new_state

    def get_encoded_state(self, state):
        """
        Encode state for neural network input.

        Returns 7 binary planes:
        - Square rivers (current player perspective)
        - Square stones
        - Square scoring area
        - Circle rivers (opponent)
        - Circle stones
        - Circle scoring area
        - Blank spaces
        """
        if len(state.shape) == 3:
            # Single state
            encoded = self._encode_single_state(state)
            return encoded
        else:
            # Batch of states
            batch_size = state.shape[0]
            encoded_batch = np.zeros((batch_size, 7, self.row_count, self.column_count),
                                    dtype=np.float32)

            for i in range(batch_size):
                encoded_batch[i] = self._encode_single_state(state[i])

            return encoded_batch

    def _encode_single_state(self, state):
        """Encode a single state into 7 binary planes."""
        encoded = np.zeros((7, self.row_count, self.column_count), dtype=np.float32)

        # Plane 0: Square rivers (treated as current player)
        encoded[0] = (state[:, :, 3] > 0).astype(np.float32)

        # Plane 1: Square stones
        encoded[1] = (state[:, :, 2] > 0).astype(np.float32)

        # Plane 2: Square scoring area
        square_score_row = bottom_score_row(self.row_count)
        for x in self.score_cols:
            if in_bounds(x, square_score_row, self.row_count, self.column_count):
                encoded[2, square_score_row, x] = 1.0

        # Plane 3: Circle rivers
        encoded[3] = (state[:, :, 1] > 0).astype(np.float32)

        # Plane 4: Circle stones
        encoded[4] = (state[:, :, 0] > 0).astype(np.float32)

        # Plane 5: Circle scoring area
        circle_score_row = top_score_row()
        for x in self.score_cols:
            if in_bounds(x, circle_score_row, self.row_count, self.column_count):
                encoded[5, circle_score_row, x] = 1.0

        # Plane 6: Blank spaces
        encoded[6] = (np.sum(state[:, :, :], axis=2) == 0).astype(np.float32)

        return encoded

    def get_canonical_form(self, state, player):
        """Get canonical form where current player is always player 1."""
        return self.change_perspective(state, player)
