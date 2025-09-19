"""
Student Agent Implementation for River and Stones Game

This file contains the essential utilities and template for implementing your AI agent.
Your task is to complete the StudentAgent class with intelligent move selection.

Game Rules:
- Goal: Get 4 of your stones into the opponent's scoring area
- Pieces can be stones or rivers (horizontal/vertical orientation)  
- Actions: move, push, flip (stone↔river), rotate (river orientation)
- Rivers enable flow-based movement across the board

Your Task:
Implement the choose() method in the StudentAgent class to select optimal moves.
You may add any helper methods and modify the evaluation function as needed.
"""

import random
import copy
import math
import time
from typing import List, Dict, Any, Optional, Tuple
from abc import ABC, abstractmethod

# ==================== GAME UTILITIES ====================
# Essential utility functions for game state analysis

def in_bounds(x: int, y: int, rows: int, cols: int) -> bool:
    """Check if coordinates are within board boundaries."""
    return 0 <= x < cols and 0 <= y < rows

def score_cols_for(cols: int) -> List[int]:
    """Get the column indices for scoring areas."""
    w = 4
    start = max(0, (cols - w) // 2)
    return list(range(start, start + w))

def top_score_row() -> int:
    """Get the row index for Circle's scoring area."""
    return 2

def bottom_score_row(rows: int) -> int:
    """Get the row index for Square's scoring area."""
    return rows - 3

def is_opponent_score_cell(x: int, y: int, player: str, rows: int, cols: int, score_cols: List[int]) -> bool:
    """Check if a cell is in the opponent's scoring area."""
    if player == "circle":
        return (y == bottom_score_row(rows)) and (x in score_cols)
    else:
        return (y == top_score_row()) and (x in score_cols)

def is_own_score_cell(x: int, y: int, player: str, rows: int, cols: int, score_cols: List[int]) -> bool:
    """Check if a cell is in the player's own scoring area."""
    if player == "circle":
        return (y == top_score_row()) and (x in score_cols)
    else:
        return (y == bottom_score_row(rows)) and (x in score_cols)

def get_opponent(player: str) -> str:
    """Get the opponent player identifier."""
    return "square" if player == "circle" else "circle"

# ==================== MOVE GENERATION HELPERS ====================

def get_valid_moves_for_piece(board, x: int, y: int, player: str, rows: int, cols: int, score_cols: List[int]) -> List[Dict[str, Any]]:
    """
    Generate all valid moves for a specific piece.
    
    Args:
        board: Current board state
        x, y: Piece position
        player: Current player
        rows, cols: Board dimensions
        score_cols: Scoring column indices
    
    Returns:
        List of valid move dictionaries
    """
    moves = []
    piece = board[y][x]
    
    if piece is None or piece.owner != player:
        return moves
    
    directions = [(1, 0), (-1, 0), (0, 1), (0, -1)]
    
    if piece.side == "stone":
        # Stone movement
        for dx, dy in directions:
            nx, ny = x + dx, y + dy
            if not in_bounds(nx, ny, rows, cols):
                continue
            
            if is_opponent_score_cell(nx, ny, player, rows, cols, score_cols):
                continue
            
            if board[ny][nx] is None:
                # Simple move
                moves.append({"action": "move", "from": [x, y], "to": [nx, ny]})
            elif board[ny][nx].owner != player:
                # Push move
                px, py = nx + dx, ny + dy
                if (in_bounds(px, py, rows, cols) and 
                    board[py][px] is None and 
                    not is_opponent_score_cell(px, py, player, rows, cols, score_cols)):
                    moves.append({"action": "push", "from": [x, y], "to": [nx, ny], "pushed_to": [px, py]})
        
        # Stone to river flips
        for orientation in ["horizontal", "vertical"]:
            moves.append({"action": "flip", "from": [x, y], "orientation": orientation})
    
    else:  # River piece
        # River to stone flip
        moves.append({"action": "flip", "from": [x, y]})
        
        # River rotation
        moves.append({"action": "rotate", "from": [x, y]})
    
    return moves

def generate_all_moves(board: List[List[Any]], player: str, rows: int, cols: int, score_cols: List[int]) -> List[Dict[str, Any]]:
    """Prefer authoritative generator from gameEngine (includes river flow), fallback to local."""
    try:
        from gameEngine import generate_all_moves as engine_generate_all_moves
        return engine_generate_all_moves(board, player, rows, cols, score_cols)
    except Exception:
        all_moves = []
        for y in range(rows):
            for x in range(cols):
                piece = board[y][x]
                if piece and piece.owner == player:
                    piece_moves = get_valid_moves_for_piece(board, x, y, player, rows, cols, score_cols)
                    all_moves.extend(piece_moves)
        return all_moves

# ==================== BOARD EVALUATION ====================

def count_stones_in_scoring_area(board: List[List[Any]], player: str, rows: int, cols: int, score_cols: List[int]) -> int:
    """Count how many stones a player has in their scoring area."""
    count = 0
    
    if player == "circle":
        score_row = top_score_row()
    else:
        score_row = bottom_score_row(rows)
    
    for x in score_cols:
        if in_bounds(x, score_row, rows, cols):
            piece = board[score_row][x]
            if piece and piece.owner == player and piece.side == "stone":
                count += 1
    
    return count

def basic_evaluate_board(board: List[List[Any]], player: str, rows: int, cols: int, score_cols: List[int]) -> float:
    """
    Basic board evaluation function.
    
    Returns a score where higher values are better for the given player.
    Students can use this as a starting point and improve it.
    """
    score = 0.0
    opponent = get_opponent(player)
    
    # Count stones in scoring areas
    player_scoring_stones = count_stones_in_scoring_area(board, player, rows, cols, score_cols)
    opponent_scoring_stones = count_stones_in_scoring_area(board, opponent, rows, cols, score_cols)
    
    score += player_scoring_stones * 100  
    score -= opponent_scoring_stones * 100  
    
    # Count total pieces and positional factors
    for y in range(rows):
        for x in range(cols):
            piece = board[y][x]
            if piece and piece.owner == player and piece.side == "stone":
                # Basic positional scoring
                if player == "circle":
                    score += (rows - y) * 0.1
                else:
                    score += y * 0.1
    
    return score

def simulate_move(board: List[List[Any]], move: Dict[str, Any], player: str, rows: int, cols: int, score_cols: List[int]) -> Tuple[bool, Any]:
    """
    Simulate a move on a copy of the board.
    
    Args:
        board: Current board state
        move: Move to simulate
        player: Player making the move
        rows, cols: Board dimensions
        score_cols: Scoring column indices
    
    Returns:
        (success: bool, new_board_state or error_message)
    """
    # Import the game engine's move validation function
    try:
        from gameEngine import validate_and_apply_move
        board_copy = copy.deepcopy(board)
        success, message = validate_and_apply_move(board_copy, move, player, rows, cols, score_cols)
        return success, board_copy if success else message
    except ImportError:
        # Fallback to basic simulation if game engine not available
        return True, copy.deepcopy(board)

# ==================== BASE AGENT CLASS ====================

class BaseAgent(ABC):
    """
    Abstract base class for all agents.
    """
    
    def __init__(self, player: str):
        """Initialize agent with player identifier."""
        self.player = player
        self.opponent = get_opponent(player)
    
    @abstractmethod
    def choose(self, board: List[List[Any]], rows: int, cols: int, score_cols: List[int]) -> Optional[Dict[str, Any]]:
        """
        Choose the best move for the current board state.
        
        Args:
            board: 2D list representing the game board
            rows, cols: Board dimensions
            score_cols: List of column indices for scoring areas
        
        Returns:
            Dictionary representing the chosen move, or None if no moves available
        """
        pass

# ==================== STUDENT AGENT IMPLEMENTATION ====================

class StudentAgent(BaseAgent):
    """
    Student Agent Implementation
    
    TODO: Implement your AI agent for the River and Stones game.
    The goal is to get 4 of your stones into the opponent's scoring area.
    
    You have access to these utility functions:
    - generate_all_moves(): Get all legal moves for current player
    - basic_evaluate_board(): Basic position evaluation 
    - simulate_move(): Test moves on board copy
    - count_stones_in_scoring_area(): Count stones in scoring positions
    """
    
    def __init__(self, player: str):
        super().__init__(player)
        # Strategy settings (you can tweak these or expose via CLI/env)
        # strategy: "minimax" or "mcts"
        self.strategy: str = "mcts"
        # Minimax search depth (ply)
        self.minimax_depth: int = 2
        # MCTS configuration
        self.mcts_time_limit_s: float = 0.01  # wall-clock seconds per move
        self.mcts_exploration_c: float = 1.414
        self.mcts_rollout_depth_limit: int = 30
        # Heuristic tunables
        self.weight_scoring_stones: float = 100.0
        self.weight_forward_push: float = 0.5
        self.weight_center_control: float = 0.05
        self.weight_advancement: float = 0.1
        # Debug toggles
        self.debug_mcts: bool = True

    # --------------- Core choose entrypoint ---------------
    
    def choose(self, board: List[List[Any]], rows: int, cols: int, score_cols: List[int]) -> Optional[Dict[str, Any]]:
        """
        Choose the best move for the current board state.
        
        Args:
            board: 2D list representing the game board
            rows, cols: Board dimensions  
            score_cols: Column indices for scoring areas
            
        Returns:
            Dictionary representing your chosen move
        """
        moves = generate_all_moves(board, self.player, rows, cols, score_cols)
        if not moves:
            return None

        # Immediate tactical: if any move wins now, take it.
        for m in moves:
            ok, nb = simulate_move(board, m, self.player, rows, cols, score_cols)
            if ok and self._check_terminal_winner(nb, rows, cols, score_cols) == self.player:
                return m

        if self.strategy == "minimax":
            best_move, _ = self._minimax_root(board, rows, cols, score_cols, self.minimax_depth)
            return best_move if best_move else random.choice(moves)
        elif self.strategy == "mcts":
            if self.debug_mcts:
                print(f"[MCTS] Starting search (limit={self.mcts_time_limit_s:.3f}s, c={self.mcts_exploration_c}, rollout_limit={self.mcts_rollout_depth_limit})")
            return self._mcts_search(board, rows, cols, score_cols, time_limit_s=self.mcts_time_limit_s)
        else:
            # Fallback
            return random.choice(moves)

    # --------------- Game termination helpers ---------------
    def _check_terminal_winner(self, board: List[List[Any]], rows: int, cols: int, score_cols: List[int]) -> Optional[str]:
        """Return winner ("circle"/"square") if win else None."""
        try:
            from gameEngine import check_win
            return check_win(board, rows, cols, score_cols)
        except Exception:
            # Fallback heuristic terminal check matching our utilities
            c = count_stones_in_scoring_area(board, "circle", rows, cols, score_cols)
            s = count_stones_in_scoring_area(board, "square", rows, cols, score_cols)
            if c >= 4:
                return "circle"
            if s >= 4:
                return "square"
            return None

    def _is_terminal(self, board: List[List[Any]], rows: int, cols: int, score_cols: List[int]) -> Tuple[bool, Optional[str]]:
        """Return (is_terminal, winner_or_None). Also terminal if no legal moves for current player.
        Note: In search we pass the perspective player explicitly when needed.
        """
        winner = self._check_terminal_winner(board, rows, cols, score_cols)
        if winner is not None:
            return True, winner
        return False, None

    # --------------- Heuristic evaluation ---------------
    def _evaluate(self, board, player, rows, cols, score_cols):
        value = 0.0
        opponent = get_opponent(player)

        # Stones in scoring area (win)
        value += 120 * count_stones_in_scoring_area(board, player, rows, cols, score_cols)
        value -= 120 * count_stones_in_scoring_area(board, opponent, rows, cols, score_cols)

        # Stones that can reach scoring area in one move
        try:
            from gameEngine import countreachableinone
            value += 50 * countreachableinone(board, player, rows, cols, score_cols)
        except Exception:
            pass

        # Blocking opponent's entrance and rivers that block flows
        block_row = top_score_row() if player == "square" else bottom_score_row(rows)
        for x in score_cols:
            p = board[block_row][x]
            if p and p.owner == player:
                if p.side == "stone":
                    value += 40
                if p.side == "river":
                    value += 15

        # Central control, advancement and clustering
        center_cols = set(score_cols)
        for y in range(rows):
            for x in range(cols):
                p = board[y][x]
                if p and p.owner == player:
                    if p.side == "stone":
                        if x in center_cols:
                            value += 0.25
                        value += (rows - y) * 0.15 if player == "circle" else y * 0.15
                        # Clustering: adjacent stones
                        for dx in [-1, 0, 1]:
                            for dy in [-1, 0, 1]:
                                nx, ny = x + dx, y + dy
                                if in_bounds(nx, ny, rows, cols):
                                    q = board[ny][nx]
                                    if q and q.owner == player and q.side == "stone" and (nx,ny)!=(x,y):
                                        value += 1
                    # Rivers: potentially add logic to score useful river networks
        
        # Penalize pieces threatened by opponent (not detailed here for speed)
        # Bonus: reward for controlling flows, strategic flips, or pins

        return value


    # --------------- Minimax with alpha-beta ---------------
    def _minimax_root(self, board: List[List[Any]], rows: int, cols: int, score_cols: List[int], depth: int) -> Tuple[Optional[Dict[str, Any]], float]:
        best_score = -math.inf
        best_move: Optional[Dict[str, Any]] = None
        alpha = -math.inf
        beta = math.inf

        legal = generate_all_moves(board, self.player, rows, cols, score_cols)
        random.shuffle(legal)
        for m in legal:
            ok, nb = simulate_move(board, m, self.player, rows, cols, score_cols)
            if not ok:
                continue
            score = self._minimax(nb, rows, cols, score_cols, depth - 1, alpha, beta, maximizing=False, to_move=self.opponent)
            if score > best_score:
                best_score = score
                best_move = m
            alpha = max(alpha, best_score)
            if beta <= alpha:
                break
        return best_move, best_score

    def _minimax(self, board: List[List[Any]], rows: int, cols: int, score_cols: List[int], depth: int,
                 alpha: float, beta: float, maximizing: bool, to_move: str) -> float:
        terminal, winner = self._is_terminal(board, rows, cols, score_cols)
        if terminal:
            if winner == self.player:
                return 1e9  # large win
            elif winner == self.opponent:
                return -1e9  # large loss
            else:
                return 0.0
        if depth <= 0:
            return self._evaluate(board, self.player, rows, cols, score_cols)

        moves = generate_all_moves(board, to_move, rows, cols, score_cols)
        if not moves:
            # No legal moves for current to_move; treat as evaluation
            return self._evaluate(board, self.player, rows, cols, score_cols)

        if maximizing:
            value = -math.inf
            random.shuffle(moves)
            for m in moves:
                ok, nb = simulate_move(board, m, to_move, rows, cols, score_cols)
                if not ok:
                    continue
                value = max(value, self._minimax(nb, rows, cols, score_cols, depth - 1, alpha, beta, False, get_opponent(to_move)))
                alpha = max(alpha, value)
                if beta <= alpha:
                    break
            return value
        else:
            value = math.inf
            random.shuffle(moves)
            for m in moves:
                ok, nb = simulate_move(board, m, to_move, rows, cols, score_cols)
                if not ok:
                    continue
                value = min(value, self._minimax(nb, rows, cols, score_cols, depth - 1, alpha, beta, True, get_opponent(to_move)))
                beta = min(beta, value)
                if beta <= alpha:
                    break
            return value

    # --------------- Monte Carlo Tree Search (UCT) ---------------
    class _MCTSNode:
        def __init__(self, parent: Optional['StudentAgent._MCTSNode'], move: Optional[Dict[str, Any]],
                     player_to_move: str, board: List[List[Any]]):
            self.parent = parent
            self.move = move  # move that led to this node from parent
            self.player_to_move = player_to_move
            self.board = board
            self.children: List['StudentAgent._MCTSNode'] = []
            self.untried_moves: List[Dict[str, Any]] = []
            self.N: int = 0  # visit count
            self.W: float = 0.0  # total value (from root player perspective)
            self.Q: float = 0.0  # mean value

        def is_fully_expanded(self) -> bool:
            return len(self.untried_moves) == 0

        def best_child(self, c: float):
            def uct(n):
                if n.N == 0:
                    return float("inf")
                h = self._evaluate(n.board, self.player, len(n.board), len(n.board[0]), score_cols_for(len(n.board[0])))
                return n.Q + c * math.sqrt(math.log(self.N + 1) / n.N) + 0.002 * h  # Tune the bias weight
            return max(self.children, key=uct)


    def _mcts_search(self, board: List[List[Any]], rows: int, cols: int, score_cols: List[int], time_limit_s: float) -> Dict[str, Any]:
        root = StudentAgent._MCTSNode(parent=None, move=None, player_to_move=self.player, board=copy.deepcopy(board))
        root.untried_moves = generate_all_moves(root.board, root.player_to_move, rows, cols, score_cols)

        start_time = time.time()
        end_time = start_time + max(0.01, time_limit_s)
        iterations = 0

        while time.time() < end_time:
            node = root
            iterations += 1

            # 1) Selection
            while node.is_fully_expanded() and node.children:
                node = node.best_child(self.mcts_exploration_c)

            # 2) Expansion
            if node.untried_moves:
                m = node.untried_moves.pop(random.randrange(len(node.untried_moves)))
                ok, nb = simulate_move(node.board, m, node.player_to_move, rows, cols, score_cols)
                if not ok:
                    # Skip invalid; continue next iteration
                    continue
                next_player = get_opponent(node.player_to_move)
                child = StudentAgent._MCTSNode(parent=node, move=m, player_to_move=next_player, board=nb)
                child.untried_moves = generate_all_moves(nb, next_player, rows, cols, score_cols)
                node.children.append(child)
                node = child

            # 3) Simulation (rollout)
            rollout_value = self._mcts_rollout(node.board, node.player_to_move, rows, cols, score_cols)

            # 4) Backpropagation (value from root player's perspective)
            self._mcts_backpropagate(node, rollout_value)

        # Choose the most visited child move
        if not root.children:
            legal = generate_all_moves(board, self.player, rows, cols, score_cols)
            return random.choice(legal) if legal else None
        best = max(root.children, key=lambda n: n.N)
        if self.debug_mcts:
            elapsed = max(1e-6, time.time() - start_time)
            iters_per_s = iterations / elapsed
            print(f"[MCTS] Finished: iterations={iterations}, elapsed={elapsed:.3f}s, iters/s={iters_per_s:.0f}, children={len(root.children)}")
            # Show top-3 children by visits
            top = sorted(root.children, key=lambda n: n.N, reverse=True)[:3]
            for i, ch in enumerate(top, 1):
                print(f"[MCTS]  #{i} visits={ch.N}, Q={ch.Q:.3f}, move={ch.move}")
            print(f"[MCTS] Selected move visits={best.N}, Q={best.Q:.3f}, move={best.move}")
        return best.move if best.move else random.choice(generate_all_moves(board, self.player, rows, cols, score_cols))

    def _mcts_rollout(self, board, to_move, rows, cols, score_cols):
        depth = 0
        current_board = copy.deepcopy(board)
        current_player = to_move
        while depth < self.mcts_rollout_depth_limit:
            winner = self._check_terminal_winner(current_board, rows, cols, score_cols)
            if winner is not None:
                if winner == self.player:
                    return 1.0
                elif winner == self.opponent:
                    return -1.0
                return 0.0
            moves = generate_all_moves(current_board, current_player, rows, cols, score_cols)
            if not moves:
                v = self._evaluate(current_board, self.player, rows, cols, score_cols)
                return max(-1.0, min(1.0, v / 1000.0))
            
            # FIXED: Evaluate from current player's perspective
            best_m = None
            best_v = -math.inf
            for m in moves:
                ok, nb = simulate_move(current_board, m, current_player, rows, cols, score_cols)
                if not ok:
                    continue
                # KEY FIX: Evaluate from current_player perspective
                v = self._evaluate(nb, current_player, rows, cols, score_cols)
                # Convert to root player perspective for consistent comparison
                if current_player != self.player:
                    v = -v  # Invert score if opponent is moving
                if v > best_v:
                    best_v = v
                    best_m = m
            m = best_m if best_m else random.choice(moves)
            
            ok, current_board = simulate_move(current_board, m, current_player, rows, cols, score_cols)
            if not ok:
                return 0.0
            current_player = get_opponent(current_player)
            depth += 1
        v = self._evaluate(current_board, self.player, rows, cols, score_cols)
        return max(-1.0, min(1.0, v / 1000.0))



    def _mcts_backpropagate(self, node: 'StudentAgent._MCTSNode', value: float) -> None:
        cur = node
        while cur is not None:
            cur.N += 1
            # If the node's player_to_move is the same as root player, value is as-is; otherwise invert
            if cur.player_to_move == self.player:
                cur.W += value
            else:
                cur.W -= value
            cur.Q = cur.W / cur.N
            cur = cur.parent

# ==================== TESTING HELPERS ====================

def test_student_agent():
    """
    Basic test to verify the student agent can be created and make moves.
    """
    print("Testing StudentAgent...")
    
    try:
        from gameEngine import default_start_board, DEFAULT_ROWS, DEFAULT_COLS
        
        rows, cols = DEFAULT_ROWS, DEFAULT_COLS
        score_cols = score_cols_for(cols)
        board = default_start_board(rows, cols)
        
        # Test minimax
        agent = StudentAgent("circle")
        agent.strategy = "minimax"
        agent.minimax_depth = 2
        move = agent.choose(board, rows, cols, score_cols)
        
        if move:
            print("✓ Agent successfully generated a move")
        else:
            print("✗ Agent returned no move")
        
        # Test MCTS
        agent_mcts = StudentAgent("circle")
        agent_mcts.strategy = "mcts"
        agent_mcts.mcts_time_limit_s = 0.2
        move2 = agent_mcts.choose(board, rows, cols, score_cols)
        if move2:
            print("✓ MCTS generated a move")
        else:
            print("✗ MCTS returned no move")
    
    except ImportError:
        agent = StudentAgent("circle")
        print("✓ StudentAgent created successfully")

if __name__ == "__main__":
    # Run basic test when file is executed directly
    test_student_agent()
