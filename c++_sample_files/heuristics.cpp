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

void Heuristics::adjust_weights(const GameState& state, const std::string& player, double delta) {
    // Simple linear update: w_i += lr * delta * component_i
    const double lr = 0.01; // small learning rate

    // self: attack
    weights_.vertical_push += lr * delta * vertical_push_h(state, player, true);
    weights_.connectedness_self += lr * delta * connectedness_h(state, player, true, true);
    weights_.connectedness_all += lr * delta * connectedness_h(state, player, false, true);
    weights_.pieces_in_scoring_attack += lr * delta * pieces_in_scoring_h(state, player, true);
    weights_.manhattan_distance += lr * delta * manhattan_distance_h(state, player, true);
    weights_.possible_moves_self += lr * delta * possible_moves_h(state, player, true);
    weights_.stones_reaching_self += lr * delta * stones_reaching_riv_h(state, player, true, true);
    weights_.horizontal_attack_self += lr * delta * horizontal_attack(state, player, true);
    weights_.inactive_self += lr * delta * inactive_pieces(state, player, true);

    // self: defense
    weights_.pieces_blocking_vertical_self += lr * delta * pieces_blocking_vertical_h(state, player, true);
    weights_.horizontal_base_self += lr * delta * horizontal_base_rivers(state, player, true);
    weights_.horizontal_negative_self += lr * delta * horizontal_negative(state, player, true);

    // opponent related
    weights_.pieces_in_scoring_defense += lr * delta * pieces_in_scoring_h(state, player, true);
    std::string opponent = get_opponent(player);
    weights_.possible_moves_opp += lr * delta * possible_moves_h(state, opponent, false);
    weights_.pieces_blocking_vertical_opp += lr * delta * pieces_blocking_vertical_h(state, opponent, false);
    weights_.horizontal_base_opp += lr * delta * horizontal_base_rivers(state, opponent, false);
    weights_.horizontal_attack_opp += lr * delta * horizontal_attack(state, opponent, false);
    weights_.inactive_opp += lr * delta * inactive_pieces(state, opponent, false);

    // Print updated weights for debugging/visibility
    // std::cout
    //     << "[Heuristics] Updated weights (delta=" << delta << ")\n"
    //     << "  vertical_push=" << weights_.vertical_push
    //     << ", connectedness_self=" << weights_.connectedness_self
    //     << ", connectedness_all=" << weights_.connectedness_all
    //     << ", pieces_in_scoring_attack=" << weights_.pieces_in_scoring_attack
    //     << ", manhattan_distance=" << weights_.manhattan_distance
    //     << ", possible_moves_self=" << weights_.possible_moves_self
    //     << ", stones_reaching_self=" << weights_.stones_reaching_self
    //     << ", horizontal_attack_self=" << weights_.horizontal_attack_self
    //     << ", inactive_self=" << weights_.inactive_self
    //     << "\n  pieces_blocking_vertical_self=" << weights_.pieces_blocking_vertical_self
    //     << ", horizontal_base_self=" << weights_.horizontal_base_self
    //     << ", horizontal_negative_self=" << weights_.horizontal_negative_self
    //     << ", pieces_in_scoring_defense=" << weights_.pieces_in_scoring_defense
    //     << ", possible_moves_opp=" << weights_.possible_moves_opp
    //     << ", pieces_blocking_vertical_opp=" << weights_.pieces_blocking_vertical_opp
    //     << ", horizontal_base_opp=" << weights_.horizontal_base_opp
    //     << ", horizontal_attack_opp=" << weights_.horizontal_attack_opp
    //     << ", inactive_opp=" << weights_.inactive_opp
    //     << std::endl;
}

// compute_components removed; calls are inlined


int Heuristics::max(int a, int b) {
    return (a > b) ? a : b;
}

// ---- Custom Game-Specific Heuristic Functions ----

