#include "game_state.h"
#include <algorithm>
#include <queue>
#include <cmath>
#include <iostream>

// ---- Helper functions ----
int top_score_row() {
    return 2;
}

int bottom_score_row(int rows) {
    return rows - 3;
}

std::string get_opponent(const std::string& player) {
    return (player == "circle") ? "square" : "circle";
}

bool in_bounds(int x, int y, int rows, int cols) {
    return x >= 0 && x < cols && y >= 0 && y < rows;
}

bool is_opponent_score_cell(int x, int y, const std::string& player, int rows, int cols, const std::vector<int>& score_cols) {
    // Check if position is in the score_cols
    bool in_score_cols = std::find(score_cols.begin(), score_cols.end(), x) != score_cols.end();
    if (!in_score_cols) return false;
    
    if (player == "circle") {
        return y == bottom_score_row(rows);
    } else {
        return y == top_score_row();
    }
}

bool is_my_score_cell(int x, int y, const std::string& player, int rows, int cols, const std::vector<int>& score_cols) {
    // Check if position is in the score_cols
    bool in_score_cols = std::find(score_cols.begin(), score_cols.end(), x) != score_cols.end();
    if (!in_score_cols) return false;
    
    if (player == "circle") {
        return y == top_score_row();
    } else {
        return y == bottom_score_row(rows);
    }
}

bool check_win_state(const std::vector<std::vector<std::map<std::string, std::string>>>& board,
                     int rows, int cols, const std::vector<int>& score_cols) {
    int WIN_COUNT = 4;
    int top = top_score_row();
    int bot = bottom_score_row(rows);
    int ccount = 0, scount = 0;

    for (int x : score_cols) {
        if (x >= 0 && x < cols) {
            if (top >= 0 && top < rows) {
                const auto& cell = board[top][x];
                if (!cell.empty() && cell.at("owner") == "circle" && cell.at("side") == "stone") {
                    ccount++;
                }
            }
            if (bot >= 0 && bot < rows) {
                const auto& cell = board[bot][x];
                if (!cell.empty() && cell.at("owner") == "square" && cell.at("side") == "stone") {
                    scount++;
                }
            }
        }
    }

    return (ccount >= WIN_COUNT || scount >= WIN_COUNT);
}

// ---- Encoding functions ----
EncodedCell GameState::encode_cell(const std::map<std::string, std::string>& cell) {
    if (cell.empty()) return 0;

    const std::string& owner = cell.at("owner");
    const std::string& side = cell.at("side");

    if (owner == "square") {
        if (side == "stone") return 3;
        else if (side == "river") {
            const std::string& orientation = cell.at("orientation");
            return (orientation == "horizontal") ? 1 : 2;
        }
    } else if (owner == "circle") {
        if (side == "stone") return 6;
        else if (side == "river") {
            const std::string& orientation = cell.at("orientation");
            return (orientation == "horizontal") ? 4 : 5;
        }
    }
    return 0;
}

void GameState::encode_board() {
    encoded_board.resize(rows, std::vector<EncodedCell>(cols, 0));
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            encoded_board[y][x] = encode_cell(board[y][x]);
        }
    }
}

// Helper to update a single cell's encoding
void update_cell_encoding(std::vector<std::vector<EncodedCell>>& encoded_board,
                          const std::vector<std::vector<std::map<std::string, std::string>>>& board,
                          int x, int y) {
    encoded_board[y][x] = GameState::encode_cell(board[y][x]);
}

