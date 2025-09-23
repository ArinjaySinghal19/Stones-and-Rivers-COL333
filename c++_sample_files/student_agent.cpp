#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <set>
#include <queue>
#include <algorithm>
#include <cmath>
#include <memory>
#include <limits>
#include <thread>

namespace py = pybind11;

/*
 Patched student_agent.cpp for Stones & Rivers:
  - Fixes backpropagation bug
  - Removes dummy fallback move
  - Marks states with no legal moves as terminal
  - Improves UCB1 and selection null checks
  - Single RNG per thread, improved rollout (epsilon-greedy biased)
  - Larger/adaptive rollout depth
*/

// ---- Forward declarations ----
struct ValidTargets {
    std::set<std::pair<int,int>> moves;
    std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> pushes; // ((target_x,target_y), (pushed_to_x,pushed_to_y))
};

ValidTargets compute_valid_targets(
    const std::vector<std::vector<std::map<std::string, std::string>>>& board,
    int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols);

// ---- Move struct ----
struct Move {
    std::string action;
    std::vector<int> from;
    std::vector<int> to;
    std::vector<int> pushed_to;
    std::string orientation;
};

// ---- Game State representation ----
struct GameState {
    std::vector<std::vector<std::map<std::string, std::string>>> board;
    std::string current_player;
    int rows, cols;
    std::vector<int> score_cols;
    
    GameState(const std::vector<std::vector<std::map<std::string, std::string>>>& b, 
              const std::string& player, int r, int c, const std::vector<int>& sc)
        : board(b), current_player(player), rows(r), cols(c), score_cols(sc) {}
    
    GameState copy() const {
        return GameState(board, current_player, rows, cols, score_cols);
    }
    
    void apply_move(const Move& move);
    std::vector<Move> get_legal_moves() const;
    bool is_terminal() const;
    double evaluate(const std::string& player) const;
};

// ---- MCTS Node ----
class MCTSNode {
public:
    GameState state;
    MCTSNode* parent;
    std::vector<std::unique_ptr<MCTSNode>> children;
    std::vector<Move> untried_moves;
    int visits;
    double wins;
    Move move_from_parent;
    
    MCTSNode(const GameState& s, MCTSNode* p = nullptr, const Move& m = Move{})
        : state(s), parent(p), visits(0), wins(0.0), move_from_parent(m) {
        untried_moves = state.get_legal_moves();
    }
    