int Heuristics::vertical_push_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& board = state.board;
    int rows = state.rows;
    int cols = state.cols;
    const auto& score_cols = state.score_cols;

    int score = 0;
    const int COLUMN_WEIGHT = 1;
    std::vector<double> col_weight(12);
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
    std::vector<std::vector<int>> reach(rows, std::vector<int>(cols, 0));


    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const auto& cell = board[y][x];
            if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == player &&
                cell.find("side") != cell.end() && cell.at("side") == "river" && 
                cell.find("orientation") != cell.end() && cell.at("orientation") == "vertical") {

                for (int dist = 1; dist < rows; ++dist) {
                    int ny = y + push_direction * dist;
                    if (!in_bounds(x, ny, rows, cols)) {
                        break;
                    }

                    const auto& next_cell = board[ny][x];
                    
                    //addd scoring rea constraint here
                    if (next_cell.empty()) {
                        reach[y][x] = col_weight[x];
                        continue;
                    }
                    else if (next_cell.find("owner") != next_cell.end() && next_cell.at("owner") == opponent) {
                        break;
                    }
                    else {
                        reach[y][x] = col_weight[x];
                        continue;
                    }
                }
            }
        }
    }
    for (auto &e1 : reach){
        for (auto &e2 : e1){
            score += e2;
        }
    }
    return wrt_self ? score : -score;
}


int Heuristics::connectedness_h(const GameState& state, const std::string& player, bool self, bool wrt_self) {
    const auto& board = state.board;
    int rows = state.rows;
    int cols = state.cols;

    // 1. Collect all rivers that we should consider for the search.
    std::vector<std::pair<int, int>> target_rivers;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const auto& cell = board[y][x];
            if (cell.empty() || cell.at("side") != "river") {
                continue;
            }
            // If 'self' is true, only include the current player's rivers.
            if (self && cell.at("owner") != player) {
                continue;
            }
            target_rivers.push_back({x, y});
        }
    }

    int total_connected_pairs = 0;
    std::set<std::pair<int, int>> globally_visited;

    // 2. Iterate through each river to find its connected component.
    for (const auto& start_pos : target_rivers) {
        if (globally_visited.count(start_pos)) {
            continue; // This river is already part of a component we've counted.
        }

        // 3. Found a new component. Start a BFS to find all its members.
        int component_size = 0;
        std::queue<std::pair<int, int>> q;

        q.push(start_pos);
        globally_visited.insert(start_pos);

        while (!q.empty()) {
            auto [x, y] = q.front();
            q.pop();
            component_size++;

            const auto& current_river = board[y][x];
            const std::string& orientation = current_river.at("orientation");

            std::vector<std::pair<int, int>> dirs;
            if (orientation == "horizontal") {
                dirs = {{-1, 0}, {1, 0}}; // Check left and right
            } else { // vertical
                dirs = {{0, -1}, {0, 1}}; // Check up and down
            }

            // 4. Trace outwards from the current river.
            for (auto [dx, dy] : dirs) {
                int nx = x + dx;
                int ny = y + dy;

                while (in_bounds(nx, ny, rows, cols)) {
                    const auto& next_cell = board[ny][nx];

                    // Path is blocked if the cell is not empty.
                    if (!next_cell.empty()) {
                        // Check if the blocker is a valid, unvisited river of the SAME orientation.
                        if (next_cell.at("side") == "river" &&
                            next_cell.at("orientation") == orientation &&
                            !globally_visited.count({nx, ny}))
                        {
                            // Check ownership if 'self' mode is enabled.
                            bool is_valid_target = true;
                            if (self && next_cell.at("owner") != player) {
                                is_valid_target = false;
                            }

                            if (is_valid_target) {
                                q.push({nx, ny});
                                globally_visited.insert({nx, ny});
                            }
                        }
                        // Any piece (stone, wrong river type, etc.) stops the flow.
                        break;
                    }
                    // Continue tracing if the path is clear.
                    nx += dx;
                    ny += dy;
                }
            }
        }
        
        // 5. Once the component is fully explored, calculate and add its pairs.
        if (component_size > 1) {
            total_connected_pairs += component_size * (component_size - 1) / 2;
        }
    }

    return wrt_self ? total_connected_pairs : -total_connected_pairs;
}
     

