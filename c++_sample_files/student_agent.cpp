#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <set>
#include <queue>

namespace py = pybind11;


/*
=========================================================
 STUDENT AGENT FOR STONES & RIVERS GAME
---------------------------------------------------------
 The Python game engine passes the BOARD state into C++.
 Each board cell is represented as a dictionary in Python:

    {
        "owner": "circle" | "square",          // which player owns this piece
        "side": "stone" | "river",             // piece type
        "orientation": "horizontal" | "vertical"  // only relevant if side == "river"
    }

 In C++ with pybind11, this becomes:

    std::vector<std::vector<std::map<std::string, std::string>>>

 Meaning:
   - board[y][x] gives the cell at (x, y).
   - board[y][x].empty() → true if the cell is empty (no piece).
   - board[y][x].at("owner") → "circle" or "square".
   - board[y][x].at("side") → "stone" or "river".
   - board[y][x].at("orientation") → "horizontal" or "vertical".

=========================================================
*/

// ---- Move struct ----
struct Move {
    std::string action;
    std::vector<int> from;
    std::vector<int> to;
    std::vector<int> pushed_to;
    std::string orientation;
};

// ---- Helper functions for scoring areas and river flow ----
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

// River flow destination calculation (based on gameEngine.py implementation)
std::vector<std::pair<int,int>> get_river_flow_destinations(
    const std::vector<std::vector<std::map<std::string, std::string>>>& board,
    int rx, int ry, int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols,
    bool river_push = false) {
    
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
struct ValidTargets {
    std::set<std::pair<int,int>> moves;
    std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> pushes; // ((target_x,target_y), (pushed_to_x,pushed_to_y))
};

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
                if (in_bounds(px, py, rows, cols) && board[py][px].empty() && 
                    !is_opponent_score_cell(px, py, piece.at("owner"), rows, cols, score_cols)) {
                    result.pushes.push_back({{tx, ty}, {px, py}});
                }
            } else {
                // River pushing stone
                std::string pushed_player = target.at("owner");
                auto flow = get_river_flow_destinations(board, tx, ty, sx, sy, pushed_player, rows, cols, score_cols, true);
                for (const auto& d : flow) {
                    if (!is_opponent_score_cell(d.first, d.second, player, rows, cols, score_cols)) {
                        result.pushes.push_back({{tx, ty}, d});
                    }
                }
            }
        }
    }
    
    return result;
}

// ---- Student Agent ----
class StudentAgent {
public:
    explicit StudentAgent(std::string side) : side(std::move(side)), gen(rd()) {}

    Move choose(const std::vector<std::vector<std::map<std::string, std::string>>>& board, int row, int col, const std::vector<int>& score_cols, float current_player_time, float opponent_time) {
        int rows = board.size();
        int cols = board[0].size();

        std::vector<Move> moves;

        // Iterate over board to find my pieces
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                const auto &cell = board[y][x];
                if (cell.empty()) continue;

                if (cell.at("owner") != side) continue; // only my pieces

                std::string side_type = cell.at("side");

                // ---- MOVES (including river flow) ----
                auto valid_targets = compute_valid_targets(board, x, y, side, rows, cols, score_cols);
                
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
                        
                        auto flow = get_river_flow_destinations(test_board, x, y, x, y, side, rows, cols, score_cols);
                        for (const auto& dest : flow) {
                            if (is_opponent_score_cell(dest.first, dest.second, side, rows, cols, score_cols)) {
                                safe = false;
                                break;
                            }
                        }
                        
                        if (safe) {
                            moves.push_back({"flip", {x,y}, {x,y}, {}, orientation});
                        }
                    }
                }

                // ---- ROTATE ----
                if (side_type == "river") {
                    // Check if rotation would be safe
                    std::string current_orientation = cell.at("orientation");
                    std::string new_orientation = (current_orientation == "horizontal") ? "vertical" : "horizontal";
                    
                    // Create a temporary modified board to test the rotation
                    auto test_board = board;
                    test_board[y][x]["orientation"] = new_orientation;
                    
                    auto flow = get_river_flow_destinations(test_board, x, y, x, y, side, rows, cols, score_cols);
                    bool safe = true;
                    for (const auto& dest : flow) {
                        if (is_opponent_score_cell(dest.first, dest.second, side, rows, cols, score_cols)) {
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
            return {"move", {0,0}, {0,0}, {}, ""}; // fallback
        }

        std::uniform_int_distribution<> dist(0, moves.size()-1);
        return moves[dist(gen)];
    }

private:
    std::string side;
    std::random_device rd;
    std::mt19937 gen;
};

// ---- PyBind11 bindings ----
PYBIND11_MODULE(student_agent_module, m) {
    py::class_<Move>(m, "Move")
        .def_readonly("action", &Move::action)
        .def_readonly("from_pos", &Move::from)
        .def_readonly("to_pos", &Move::to)
        .def_readonly("pushed_to", &Move::pushed_to)
        .def_readonly("orientation", &Move::orientation);

    py::class_<StudentAgent>(m, "StudentAgent")
        .def(py::init<std::string>())
        .def("choose", &StudentAgent::choose);
}