// River flow destination calculation (based on gameEngine.py implementation)
std::vector<std::pair<int,int>> get_river_flow_destinations(
    const std::vector<std::vector<std::map<std::string, std::string>>>& board,
    int rx, int ry, int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols,
    bool river_push) {
    
    std::vector<std::pair<int,int>> destinations;
    std::set<std::pair<int,int>> visited;
    std::queue<std::pair<int,int>> queue;
    
    queue.push({rx, ry});
    
    while (!queue.empty()) {
        auto [x, y] = queue.front();
        queue.pop();
        
        if (visited.count({x, y}) || !in_bounds(x, y, rows, cols)) continue;
        visited.insert({x, y});
        
        const auto& cell = board[y][x];
        
        // Handle special case for river_push
        const auto* actual_cell = &cell;
        if (river_push && x == rx && y == ry) {
            actual_cell = &board[sy][sx];
        }
        
        if (actual_cell->empty()) {
            if (is_opponent_score_cell(x, y, player, rows, cols, score_cols)) {
                // Block entering opponent score
                continue;
            } else {
                destinations.push_back({x, y});
            }
            continue;
        }
        
        if (actual_cell->at("side") != "river") {
            continue;
        }
        
        // Get directions based on river orientation
        std::vector<std::pair<int,int>> dirs;
        if (actual_cell->at("orientation") == "horizontal") {
            dirs = {{1, 0}, {-1, 0}};
        } else {
            dirs = {{0, 1}, {0, -1}};
        }
        
        for (auto [dx, dy] : dirs) {
            int nx = x + dx, ny = y + dy;
            while (in_bounds(nx, ny, rows, cols)) {
                if (is_opponent_score_cell(nx, ny, player, rows, cols, score_cols)) {
                    break;
                }
                
                const auto& next_cell = board[ny][nx];
                if (next_cell.empty()) {
                    destinations.push_back({nx, ny});
                    nx += dx; ny += dy;
                    continue;
                }
                if (nx == sx && ny == sy) {
                    nx += dx; ny += dy;
                    continue;
                }
                if (next_cell.at("side") == "river") {
                    queue.push({nx, ny});
                    break;
                }
                break;
            }
        }
    }
    
    // Remove duplicates
    std::vector<std::pair<int,int>> out;
    std::set<std::pair<int,int>> seen;
    for (const auto& d : destinations) {
        if (seen.find(d) == seen.end()) {
            seen.insert(d);
            out.push_back(d);
        }
    }
    return out;
}

// Compute valid targets for a piece (based on compute_valid_targets in gameEngine.py)
ValidTargets compute_valid_targets(
    const std::vector<std::vector<std::map<std::string, std::string>>>& board,
    int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols) {
    
    ValidTargets result;
    
    if (!in_bounds(sx, sy, rows, cols)) {
        return result;
    }
    
    const auto& piece = board[sy][sx];
    if (piece.empty() || piece.at("owner") != player) {
        return result;
    }
    
    std::vector<std::pair<int,int>> dirs = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    
    for (auto [dx, dy] : dirs) {
        int tx = sx + dx, ty = sy + dy;
        if (!in_bounds(tx, ty, rows, cols)) continue;
        
        // Block entering opponent score cell
        if (is_opponent_score_cell(tx, ty, player, rows, cols, score_cols)) {
            continue;
        }
        
        const auto& target = board[ty][tx];
        if (target.empty()) {
            result.moves.insert({tx, ty});
        } else if (target.at("side") == "river") {
            auto flow = get_river_flow_destinations(board, tx, ty, sx, sy, player, rows, cols, score_cols);
            for (const auto& d : flow) {
                result.moves.insert(d);
            }
        } else {
            // Stone occupied
            if (piece.at("side") == "stone") {
                int px = tx + dx, py = ty + dy;
                std::string pushed_player = target.at("owner");
                if (in_bounds(px, py, rows, cols) && board[py][px].empty() && 
                    !is_opponent_score_cell(px, py, pushed_player, rows, cols, score_cols) && !is_opponent_score_cell(tx, ty, piece.at("owner"), rows, cols, score_cols)) {
                    result.pushes.push_back({{tx, ty}, {px, py}});
                }
            } else {
                // River pushing stone
                std::string pushed_player = target.at("owner");
                auto flow = get_river_flow_destinations(board, tx, ty, sx, sy, pushed_player, rows, cols, score_cols, true);
                for (const auto& d : flow) {
                    if (!is_opponent_score_cell(d.first, d.second, pushed_player, rows, cols, score_cols)) {
                        result.pushes.push_back({{tx, ty}, d});
                    }
                }
            }
        }
    }
    
    return result;
}