// int Heuristics::pieces_in_scoring_h(const GameState& state, const std::string& player, bool attack, bool wrt_self) {
//     const auto& board = state.board;
//     int rows = state.rows;
//     int cols = state.cols;
//     const auto& score_cols = state.score_cols;

//     int score = 0;
//     const int w1 = 1e4;  // Pieces in exact scoring row/cols

//     int target_row, direction;
//     std::string player_to_check;

//     if ((player == "circle" && attack) || (player == "square" && !attack)) {
//         target_row = top_score_row();
//         direction = -1; 
//         player_to_check = "circle";
//     } else {
//         target_row = bottom_score_row(rows);
//         direction = 1;
//         player_to_check = "square";
//     }

//     int in_score_area = 0;
//     std::vector <int> virgin_cols;
//     for (int col = 4; col <= 7; col++) {
//         if (board[target_row][col].empty()) {
//             virgin_cols.push_back(col);
//             continue;
//         }
//         score += w1;
//         in_score_area++;
//         if(board[target_row][col].at("side") == "stone" && board[target_row][col].at("owner") == player_to_check) {
//             score += 4000; // Extra bonus for stone in scoring area
//         }
//     }

//     for(int col = 1; col <= 10; col++) {
//         for(int row = target_row - 2; row <= target_row + 2; row++) {
//             if(row < 0 || row >= rows) continue;
//             if(board[row][col].empty()) continue;
//             if(board[row][col].at("owner") != player_to_check) continue;
//             if(row==target_row && col>=4 && col<=7) continue;
//             int max_score = 0;
//             for(int vc: virgin_cols){
//                 int man_dist = abs(vc - col) + abs(target_row - row);
//                 if(man_dist == 1){
//                     max_score = max(max_score, 300*in_score_area);
//                 }
//                 if(man_dist == 2){
//                     max_score = max(max_score, 100*in_score_area);
//                 }
//                 if(man_dist == 3){
//                     max_score = max(max_score, 50*in_score_area);
//                 }
//             }
//             score += max_score;
            
//         }
//     }

//     return wrt_self ? score : -score;
// }


int Heuristics::pieces_in_scoring_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& board = state.board;
    int rows = state.rows;
    int cols = state.cols;
    const auto& score_cols = state.score_cols;

    int score = 0;
    const int w1 = 1e3;  // Pieces in exact scoring row/cols
    const int w2 = 350;   // Pieces very close to scoring area
    const int w3 = 175;   // Pieces moderately close
    const int w4 = 80;   // Pieces somewhat close
    const int w5 = 4;   // Pieces far but still relevant

    int target_row, direction;
    std::string player_to_check;
