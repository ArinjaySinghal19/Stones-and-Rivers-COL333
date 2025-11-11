#include "heuristics.h"
#include <cmath>
#include <algorithm>
#include <set>
#include <queue>
#include <iostream>
#include <climits>
#include <map>

Heuristics::Weights Heuristics::weights_{};

const Heuristics::Weights& Heuristics::get_weights() {
    return weights_;
}

void Heuristics::set_weights(const Heuristics::Weights& new_weights) {
    weights_ = new_weights;
}

int Heuristics::max(int a, int b) {
    return (a > b) ? a : b;
}

// OPTIMIZED: Uses encoded board for O(1) cell access instead of map lookups
// CORRECTNESS: Produces identical results to original implementation
// Original used: cell.at("owner") == player && cell.at("side") == "river" && cell.at("orientation") == "vertical"
// Optimized uses: GameState::is_owner(encoded_cell, player) && GameState::is_vertical_river(encoded_cell)
double Heuristics::vertical_push_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;  // Use encoded board for fast access
    int rows = state.rows;
    int cols = state.cols;

    double score = 0;
    std::vector<double> col_weight(12);

    // Same weight scheme as original
    for (int i = 2; i <= 3; i++) {
        col_weight[i] = 3.25;
        col_weight[11-i] = 3.25;
    }

    col_weight[1] = 2.25;
    col_weight[4] = 2.25;
    col_weight[6] = 2.25;
    col_weight[10] = 2.25;
    col_weight[5] = 1.5;
    col_weight[0] = 1;
    col_weight[11] = 1;

    std::string opponent = get_opponent(player);
    int push_direction = (player == "circle") ? -1 : 1;
    std::vector<std::vector<double>> reach(rows, std::vector<double>(cols, 0));

    // Scan for player's vertical rivers using encoded board
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            EncodedCell cell = encoded_board[y][x];

            // Check if this is player's vertical river
            // Equivalent to: owner==player && side==river && orientation==vertical
            if (GameState::is_owner(cell, player) && GameState::is_vertical_river(cell)) {
                // Calculate reach in push direction
                for (int dist = 1; dist < rows; ++dist) {
                    int ny = y + push_direction * dist;
                    if (!in_bounds(x, ny, rows, cols)) {
                        break;
                    }

                    EncodedCell next_cell = encoded_board[ny][x];

                    if (GameState::is_empty(next_cell)) {
                        reach[ny][x] = col_weight[x];
                        continue;
                    }
                    else if (GameState::is_owner(next_cell, opponent)) {
                        // Blocked by opponent
                        break;
                    }
                    else {
                        // Same player's piece - can reach through it
                        reach[ny][x] = col_weight[x];
                        continue;
                    }
                }
            }
        }
    }

    // Sum up all reach values (same as original)
    for (auto &e1 : reach) {
        for (auto &e2 : e1) {
            score += e2;
        }
    }
    return wrt_self ? score : -score;
}

