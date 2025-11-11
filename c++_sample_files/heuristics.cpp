#include "heuristics.h"
#include <cmath>
#include <algorithm>
#include <set>
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

// Vertical push heuristic - calculates reach of vertical rivers
double Heuristics::vertical_push_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;

    double score = 0;
    std::vector<double> col_weight(12);

    // Column weights
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

    // Find all player's vertical rivers and calculate their reach
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            EncodedCell cell = encoded_board[y][x];

            if (GameState::is_owner(cell, player) && GameState::is_vertical_river(cell)) {
                // Calculate reach in push direction
                for (int dist = 1; dist < rows; ++dist) {
                    int ny = y + push_direction * dist;
                    if (!in_bounds(x, ny, rows, cols)) break;

                    EncodedCell next_cell = encoded_board[ny][x];

                    if (GameState::is_empty(next_cell)) {
                        reach[ny][x] = col_weight[x];
                        continue;
                    }
                    else if (GameState::is_owner(next_cell, opponent)) {
                        break;  // Blocked by opponent
                    }
                    else {
                        reach[ny][x] = col_weight[x];  // Can reach through own pieces
                        continue;
                    }
                }
            }
        }
    }

    // Sum all reach values
    for (auto &e1 : reach) {
        for (auto &e2 : e1) {
            score += e2;
        }
    }
    return wrt_self ? score : -score;
}

// Split scoring heuristic - Part 1: Virgin columns proximity bonus (recalculated each time)
int Heuristics::pieces_in_scoring_virgin_cols(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;

    int score = 0;
    std::string player_to_check = player;
    if (!wrt_self) {
        player_to_check = get_opponent(player);
    }

    int target_row = (player_to_check == "circle") ? top_score_row() : bottom_score_row(rows);
    
    // Find virgin (empty) columns in scoring area
    int in_score_area = 0;
    std::vector<int> virgin_cols;
    for (int col = 4; col <= 7; col++) {
        EncodedCell cell = encoded_board[target_row][col];
        if (GameState::is_empty(cell)) {
            virgin_cols.push_back(col);
        } else {
            in_score_area++;
            if (GameState::is_stone(cell) && GameState::is_owner(cell, player_to_check)) {
                score += 1000;  // Bonus for stones actually in scoring area
            }
        }
    }

    // Reward pieces near virgin columns based on Manhattan distance
    for (int col = 1; col <= 10; col++) {
        for (int row = target_row - 2; row <= target_row + 2; row++) {
            if (row < 0 || row >= rows) continue;
            EncodedCell cell = encoded_board[row][col];
            if (GameState::is_empty(cell)) continue;
            if (!GameState::is_owner(cell, player_to_check)) continue;
            if (row == target_row && col >= 4 && col <= 7) continue;  // Skip scoring area itself

            for (int vc : virgin_cols) {
                int man_dist = abs(vc - col) + abs(target_row - row);
                if (man_dist == 1) score += 200 * in_score_area;
                else if (man_dist == 2) score += 100 * in_score_area;
                else if (man_dist == 3) score += 50 * in_score_area;
            }
        }
    }

    return wrt_self ? score : -score;
}

// Split scoring heuristic - Part 2: Zonewise weighted pieces (can be incrementally updated)
int Heuristics::pieces_in_scoring_zonewise(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;

    int score = 0;
    const int w1 = 100000;  // Pieces in scoring row
    const int w2 = 350;     // Zone 1: rows±0-1, cols 3-8
    const int w3 = 175;     // Zone 2: rows±(-1)-1, cols 2-9
    const int w4 = 80;      // Zone 3: rows±(-2)-2, cols 1-10
    const int w5 = 4;       // Zone 4: rows±(-2)-2, cols 0-11

    std::string player_to_check = player;
    if (!wrt_self) {
        player_to_check = get_opponent(player);
    }

    int target_row, direction;
    if (player_to_check == "circle") {
        target_row = top_score_row();
        direction = -1;
    } else {
        target_row = bottom_score_row(rows);
        direction = 1;
    }

    std::map<int, std::map<int, int>> val;

    // w1: Scoring row (columns 4-7)
    for (int col = 4; col <= 7; col++) {
        EncodedCell cell = encoded_board[target_row][col];
        if (GameState::is_empty(cell)) continue;
        if (GameState::is_owner(cell, player_to_check)) {
            val[target_row][col] = max(val[target_row][col], w1);
        }
    }

    // w2: Zone 1 (rows±0-1, cols 3-8)
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

    // w3: Zone 2 (rows±(-1)-1, cols 2-9)
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

    // w4: Zone 3 (rows±(-2)-2, cols 1-10)
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

    // w5: Zone 4 (rows±(-2)-2, cols 0-11)
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

    // Sum all zone values
    for (auto &e1 : val) {
        for (auto &e2 : e1.second) {
            score += e2.second;
        }
    }

    return wrt_self ? score : -score;
}