if (!wrt_self){
    if (player == "circle") player_to_check = "square";
    else player_to_check = "circle";
}
if (player_to_check == "circle"){
        target_row = top_score_row();
        direction = -1; 
        // player_to_check = "circle";
    } else {
        target_row = bottom_score_row(rows);
        direction = 1;
        // player_to_check = "square";
}

   

    std::map<int, std::map<int, int>> val;
    int in_score_area = 0;
    std::vector <int> virgin_cols;
    for (int col = 4; col <= 7; col++) {
        if (board[target_row][col].empty()) {
            virgin_cols.push_back(col);
            continue;
        }
        val[target_row][col] = max(val[target_row][col], w1);
        in_score_area++;
        if(board[target_row][col].at("side") == "stone" && board[target_row][col].at("owner") == player_to_check) {
            score += 400; // Extra bonus for stone in scoring area
        }
    }
    // for(int col = 4; col <= 7; col++) {
    //     if(board[target_row][col].empty()) {
    //         if(!board[target_row+1][col].empty() && board[target_row+1][col].at("owner") == player_to_check) {
    //             score += 300*in_score_area;
    //         }
    //         if(!board[target_row-1][col].empty() && board[target_row-1][col].at("owner") == player_to_check) {
    //             score += 300*in_score_area;
    //         }
    //     }
    // }
    // if(board[target_row][4].empty()) {
    //     if(!board[target_row][3].empty() && board[target_row][3].at("owner") == player_to_check) {
    //         score += 300*in_score_area;
    //     }
    // }
    // if(board[target_row][7].empty()) {
    //     if(!board[target_row][8].empty() && board[target_row][8].at("owner") == player_to_check) {
    //         score += 300*in_score_area;
    //     }
    // }

    for(int col = 1; col <= 10; col++) {
        for(int row = target_row - 2; row <= target_row + 2; row++) {
            if(row < 0 || row >= rows) continue;
            if(board[row][col].empty()) continue;
            if(board[row][col].at("owner") != player_to_check) continue;
            if(row==target_row && col>=4 && col<=7) continue;
            for(int vc: virgin_cols){
                int man_dist = abs(vc - col) + abs(target_row - row);
                if(man_dist == 1){
                    score += 200*in_score_area;
                }
                if(man_dist == 2){
                    score += 100*in_score_area;
                }
                if(man_dist == 3){
                    score += 50*in_score_area;
                }
            }
            
        }
    }
    
    


    
    for (int col = 3; col <= 8; col++) {
        for (int r = 0; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row < 0 || check_row >= rows || board[check_row][col].empty()) continue;
            if (board[check_row][col].find("owner") != board[check_row][col].end() && 
                board[check_row][col].at("owner") == player_to_check) {
                val[check_row][col] = max(val[check_row][col], w2);
            }
        }
    }
    
    for (int col = 2; col <= 9; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row < 0 || check_row >= rows || board[check_row][col].empty()) continue;
            if (board[check_row][col].find("owner") != board[check_row][col].end() && 
                board[check_row][col].at("owner") == player_to_check) {
                val[check_row][col] = max(val[check_row][col], w3);
            }
        }
    }
    
    for (int col = 1; col <= 10; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row < 0 || check_row >= rows || board[check_row][col].empty()) continue;
            if (board[check_row][col].find("owner") != board[check_row][col].end() && 
                board[check_row][col].at("owner") == player_to_check) {
                val[check_row][col] = max(val[check_row][col], w4);
            }
        }
    }
    
    for (int col = 0; col <= 11; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row < 0 || check_row >= rows || board[check_row][col].empty()) continue;
            if (board[check_row][col].find("owner") != board[check_row][col].end() && 
                board[check_row][col].at("owner") == player_to_check) {
                val[check_row][col] = max(val[check_row][col], w5);
            }
        }
    }
    
    for (auto &e1 : val) {
        for (auto &e2 : e1.second) {
            score += e2.second;
            // std::cout << "Row: " << e1.first << ", Col: " << e2.first << ", Value: " << e2.second << std::endl;
        }
    }

    return wrt_self ? score : -score;
}

