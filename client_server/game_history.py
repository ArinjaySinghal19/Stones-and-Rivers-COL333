"""
Game History and Undo/Redo functionality for Stones and Rivers
"""

import copy
from typing import List, Optional, Dict, Any, Tuple
from gameEngine import Piece


class GameState:
    """Represents a complete game state at a point in time."""

    def __init__(self, board, player, move=None):
        self.board = copy.deepcopy(board)
        self.player = player
        self.move = move  # The move that led to this state

    def copy_board(self):
        """Return a deep copy of the board."""
        return copy.deepcopy(self.board)


class GameHistory:
    """
    Manages game history with undo/redo capability.
    """

    def __init__(self, initial_board, initial_player='circle'):
        """
        Initialize game history.

        Args:
            initial_board: Starting board configuration
            initial_player: Starting player ('circle' or 'square')
        """
        self.states = [GameState(initial_board, initial_player)]
        self.current_index = 0

    def add_state(self, board, player, move):
        """
        Add a new state to history.

        Args:
            board: Current board
            player: Current player
            move: Move that was made
        """
        # Remove any states after current index (if we're not at the end)
        if self.current_index < len(self.states) - 1:
            self.states = self.states[:self.current_index + 1]

        # Add new state
        new_state = GameState(board, player, move)
        self.states.append(new_state)
        self.current_index += 1

    def can_undo(self) -> bool:
        """Check if undo is possible."""
        return self.current_index > 0

    def can_redo(self) -> bool:
        """Check if redo is possible."""
        return self.current_index < len(self.states) - 1

    def undo(self) -> Optional[Tuple[Any, str]]:
        """
        Undo last move.

        Returns:
            (board, player) tuple if successful, None otherwise
        """
        if not self.can_undo():
            return None

        self.current_index -= 1
        state = self.states[self.current_index]
        return state.copy_board(), state.player

    def redo(self) -> Optional[Tuple[Any, str]]:
        """
        Redo previously undone move.

        Returns:
            (board, player) tuple if successful, None otherwise
        """
        if not self.can_redo():
            return None

        self.current_index += 1
        state = self.states[self.current_index]
        return state.copy_board(), state.player

    def get_current_state(self) -> GameState:
        """Get current game state."""
        return self.states[self.current_index]

    def get_move_history(self) -> List[Dict[str, Any]]:
        """
        Get list of all moves made (up to current index).

        Returns:
            List of move dictionaries
        """
        moves = []
        for i in range(1, self.current_index + 1):
            if self.states[i].move:
                moves.append(self.states[i].move)
        return moves

    def get_history_length(self) -> int:
        """Get total number of states in history."""
        return len(self.states)

    def reset(self):
        """Reset to initial state."""
        self.current_index = 0