// ---- Game State Implementation ----
GameState GameState::copy() const {
    GameState copied(board, current_player, rows, cols, score_cols);
    copied.encoded_board = encoded_board;  // Copy encoded board as well
    return copied;
}

void GameState::apply_move(const Move& move) {
    if (move.action == "move") {
        int fx = move.from[0], fy = move.from[1];
        int tx = move.to[0], ty = move.to[1];
        
        // Validate bounds
        if (!in_bounds(fx, fy, rows, cols) || !in_bounds(tx, ty, rows, cols)) {
            return;
        }
        
        // Validate piece ownership
        if (board[fy][fx].empty() || board[fy][fx].at("owner") != current_player) {
            return;
        }
        
        // Check if destination is opponent's score cell
        if (is_opponent_score_cell(tx, ty, current_player, rows, cols, score_cols)) {
            return;
        }
        
        // If destination is empty, simple move
        if (board[ty][tx].empty()) {
            board[ty][tx] = board[fy][fx];
            board[fy][fx].clear();
            // Update encoded board
            update_cell_encoding(encoded_board, board, tx, ty);
            update_cell_encoding(encoded_board, board, fx, fy);
        }
        // If destination is occupied, this should be a push with pushed_to
        else {
            // Check if pushed_to is provided
            if (move.pushed_to.empty()) {
                return; // Invalid: destination occupied but no pushed_to
            }

            int ptx = move.pushed_to[0], pty = move.pushed_to[1];
            int dx = tx - fx, dy = ty - fy;

            // Validate pushed_to is in correct direction
            if (ptx != tx + dx || pty != ty + dy) {
                return;
            }

            // Validate pushed_to bounds
            if (!in_bounds(ptx, pty, rows, cols)) {
                return;
            }

            // Check if pushed_to is opponent's score cell
            if (is_opponent_score_cell(ptx, pty, current_player, rows, cols, score_cols)) {
                return;
            }

            // Validate pushed_to is empty
            if (!board[pty][ptx].empty()) {
                return;
            }

            // Apply move with push
            board[pty][ptx] = board[ty][tx];
            board[ty][tx] = board[fy][fx];
            board[fy][fx].clear();
            // Update encoded board
            update_cell_encoding(encoded_board, board, ptx, pty);
            update_cell_encoding(encoded_board, board, tx, ty);
            update_cell_encoding(encoded_board, board, fx, fy);
        }
    }
    else if (move.action == "push") {
        int fx = move.from[0], fy = move.from[1];
        int tx = move.to[0], ty = move.to[1];
        int px = move.pushed_to[0], py = move.pushed_to[1];
        
        // Validate bounds
        if (!in_bounds(fx, fy, rows, cols) || !in_bounds(tx, ty, rows, cols) || 
            !in_bounds(px, py, rows, cols)) {
            return;
        }
        
        // Validate piece ownership
        if (board[fy][fx].empty() || board[fy][fx].at("owner") != current_player) {
            return;
        }
        
        // Get pushed player
        std::string pushed_player = board[ty][tx].empty() ? "" : board[ty][tx].at("owner");
        
        // Check if either position is opponent's score cell
        if (is_opponent_score_cell(tx, ty, current_player, rows, cols, score_cols) ||
            (!pushed_player.empty() && is_opponent_score_cell(px, py, pushed_player, rows, cols, score_cols))) {
            return;
        }
        
        // Validate target is occupied
        if (board[ty][tx].empty()) {
            return;
        }
        
        // Validate pushed_to is empty
        if (!board[py][px].empty()) {
            return;
        }
        
        // Rivers cannot push rivers
        if (board[fy][fx].at("side") == "river" && board[ty][tx].at("side") == "river") {
            return;
        }
        
        // Validate push is legal using compute_valid_targets
        auto valid_targets = compute_valid_targets(board, fx, fy, current_player, rows, cols, score_cols);
        bool valid_push = false;
        for (const auto& push_pair : valid_targets.pushes) {
            if (push_pair.first == std::make_pair(tx, ty) && push_pair.second == std::make_pair(px, py)) {
                valid_push = true;
                break;
            }
        }
        
        if (!valid_push) {
            return;
        }
        
        // Apply the push
        board[py][px] = board[ty][tx];  // Enemy goes to pushed_to
        board[ty][tx] = board[fy][fx];  // Mover goes into enemy's cell
        board[fy][fx].clear();           // Origin cleared

        // If mover is a river, flip it to stone
        if (board[ty][tx].at("side") == "river") {
            board[ty][tx]["side"] = "stone";
            board[ty][tx].erase("orientation");
        }

        // Update encoded board
        update_cell_encoding(encoded_board, board, px, py);
        update_cell_encoding(encoded_board, board, tx, ty);
        update_cell_encoding(encoded_board, board, fx, fy);
    }
    else if (move.action == "flip") {
        int fx = move.from[0], fy = move.from[1];
        
        // Validate bounds
        if (!in_bounds(fx, fy, rows, cols)) {
            return;
        }
        
        // Validate piece exists and belongs to current player
        if (board[fy][fx].empty() || board[fy][fx].at("owner") != current_player) {
            return;
        }
        
        if (board[fy][fx].at("side") == "stone") {
            // Flipping stone to river requires orientation
            if (move.orientation.empty() || 
                (move.orientation != "horizontal" && move.orientation != "vertical")) {
                return;
            }
            
            // Temporarily flip to check if river flow would reach opponent score
            std::string old_side = board[fy][fx].at("side");
            board[fy][fx]["side"] = "river";
            board[fy][fx]["orientation"] = move.orientation;
            
            auto flow = get_river_flow_destinations(board, fx, fy, fx, fy, current_player, rows, cols, score_cols);
            
            // Check if any flow destination is opponent's score cell
            bool invalid_flow = false;
            for (const auto& dest : flow) {
                if (is_opponent_score_cell(dest.first, dest.second, current_player, rows, cols, score_cols)) {
                    invalid_flow = true;
                    break;
                }
            }
            
            // Revert if invalid
            if (invalid_flow) {
                board[fy][fx]["side"] = old_side;
                board[fy][fx].erase("orientation");
                return;
            }
            // Otherwise flip is already applied
            update_cell_encoding(encoded_board, board, fx, fy);
        } else {
            // Flipping river to stone
            board[fy][fx]["side"] = "stone";
            board[fy][fx].erase("orientation");
            update_cell_encoding(encoded_board, board, fx, fy);
        }
    }
    else if (move.action == "rotate") {
        int fx = move.from[0], fy = move.from[1];
        
        // Validate bounds
        if (!in_bounds(fx, fy, rows, cols)) {
            return;
        }
        
        // Validate piece exists and belongs to current player
        if (board[fy][fx].empty() || board[fy][fx].at("owner") != current_player) {
            return;
        }
        
        // Only rivers can be rotated
        if (board[fy][fx].at("side") != "river") {
            return;
        }
        
        // Perform rotation
        std::string current_ori = board[fy][fx].at("orientation");
        std::string new_ori = (current_ori == "horizontal") ? "vertical" : "horizontal";
        board[fy][fx]["orientation"] = new_ori;
        
        // Check if rotation causes flow into opponent score
        auto flow = get_river_flow_destinations(board, fx, fy, fx, fy, current_player, rows, cols, score_cols);
        
        bool invalid_flow = false;
        for (const auto& dest : flow) {
            if (is_opponent_score_cell(dest.first, dest.second, current_player, rows, cols, score_cols)) {
                invalid_flow = true;
                break;
            }
        }
        
        // Revert rotation if invalid
        if (invalid_flow) {
            board[fy][fx]["orientation"] = current_ori;
            return;
        }
        // Update encoded board
        update_cell_encoding(encoded_board, board, fx, fy);
    }

    // Switch current player
    current_player = (current_player == "circle") ? "square" : "circle";
}