int Heuristics::possible_moves_h(const GameState& state, const std::string& player, bool wrt_self) {
    int num_moves = 0;

    // Iterate over board to find current player's pieces
    for (int y = 0; y < state.rows; y++) {
        for (int x = 0; x < state.cols; x++) {
            const auto &cell = state.board[y][x];
            if (cell.empty()) continue;

            if (cell.at("owner") != player) continue; // only current player's pieces

            std::string side_type = cell.at("side");

            // Check if piece is in scoring area
            bool in_scoring_area = is_my_score_cell(x, y, player, state.rows, state.cols, state.score_cols);

            // ---- MOVES (including river flow) ----
            // Only allow moves if piece is NOT in scoring area
            // if (!in_scoring_area) {
                auto valid_targets = compute_valid_targets(state.board, x, y, player, state.rows, state.cols, state.score_cols);
                
                // Add regular moves and river flow moves
                for (const auto& target : valid_targets.moves) {
                    num_moves++;
                }

                // ---- PUSHES (including river flow pushes) ----
                for (const auto& push : valid_targets.pushes) {
                    auto target_pos = push.first;
                    auto pushed_pos = push.second;
                    num_moves++;
                }
            // }

            // ---- FLIP ----
            if (side_type == "stone") {
                // Only allow flipping stone to river if NOT in scoring area
                // if (!in_scoring_area) {
                    // Check if flipping to river would be safe (not flowing into opponent score)
                    for (const std::string& orientation : {"horizontal", "vertical"}) {
                        // Simulate the flip and check resulting flow
                        bool safe = true;
                        
                        // Create a temporary modified board to test the flip
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
                // }
            } else if (side_type == "river") {
                // Always allow flipping river to stone (including in scoring area)
                num_moves++;
            }

            // ---- ROTATE ----
            // Only allow rotation of rivers
            if (side_type == "river") {
                // Check if rotation would be safe
                std::string current_orientation = cell.at("orientation");
                std::string new_orientation = (current_orientation == "horizontal") ? "vertical" : "horizontal";
                
                // Create a temporary modified board to test the rotation
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

int Heuristics::stones_reaching_riv_h(const GameState& state, const std::string& player, bool self, bool wrt_self) {
    const auto& board = state.board;
    int rows = state.rows;
    int cols = state.cols;

    int count = 0;
    std::string target_owner = self ? player : get_opponent(player);
    std::vector<std::pair<int, int>> dirs = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const auto& cell = board[y][x];
            if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == player && 
                cell.find("side") != cell.end() && cell.at("side") == "stone") {
                bool can_reach = false;
                for (auto [dx, dy] : dirs) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (in_bounds(nx, ny, rows, cols)) {
                        const auto& target_cell = board[ny][nx];
                        if (!target_cell.empty() && 
                            target_cell.find("side") != target_cell.end() && target_cell.at("side") == "river" && 
                            target_cell.find("owner") != target_cell.end() && target_cell.at("owner") == target_owner) {
                           can_reach = true;
                           break;
                        }
                    }
                }
                if (can_reach) {
                    count++;
                }
            }
        }
    }
    return wrt_self ? count : -count;
}

int Heuristics::pieces_blocking_vertical_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& board = state.board;
    int rows = state.rows;
    int cols = state.cols;

    int block_count = 0;
    std::string opponent = get_opponent(player);
    int flow_dy = (opponent == "square") ? 1 : -1;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const auto& cell = board[y][x];
            if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == opponent &&
                cell.find("side") != cell.end() && cell.at("side") == "river" && 
                cell.find("orientation") != cell.end() && cell.at("orientation") == "vertical") {

                int ny = y + flow_dy;
                while (in_bounds(x, ny, rows, cols)) {
                    const auto& blocking_cell = board[ny][x];
                    if (blocking_cell.empty()) {
                        ny += flow_dy;
                        continue;
                    }
                    if (blocking_cell.find("owner") != blocking_cell.end() && blocking_cell.at("owner") == player) {
                        if (blocking_cell.find("side") != blocking_cell.end() && blocking_cell.at("side") == "river" && 
                            blocking_cell.find("orientation") != blocking_cell.end() && blocking_cell.at("orientation") == "horizontal") {
                            block_count++;
                            break;
                        }
                        if (blocking_cell.find("side") != blocking_cell.end() && blocking_cell.at("side") == "stone" && 
                            std::abs(ny - y) > 1) {
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

int Heuristics::vertical_river_on_top_peri_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& board = state.board;
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
        const auto& cell = board[perimeter_row][x];
        if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == player &&
            cell.find("side") != cell.end() && cell.at("side") == "river" && 
            cell.find("orientation") != cell.end() && cell.at("orientation") == "vertical") {
            count++;
        }
    }
    return wrt_self ? count : -count;
}

int Heuristics::horizontal_base_rivers(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& board = state.board;
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
                const auto& cell = board[r][x];
                if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == player &&
                    cell.find("side") != cell.end() && cell.at("side") == "river" && 
                    cell.find("orientation") != cell.end() && cell.at("orientation") == "horizontal") {
                    count++;
                }
            }
        }
    }
    return wrt_self ? count : -count;
}