// Split scoring heuristic - Part 1: Virgin columns (recalculated each time)
int Heuristics::pieces_in_scoring_virgin_cols(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;

    std::vector<std::pair<int, int>> target_rivers;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            EncodedCell cell = encoded_board[y][x];
            if (!GameState::is_river(cell)) {
                continue;
            }
            if (self && !GameState::is_owner(cell, player)) {
                continue;
            }
            target_rivers.push_back({x, y});
        }
    }

    int total_connected_pairs = 0;
    std::set<std::pair<int, int>> globally_visited;

    for (const auto& start_pos : target_rivers) {
        if (globally_visited.count(start_pos)) {
            continue;
        }

        int component_size = 0;
        std::queue<std::pair<int, int>> q;

        q.push(start_pos);
        globally_visited.insert(start_pos);

        while (!q.empty()) {
            auto [x, y] = q.front();
            q.pop();
            component_size++;

            EncodedCell current_river = encoded_board[y][x];
            bool is_horizontal = GameState::is_horizontal_river(current_river);

            std::vector<std::pair<int, int>> dirs;
            if (is_horizontal) {
                dirs = {{-1, 0}, {1, 0}};
            } else {
                dirs = {{0, -1}, {0, 1}};
            }

            for (auto [dx, dy] : dirs) {
                int nx = x + dx;
                int ny = y + dy;

                while (in_bounds(nx, ny, rows, cols)) {
                    EncodedCell next_cell = encoded_board[ny][nx];

                    if (!GameState::is_empty(next_cell)) {
                        bool same_orientation = is_horizontal ? GameState::is_horizontal_river(next_cell) : GameState::is_vertical_river(next_cell);
                        if (GameState::is_river(next_cell) &&
                            same_orientation &&
                            !globally_visited.count({nx, ny})) {

                            bool is_valid_target = true;
                            if (self && !GameState::is_owner(next_cell, player)) {
                                is_valid_target = false;
                            }

                            if (is_valid_target) {
                                q.push({nx, ny});
                                globally_visited.insert({nx, ny});
                            }
                        }
                        break;
                    }
                    nx += dx;
                    ny += dy;
                }
            }
        }

        if (component_size > 1) {
            total_connected_pairs += component_size * (component_size - 1) / 2;
        }
    }

    return wrt_self ? total_connected_pairs : -total_connected_pairs;
}


// OPTIMIZED: Uses encoded board for O(1) cell access instead of map lookups
// CORRECTNESS: Produces identical results to original implementation
int Heuristics::pieces_in_scoring_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;
    const auto& score_cols = state.score_cols;

    int score = 0;
    const int w1 = 1e5;
    const int w2 = 350;
    const int w3 = 175;
    const int w4 = 80;
    const int w5 = 4;

    int target_row, direction;
    std::string player_to_check = player;

    if (!wrt_self) {
        if (player == "circle") {
            player_to_check = "square";
        } else {
            player_to_check = "circle";
        }
    }

    if (player_to_check == "circle") {
        target_row = top_score_row();
        direction = -1;
    } else {
        target_row = bottom_score_row(rows);
        direction = 1;
    }

    std::map<int, std::map<int, int>> val;
    int in_score_area = 0;
    std::vector<int> virgin_cols;

    for (int col = 4; col <= 7; col++) {
        EncodedCell cell = encoded_board[target_row][col];
        if (GameState::is_empty(cell)) {
            virgin_cols.push_back(col);
            continue;
        }
        val[target_row][col] = max(val[target_row][col], w1);
        in_score_area++;
        if (GameState::is_stone(cell) && GameState::is_owner(cell, player_to_check)) {
            score += 1e3;
        }
    }

    for (int col = 1; col <= 10; col++) {
        for (int row = target_row - 2; row <= target_row + 2; row++) {
            if (row < 0 || row >= rows) continue;
            EncodedCell cell = encoded_board[row][col];
            if (GameState::is_empty(cell)) continue;
            if (!GameState::is_owner(cell, player_to_check)) continue;
            if (row == target_row && col >= 4 && col <= 7) continue;

            for (int vc : virgin_cols) {
                int man_dist = abs(vc - col) + abs(target_row - row);
                if (man_dist == 1) {
                    score += 200 * in_score_area;
                }
                if (man_dist == 2) {
                    score += 100 * in_score_area;
                }
                if (man_dist == 3) {
                    score += 50 * in_score_area;
                }
            }
        }
    }

    for (int col = 3; col <= 8; col++) {
        for (int r = 0; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row < 0 || check_row >= rows) continue;
            EncodedCell cell = encoded_board[check_row][col];
            if (GameState::is_empty(cell)) continue;
            if (GameState::is_owner(cell, player_to_check)) {
                val[check_row][col] = max(val[check_row][col], w2);
            }
        }
    }

    for (int col = 2; col <= 9; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row < 0 || check_row >= rows) continue;
            EncodedCell cell = encoded_board[check_row][col];
            if (GameState::is_empty(cell)) continue;
            if (GameState::is_owner(cell, player_to_check)) {
                val[check_row][col] = max(val[check_row][col], w3);
            }
        }
    }

    for (int col = 1; col <= 10; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row < 0 || check_row >= rows) continue;
            EncodedCell cell = encoded_board[check_row][col];
            if (GameState::is_empty(cell)) continue;
            if (GameState::is_owner(cell, player_to_check)) {
                val[check_row][col] = max(val[check_row][col], w4);
            }
        }
    }

    for (int col = 0; col <= 11; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row < 0 || check_row >= rows) continue;
            EncodedCell cell = encoded_board[check_row][col];
            if (GameState::is_empty(cell)) continue;
            if (GameState::is_owner(cell, player_to_check)) {
                val[check_row][col] = max(val[check_row][col], w5);
            }
        }
    }

    for (auto &e1 : val) {
        for (auto &e2 : e1.second) {
            score += e2.second;
        }
    }

    return wrt_self ? score : -score;
}