std::vector<Move> GameState::get_legal_moves() const {
    std::vector<Move> moves;

    // Iterate over board to find current player's pieces
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            const auto &cell = board[y][x];
            if (cell.empty()) continue;

            if (cell.at("owner") != current_player) continue; // only current player's pieces

            std::string side_type = cell.at("side");

            // ---- MOVES (including river flow) ----
            auto valid_targets = compute_valid_targets(board, x, y, current_player, rows, cols, score_cols);

            // Add regular moves and river flow moves
            for (const auto& target : valid_targets.moves) {
                moves.push_back({"move", {x,y}, {target.first, target.second}, {}, ""});
                }

                // ---- PUSHES (including river flow pushes) ----
                for (const auto& push : valid_targets.pushes) {
                    auto target_pos = push.first;
                    auto pushed_pos = push.second;
                    moves.push_back({"push", {x,y}, {target_pos.first, target_pos.second}, 
                                   {pushed_pos.first, pushed_pos.second}, ""});
                }
            // }

            // ---- FLIP ----
            if (side_type == "stone") {

                    // Check if flipping to river would be safe (not flowing into opponent score)
                    for (const std::string& orientation : {"horizontal", "vertical"}) {
                        // Simulate the flip and check resulting flow
                        bool safe = true;
                        
                        // Create a temporary modified board to test the flip
                        auto test_board = board;
                        test_board[y][x]["side"] = "river";
                        test_board[y][x]["orientation"] = orientation;
                        
                        auto flow = get_river_flow_destinations(test_board, x, y, x, y, current_player, rows, cols, score_cols);
                        for (const auto& dest : flow) {
                            if (is_opponent_score_cell(dest.first, dest.second, current_player, rows, cols, score_cols)) {
                                safe = false;
                                break;
                            }
                        }
                        
                        if (safe) {
                            moves.push_back({"flip", {x,y}, {x,y}, {}, orientation});
                        }
                    }
            } else if (side_type == "river") {
                // Always allow flipping river to stone (including in scoring area)
                moves.push_back({"flip", {x,y}, {x,y}, {}, ""});
            }

            // ---- ROTATE ----
            // Only allow rotation of rivers
            if (side_type == "river") {
                // Check if rotation would be safe
                std::string current_orientation = cell.at("orientation");
                std::string new_orientation = (current_orientation == "horizontal") ? "vertical" : "horizontal";
                
                // Create a temporary modified board to test the rotation
                auto test_board = board;
                test_board[y][x]["orientation"] = new_orientation;
                
                auto flow = get_river_flow_destinations(test_board, x, y, x, y, current_player, rows, cols, score_cols);
                bool safe = true;
                for (const auto& dest : flow) {
                    if (is_opponent_score_cell(dest.first, dest.second, current_player, rows, cols, score_cols)) {
                        safe = false;
                        break;
                    }
                }
                
                if (safe) {
                    moves.push_back({"rotate", {x,y}, {x,y}, {}, ""});
                }
            }
        }
    }

    if (moves.empty()) {
        moves.push_back({"move", {0,0}, {0,0}, {}, ""}); // fallback
    }

    return moves;
}