// Pieces blocking opponent's vertical rivers
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
            if (GameState::is_owner(cell, opponent) && GameState::is_vertical_river(cell)) {
                int ny = y + flow_dy;
                while (in_bounds(x, ny, rows, cols)) {
                    EncodedCell blocking_cell = encoded_board[ny][x];
                    if (GameState::is_empty(blocking_cell)) {
                        ny += flow_dy;
                        continue;
                    }
                    if (GameState::is_owner(blocking_cell, player)) {
                        // Horizontal river blocks
                        if (GameState::is_horizontal_river(blocking_cell)) {
                            block_count++;
                            break;
                        }
                        // Stone blocks if distance > 1
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

// Horizontal rivers on base rows (perimeter of scoring area)
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

// Horizontal rivers in "negative" area (wrong side of board)
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

// Horizontal attack: horizontal rivers near opponent's scoring row
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
                                if (curr_col < 4 || curr_col > 7) break;
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
                                if (curr_col < 4 || curr_col > 7) break;
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

// Inactive pieces: pieces on board edges
int Heuristics::inactive_pieces(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    int cols = state.cols;

    int inactive_count = 0;

    // Count pieces on left and right edges
    for (int y = 0; y < rows; ++y) {
        if (GameState::is_owner(encoded_board[y][0], player)) {
            inactive_count++;
        }
        if (GameState::is_owner(encoded_board[y][cols-1], player)) {
            inactive_count++;
        }
    }

    // Count pieces on top and bottom edges
    for (int x = 0; x < cols; ++x) {
        if (GameState::is_owner(encoded_board[0][x], player)) {
            inactive_count++;
        }
        if (GameState::is_owner(encoded_board[rows-1][x], player)) {
            inactive_count++;
        }
    }

    return wrt_self ? -inactive_count : +inactive_count;
}

// Terminal result: check for win condition
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

// Main evaluation function
Heuristics::HeuristicsInfo Heuristics::evaluate_position(const GameState& state, const std::string& player) {
    HeuristicsInfo info;
    if (state.is_terminal()){
        info.total_score = terminal_result(state, player, true);
        return info;
    }
    
    double final_score = 0.0;

    // Self heuristics
    info.vertical_push_value = weights_.vertical_push * vertical_push_h(state, player, true);
    final_score += info.vertical_push_value;
    
    info.pieces_in_scoring_attack_value = weights_.pieces_in_scoring_attack * (pieces_in_scoring_virgin_cols(state, player, true) + pieces_in_scoring_zonewise(state, player, true));
    final_score += info.pieces_in_scoring_attack_value;
    
    info.horizontal_attack_self_value = weights_.horizontal_attack_self * horizontal_attack(state, player, true);
    final_score += info.horizontal_attack_self_value;
    
    info.inactive_self_value = weights_.inactive_self * inactive_pieces(state, player, true);
    final_score += info.inactive_self_value;
    
    info.pieces_blocking_vertical_self_value = weights_.pieces_blocking_vertical_self * pieces_blocking_vertical_h(state, player, true);
    final_score += info.pieces_blocking_vertical_self_value;
    
    info.horizontal_base_self_value = weights_.horizontal_base_self * horizontal_base_rivers(state, player, true);
    final_score += info.horizontal_base_self_value;
    
    info.horizontal_negative_self_value = weights_.horizontal_negative_self * horizontal_negative(state, player, true);
    final_score += info.horizontal_negative_self_value;

    // Opponent heuristics
    std::string opponent = get_opponent(player);
    
    info.pieces_in_scoring_defense_value = weights_.pieces_in_scoring_defense * (pieces_in_scoring_virgin_cols(state, player, false) + pieces_in_scoring_zonewise(state, player, false));
    final_score += info.pieces_in_scoring_defense_value;
    
    info.pieces_blocking_vertical_opp_value = weights_.pieces_blocking_vertical_opp * pieces_blocking_vertical_h(state, opponent, false);
    final_score += info.pieces_blocking_vertical_opp_value;
    
    info.horizontal_base_opp_value = weights_.horizontal_base_opp * horizontal_base_rivers(state, opponent, false);
    final_score += info.horizontal_base_opp_value;
    
    info.horizontal_attack_opp_value = weights_.horizontal_attack_opp * horizontal_attack(state, opponent, false);
    final_score += info.horizontal_attack_opp_value;
    
    info.inactive_opp_value = weights_.inactive_opp * inactive_pieces(state, opponent, false);
    final_score += info.inactive_opp_value;

    info.total_score = final_score;
    return info;
}

void Heuristics::debug_heuristic(const GameState& state, const std::string& player) {
    std::cout << "----- Debug Heuristic -----" << std::endl;
    std::cout << "Player: " << player << std::endl;
    std::cout << "Vertical Push: " << vertical_push_h(state, player, true) 
              << " weighted: " << weights_.vertical_push * vertical_push_h(state, player, true) << std::endl;
    std::cout << "Scoring Virgin Cols: " << pieces_in_scoring_virgin_cols(state, player, true)
              << " weighted: " << weights_.pieces_in_scoring_attack * pieces_in_scoring_virgin_cols(state, player, true) << std::endl;
    std::cout << "Scoring Zonewise: " << pieces_in_scoring_zonewise(state, player, true)
              << " weighted: " << weights_.pieces_in_scoring_attack * pieces_in_scoring_zonewise(state, player, true) << std::endl;
    std::cout << "Horizontal Attack: " << horizontal_attack(state, player, true)
              << " weighted: " << weights_.horizontal_attack_self * horizontal_attack(state, player, true) << std::endl;
    std::cout << "Inactive Pieces: " << inactive_pieces(state, player, true)
              << " weighted: " << weights_.inactive_self * inactive_pieces(state, player, true) << std::endl;
    std::cout << "Pieces Blocking Vertical: " << pieces_blocking_vertical_h(state, player, true)
              << " weighted: " << weights_.pieces_blocking_vertical_self * pieces_blocking_vertical_h(state, player, true) << std::endl;
    std::cout << "Horizontal Base: " << horizontal_base_rivers(state, player, true)
              << " weighted: " << weights_.horizontal_base_self * horizontal_base_rivers(state, player, true) << std::endl;
    std::cout << "Horizontal Negative: " << horizontal_negative(state, player, true)
              << " weighted: " << weights_.horizontal_negative_self * horizontal_negative(state, player, true) << std::endl;

    std::string opponent = get_opponent(player);
    std::cout << "--- Opponent (" << opponent << ") ---" << std::endl;
    std::cout << "Scoring (Defense): " << (pieces_in_scoring_virgin_cols(state, player, false) + pieces_in_scoring_zonewise(state, player, false))
              << " weighted: " << weights_.pieces_in_scoring_defense * (pieces_in_scoring_virgin_cols(state, player, false) + pieces_in_scoring_zonewise(state, player, false)) << std::endl;
    std::cout << "Pieces Blocking Vertical: " << pieces_blocking_vertical_h(state, opponent, false)
              << " weighted: " << weights_.pieces_blocking_vertical_opp * pieces_blocking_vertical_h(state, opponent, false) << std::endl;
    std::cout << "Horizontal Base: " << horizontal_base_rivers(state, opponent, false)
              << " weighted: " << weights_.horizontal_base_opp * horizontal_base_rivers(state, opponent, false) << std::endl;
    std::cout << "Horizontal Attack: " << horizontal_attack(state, opponent, false)
              << " weighted: " << weights_.horizontal_attack_opp * horizontal_attack(state, opponent, false) << std::endl;
    std::cout << "Inactive Pieces: " << inactive_pieces(state, opponent, false)
              << " weighted: " << weights_.inactive_opp * inactive_pieces(state, opponent, false) << std::endl;
    
    HeuristicsInfo info = evaluate_position(state, player);
    std::cout << "Total Evaluation: " << info.total_score << std::endl;
    std::cout << "---------------------------" << std::endl;
}