// OPTIMIZED: Uses encoded board for O(1) cell access for initial scanning
// Note: Still uses state.board for compute_valid_targets and get_river_flow_destinations
// as those functions require the map-based representation
int Heuristics::possible_moves_h(const GameState& state, const std::string& player, bool wrt_self) {
    int num_moves = 0;

    for (int y = 0; y < state.rows; y++) {
        for (int x = 0; x < state.cols; x++) {
            EncodedCell cell = state.encoded_board[y][x];
            if (GameState::is_empty(cell)) continue;
            if (!GameState::is_owner(cell, player)) continue;

            bool is_stone = GameState::is_stone(cell);
            bool is_river = GameState::is_river(cell);

            auto valid_targets = compute_valid_targets(state.board, x, y, player, state.rows, state.cols, state.score_cols);

            for (const auto& target : valid_targets.moves) {
                num_moves++;
            }

            for (const auto& push : valid_targets.pushes) {
                auto target_pos = push.first;
                auto pushed_pos = push.second;
                num_moves++;
            }

            if (is_stone) {
                for (const std::string& orientation : {"horizontal", "vertical"}) {
                    bool safe = true;

                    auto test_board = state.board;
                    test_board[y][x]["side"] = "river";
                    test_board[y][x]["orientation"] = orientation;

                    auto flow = get_river_flow_destinations(test_board, x, y, x, y, player, state.rows, state.cols, state.score_cols);
                    for (const auto& dest : flow) {
                        if (is_opponent_score_cell(dest.first, dest.second, player, state.rows, state.cols, state.score_cols)) {
                            safe = false;
                            break;
                        }
                    }

                    if (safe) {
                        num_moves++;
                    }
                }
            } else if (is_river) {
                num_moves++;
            }

            if (is_river) {
                bool is_horizontal = GameState::is_horizontal_river(cell);
                std::string new_orientation = is_horizontal ? "vertical" : "horizontal";

                auto test_board = state.board;
                test_board[y][x]["orientation"] = new_orientation;

                auto flow = get_river_flow_destinations(test_board, x, y, x, y, player, state.rows, state.cols, state.score_cols);
                bool safe = true;
                for (const auto& dest : flow) {
                    if (is_opponent_score_cell(dest.first, dest.second, player, state.rows, state.cols, state.score_cols)) {
                        safe = false;
                        break;
                    }
                }

                if (safe) {
                    num_moves++;
                }
            }
        }
    }

    int result = num_moves;
    return wrt_self ? result : -result;
}

