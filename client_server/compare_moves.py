from typing import List, Dict, Any, Optional, Tuple, Set
import copy

# Reuse engine functions
from gameEngine import (
    Piece,
    default_start_board,
    score_cols_for,
    get_river_flow_destinations,
    in_bounds,
    is_opponent_score_cell,
    generate_all_moves,
)

Move = Dict[str, Any]


def cpp_like_get_legal_moves(board: List[List[Optional[Piece]]],
                             player: str,
                             rows: int,
                             cols: int,
                             score_cols: List[int]) -> List[Move]:
    """
    Simulate C++ get_legal_moves but with Python generate_all_moves' mutation semantics:
    - Use a mutated view of the board as we iterate row-major.
    - After processing a stone, leave it as a vertical river.
    - After processing a river, toggle its orientation and leave it toggled.
    - Add both stone->river flips (both orientations) unconditionally.
    - Add river->stone flip and rotate unconditionally.
    """
    moves: List[Move] = []
    # Build a lightweight mutable view: represent as tuples (owner, side, ori) or None
    mut = [[None if cell is None else (cell.owner, cell.side, cell.orientation) for cell in row] for row in board]

    def owner_of(x: int, y: int) -> Optional[str]:
        c = mut[y][x]
        return c[0] if c else None

    dirs = [(1,0),(-1,0),(0,1),(0,-1)]

    for y in range(rows):
        for x in range(cols):
            cell = mut[y][x]
            if cell is None: 
                continue
            owner, side, ori = cell
            if owner != player:
                continue

            # Moves/pushes using current mutated board state
            for dx,dy in dirs:
                tx,ty = x+dx,y+dy
                if not in_bounds(tx,ty,rows,cols):
                    continue
                if is_opponent_score_cell(tx,ty,player,rows,cols,score_cols):
                    continue
                target = mut[ty][tx]
                if target is None:
                    moves.append({"action":"move","from":[x,y],"to":[tx,ty]})
                elif target[1] == "river":
                    # build a transient Piece grid from mut to reuse engine's river flow
                    # (river flow only reads owner/side/orientation, so conversion is fine)
                    tmp_board: List[List[Optional[Piece]]] = [[None for _ in range(cols)] for _ in range(rows)]
                    for yy in range(rows):
                        for xx in range(cols):
                            t = mut[yy][xx]
                            if t:
                                tmp_board[yy][xx] = Piece(t[0], t[1], t[2])
                    flow = get_river_flow_destinations(tmp_board, tx, ty, x, y, player, rows, cols, score_cols)
                    for d in flow:
                        moves.append({"action":"move","from":[x,y],"to":[d[0],d[1]]})
                else:
                    if side == "stone":
                        px,py = tx+dx, ty+dy
                        if in_bounds(px,py,rows,cols) and mut[py][px] is None and not is_opponent_score_cell(px,py, owner_of(tx,ty),rows,cols,score_cols):
                            moves.append({"action":"push","from":[x,y],"to":[tx,ty],"pushed_to":[px,py]})
                    else:
                        pushed_player = owner_of(tx,ty)
                        # transient board for river push flow
                        tmp_board: List[List[Optional[Piece]]] = [[None for _ in range(cols)] for _ in range(rows)]
                        for yy in range(rows):
                            for xx in range(cols):
                                t = mut[yy][xx]
                                if t:
                                    tmp_board[yy][xx] = Piece(t[0], t[1], t[2])
                        flow = get_river_flow_destinations(tmp_board, tx, ty, x, y, pushed_player, rows, cols, score_cols, river_push=True)
                        for d in flow:
                            if not is_opponent_score_cell(d[0],d[1],pushed_player,rows,cols,score_cols):
                                moves.append({"action":"push","from":[x,y],"to":[tx,ty],"pushed_to":[d[0],d[1]]})

            # flips/rotates (unconditional, and mutate mut to mimic Python behavior)
            if side == "stone":
                # add both orientations
                for o in ("horizontal","vertical"):
                    moves.append({"action":"flip","from":[x,y],"orientation":o})
                # leave as vertical river afterwards
                mut[y][x] = (owner, "river", "vertical")
            else:
                # river -> stone flip
                moves.append({"action":"flip","from":[x,y]})
                # rotate
                new_o = "horizontal" if ori == "vertical" else "vertical"
                mut[y][x] = (owner, "river", new_o)
                moves.append({"action":"rotate","from":[x,y]})

    if not moves:
        moves.append({"action":"move","from":[0,0],"to":[0,0]})
    return moves


def normalize_move(m: Move) -> Tuple:
    action = m.get("action")
    fr = tuple(m.get("from", []))
    to = tuple(m.get("to", []))
    pushed = tuple(m.get("pushed_to", [])) if m.get("pushed_to") is not None else tuple()
    ori = m.get("orientation") if m.get("orientation") else None
    return (action, fr, to, pushed, ori)


def compare_on_board(board: List[List[Optional[Piece]]], player: str, rows: int, cols: int, score_cols: List[int]) -> Dict[str, Any]:
    # Use independent deep copies because generate_all_moves mutates pieces during generation
    py_board = copy.deepcopy(board)
    cpp_board = copy.deepcopy(board)
    py_moves = generate_all_moves(py_board, player, rows, cols, score_cols)
    cpp_like_moves = cpp_like_get_legal_moves(cpp_board, player, rows, cols, score_cols)
    set_py = {normalize_move(m) for m in py_moves}
    set_cpp = {normalize_move(m) for m in cpp_like_moves}

    missing_in_cpp = sorted(set_py - set_cpp)
    extra_in_cpp = sorted(set_cpp - set_py)
    return {
        "py_count": len(set_py),
        "cpp_like_count": len(set_cpp),
        "missing_in_cpp": missing_in_cpp,
        "extra_in_cpp": extra_in_cpp,
    }


def main():
    rows, cols = 13, 12
    score_cols = score_cols_for(cols)
    board = default_start_board(rows, cols)

    for player in ("circle", "square"):
        result = compare_on_board(copy.deepcopy(board), player, rows, cols, score_cols)
        print(f"Player {player}: Python moves={result['py_count']}, C++-like moves={result['cpp_like_count']}")
        print(f"Missing in C++-like (first 10): {result['missing_in_cpp'][:10]}")
        print(f"Extra in C++-like (first 10): {result['extra_in_cpp'][:10]}")
        print("-")

if __name__ == "__main__":
    main()