    MCTSNode* select_child();
    MCTSNode* expand();
    double simulate();
    void backpropagate(double result);
    bool is_fully_expanded() const { return untried_moves.empty(); }
    double ucb1_value(double exploration_param = 1.41) const;
    Move get_best_move() const;
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

// Thread-local RNG (one per thread)
static thread_local std::mt19937 tls_rng((std::random_device())());

// River flow destination calculation (based on gameEngine.py implementation)
std::vector<std::pair<int,int>> get_river_flow_destinations(
    const std::vector<std::vector<std::map<std::string, std::string>>>& board,
    int rx, int ry, int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols,
    bool river_push = false) {
    
    std::vector<std::pair<int,int>> destinations;
    std::set<std::pair<int,int>> visited;
    std::queue<std::pair<int,int>> queue;
    
    if (!in_bounds(rx, ry, rows, cols)) return destinations;
    queue.push({rx, ry});
    
    while (!queue.empty()) {
        auto [x, y] = queue.front();
        queue.pop();
        
        if (!in_bounds(x, y, rows, cols)) continue;
        if (visited.count({x, y})) continue;
        visited.insert({x, y});
        
        const auto& cell = board[y][x];
        
        // Handle special case for river_push: treat original source as occupied by the push origin
        const auto* actual_cell = &cell;
        std::map<std::string, std::string> fake_cell;
        if (river_push && x == rx && y == ry) {
            if (in_bounds(sx, sy, rows, cols)) {
                fake_cell = board[sy][sx];
                actual_cell = &fake_cell;
            }
        }
        
        if (actual_cell->empty()) {
            // empty cell is a valid destination unless it's opponent score (we block entering opponent score)
            if (!is_opponent_score_cell(x, y, player, rows, cols, score_cols)) {
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

// ---- Game State Implementation ----
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

void GameState::apply_move(const Move& move) {
    if (move.action == "move") {
        int fx = move.from[0], fy = move.from[1];
        int tx = move.to[0], ty = move.to[1];
        
        if (in_bounds(fx, fy, rows, cols) && in_bounds(tx, ty, rows, cols)) {
            board[ty][tx] = board[fy][fx];
            board[fy][fx].clear();
        }
    }
    else if (move.action == "push") {
        int fx = move.from[0], fy = move.from[1];
        int tx = move.to[0], ty = move.to[1];
        int px = move.pushed_to[0], py = move.pushed_to[1];
        
        if (in_bounds(fx, fy, rows, cols) && in_bounds(tx, ty, rows, cols) && 
            in_bounds(px, py, rows, cols)) {
            board[py][px] = board[ty][tx];  // Move pushed piece
            board[ty][tx] = board[fy][fx];  // Move pushing piece
            board[fy][fx].clear();
        }
    }
    else if (move.action == "flip") {
        int fx = move.from[0], fy = move.from[1];
        if (in_bounds(fx, fy, rows, cols) && !board[fy][fx].empty()) {
            if (board[fy][fx].at("side") == "stone") {
                board[fy][fx]["side"] = "river";
                board[fy][fx]["orientation"] = move.orientation;
            } else {
                board[fy][fx]["side"] = "stone";
                // orientation key may remain but is irrelevant for stones
            }
        }
    }
    else if (move.action == "rotate") {
        int fx = move.from[0], fy = move.from[1];
        if (in_bounds(fx, fy, rows, cols) && !board[fy][fx].empty()) {
            if (board[fy][fx].at("side") == "river") {
                std::string current = board[fy][fx].at("orientation");
                board[fy][fx]["orientation"] = (current == "horizontal") ? "vertical" : "horizontal";
            }
        }
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

            // ---- FLIP ----
            if (side_type == "stone") {
                // Check if flipping to river would be safe (not flowing into opponent score)
                for (const std::string& orientation : {"horizontal", "vertical"}) {
                    // Create a temporary modified board to test the flip
                    auto test_board = board;
                    test_board[y][x]["side"] = "river";
                    test_board[y][x]["orientation"] = orientation;
                    
                    auto flow = get_river_flow_destinations(test_board, x, y, x, y, current_player, rows, cols, score_cols);
                    bool safe = true;
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
            }

            // ---- ROTATE ----
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

    // NOTE: DO NOT inject dummy moves here. If there are no legal moves, the caller
    // can treat the state as terminal / no-move available.
    return moves;
}

bool GameState::is_terminal() const {
    // Terminal if somebody has won, OR no legal moves available for the current player
    if (check_win_state(board, rows, cols, score_cols)) return true;
    auto legal = get_legal_moves();
    return legal.empty();
}

double GameState::evaluate(const std::string& player) const {
    // Count pieces in scoring areas and pieces that can reach in one move.
    // Provide a stronger signal (wider range) to guide biased rollouts.
    int WIN_COUNT = 4;
    int top = top_score_row();
    int bot = bottom_score_row(rows);
    
    // Count current pieces in scoring areas
    int my_in_score = 0, opp_in_score = 0;
    int my_can_reach = 0, opp_can_reach = 0;
    
    std::string opponent = get_opponent(player);
    
    // Count pieces in scoring areas
    for (int x : score_cols) {
        if (x >= 0 && x < cols) {
            if (top >= 0 && top < rows) {
                const auto& cell = board[top][x];
                if (!cell.empty() && cell.at("side") == "stone") {
                    if (cell.at("owner") == "circle") {
                        if (player == "circle") my_in_score++; else opp_in_score++;
                    }
                }
            }
            if (bot >= 0 && bot < rows) {
                const auto& cell = board[bot][x];
                if (!cell.empty() && cell.at("side") == "stone") {
                    if (cell.at("owner") == "square") {
                        if (player == "square") my_in_score++; else opp_in_score++;
                    }
                }
            }
        }
    }
    
    // Immediate win/loss
    if (my_in_score >= WIN_COUNT) return 1.0;
    if (opp_in_score >= WIN_COUNT) return 0.0;
    
    // Check pieces that can reach scoring area in one move
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            const auto& cell = board[y][x];
            if (cell.empty()) continue;
            
            if (cell.at("owner") == player) {
                if (((player == "circle" && y != top) || (player == "square" && y != bot)) && cell.at("side")=="stone") {
                    auto valid_targets = compute_valid_targets(board, x, y, player, rows, cols, score_cols);
                    for (const auto& target : valid_targets.moves) {
                        if ((player == "circle" && target.second == top && 
                             std::find(score_cols.begin(), score_cols.end(), target.first) != score_cols.end()) ||
                            (player == "square" && target.second == bot && 
                             std::find(score_cols.begin(), score_cols.end(), target.first) != score_cols.end())) {
                            my_can_reach++;
                            break;
                        }
                    }
                    for (const auto& push : valid_targets.pushes) {
                        auto pushed_pos = push.second;
                        if ((player == "circle" && pushed_pos.second == top && 
                             std::find(score_cols.begin(), score_cols.end(), pushed_pos.first) != score_cols.end()) ||
                            (player == "square" && pushed_pos.second == bot && 
                             std::find(score_cols.begin(), score_cols.end(), pushed_pos.first) != score_cols.end())) {
                            my_can_reach++;
                            break;
                        }
                    }
                }
                if (cell.at("side") == "river") {
                    if ((player == "circle" && y == top && 
                         std::find(score_cols.begin(), score_cols.end(), x) != score_cols.end()) ||
                        (player == "square" && y == bot && 
                         std::find(score_cols.begin(), score_cols.end(), x) != score_cols.end())) {
                        my_can_reach++;
                    }
                }
            }
            else if (cell.at("owner") == opponent) {
                if (((opponent == "circle" && y != top) || (opponent == "square" && y != bot)) && cell.at("side") == "stone") {
                    auto valid_targets = compute_valid_targets(board, x, y, opponent, rows, cols, score_cols);
                    for (const auto& target : valid_targets.moves) {
                        if ((opponent == "circle" && target.second == top && 
                             std::find(score_cols.begin(), score_cols.end(), target.first) != score_cols.end()) ||
                            (opponent == "square" && target.second == bot && 
                             std::find(score_cols.begin(), score_cols.end(), target.first) != score_cols.end())) {
                            opp_can_reach++;
                            break;
                        }
                    }
                    for (const auto& push : valid_targets.pushes) {
                        auto pushed_pos = push.second;
                        if ((opponent == "circle" && pushed_pos.second == top && 
                             std::find(score_cols.begin(), score_cols.end(), pushed_pos.first) != score_cols.end()) ||
                            (opponent == "square" && pushed_pos.second == bot && 
                             std::find(score_cols.begin(), score_cols.end(), pushed_pos.first) != score_cols.end())) {
                            opp_can_reach++;
                            break;
                        }
                    }
                }
                if (cell.at("side") == "river") {
                    if ((opponent == "circle" && y == top && 
                         std::find(score_cols.begin(), score_cols.end(), x) != score_cols.end()) ||
                        (opponent == "square" && y == bot && 
                         std::find(score_cols.begin(), score_cols.end(), x) != score_cols.end())) {
                        opp_can_reach++;
                    }
                }
            }
        }
    }
    
    // Weighted score: heavier weight to pieces already in scoring area and nearer threats
    double score = (my_in_score * 12.0 + my_can_reach * 2.0) - (opp_in_score * 12.0 + opp_can_reach * 2.0);
    
    // Map raw score into [0,1] but keep steeper slope for discrimination
    double scaled = 1.0 / (1.0 + std::exp(-score / 4.0)); // logistic
    return scaled;
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

// ---- MCTS Implementation ----
double MCTSNode::ucb1_value(double exploration_param) const {
    // Standard UCB1: if child unvisited -> +inf (force visit)
    if (visits == 0) return std::numeric_limits<double>::infinity();
    if (parent == nullptr || parent->visits == 0) {
        // If parent has zero visits (rare), provide a conservative exploration term
        double avg_reward = wins / (double)visits;
        double exploration = exploration_param * std::sqrt(1.0 / (double)visits);
        return avg_reward + exploration;
    }
    double avg_reward = wins / (double)visits;
    double exploration = exploration_param * std::sqrt(std::log((double)parent->visits) / (double)visits);
    return avg_reward + exploration;
}

MCTSNode* MCTSNode::select_child() {
    MCTSNode* best_child = nullptr;
    double best_value = -std::numeric_limits<double>::infinity();
    
    for (auto& child : children) {
        double value = child->ucb1_value();
        if (value > best_value) {
            best_value = value;
            best_child = child.get();
        }
    }
    
    return best_child;
}

MCTSNode* MCTSNode::expand() {
    if (untried_moves.empty()) return nullptr;
    
    std::uniform_int_distribution<> dist(0, (int)untried_moves.size() - 1);
    int idx = dist(tls_rng);
    
    Move move = untried_moves[idx];
    untried_moves.erase(untried_moves.begin() + idx);
    
    // Create new state by applying the move
    GameState new_state = state.copy();
    new_state.apply_move(move);
    
    // Create new child node
    auto child = std::make_unique<MCTSNode>(new_state, this, move);
    MCTSNode* child_ptr = child.get();
    children.push_back(std::move(child));
    
    return child_ptr;
}

double MCTSNode::simulate() {
    GameState sim_state = state.copy();
    // Adaptive max depth: at most rows*cols*2 or 200 cap
    int max_depth = std::min(200, std::max(50, sim_state.rows * sim_state.cols * 2));
    int depth = 0;
    
    // Evaluate from perspective of the player who is about to move at this node
    std::string rollout_player = sim_state.current_player;
    
    // epsilon-greedy biased rollout: with probability eps choose a random move,
    // otherwise choose move that maximizes immediate evaluation for current player.
    const double eps = 0.15;
    std::uniform_real_distribution<> coin(0.0, 1.0);
    
    while (!sim_state.is_terminal() && depth < max_depth) {
        std::vector<Move> moves = sim_state.get_legal_moves();
        if (moves.empty()) break;
        
        Move chosen;
        if (coin(tls_rng) < eps) {
            std::uniform_int_distribution<> r(0, (int)moves.size() - 1);
            chosen = moves[r(tls_rng)];
        } else {
            // Evaluate resulting states and take best (greedy)
            double best_val = -1.0;
            bool set = false;
            for (const auto &m : moves) {
                GameState temp = sim_state.copy();
                temp.apply_move(m);
                double val = temp.evaluate(rollout_player);
                if (!set || val > best_val) {
                    best_val = val;
                    chosen = m;
                    set = true;
                }
            }
            // if something failed fallback to random
            if (!set) {
                std::uniform_int_distribution<> r(0, (int)moves.size() - 1);
                chosen = moves[r(tls_rng)];
            }
        }
        sim_state.apply_move(chosen);
        depth++;
    }
    
    // Return evaluation from the perspective of the rollout root player
    return sim_state.evaluate(rollout_player);
}

void MCTSNode::backpropagate(double result) {
    // Iterative propagation, alternating perspective (result is for this node's current player at time of simulation)
    MCTSNode* node = this;
    double value = result;
    while (node != nullptr) {
        node->visits++;
        node->wins += value;
        // Switch perspective for parent
        value = 1.0 - value;
        node = node->parent;
    }
}

Move MCTSNode::get_best_move() const {
    if (children.empty()) {
        // If there are no children, return a random legal move if available
        std::vector<Move> legal_moves = state.get_legal_moves();
        if (!legal_moves.empty()) {
            std::uniform_int_distribution<> dist(0, (int)legal_moves.size() - 1);
            return legal_moves[dist(tls_rng)];
        }
        return {"move", {0,0}, {0,0}, {}, ""};
    }
    
    // Select child with highest visit count (most explored)
    MCTSNode* best_child = nullptr;
    int max_visits = -1;
    
    for (const auto& child : children) {
        if (child->visits > max_visits) {
            max_visits = child->visits;
            best_child = child.get();
        }
    }
    
    return best_child ? best_child->move_from_parent : Move{"move", {0,0}, {0,0}, {}, ""};
}

// ---- MCTS Algorithm ----
Move run_mcts(const GameState& initial_state, int max_iterations) {
    const std::string player = initial_state.current_player;
    std::vector<Move> root_legal_moves = initial_state.get_legal_moves();

    // 0. Deterministic scoring move check
    int target_row = (player == "circle") ? top_score_row() : bottom_score_row(initial_state.rows);

    for (const Move& candidate_move : root_legal_moves) {
        GameState test_state = initial_state.copy();
        test_state.apply_move(candidate_move);

        // Check if this move places *any* of player's stones in scoring row
        bool scored = false;
        for (int x : initial_state.score_cols) {
            if (in_bounds(x, target_row, initial_state.rows, initial_state.cols)) {
                const auto& cell = test_state.board[target_row][x];
                if (!cell.empty() && cell.at("side") == "stone" && cell.at("owner") == player) {
                    scored = true;
                    break;
                }
            }
        }
        if (scored) {
            return candidate_move; // take the scoring move immediately
        }
    }

    // 1. Initialize root
    auto root = std::make_unique<MCTSNode>(initial_state);

    // 2. Run MCTS iterations
    for (int iteration = 0; iteration < max_iterations; iteration++) {
        MCTSNode* node = root.get();

        // Selection
        while (node->is_fully_expanded() && !node->children.empty()) {
            MCTSNode* next = node->select_child();
            if (next == nullptr) break;
            node = next;
        }

        // Expansion
        if (node != nullptr && !node->state.is_terminal() && !node->is_fully_expanded()) {
            MCTSNode* expanded = node->expand();
            if (expanded != nullptr) node = expanded;
        }

        // Simulation
        double result = 0.5;
        if (node != nullptr) {
            result = node->simulate();
        }

        // Backpropagation
        if (node != nullptr) {
            node->backpropagate(result);
        }
    }

    // 3. Return best move from root
    return root->get_best_move();
}

// ---- Student Agent ----
class StudentAgent {
public:
    explicit StudentAgent(std::string side) : side(std::move(side)) {}

    Move choose(const std::vector<std::vector<std::map<std::string, std::string>>>& board, int row, int col, const std::vector<int>& score_cols, float current_player_time, float opponent_time) {
        int rows = board.size();
        int cols = board[0].size();

        // Create game state
        GameState current_state(board, side, rows, cols, score_cols);
        
        // Base iterations and simple time-based scaling
        int base_iterations = 400;
        int max_iterations = base_iterations;
        if (current_player_time < 10.0) max_iterations = base_iterations / 2;
        if (current_player_time < 5.0) max_iterations = base_iterations / 4;
        if (current_player_time < 2.0) max_iterations = std::max(10, base_iterations / 10);
        
        // Run MCTS
        Move best_move = run_mcts(current_state, max_iterations);
        
        return best_move;
    }

private:
    std::string side;
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