bool GameState::is_terminal() const {
    return check_win_state(board, rows, cols, score_cols);
}

// ---- Make/Undo Move Implementation (for efficient minimax without copying) ----
// IMPORTANT: These methods assume moves are legal (already validated by get_legal_moves)
// They modify the board in-place and track undo information

GameState::UndoInfo GameState::make_move(const Move& move) {
    UndoInfo undo;
    undo.valid = true;
    undo.prev_player = current_player;

    int fx = move.from[0], fy = move.from[1];
    int tx = move.to[0], ty = move.to[1];

    // Save state for undo
    undo.from_cell = board[fy][fx];
    undo.to_cell = board[ty][tx];
    undo.from_encoded = encoded_board[fy][fx];
    undo.to_encoded = encoded_board[ty][tx];

    if (move.action == "move") {
        // Simple move or move with inline push
        if (board[ty][tx].empty()) {
            // Simple move: from -> to, from becomes empty
            board[ty][tx] = board[fy][fx];
            board[fy][fx].clear();
            encoded_board[ty][tx] = undo.from_encoded;
            encoded_board[fy][fx] = 0;
        } else {
            // Move with push (pushed_to is provided)
            int ptx = move.pushed_to[0], pty = move.pushed_to[1];
            undo.pushed_cell = std::map<std::string, std::string>(); // empty
            undo.pushed_encoded = 0;

            // from -> to, to -> pushed_to, from becomes empty
            board[pty][ptx] = board[ty][tx];
            board[ty][tx] = board[fy][fx];
            board[fy][fx].clear();

            encoded_board[pty][ptx] = undo.to_encoded;
            encoded_board[ty][tx] = undo.from_encoded;
            encoded_board[fy][fx] = 0;
        }
    }
    else if (move.action == "push") {
        int px = move.pushed_to[0], py = move.pushed_to[1];
        undo.pushed_cell = std::map<std::string, std::string>(); // empty
        undo.pushed_encoded = 0;

        bool was_river = (board[fy][fx].at("side") == "river");

        // Apply push: from -> to, to -> pushed_to, from becomes empty
        board[py][px] = board[ty][tx];
        board[ty][tx] = board[fy][fx];
        board[fy][fx].clear();

        // If pusher is a river, flip it to stone
        if (was_river) {
            board[ty][tx]["side"] = "stone";
            board[ty][tx].erase("orientation");
        }

        // Update encoded board
        encoded_board[py][px] = undo.to_encoded;
        encoded_board[ty][tx] = encode_cell(board[ty][tx]);
        encoded_board[fy][fx] = 0;
    }
    else if (move.action == "flip") {
        // Flip stone <-> river
        if (board[fy][fx].at("side") == "stone") {
            // Stone -> River with orientation
            board[fy][fx]["side"] = "river";
            board[fy][fx]["orientation"] = move.orientation;
        } else {
            // River -> Stone
            board[fy][fx]["side"] = "stone";
            board[fy][fx].erase("orientation");
        }
        encoded_board[fy][fx] = encode_cell(board[fy][fx]);
    }
    else if (move.action == "rotate") {
        // Rotate river orientation
        std::string current_ori = board[fy][fx].at("orientation");
        std::string new_ori = (current_ori == "horizontal") ? "vertical" : "horizontal";
        board[fy][fx]["orientation"] = new_ori;
        encoded_board[fy][fx] = encode_cell(board[fy][fx]);
    }

    // Switch player
    current_player = get_opponent(current_player);

    return undo;
}