int Heuristics::horizontal_negative(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& board = state.board;
    int rows = state.rows;
    int cols = state.cols;

    int count = 0;

    if (player == "square") {
        int base_row = top_score_row();
        for (int y = 0; y < base_row; ++y) {
            for (int x = 0; x < cols; ++x) {
                const auto& cell = board[y][x];
                if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == player &&
                    cell.find("side") != cell.end() && cell.at("side") == "river" && 
                    cell.find("orientation") != cell.end() && cell.at("orientation") == "horizontal") {
                    count++;
                }
            }
        }
    } else {
        int base_row = bottom_score_row(rows);
        for (int y = base_row + 1; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                const auto& cell = board[y][x];
                if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == player &&
                    cell.find("side") != cell.end() && cell.at("side") == "river" && 
                    cell.find("orientation") != cell.end() && cell.at("orientation") == "horizontal") {
                    count++;
                }
            }
        }
    }
    return wrt_self ? -count : count;
}



int Heuristics::horizontal_attack(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& board = state.board;
    int rows = state.rows;
    int cols = state.cols;

    int count = 0;
    std::vector<int> check_rows;

    if (player == "circle") {
        check_rows.push_back(top_score_row()-1);
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
                const auto& cell = board[r][x];
                if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == player && cell.at("side") == "river" && cell.at("orientation") == "horizontal") {
                    //first left side case
                    if (x < 4){
                        int num_possible = 0;
                        int curr_col = x+1;
                        while(in_bounds(curr_col, r, rows, cols) && ((board[r][curr_col].empty() || (board[r][curr_col].at("side") == "river" && board[r][curr_col].at("orientation") == "horizontal") || (board[r][curr_col].at("owner") == player)))) {
                            if( curr_col >7) break;
                            num_possible+=mul;
                            curr_col++;
                        }
                        count += num_possible;
                    }
                    else if (x > 7){ //right side case
                        int num_possible = 0;
                        int curr_col = x-1;
                        while(in_bounds(curr_col, r, rows, cols) && ((board[r][curr_col].empty() || (board[r][curr_col].at("side") == "river" && board[r][curr_col].at("orientation") == "horizontal") || (board[r][curr_col].at("owner") == player)))) {
                            if(curr_col <4) break;
                            num_possible+=mul;
                            curr_col--;
                        }
                        count += num_possible;
                    }
                    else{ //middle case
                    int num_possible = 0;
                    int curr_col = x+1;
                    while(in_bounds(curr_col, r, rows, cols) && ((board[r][curr_col].empty() || (board[r][curr_col].at("side") == "river" && board[r][curr_col].at("orientation") == "horizontal") || (board[r][curr_col].at("owner") == player)))) {
                        if(curr_col >=4 && curr_col <=7) break;
                        num_possible+=mul;
                        curr_col++;
                    }
                    curr_col = x-1;
                    while(in_bounds(curr_col, r, rows, cols) && ((board[r][curr_col].empty() || (board[r][curr_col].at("side") == "river" && board[r][curr_col].at("orientation") == "horizontal") || (board[r][curr_col].at("owner") == player)))) {
                        if(curr_col >=4 && curr_col <=7) break;
                        num_possible+=mul;
                        curr_col--;
                    }
                    count += num_possible;
                }
            }
        }
    }
}
    return wrt_self ? count : -count;
}