// OPTIMIZED: Uses encoded board for O(1) cell access instead of map lookups
// CORRECTNESS: Produces identical results to original implementation
int Heuristics::pieces_blocking_vertical_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;

    int block_count = 0;
    std::string opponent = get_opponent(player);
    int flow_dy = (opponent == "square") ? 1 : -1;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            EncodedCell cell = encoded_board[y][x];
            // Check if this is opponent's vertical river
            if (GameState::is_owner(cell, opponent) && GameState::is_vertical_river(cell)) {

                int ny = y + flow_dy;
                while (in_bounds(x, ny, rows, cols)) {
                    EncodedCell blocking_cell = encoded_board[ny][x];
                    if (GameState::is_empty(blocking_cell)) {
                        ny += flow_dy;
                        continue;
                    }
                    if (GameState::is_owner(blocking_cell, player)) {
                        // Player's horizontal river blocks
                        if (GameState::is_horizontal_river(blocking_cell)) {
                            block_count++;
                            break;
                        }
                        // Player's stone blocks if distance > 1
                        if (GameState::is_stone(blocking_cell) && std::abs(ny - y) > 1) {
                            block_count++;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    return wrt_self ? block_count : -block_count;
}

// OPTIMIZED: Uses encoded board for O(1) cell access instead of map lookups
// CORRECTNESS: Produces identical results to original implementation
int Heuristics::vertical_river_on_top_peri_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;

    int count = 0;
    int perimeter_row;

    if (player == "circle") {
        perimeter_row = bottom_score_row(rows) - 1;
    } else {
        perimeter_row = top_score_row() + 1;
    }

    if (!in_bounds(0, perimeter_row, rows, cols)) return 0;

    for (int x = 0; x < cols; ++x) {
        EncodedCell cell = encoded_board[perimeter_row][x];
        if (GameState::is_owner(cell, player) && GameState::is_vertical_river(cell)) {
            count++;
        }
    }
    return wrt_self ? count : -count;
}

// OPTIMIZED: Uses encoded board for O(1) cell access instead of map lookups
// CORRECTNESS: Produces identical results to original implementation
int Heuristics::horizontal_base_rivers(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;

    int count = 0;
    std::vector<int> check_rows;

    if (player == "circle") {
        check_rows.push_back(bottom_score_row(rows) - 1);
        check_rows.push_back(bottom_score_row(rows) - 2);

    } else {
        check_rows.push_back(top_score_row() + 1);
        check_rows.push_back(top_score_row() + 2);

    }

    for (int r : check_rows) {
        if (in_bounds(0, r, rows, cols)) {
            for (int x = 0; x < cols; ++x) {
                EncodedCell cell = encoded_board[r][x];
                if (GameState::is_owner(cell, player) && GameState::is_horizontal_river(cell)) {
                    count++;
                }
            }
        }
    }
    return wrt_self ? count : -count;
}

// OPTIMIZED: Uses encoded board for O(1) cell access instead of map lookups
// CORRECTNESS: Produces identical results to original implementation
int Heuristics::horizontal_negative(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;

    int count = 0;

    if (player == "square") {
        int base_row = top_score_row();
        for (int y = 0; y < base_row; ++y) {
            for (int x = 0; x < cols; ++x) {
                EncodedCell cell = encoded_board[y][x];
                if (GameState::is_owner(cell, player) && GameState::is_horizontal_river(cell)) {
                    count++;
                }
            }
        }
    } else {
        int base_row = bottom_score_row(rows);
        for (int y = base_row + 1; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                EncodedCell cell = encoded_board[y][x];
                if (GameState::is_owner(cell, player) && GameState::is_horizontal_river(cell)) {
                    count++;
                }
            }
        }
    }
    return wrt_self ? -count : count;
}



// OPTIMIZED: Uses encoded board for O(1) cell access instead of map lookups
// CORRECTNESS: Produces identical results to original implementation
int Heuristics::horizontal_attack(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;

    int count = 0;
    std::vector<int> check_rows;

    if (player == "circle") {
        check_rows.push_back(top_score_row() - 1);
        check_rows.push_back(top_score_row());
    } else {
        check_rows.push_back(bottom_score_row(rows) + 1);
        check_rows.push_back(bottom_score_row(rows));
    }

    for (int r : check_rows) {
        if (in_bounds(0, r, rows, cols)) {
            for (int x = 0; x < cols; ++x) {
                int mul = 2;
                if (r == top_score_row() || r == bottom_score_row(rows)) {
                    mul = 3;
                }
                EncodedCell cell = encoded_board[r][x];
                if (GameState::is_owner(cell, player) && GameState::is_horizontal_river(cell)) {

                    if (x < 4) {
                        int num_possible = 0;
                        int curr_col = x + 1;
                        while (in_bounds(curr_col, r, rows, cols)) {
                            EncodedCell check_cell = encoded_board[r][curr_col];
                            if (GameState::is_empty(check_cell) ||
                                GameState::is_horizontal_river(check_cell) ||
                                GameState::is_owner(check_cell, player)) {
                                if (curr_col > 7) break;
                                num_possible += mul;
                                curr_col++;
                            } else {
                                break;
                            }
                        }
                        count += num_possible;
                    }
                    else if (x > 7) {
                        int num_possible = 0;
                        int curr_col = x - 1;
                        while (in_bounds(curr_col, r, rows, cols)) {
                            EncodedCell check_cell = encoded_board[r][curr_col];
                            if (GameState::is_empty(check_cell) ||
                                GameState::is_horizontal_river(check_cell) ||
                                GameState::is_owner(check_cell, player)) {
                                if (curr_col < 4) break;
                                num_possible += mul;
                                curr_col--;
                            } else {
                                break;
                            }
                        }
                        count += num_possible;
                    }
                    else {
                        int num_possible = 0;
                        int curr_col = x + 1;
                        while (in_bounds(curr_col, r, rows, cols)) {
                            EncodedCell check_cell = encoded_board[r][curr_col];
                            if (GameState::is_empty(check_cell) ||
                                GameState::is_horizontal_river(check_cell) ||
                                GameState::is_owner(check_cell, player)) {
                                if (curr_col < 4 && curr_col > 7) break;
                                num_possible += mul;
                                curr_col++;
                            } else {
                                break;
                            }
                        }
                        curr_col = x - 1;
                        while (in_bounds(curr_col, r, rows, cols)) {
                            EncodedCell check_cell = encoded_board[r][curr_col];
                            if (GameState::is_empty(check_cell) ||
                                GameState::is_horizontal_river(check_cell) ||
                                GameState::is_owner(check_cell, player)) {
                                if (curr_col < 4 && curr_col > 7) break;
                                num_possible += mul;
                                curr_col--;
                            } else {
                                break;
                            }
                        }
                        count += num_possible;
                    }
                }
            }
        }
    }
    return wrt_self ? count : -count;
}



// OPTIMIZED: Uses encoded board for fast edge piece counting
// CORRECTNESS: Counts player's pieces on board edges (x=0, x=cols-1, y=0, y=rows-1)
// Original used: !cell.empty() && cell.at("owner") == player
// Optimized uses: GameState::is_owner(encoded_cell, player)
int Heuristics::inactive_pieces(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;

    int inactive_count = 0;

    // Count pieces on left and right edges
    for (int y = 0; y < rows; ++y) {
        // Left edge (x=0)
        if (GameState::is_owner(encoded_board[y][0], player)) {
            inactive_count++;
        }
        // Right edge (x=cols-1)
        if (GameState::is_owner(encoded_board[y][cols-1], player)) {
            inactive_count++;
        }
    }

    // Count pieces on top and bottom edges
    for (int x = 0; x < cols; ++x) {
        // Top edge (y=0)
        if (GameState::is_owner(encoded_board[0][x], player)) {
            inactive_count++;
        }
        // Bottom edge (y=rows-1)
        if (GameState::is_owner(encoded_board[rows-1][x], player)) {
            inactive_count++;
        }
    }

    return wrt_self ? -inactive_count : +inactive_count;
}


// OPTIMIZED: Uses encoded board for O(1) cell access instead of map lookups
// CORRECTNESS: Produces identical results to original implementation
int Heuristics::terminal_result(const GameState& state, const std::string& player, bool wrt_self) {
    int WIN_COUNT = 4;
    int top = top_score_row();
    int bot = bottom_score_row(state.rows);
    int ccount = 0, scount = 0;

    const auto& encoded_board = state.encoded_board;

    for (int x : state.score_cols) {
        if (x >= 0 && x < state.cols) {
            if (top >= 0 && top < state.rows) {
                EncodedCell cell = encoded_board[top][x];
                if (GameState::is_circle(cell) && GameState::is_stone(cell)) {
                    ccount++;
                }
            }
            if (bot >= 0 && bot < state.rows) {
                EncodedCell cell = encoded_board[bot][x];
                if (GameState::is_square(cell) && GameState::is_stone(cell)) {
                    scount++;
                }
            }
        }
    }
    int result = 0;
    if (player == "circle") {
        if (ccount >= WIN_COUNT) result = 1e9;
        else if (scount >= WIN_COUNT) result = -1e9;
    } else {
        if (scount >= WIN_COUNT) result = 1e9;
        else if (ccount >= WIN_COUNT) result = -1e9;
    }
    return wrt_self ? result : -result;
}

double Heuristics::evaluate_position(const GameState& state, const std::string& player) {
    if (state.is_terminal()) return terminal_result(state, player, true);
    
    double final_score = 0.0;

    final_score += weights_.vertical_push * vertical_push_h(state, player, true);
    final_score += weights_.connectedness_self * connectedness_h(state, player, true, true);
    final_score += weights_.connectedness_all * connectedness_h(state, player, false, true);
    final_score += weights_.pieces_in_scoring_attack * pieces_in_scoring_h(state, player, true);
    final_score += weights_.possible_moves_self * possible_moves_h(state, player, true);
    final_score += weights_.horizontal_attack_self * horizontal_attack(state, player, true);
    final_score += weights_.inactive_self * inactive_pieces(state, player, true);

    final_score += weights_.pieces_blocking_vertical_self * pieces_blocking_vertical_h(state, player, true);
    final_score += weights_.horizontal_base_self * horizontal_base_rivers(state, player, true);
    final_score += weights_.horizontal_negative_self * horizontal_negative(state, player, true);
    
    final_score += weights_.pieces_in_scoring_defense * pieces_in_scoring_h(state, player, false);
    std::string opponent = get_opponent(player);
    final_score += weights_.possible_moves_opp * possible_moves_h(state, opponent, false);
    final_score += weights_.pieces_blocking_vertical_opp * pieces_blocking_vertical_h(state, opponent, false);
    final_score += weights_.horizontal_base_opp * horizontal_base_rivers(state, opponent, false);
    final_score += weights_.horizontal_attack_opp * horizontal_attack(state, opponent, false);
    final_score += weights_.inactive_opp * inactive_pieces(state, opponent, false);
    final_score += weights_.connectedness_self_opp * connectedness_h(state, opponent, true, true);
    final_score += weights_.connectedness_all_opp * connectedness_h(state, opponent, false, true);

    return final_score;
}

// Simplified version that excludes expensive heuristics
// Excludes: connectedness_h, possible_moves_h, vertical_river_on_top_peri_h
double Heuristics::evaluate_position_incremental(GameState& state, const std::string& player) {
    if (state.is_terminal()) return terminal_result(state, player, true);

    double final_score = 0.0;

    // Use actual heuristic functions (optimized with encoded board)
    final_score += weights_.vertical_push * vertical_push_h(state, player, true);
    final_score += weights_.pieces_in_scoring_attack * pieces_in_scoring_h(state, player, true);
    final_score += weights_.horizontal_attack_self * horizontal_attack(state, player, true);
    final_score += weights_.inactive_self * inactive_pieces(state, player, true);
    final_score += weights_.pieces_blocking_vertical_self * pieces_blocking_vertical_h(state, player, true);
    final_score += weights_.horizontal_base_self * horizontal_base_rivers(state, player, true);
    final_score += weights_.horizontal_negative_self * horizontal_negative(state, player, true);

    // Opponent heuristics (defense)
    final_score += weights_.pieces_in_scoring_defense * pieces_in_scoring_h(state, player, false);
    std::string opponent = get_opponent(player);
    final_score += weights_.pieces_blocking_vertical_opp * pieces_blocking_vertical_h(state, opponent, false);
    final_score += weights_.horizontal_base_opp * horizontal_base_rivers(state, opponent, false);
    final_score += weights_.horizontal_attack_opp * horizontal_attack(state, opponent, false);
    final_score += weights_.inactive_opp * inactive_pieces(state, opponent, false);

    return final_score;
}

void Heuristics::debug_heuristic(const GameState& state, const std::string& player) {
    std::cout << "----- Debug Heuristic -----" << std::endl;
    std::cout << "Debugging Heuristic Components for player: " << player << std::endl;
    std::cout << "Vertical Push Heuristic: " << vertical_push_h(state, player, true) << " weighted: " << weights_.vertical_push * vertical_push_h(state, player, true) << std::endl;
    std::cout << "Connectedness Heuristic (self): " << connectedness_h(state, player, true, true) << " weighted: " << weights_.connectedness_self * connectedness_h(state, player, true, true) << std::endl;
    std::cout << "Connectedness Heuristic (all): " << connectedness_h(state, player, false, true) << " weighted: " << weights_.connectedness_all * connectedness_h(state, player, false, true) << std::endl;
    std::cout << "Pieces in Scoring Area Heuristic (attack): " << pieces_in_scoring_h(state, player, true) << " weighted: " << weights_.pieces_in_scoring_attack * pieces_in_scoring_h(state, player, true) << std::endl;
    std::cout << "Possible Moves Heuristic: " << possible_moves_h(state, player, true) << " weighted: " << weights_.possible_moves_self * possible_moves_h(state, player, true) << std::endl;
    std::cout << "Pieces Blocking Vertical Rivers Heuristic: " << pieces_blocking_vertical_h(state, player, true) << " weighted: " << weights_.pieces_blocking_vertical_self * pieces_blocking_vertical_h(state, player, true) << std::endl;
    std::cout << "Horizontal Base Rivers Heuristic: " << horizontal_base_rivers(state, player, true) << " weighted: " << weights_.horizontal_base_self * horizontal_base_rivers(state, player, true) << std::endl;
    std::cout << "Horizontal Negative Heuristic: " << horizontal_negative(state, player, true) << " weighted: " << weights_.horizontal_negative_self * horizontal_negative(state, player, true) << std::endl;
    std::cout << "Horizontal Attack Heuristic: " << horizontal_attack(state, player, true) << " weighted: " << weights_.horizontal_attack_self * horizontal_attack(state, player, true) << std::endl;
    std::cout << "Inactive Pieces Heuristic: " << inactive_pieces(state, player, true) << " weighted: " << weights_.inactive_self * inactive_pieces(state, player, true) << std::endl;

    std::string opponent = get_opponent(player);
    std::cout << "--- Opponent (" << opponent << ") Heuristics ---" << std::endl;
    std::cout << "Pieces in Scoring Area Heuristic (defense): " << pieces_in_scoring_h(state, player, false) << " weighted: " << weights_.pieces_in_scoring_defense * pieces_in_scoring_h(state, player, false) << std::endl;
    std::cout << "Possible Moves Heuristic: " << possible_moves_h(state, opponent, false) << " weighted: " << weights_.possible_moves_opp * possible_moves_h(state, opponent, false) << std::endl;
    std::cout << "Pieces Blocking Vertical Rivers Heuristic: " << pieces_blocking_vertical_h(state, opponent, false) << " weighted: " << weights_.pieces_blocking_vertical_opp * pieces_blocking_vertical_h(state, opponent, false) << std::endl;
    std::cout << "Horizontal Base Rivers Heuristic: " << horizontal_base_rivers(state, opponent, false) << " weighted: " << weights_.horizontal_base_opp * horizontal_base_rivers(state, opponent, false) << std::endl;
    std::cout << "Horizontal Attack Heuristic: " << horizontal_attack(state, opponent, false) << " weighted: " << weights_.horizontal_attack_opp * horizontal_attack(state, opponent, false) << std::endl;
    std::cout << "Inactive Pieces Heuristic: " << inactive_pieces(state, opponent, false) << " weighted: " << weights_.inactive_opp * inactive_pieces(state, opponent, false) << std::endl;
    std::cout << "---------------------------" << std::endl;

}