void GameState::undo_move(const Move& move, const UndoInfo& undo_info) {
    if (!undo_info.valid) return;

    int fx = move.from[0], fy = move.from[1];
    int tx = move.to[0], ty = move.to[1];

    // Restore player
    current_player = undo_info.prev_player;

    if (move.action == "move") {
        if (move.pushed_to.empty()) {
            // Simple move: restore from and to
            board[fy][fx] = undo_info.from_cell;
            board[ty][tx] = undo_info.to_cell;
            encoded_board[fy][fx] = undo_info.from_encoded;
            encoded_board[ty][tx] = undo_info.to_encoded;
        } else {
            // Move with push: restore from, to, and pushed_to
            int ptx = move.pushed_to[0], pty = move.pushed_to[1];
            board[fy][fx] = undo_info.from_cell;
            board[ty][tx] = undo_info.to_cell;
            board[pty][ptx].clear();
            encoded_board[fy][fx] = undo_info.from_encoded;
            encoded_board[ty][tx] = undo_info.to_encoded;
            encoded_board[pty][ptx] = 0;
        }
    }
    else if (move.action == "push") {
        // Restore from, to, and pushed_to
        int px = move.pushed_to[0], py = move.pushed_to[1];
        board[fy][fx] = undo_info.from_cell;
        board[ty][tx] = undo_info.to_cell;
        board[py][px].clear();
        encoded_board[fy][fx] = undo_info.from_encoded;
        encoded_board[ty][tx] = undo_info.to_encoded;
        encoded_board[py][px] = 0;
    }
    else if (move.action == "flip" || move.action == "rotate") {
        // Restore the cell
        board[fy][fx] = undo_info.from_cell;
        encoded_board[fy][fx] = undo_info.from_encoded;
    }
}