int Heuristics::inactive_pieces(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& board = state.board;
    int rows = state.rows;
    int cols = state.cols;

    int inactive_count = 0;

    for (int y = 0; y < rows; ++y) {
        int x = 0;
        const auto& cell = board[y][x];
        if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == player) {
            inactive_count++;
        }
        x = cols - 1;
        const auto& cell2 = board[y][x];
        if (!cell2.empty() && cell2.find("owner") != cell2.end() && cell2.at("owner") == player) {
            inactive_count++;
        }
    }
    for (int x = 0; x < cols; ++x) {
        int y = 0;
        const auto& cell = board[y][x];
        if (!cell.empty() && cell.find("owner") != cell.end() && cell.at("owner") == player) {
            inactive_count++;
        }
        y = rows - 1;
        const auto& cell2 = board[y][x];
        if (!cell2.empty() && cell2.find("owner") != cell2.end() && cell2.at("owner") == player) {
            inactive_count++;
        }
    }
    return wrt_self ? -inactive_count : +inactive_count;
}

int Heuristics::manhattan_distance_h(const GameState& state, const std::string& player, bool wrt_self) {
    const auto& board = state.board;
    int rows = state.rows;
    int cols = state.cols;
    const auto& score_cols = state.score_cols;

    int total_distance = 0;
    int piece_count = 0;
    int target_row;

    // Determine the target scoring row based on player
    if (player == "circle") {
        target_row = top_score_row();
    } else {
        target_row = bottom_score_row(rows);
    }

    // Iterate through all board positions to find player's pieces
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const auto& cell = board[y][x];
            
            // Check if this cell contains a piece owned by the current player
            if (!cell.empty() && 
                cell.find("owner") != cell.end() && 
                cell.at("owner") == player) {
                
                piece_count++;
                
                // Calculate Manhattan distance to the closest scoring column
                int min_col_distance = INT_MAX;
                for (int score_col : score_cols) {
                    if (score_col >= 0 && score_col < cols) {
                        int col_distance = std::abs(x - score_col);
                        min_col_distance = std::min(min_col_distance, col_distance);
                    }
                }
                
                // Add row distance to target row
                int row_distance = std::abs(y - target_row);
                
                // Total Manhattan distance for this piece
                int piece_distance = row_distance + min_col_distance;
                total_distance += piece_distance;
            }
        }
    }

        int value = -(total_distance);
        return wrt_self ? value : -value;
}


int Heuristics::terminal_result(const GameState& state, const std::string& player, bool wrt_self) {
    int WIN_COUNT = 4;
    int top = top_score_row();
    int bot = bottom_score_row(state.rows);
    int ccount = 0, scount = 0;
    
    for (int x : state.score_cols) {
        if (x >= 0 && x < state.cols) {
            if (top >= 0 && top < state.rows) {
                const auto& cell = state.board[top][x];
                if (!cell.empty() && cell.at("owner") == "circle" && cell.at("side") == "stone") {
                    ccount++;
                }
            }
            if (bot >= 0 && bot < state.rows) {
                const auto& cell = state.board[bot][x];
                if (!cell.empty() && cell.at("owner") == "square" && cell.at("side") == "stone") {
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

    // // self: attack
    final_score += weights_.vertical_push * vertical_push_h(state, player, true);
    final_score += weights_.connectedness_self * connectedness_h(state, player, true, true);
    final_score += weights_.connectedness_all * connectedness_h(state, player, false, true);
    final_score += weights_.pieces_in_scoring_attack * pieces_in_scoring_h(state, player, true);
    // final_score += weights_.manhattan_distance * manhattan_distance_h(state, player, true);
    // std::cout << "md: " << manhattan_distance_h(state, player, true) << std::endl;
    final_score += weights_.possible_moves_self * possible_moves_h(state, player, true);
    // final_score += weights_.stones_reaching_self * stones_reaching_riv_h(state, player, true, true);
    final_score += weights_.horizontal_attack_self * horizontal_attack(state, player, true);
    final_score += weights_.inactive_self * inactive_pieces(state, player, true);

    // // self: defense
    final_score += weights_.pieces_blocking_vertical_self * pieces_blocking_vertical_h(state, player, true);
    final_score += weights_.horizontal_base_self * horizontal_base_rivers(state, player, true);
    final_score += weights_.horizontal_negative_self * horizontal_negative(state, player, true);
    
    // // opponent related
    final_score += weights_.pieces_in_scoring_defense * pieces_in_scoring_h(state, player, false);
    std::string opponent = get_opponent(player);
    final_score += weights_.possible_moves_opp * possible_moves_h(state, opponent, false);
    final_score += weights_.pieces_blocking_vertical_opp * pieces_blocking_vertical_h(state, opponent, false);
    final_score += weights_.horizontal_base_opp * horizontal_base_rivers(state, opponent, false);
    final_score += weights_.horizontal_attack_opp * horizontal_attack(state, opponent, false);
    final_score += weights_.inactive_opp * inactive_pieces(state, opponent, false);
    final_score += weights_.connectedness_self_opp * connectedness_h(state, opponent, true, true);
    final_score += weights_.connectedness_all_opp * connectedness_h(state, opponent, false, true);

    // std::cout << final_score << std::endl;

    return final_score;
}



void Heuristics::debug_heuristic(const GameState& state, const std::string& player) {
    std::cout << "----- Debug Heuristic -----" << std::endl;
    std::cout << "Debugging Heuristic Components for player: " << player << std::endl;
    std::cout << "Vertical Push Heuristic: " << vertical_push_h(state, player, true) << std::endl;
    std::cout << "Connectedness Heuristic (self): " << connectedness_h(state, player, true, true) << std::endl;
    std::cout << "Connectedness Heuristic (all): " << connectedness_h(state, player, false, true) << std::endl;
    std::cout << "Pieces in Scoring Area Heuristic (attack): " << pieces_in_scoring_h(state, player, true) << std::endl;
    std::cout << "Possible Moves Heuristic: " << possible_moves_h(state, player, true) << std::endl;
    std::cout << "Stones Reaching River Heuristic (self): " << stones_reaching_riv_h(state, player, true, true) << std::endl;
    std::cout << "Pieces Blocking Vertical Rivers Heuristic: " << pieces_blocking_vertical_h(state, player, true) << std::endl;
    std::cout << "Vertical River on Top Perimeter Heuristic: " << vertical_river_on_top_peri_h(state, player, true) << std::endl;
    std::cout << "Horizontal Base Rivers Heuristic: " << horizontal_base_rivers(state, player, true) << std::endl;
    std::cout << "Horizontal Negative Heuristic: " << horizontal_negative(state, player, true) << std::endl;
    std::cout << "Horizontal Attack Heuristic: " << horizontal_attack(state, player, true) << std::endl;
    std::cout << "Inactive Pieces Heuristic: " << inactive_pieces(state, player, true) << std::endl;
    std::cout << "Manhattan Distance Heuristic: " << manhattan_distance_h(state, player, true) << std::endl;

    std::string opponent = get_opponent(player);
    std::cout << "--- Opponent (" << opponent << ") Heuristics ---" << std::endl;
    std::cout << "Pieces in Scoring Area Heuristic (defense): " << pieces_in_scoring_h(state, player, false) << std::endl;
    std::cout << "Possible Moves Heuristic: " << possible_moves_h(state, opponent, false) << std::endl;
    std::cout << "Pieces Blocking Vertical Rivers Heuristic: " << pieces_blocking_vertical_h(state, opponent, false) << std::endl;
    std::cout << "Horizontal Base Rivers Heuristic: " << horizontal_base_rivers(state, opponent, false) << std::endl;
    std::cout << "Horizontal Attack Heuristic: " << horizontal_attack(state, opponent, false) << std::endl;
    std::cout << "Inactive Pieces Heuristic: " << inactive_pieces(state, opponent, false) << std::endl;
    std::cout << "---------------------------" << std::endl;

}