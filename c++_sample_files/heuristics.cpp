#include "heuristics.h"
#include <cmath>
#include <algorithm>
#include <set>
#include <iostream>
#include <climits>
#include <map>

Heuristics::Weights Heuristics::weights_{};

// Initialize the global 2D scoring area weights matrix
int Heuristics::scoring_area_weights_small_[2][13][12] = {};
int Heuristics::scoring_area_weights_medium_[2][15][14] = {};
int Heuristics::scoring_area_weights_large_[2][17][16] = {};


// Static initialization flag
namespace {
    bool scoring_weights_initialized = false;
}

const Heuristics::Weights& Heuristics::get_weights() {
    return weights_;
}

void Heuristics::set_weights(const Heuristics::Weights& new_weights) {
    weights_ = new_weights;
}

// Initialize scoring weights based on zones for both players
void Heuristics::initialize_scoring_weights_small(int rows) {
    const int w1 = 100000;  // Pieces in scoring row
    const int w2 = 350;     // Zone 1: rows±0-1, cols 3-8
    const int w3 = 175;     // Zone 2: rows±(-1)-1, cols 2-9
    const int w4 = 80;      // Zone 3: rows±(-2)-2, cols 1-10
    const int w5 = 4;       // Zone 4: rows±(-2)-2, cols 0-11
    
    // Initialize for circle (player_type = 0)
    int target_row = top_score_row();
    int direction = -1;
    
    // w1: Scoring row (columns 4-7)
    for (int col = 4; col <= 7; col++) {
        scoring_area_weights_small_[0][target_row][col] = std::max(scoring_area_weights_small_[0][target_row][col], w1);
    }
    
    // w2: Zone 1 (rows±0-1, cols 3-8)
    for (int col = 3; col <= 8; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_small_[0][check_row][col] = std::max(scoring_area_weights_small_[0][check_row][col], w2);
            }
        }
    }
    
    // w3: Zone 2 (rows±(-1)-1, cols 2-9)
    for (int col = 2; col <= 9; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_small_[0][check_row][col] = std::max(scoring_area_weights_small_[0][check_row][col], w3);
            }
        }
    }
    
    // w4: Zone 3 (rows±(-2)-2, cols 1-10)
    for (int col = 1; col <= 10; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_small_[0][check_row][col] = std::max(scoring_area_weights_small_[0][check_row][col], w4);
            }
        }
    }
    
    // w5: Zone 4 (rows±(-2)-2, cols 0-11)
    for (int col = 0; col <= 11; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_small_[0][check_row][col] = std::max(scoring_area_weights_small_[0][check_row][col], w5);
            }
        }
    }
    
    // Initialize for square (player_type = 1)
    target_row = bottom_score_row(rows);
    direction = 1;
    
    // w1: Scoring row (columns 4-7)
    for (int col = 4; col <= 7; col++) {
        scoring_area_weights_small_[1][target_row][col] = std::max(scoring_area_weights_small_[1][target_row][col], w1);
    }
    
    // w2: Zone 1 (rows±0-1, cols 3-8)
    for (int col = 3; col <= 8; col++) {
        for (int r = 0; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_small_[1][check_row][col] = std::max(scoring_area_weights_small_[1][check_row][col], w2);
            }
        }
    }
    
    // w3: Zone 2 (rows±(-1)-1, cols 2-9)
    for (int col = 2; col <= 9; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_small_[1][check_row][col] = std::max(scoring_area_weights_small_[1][check_row][col], w3);
            }
        }
    }
    
    // w4: Zone 3 (rows±(-2)-2, cols 1-10)
    for (int col = 1; col <= 10; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_small_[1][check_row][col] = std::max(scoring_area_weights_small_[1][check_row][col], w4);
            }
        }
    }
    
    // w5: Zone 4 (rows±(-2)-2, cols 0-11)
    for (int col = 0; col <= 11; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_small_[1][check_row][col] = std::max(scoring_area_weights_small_[1][check_row][col], w5);
            }
        }
    }
}

void Heuristics::initialize_scoring_weights_medium(int rows) {
    const int w1 = 100000;  // Pieces in scoring row
    const int w2 = 350;     // Zone 1: rows±0-1, cols 3-8
    const int w3 = 175;     // Zone 2: rows±(-1)-1, cols 2-10
    const int w4 = 80;      // Zone 3: rows±(-2)-2, cols 1-11
    const int w5 = 4;       // Zone 4: rows±(-2)-2, cols 0-13
    
    // Initialize for circle (player_type = 0)
    int target_row = top_score_row();
    int direction = -1;
    
    // w1: Scoring row (columns 4-8)
    for (int col = 4; col <= 8; col++) {
        scoring_area_weights_medium_[0][target_row][col] = std::max(scoring_area_weights_medium_[0][target_row][col], w1);
    }
    
    // w2: Zone 1 (rows±0-1, cols 3-9)
    for (int col = 3; col <= 9; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_medium_[0][check_row][col] = std::max(scoring_area_weights_medium_[0][check_row][col], w2);
            }
        }
    }
    
    // w3: Zone 2 (rows±(-1)-1, cols 2-10)
    for (int col = 2; col <= 10; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_medium_[0][check_row][col] = std::max(scoring_area_weights_medium_[0][check_row][col], w3);
            }
        }
    }
    
    // w4: Zone 3 (rows±(-2)-2, cols 1-11)
    for (int col = 1; col <= 11; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_medium_[0][check_row][col] = std::max(scoring_area_weights_medium_[0][check_row][col], w4);
            }
        }
    }
    
    // w5: Zone 4 (rows±(-2)-2, cols 0-13)
    for (int col = 0; col <= 13; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_medium_[0][check_row][col] = std::max(scoring_area_weights_medium_[0][check_row][col], w5);
            }
        }
    }
    
    // Initialize for square (player_type = 1)
    target_row = bottom_score_row(rows);
    direction = 1;
    
    // w1: Scoring row (columns 4-7)
    for (int col = 4; col <= 8; col++) {
        scoring_area_weights_medium_[1][target_row][col] = std::max(scoring_area_weights_medium_[1][target_row][col], w1);
    }
    
    // w2: Zone 1 (rows±0-1, cols 3-8)
    for (int col = 3; col <= 9; col++) {
        for (int r = 0; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_medium_[1][check_row][col] = std::max(scoring_area_weights_medium_[1][check_row][col], w2);
            }
        }
    }
    
    // w3: Zone 2 (rows±(-1)-1, cols 2-9)
    for (int col = 2; col <= 10; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_medium_[1][check_row][col] = std::max(scoring_area_weights_medium_[1][check_row][col], w3);
            }
        }
    }
    
    // w4: Zone 3 (rows±(-2)-2, cols 1-10)
    for (int col = 1; col <= 11; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_medium_[1][check_row][col] = std::max(scoring_area_weights_medium_[1][check_row][col], w4);
            }
        }
    }
    
    // w5: Zone 4 (rows±(-2)-2, cols 0-13)
    for (int col = 0; col <= 13; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_medium_[1][check_row][col] = std::max(scoring_area_weights_medium_[1][check_row][col], w5);
            }
        }
    }
}

void Heuristics::initialize_scoring_weights_large(int rows) {
    const int w1 = 100000;  // Pieces in scoring row
    const int w2 = 350;     // Zone 1: rows±0-1, cols 3-8
    const int w3 = 175;     // Zone 2: rows±(-1)-1, cols 2-9
    const int w4 = 80;      // Zone 3: rows±(-2)-2, cols 2-11
    const int w5 = 4;       // Zone 4: rows±(-2)-2, cols 1-14   
    const int w6 = 2;       // Zone 5: rows±(-3)-3, cols 0-15
    
    // Initialize for circle (player_type = 0)
    int target_row = top_score_row();
    int direction = -1;
    
    // w1: Scoring row (columns 4-7)
    for (int col = 5; col <= 10; col++) {
        scoring_area_weights_large_[0][target_row][col] = std::max(scoring_area_weights_large_[0][target_row][col], w1);
    }
    
    // w2: Zone 1 (rows±0-1, cols 3-8)
    for (int col = 4; col <= 11; col++) {
        for (int r = 0; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[0][check_row][col] = std::max(scoring_area_weights_large_[0][check_row][col], w2);
            }
        }
    }
    
    // w3: Zone 2 (rows±(-1)-1, cols 2-9)
    for (int col = 3; col <= 12; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[0][check_row][col] = std::max(scoring_area_weights_large_[0][check_row][col], w3);
            }
        }
    }
    
    // w4: Zone 3 (rows±(-2)-2, cols 1-10)
    for (int col = 2; col <= 13; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[0][check_row][col] = std::max(scoring_area_weights_large_[0][check_row][col], w4);
            }
        }
    }
    
    // w5: Zone 4 (rows±(-2)-2, cols 0-11)
    for (int col = 1; col <= 14; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[0][check_row][col] = std::max(scoring_area_weights_large_[0][check_row][col], w5);
            }
        }
    }

    // w6: Zone 5 (rows±(-3)-3, cols 0-15)
    for (int col = 0; col <= 15; col++) {
        for (int r = -2; r <= 3; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[0][check_row][col] = std::max(scoring_area_weights_large_[0][check_row][col], w6);
            }
        }
    }

    // Initialize for square (player_type = 1)
    target_row = bottom_score_row(rows);
    direction = 1;
    
    // w1: Scoring row (columns 4-7)
    for (int col = 5; col <= 10; col++) {
        scoring_area_weights_large_[1][target_row][col] = std::max(scoring_area_weights_large_[1][target_row][col], w1);
    }
    
    // w2: Zone 1 (rows±0-1, cols 3-8)
    for (int col = 4; col <= 11; col++) {
        for (int r = 0; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[1][check_row][col] = std::max(scoring_area_weights_large_[1][check_row][col], w2);
            }
        }
    }
    
    // w3: Zone 2 (rows±(-1)-1, cols 3-12)
    for (int col = 3; col <= 12; col++) {
        for (int r = -1; r <= 1; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[1][check_row][col] = std::max(scoring_area_weights_large_[1][check_row][col], w3);
            }
        }
    }
    
    // w4: Zone 3 (rows±(-2)-2, cols 2-13)
    for (int col = 2; col <= 13; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[1][check_row][col] = std::max(scoring_area_weights_large_[1][check_row][col], w4);
            }
        }
    }
    
    // w5: Zone 4 (rows±(-2)-2, cols 1-14)
    for (int col = 1; col <= 14; col++) {
        for (int r = -2; r <= 2; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[1][check_row][col] = std::max(scoring_area_weights_large_[1][check_row][col], w5);
            }
        }
    }

    // w6: Zone 5 (rows±(-3)-3, cols 0-15)
    for (int col = 0; col <= 15; col++) {
        for (int r = -2; r <= 3; r++) {
            int check_row = target_row + direction * r;
            if (check_row >= 0 && check_row < rows) {
                scoring_area_weights_large_[1][check_row][col] = std::max(scoring_area_weights_large_[1][check_row][col], w6);
            }
        }
    }
}

void Heuristics::initialize_scoring_weights(int rows) {
    if (rows <= 13) {
        initialize_scoring_weights_small(rows);
    } else if (rows <= 15) {
        initialize_scoring_weights_medium(rows);
    } else {
        initialize_scoring_weights_large(rows);
    }
}

int Heuristics::max(int a, int b) {
    return (a > b) ? a : b;
}


double Heuristics::vertical_push_h(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info){
    int rows = state.rows;
    if(rows <=13){
        return Heuristics::vertical_push_h_small(state, player, wrt_self, use_parent, parent_info, last_move, my_info);
    }
    else if(rows <=15){
        return Heuristics::vertical_push_h_medium(state, player, wrt_self, use_parent, parent_info, last_move, my_info);
    }
    else {
        return Heuristics::vertical_push_h_large(state, player, wrt_self, use_parent, parent_info, last_move, my_info);
    }
}

double compute_vertical_push_for_column(const GameState& state, int col, const std::string& player, std::vector<double>& col_weight) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;

    std::string opponent = get_opponent(player);
    int push_direction = (player == "circle") ? -1 : 1;
    double column_score = 0.0;

    // Track which cells are reached to avoid double counting
    std::vector<bool> reached(rows, false);

    // Find all player's vertical rivers in this column
    for (int y = 0; y < rows; ++y) {
        EncodedCell cell = encoded_board[y][col];

        if (GameState::is_owner(cell, player) && GameState::is_vertical_river(cell)) {
            // Calculate reach in push direction
            for (int dist = 1; dist < rows; ++dist) {
                int ny = y + push_direction * dist;
                if (!in_bounds(col, ny, rows, state.cols)) break;

                EncodedCell next_cell = encoded_board[ny][col];

                if (GameState::is_empty(next_cell)) {
                    reached[ny] = true;
                    continue;
                }
                else if (GameState::is_owner(next_cell, opponent)) {
                    break;  // Blocked by opponent
                }
                else {
                    reached[ny] = true;  // Can reach through own pieces
                    continue;
                }
            }
        }
    }

    // Sum up the weights for all reached cells
    for (int y = 0; y < rows; ++y) {
        if (reached[y]) {
            column_score += col_weight[col];
        }
    }

    return column_score;
}

// Vertical push heuristic - calculates reach of vertical rivers
double Heuristics::vertical_push_h_small(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info) {

    std::vector<double> col_weight(12);

    // Column weights
    col_weight[0] = 1.5;
    col_weight[1] = 5;
    col_weight[2] = 4;
    col_weight[3] = 2.5;
    col_weight[4] = 2;
    col_weight[5] = 1.0;
    col_weight[6] = 1.0;
    col_weight[7] = 2;
    col_weight[8] = 2.5;
    col_weight[9] = 4;
    col_weight[10] = 5;
    col_weight[11] = 1.5;  

    if(use_parent && parent_info != nullptr && last_move != nullptr){
        if(my_info != nullptr && !my_info->v_push_circle_vals.empty() && !my_info->v_push_square_vals.empty()){
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_info->v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_info->v_push_square_vals){
                    score += val;
                }
            }
            // std::cout << "Already computed vertical push heuristic using my_info.\n";
            return wrt_self ? score : -score;
        }
        if(last_move->action == "move"){
            // std::cout << "Using parent_info to update vertical push heuristic for move action.\n";
            int from_col = last_move->from[0];
            int to_col = last_move->to[0];
            double circle_val_from = compute_vertical_push_for_column(state, from_col, "circle", col_weight);
            double square_val_from = compute_vertical_push_for_column(state, from_col, "square", col_weight);
            double circle_val_to = compute_vertical_push_for_column(state, to_col, "circle", col_weight);
            double square_val_to = compute_vertical_push_for_column(state, to_col, "square", col_weight);
            std::vector <double> my_v_push_circle_vals = parent_info->v_push_circle_vals;
            std::vector <double> my_v_push_square_vals = parent_info->v_push_square_vals;
            my_v_push_circle_vals[from_col] = circle_val_from;
            my_v_push_square_vals[from_col] = square_val_from;  
            my_v_push_circle_vals[to_col] = circle_val_to;
            my_v_push_square_vals[to_col] = square_val_to;
            my_info->v_push_circle_vals = my_v_push_circle_vals;
            my_info->v_push_square_vals = my_v_push_square_vals;
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_v_push_square_vals){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
        else if(last_move->action == "push"){
            // std::cout << "Using parent_info to update vertical push heuristic for push action.\n";
            int col1 = last_move->from[0];
            int col2 = last_move->to[0];
            int col3 = last_move->pushed_to[0];
            double circle_val_col1 = compute_vertical_push_for_column(state, col1, "circle", col_weight);
            double square_val_col1 = compute_vertical_push_for_column(state, col1, "square", col_weight);
            double circle_val_col2 = circle_val_col1;
            double square_val_col2 = square_val_col1;
            if(col2 != col1){
                circle_val_col2 = compute_vertical_push_for_column(state, col2, "circle", col_weight);
                square_val_col2 = compute_vertical_push_for_column(state, col2, "square", col_weight);
            }
            double circle_val_col3 = circle_val_col1;
            double square_val_col3 = square_val_col1;
            if(col3 != col1 && col3 != col2){
                circle_val_col3 = compute_vertical_push_for_column(state, col3, "circle", col_weight);
                square_val_col3 = compute_vertical_push_for_column(state, col3, "square", col_weight);
            }
            std::vector <double> my_v_push_circle_vals = parent_info->v_push_circle_vals;
            std::vector <double> my_v_push_square_vals = parent_info->v_push_square_vals;
            my_v_push_circle_vals[col1] = circle_val_col1;
            my_v_push_square_vals[col1] = square_val_col1;  
            my_v_push_circle_vals[col2] = circle_val_col2;
            my_v_push_square_vals[col2] = square_val_col2;  
            my_v_push_circle_vals[col3] = circle_val_col3;
            my_v_push_square_vals[col3] = square_val_col3;
            my_info->v_push_circle_vals = my_v_push_circle_vals;
            my_info->v_push_square_vals = my_v_push_square_vals;
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_v_push_square_vals){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
        else if(last_move->action == "flip" || last_move->action == "rotate"){
            // std::cout << "Using parent_info to update vertical push heuristic for flip/rotate action.\n";
            int col = last_move->from[0];
            double circle_val = compute_vertical_push_for_column(state, col, "circle", col_weight);
            double square_val = compute_vertical_push_for_column(state, col, "square", col_weight);
            std::vector <double> my_v_push_circle_vals = parent_info->v_push_circle_vals;
            std::vector <double> my_v_push_square_vals = parent_info->v_push_square_vals;
            my_v_push_circle_vals[col] = circle_val;
            my_v_push_square_vals[col] = square_val;  
            my_info->v_push_circle_vals = my_v_push_circle_vals;
            my_info->v_push_square_vals = my_v_push_square_vals;
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_v_push_square_vals){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
    }
    int cols = state.cols;
    double score = 0.0;
    for(int col = 0; col < cols; ++col){
        double col_val_circle = compute_vertical_push_for_column(state, col, "circle", col_weight);
        double col_val_square = compute_vertical_push_for_column(state, col, "square", col_weight);
        my_info->v_push_circle_vals.push_back(col_val_circle);
        my_info->v_push_square_vals.push_back(col_val_square);
        if(player == "circle"){
            score += col_val_circle;
        } else {
            score += col_val_square;
        }
    }

    return wrt_self ? score : -score;

    // const auto& encoded_board = state.encoded_board;
    // int rows = state.rows;
    // int cols = state.cols;

    // double score = 0;

    // std::string opponent = get_opponent(player);
    // int push_direction = (player == "circle") ? -1 : 1;
    // std::vector<std::vector<double>> reach(rows, std::vector<double>(cols, 0));

    // // Find all player's vertical rivers and calculate their reach
    // for (int y = 0; y < rows; ++y) {
    //     for (int x = 0; x < cols; ++x) {
    //         EncodedCell cell = encoded_board[y][x];

    //         if (GameState::is_owner(cell, player) && GameState::is_vertical_river(cell)) {
    //             // Calculate reach in push direction
    //             for (int dist = 1; dist < rows; ++dist) {
    //                 int ny = y + push_direction * dist;
    //                 if (!in_bounds(x, ny, rows, cols)) break;

    //                 EncodedCell next_cell = encoded_board[ny][x];

    //                 if (GameState::is_empty(next_cell)) {
    //                     reach[ny][x] = col_weight[x];
    //                     continue;
    //                 }
    //                 else if (GameState::is_owner(next_cell, opponent)) {
    //                     break;  // Blocked by opponent
    //                 }
    //                 else {
    //                     reach[ny][x] = col_weight[x];  // Can reach through own pieces
    //                     continue;
    //                 }
    //             }
    //         }
    //     }
    // }

    // // Sum all reach values
    // for (auto &e1 : reach) {
    //     for (auto &e2 : e1) {
    //         score += e2;
    //     }
    // }

    // return wrt_self ? score : -score;
}

double Heuristics::vertical_push_h_medium(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info) {

    std::vector<double> col_weight(14);

    // Column weights
    col_weight[0] = 1;
    col_weight[1] = 3.25;
    col_weight[2] = 4;
    col_weight[3] = 3.25;
    col_weight[4] = 2;
    col_weight[5] = 1.5;
    col_weight[6] = 1;
    col_weight[7] = 1.5;
    col_weight[8] = 2;
    col_weight[9] = 3.25;
    col_weight[10] = 4;
    col_weight[11] = 4.25;
    col_weight[12] = 4.0;
    col_weight[13] = 2.0;

    if(use_parent && parent_info != nullptr && last_move != nullptr){
        if(my_info != nullptr && !my_info->v_push_circle_vals.empty() && !my_info->v_push_square_vals.empty()){
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_info->v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_info->v_push_square_vals){
                    score += val;
                }
            }
            // std::cout << "Already computed vertical push heuristic using my_info.\n";
            return wrt_self ? score : -score;
        }
        if(last_move->action == "move"){
            // std::cout << "Using parent_info to update vertical push heuristic for move action.\n";
            int from_col = last_move->from[0];
            int to_col = last_move->to[0];
            double circle_val_from = compute_vertical_push_for_column(state, from_col, "circle", col_weight);
            double square_val_from = compute_vertical_push_for_column(state, from_col, "square", col_weight);
            double circle_val_to = compute_vertical_push_for_column(state, to_col, "circle", col_weight);
            double square_val_to = compute_vertical_push_for_column(state, to_col, "square", col_weight);
            std::vector <double> my_v_push_circle_vals = parent_info->v_push_circle_vals;
            std::vector <double> my_v_push_square_vals = parent_info->v_push_square_vals;
            my_v_push_circle_vals[from_col] = circle_val_from;
            my_v_push_square_vals[from_col] = square_val_from;  
            my_v_push_circle_vals[to_col] = circle_val_to;
            my_v_push_square_vals[to_col] = square_val_to;
            my_info->v_push_circle_vals = my_v_push_circle_vals;
            my_info->v_push_square_vals = my_v_push_square_vals;
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_v_push_square_vals){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
        else if(last_move->action == "push"){
            // std::cout << "Using parent_info to update vertical push heuristic for push action.\n";
            int col1 = last_move->from[0];
            int col2 = last_move->to[0];
            int col3 = last_move->pushed_to[0];
            double circle_val_col1 = compute_vertical_push_for_column(state, col1, "circle", col_weight);
            double square_val_col1 = compute_vertical_push_for_column(state, col1, "square", col_weight);
            double circle_val_col2 = circle_val_col1;
            double square_val_col2 = square_val_col1;
            if(col2 != col1){
                circle_val_col2 = compute_vertical_push_for_column(state, col2, "circle", col_weight);
                square_val_col2 = compute_vertical_push_for_column(state, col2, "square", col_weight);
            }
            double circle_val_col3 = circle_val_col1;
            double square_val_col3 = square_val_col1;
            if(col3 != col1 && col3 != col2){
                circle_val_col3 = compute_vertical_push_for_column(state, col3, "circle", col_weight);
                square_val_col3 = compute_vertical_push_for_column(state, col3, "square", col_weight);
            }
            std::vector <double> my_v_push_circle_vals = parent_info->v_push_circle_vals;
            std::vector <double> my_v_push_square_vals = parent_info->v_push_square_vals;
            my_v_push_circle_vals[col1] = circle_val_col1;
            my_v_push_square_vals[col1] = square_val_col1;  
            my_v_push_circle_vals[col2] = circle_val_col2;
            my_v_push_square_vals[col2] = square_val_col2;  
            my_v_push_circle_vals[col3] = circle_val_col3;
            my_v_push_square_vals[col3] = square_val_col3;
            my_info->v_push_circle_vals = my_v_push_circle_vals;
            my_info->v_push_square_vals = my_v_push_square_vals;
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_v_push_square_vals){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
        else if(last_move->action == "flip" || last_move->action == "rotate"){
            // std::cout << "Using parent_info to update vertical push heuristic for flip/rotate action.\n";
            int col = last_move->from[0];
            double circle_val = compute_vertical_push_for_column(state, col, "circle", col_weight);
            double square_val = compute_vertical_push_for_column(state, col, "square", col_weight);
            std::vector <double> my_v_push_circle_vals = parent_info->v_push_circle_vals;
            std::vector <double> my_v_push_square_vals = parent_info->v_push_square_vals;
            my_v_push_circle_vals[col] = circle_val;
            my_v_push_square_vals[col] = square_val;  
            my_info->v_push_circle_vals = my_v_push_circle_vals;
            my_info->v_push_square_vals = my_v_push_square_vals;
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_v_push_square_vals){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
    }
    int cols = state.cols;
    double score = 0.0;
    for(int col = 0; col < cols; ++col){
        double col_val_circle = compute_vertical_push_for_column(state, col, "circle", col_weight);
        double col_val_square = compute_vertical_push_for_column(state, col, "square", col_weight);
        my_info->v_push_circle_vals.push_back(col_val_circle);
        my_info->v_push_square_vals.push_back(col_val_square);
        if(player == "circle"){
            score += col_val_circle;
        } else {
            score += col_val_square;
        }
    }

    return wrt_self ? score : -score;

    // const auto& encoded_board = state.encoded_board;
    // int rows = state.rows;
    // int cols = state.cols;

    // double score = 0;

    // std::string opponent = get_opponent(player);
    // int push_direction = (player == "circle") ? -1 : 1;
    // std::vector<std::vector<double>> reach(rows, std::vector<double>(cols, 0));

    // // Find all player's vertical rivers and calculate their reach
    // for (int y = 0; y < rows; ++y) {
    //     for (int x = 0; x < cols; ++x) {
    //         EncodedCell cell = encoded_board[y][x];

    //         if (GameState::is_owner(cell, player) && GameState::is_vertical_river(cell)) {
    //             // Calculate reach in push direction
    //             for (int dist = 1; dist < rows; ++dist) {
    //                 int ny = y + push_direction * dist;
    //                 if (!in_bounds(x, ny, rows, cols)) break;

    //                 EncodedCell next_cell = encoded_board[ny][x];

    //                 if (GameState::is_empty(next_cell)) {
    //                     reach[ny][x] = col_weight[x];
    //                     continue;
    //                 }
    //                 else if (GameState::is_owner(next_cell, opponent)) {
    //                     break;  // Blocked by opponent
    //                 }
    //                 else {
    //                     reach[ny][x] = col_weight[x];  // Can reach through own pieces
    //                     continue;
    //                 }
    //             }
    //         }
    //     }
    // }

    // // Sum all reach values
    // for (auto &e1 : reach) {
    //     for (auto &e2 : e1) {
    //         score += e2;
    //     }
    // }

    // return wrt_self ? score : -score;
}

double Heuristics::vertical_push_h_large(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info) {

    std::vector<double> col_weight(16);

    // Column weights
    col_weight[0] = 1;
    col_weight[1] = 2;
    col_weight[2] = 4;
    col_weight[3] = 3.25;
    col_weight[4] = 3.25;
    col_weight[5] = 2;    
    col_weight[6] = 1.5;
    col_weight[7] = 1;
    col_weight[8] = 1;
    col_weight[9] = 1.5;
    col_weight[10] = 2;
    col_weight[11] = 3.25;
    col_weight[12] = 3.25;
    col_weight[13] = 4;
    col_weight[14] = 2;
    col_weight[15] = 1;


    if(use_parent && parent_info != nullptr && last_move != nullptr){
        if(my_info != nullptr && !my_info->v_push_circle_vals.empty() && !my_info->v_push_square_vals.empty()){
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_info->v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_info->v_push_square_vals){
                    score += val;
                }
            }
            // std::cout << "Already computed vertical push heuristic using my_info.\n";
            return wrt_self ? score : -score;
        }
        if(last_move->action == "move"){
            // std::cout << "Using parent_info to update vertical push heuristic for move action.\n";
            int from_col = last_move->from[0];
            int to_col = last_move->to[0];
            double circle_val_from = compute_vertical_push_for_column(state, from_col, "circle", col_weight);
            double square_val_from = compute_vertical_push_for_column(state, from_col, "square", col_weight);
            double circle_val_to = compute_vertical_push_for_column(state, to_col, "circle", col_weight);
            double square_val_to = compute_vertical_push_for_column(state, to_col, "square", col_weight);
            std::vector <double> my_v_push_circle_vals = parent_info->v_push_circle_vals;
            std::vector <double> my_v_push_square_vals = parent_info->v_push_square_vals;
            my_v_push_circle_vals[from_col] = circle_val_from;
            my_v_push_square_vals[from_col] = square_val_from;  
            my_v_push_circle_vals[to_col] = circle_val_to;
            my_v_push_square_vals[to_col] = square_val_to;
            my_info->v_push_circle_vals = my_v_push_circle_vals;
            my_info->v_push_square_vals = my_v_push_square_vals;
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_v_push_square_vals){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
        else if(last_move->action == "push"){
            // std::cout << "Using parent_info to update vertical push heuristic for push action.\n";
            int col1 = last_move->from[0];
            int col2 = last_move->to[0];
            int col3 = last_move->pushed_to[0];
            double circle_val_col1 = compute_vertical_push_for_column(state, col1, "circle", col_weight);
            double square_val_col1 = compute_vertical_push_for_column(state, col1, "square", col_weight);
            double circle_val_col2 = circle_val_col1;
            double square_val_col2 = square_val_col1;
            if(col2 != col1){
                circle_val_col2 = compute_vertical_push_for_column(state, col2, "circle", col_weight);
                square_val_col2 = compute_vertical_push_for_column(state, col2, "square", col_weight);
            }
            double circle_val_col3 = circle_val_col1;
            double square_val_col3 = square_val_col1;
            if(col3 != col1 && col3 != col2){
                circle_val_col3 = compute_vertical_push_for_column(state, col3, "circle", col_weight);
                square_val_col3 = compute_vertical_push_for_column(state, col3, "square", col_weight);
            }
            std::vector <double> my_v_push_circle_vals = parent_info->v_push_circle_vals;
            std::vector <double> my_v_push_square_vals = parent_info->v_push_square_vals;
            my_v_push_circle_vals[col1] = circle_val_col1;
            my_v_push_square_vals[col1] = square_val_col1;  
            my_v_push_circle_vals[col2] = circle_val_col2;
            my_v_push_square_vals[col2] = square_val_col2;  
            my_v_push_circle_vals[col3] = circle_val_col3;
            my_v_push_square_vals[col3] = square_val_col3;
            my_info->v_push_circle_vals = my_v_push_circle_vals;
            my_info->v_push_square_vals = my_v_push_square_vals;
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_v_push_square_vals){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
        else if(last_move->action == "flip" || last_move->action == "rotate"){
            // std::cout << "Using parent_info to update vertical push heuristic for flip/rotate action.\n";
            int col = last_move->from[0];
            double circle_val = compute_vertical_push_for_column(state, col, "circle", col_weight);
            double square_val = compute_vertical_push_for_column(state, col, "square", col_weight);
            std::vector <double> my_v_push_circle_vals = parent_info->v_push_circle_vals;
            std::vector <double> my_v_push_square_vals = parent_info->v_push_square_vals;
            my_v_push_circle_vals[col] = circle_val;
            my_v_push_square_vals[col] = square_val;  
            my_info->v_push_circle_vals = my_v_push_circle_vals;
            my_info->v_push_square_vals = my_v_push_square_vals;
            double score = 0.0;
            if(player=="circle"){
                for(auto val: my_v_push_circle_vals){
                    score += val;
                }
            } else {
                for(auto val: my_v_push_square_vals){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
    }
    int cols = state.cols;
    double score = 0.0;
    for(int col = 0; col < cols; ++col){
        double col_val_circle = compute_vertical_push_for_column(state, col, "circle", col_weight);
        double col_val_square = compute_vertical_push_for_column(state, col, "square", col_weight);
        my_info->v_push_circle_vals.push_back(col_val_circle);
        my_info->v_push_square_vals.push_back(col_val_square);
        if(player == "circle"){
            score += col_val_circle;
        } else {
            score += col_val_square;
        }
    }

    return wrt_self ? score : -score;

    // const auto& encoded_board = state.encoded_board;
    // int rows = state.rows;
    // int cols = state.cols;

    // double score = 0;

    // std::string opponent = get_opponent(player);
    // int push_direction = (player == "circle") ? -1 : 1;
    // std::vector<std::vector<double>> reach(rows, std::vector<double>(cols, 0));

    // // Find all player's vertical rivers and calculate their reach
    // for (int y = 0; y < rows; ++y) {
    //     for (int x = 0; x < cols; ++x) {
    //         EncodedCell cell = encoded_board[y][x];

    //         if (GameState::is_owner(cell, player) && GameState::is_vertical_river(cell)) {
    //             // Calculate reach in push direction
    //             for (int dist = 1; dist < rows; ++dist) {
    //                 int ny = y + push_direction * dist;
    //                 if (!in_bounds(x, ny, rows, cols)) break;

    //                 EncodedCell next_cell = encoded_board[ny][x];

    //                 if (GameState::is_empty(next_cell)) {
    //                     reach[ny][x] = col_weight[x];
    //                     continue;
    //                 }
    //                 else if (GameState::is_owner(next_cell, opponent)) {
    //                     break;  // Blocked by opponent
    //                 }
    //                 else {
    //                     reach[ny][x] = col_weight[x];  // Can reach through own pieces
    //                     continue;
    //                 }
    //             }
    //         }
    //     }
    // }

    // // Sum all reach values
    // for (auto &e1 : reach) {
    //     for (auto &e2 : e1) {
    //         score += e2;
    //     }
    // }

    // return wrt_self ? score : -score;
}



int Heuristics::pieces_in_scoring_virgin_cols(const GameState& state, const std::string& player, bool wrt_self){
    int rows = state.rows;
    if(rows == 13){
        return pieces_in_scoring_virgin_cols_small(state, player, wrt_self);
    } else if (rows == 15){
        return pieces_in_scoring_virgin_cols_medium(state, player, wrt_self);
    } else {
        return pieces_in_scoring_virgin_cols_large(state, player, wrt_self);
    }
}

// Split scoring heuristic - Part 1: Virgin columns proximity bonus (recalculated each time)
int Heuristics::pieces_in_scoring_virgin_cols_small(const GameState& state, const std::string& player, bool wrt_self) {
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

// Split scoring heuristic - Part 1: Virgin columns proximity bonus (recalculated each time)
int Heuristics::pieces_in_scoring_virgin_cols_medium(const GameState& state, const std::string& player, bool wrt_self) {
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
    for (int col = 4; col <= 8; col++) {
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
    for (int col = 0; col <= 12; col++) {
        for (int row = target_row - 2; row <= target_row + 2; row++) {
            if (row < 0 || row >= rows) continue;
            EncodedCell cell = encoded_board[row][col];
            if (GameState::is_empty(cell)) continue;
            if (!GameState::is_owner(cell, player_to_check)) continue;
            if (row == target_row && col >= 4 && col <= 8) continue;  // Skip scoring area itself

            for (int vc : virgin_cols) {
                int man_dist = abs(vc - col) + abs(target_row - row);
                if (man_dist == 1) score += 275 * in_score_area;
                else if (man_dist == 2) score += 150 * in_score_area;
                else if (man_dist == 3) score += 100 * in_score_area;
                else if (man_dist == 4) score += 50 * in_score_area;
            }
        }
    }

    return wrt_self ? score : -score;
}

// Split scoring heuristic - Part 1: Virgin columns proximity bonus (recalculated each time)
int Heuristics::pieces_in_scoring_virgin_cols_large(const GameState& state, const std::string& player, bool wrt_self) {
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
    for (int col = 5; col <= 10; col++) {
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
    for (int col = 1; col <= 14; col++) {
        for (int row = target_row - 3; row <= target_row + 3; row++) {
            if (row < 0 || row >= rows) continue;
            EncodedCell cell = encoded_board[row][col];
            if (GameState::is_empty(cell)) continue;
            if (!GameState::is_owner(cell, player_to_check)) continue;
            if (row == target_row && col >= 5 && col <= 10) continue;  // Skip scoring area itself

            for (int vc : virgin_cols) {
                int man_dist = abs(vc - col) + abs(target_row - row);
                if (man_dist == 1) score += 275 * in_score_area;
                else if (man_dist == 2) score += 150 * in_score_area;
                else if (man_dist == 3) score += 100 * in_score_area;
                else if (man_dist == 4) score += 50 * in_score_area;
            }
        }
    }

    return wrt_self ? score : -score;
}

// Split scoring heuristic - Part 2: Zonewise weighted pieces (can be incrementally updated)
int Heuristics::pieces_in_scoring_zonewise(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info) {
    // Dispatch to appropriate size-specific function
    int rows = state.rows;
    
    if (rows <= 13) {
        return pieces_in_scoring_zonewise_small(state, player, wrt_self, use_parent, parent_info, last_move, my_info);
    } else if (rows <= 15) {
        return pieces_in_scoring_zonewise_medium(state, player, wrt_self, use_parent, parent_info, last_move, my_info);
    } else {
        return pieces_in_scoring_zonewise_large(state, player, wrt_self, use_parent, parent_info, last_move, my_info);
    }
}

// Split scoring heuristic - Part 2: Zonewise weighted pieces - SMALL BOARD
int Heuristics::pieces_in_scoring_zonewise_small(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info) {
    // Initialize weights matrix on first call
    if (!scoring_weights_initialized) {
        initialize_scoring_weights_small(state.rows);
        scoring_weights_initialized = true;
    }

    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;

    std::string player_to_check = player;
    if (!wrt_self) {
        player_to_check = get_opponent(player);
    }

    // Determine which player's weight matrix to use
    int player_type = (player_to_check == "circle") ? 0 : 1;

    // Try to use incremental update if parent info is available
    if (use_parent && parent_info != nullptr && last_move != nullptr) {
        // Check if we already computed this value
        if (my_info != nullptr) {
            int cached_val = (player_to_check == "circle") ? my_info->pieces_in_scoring_zonewise_circle : my_info->pieces_in_scoring_zonewise_square;
            if (cached_val != -1) {
                return wrt_self ? cached_val : -cached_val;
            }
        }

        // Get parent's cached value
        int parent_circle_val = parent_info->pieces_in_scoring_zonewise_circle;
        int parent_square_val = parent_info->pieces_in_scoring_zonewise_square;

        // If parent has valid cached values, do incremental update
        if (parent_circle_val != -1 && parent_square_val != -1) {
            int score_circle = parent_circle_val;
            int score_square = parent_square_val;

            if (last_move->action == "move") {
                // Remove weight from old position, add weight at new position
                int from_x = last_move->from[0];
                int from_y = last_move->from[1];
                int to_x = last_move->to[0];
                int to_y = last_move->to[1];

                EncodedCell from_cell = encoded_board[from_y][from_x]; // This is now empty
                EncodedCell to_cell = encoded_board[to_y][to_x]; // This has the moved piece

                // Safety check: to_cell should not be empty
                if (!GameState::is_empty(to_cell)) {
                    // Determine which player moved
                    std::string moved_player = GameState::is_owner(to_cell, "circle") ? "circle" : "square";
                    int moved_player_type = (moved_player == "circle") ? 0 : 1;

                    // Subtract old weight and add new weight
                    int old_weight = scoring_area_weights_small_[moved_player_type][from_y][from_x];
                    int new_weight = scoring_area_weights_small_[moved_player_type][to_y][to_x];

                    if (moved_player == "circle") {
                        score_circle = score_circle - old_weight + new_weight;
                    } else {
                        score_square = score_square - old_weight + new_weight;
                    }

                    // Store computed values
                    if (my_info != nullptr) {
                        my_info->pieces_in_scoring_zonewise_circle = score_circle;
                        my_info->pieces_in_scoring_zonewise_square = score_square;
                    }

                    // Return appropriate score
                    int score = (player_to_check == "circle") ? score_circle : score_square;
                    return wrt_self ? score : -score;
                }
            }
            else if (last_move->action == "push") {
                // Update weights for all affected positions
                int from_x = last_move->from[0];
                int from_y = last_move->from[1];
                int to_x = last_move->to[0];
                int to_y = last_move->to[1];
                int pushed_to_x = last_move->pushed_to[0];
                int pushed_to_y = last_move->pushed_to[1];

                // Get cells at current positions
                EncodedCell to_cell = encoded_board[to_y][to_x]; // Pusher piece
                EncodedCell pushed_cell = encoded_board[pushed_to_y][pushed_to_x]; // Pushed piece

                // Safety check: ensure cells are not empty
                if (GameState::is_empty(to_cell) || GameState::is_empty(pushed_cell)) {
                    // Something is wrong, fall back to full computation
                    // This shouldn't happen but better safe than sorry
                } else {
                    // Determine players
                    std::string pusher_player = GameState::is_owner(to_cell, "circle") ? "circle" : "square";
                    std::string pushed_player = GameState::is_owner(pushed_cell, "circle") ? "circle" : "square";

                    int pusher_type = (pusher_player == "circle") ? 0 : 1;
                    int pushed_type = (pushed_player == "circle") ? 0 : 1;

                    // Update pusher position: from -> to
                    int pusher_old_weight = scoring_area_weights_small_[pusher_type][from_y][from_x];
                    int pusher_new_weight = scoring_area_weights_small_[pusher_type][to_y][to_x];

                    // Update pushed piece position: to -> pushed_to
                    int pushed_old_weight = scoring_area_weights_small_[pushed_type][to_y][to_x];
                    int pushed_new_weight = scoring_area_weights_small_[pushed_type][pushed_to_y][pushed_to_x];

                    if (pusher_player == "circle") {
                        score_circle = score_circle - pusher_old_weight + pusher_new_weight;
                    } else {
                        score_square = score_square - pusher_old_weight + pusher_new_weight;
                    }

                    if (pushed_player == "circle") {
                        score_circle = score_circle - pushed_old_weight + pushed_new_weight;
                    } else {
                        score_square = score_square - pushed_old_weight + pushed_new_weight;
                    }

                    // Store computed values
                    if (my_info != nullptr) {
                        my_info->pieces_in_scoring_zonewise_circle = score_circle;
                        my_info->pieces_in_scoring_zonewise_square = score_square;
                    }

                    // Return appropriate score
                    int score = (player_to_check == "circle") ? score_circle : score_square;
                    return wrt_self ? score : -score;
                }
            }
            else if (last_move->action == "flip" || last_move->action == "rotate") {
                // Flip/rotate doesn't change position, so weights remain the same
                // Just copy parent values (already done above)
            }

            // Store computed values
            if (my_info != nullptr) {
                my_info->pieces_in_scoring_zonewise_circle = score_circle;
                my_info->pieces_in_scoring_zonewise_square = score_square;
            }

            // Return appropriate score
            int score = (player_to_check == "circle") ? score_circle : score_square;
            return wrt_self ? score : -score;
        }
    }

    // Full computation (no incremental update available)
    int score_circle = 0;
    int score_square = 0;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < state.cols; col++) {
            EncodedCell cell = encoded_board[row][col];
            if (GameState::is_empty(cell)) continue;
            
            if (GameState::is_owner(cell, "circle")) {
                score_circle += scoring_area_weights_small_[0][row][col];
            } else if (GameState::is_owner(cell, "square")) {
                score_square += scoring_area_weights_small_[1][row][col];
            }
        }
    }

    // Store computed values for future incremental updates
    if (my_info != nullptr) {
        my_info->pieces_in_scoring_zonewise_circle = score_circle;
        my_info->pieces_in_scoring_zonewise_square = score_square;
    }

    int score = (player_to_check == "circle") ? score_circle : score_square;
    return wrt_self ? score : -score;
}

// Split scoring heuristic - Part 2: Zonewise weighted pieces - MEDIUM BOARD
int Heuristics::pieces_in_scoring_zonewise_medium(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info) {
    // Initialize weights matrix on first call
    if (!scoring_weights_initialized) {
        initialize_scoring_weights_medium(state.rows);
        scoring_weights_initialized = true;
    }

    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;

    std::string player_to_check = player;
    if (!wrt_self) {
        player_to_check = get_opponent(player);
    }

    // Determine which player's weight matrix to use
    int player_type = (player_to_check == "circle") ? 0 : 1;

    // Try to use incremental update if parent info is available
    if (use_parent && parent_info != nullptr && last_move != nullptr) {
        // Check if we already computed this value
        if (my_info != nullptr) {
            int cached_val = (player_to_check == "circle") ? my_info->pieces_in_scoring_zonewise_circle : my_info->pieces_in_scoring_zonewise_square;
            if (cached_val != -1) {
                return wrt_self ? cached_val : -cached_val;
            }
        }

        // Get parent's cached value
        int parent_circle_val = parent_info->pieces_in_scoring_zonewise_circle;
        int parent_square_val = parent_info->pieces_in_scoring_zonewise_square;

        // If parent has valid cached values, do incremental update
        if (parent_circle_val != -1 && parent_square_val != -1) {
            int score_circle = parent_circle_val;
            int score_square = parent_square_val;

            if (last_move->action == "move") {
                int from_x = last_move->from[0];
                int from_y = last_move->from[1];
                int to_x = last_move->to[0];
                int to_y = last_move->to[1];

                EncodedCell to_cell = encoded_board[to_y][to_x];

                if (!GameState::is_empty(to_cell)) {
                    std::string moved_player = GameState::is_owner(to_cell, "circle") ? "circle" : "square";
                    int moved_player_type = (moved_player == "circle") ? 0 : 1;

                    int old_weight = scoring_area_weights_medium_[moved_player_type][from_y][from_x];
                    int new_weight = scoring_area_weights_medium_[moved_player_type][to_y][to_x];

                    if (moved_player == "circle") {
                        score_circle = score_circle - old_weight + new_weight;
                    } else {
                        score_square = score_square - old_weight + new_weight;
                    }

                    if (my_info != nullptr) {
                        my_info->pieces_in_scoring_zonewise_circle = score_circle;
                        my_info->pieces_in_scoring_zonewise_square = score_square;
                    }

                    int score = (player_to_check == "circle") ? score_circle : score_square;
                    return wrt_self ? score : -score;
                }
            }
            else if (last_move->action == "push") {
                int from_x = last_move->from[0];
                int from_y = last_move->from[1];
                int to_x = last_move->to[0];
                int to_y = last_move->to[1];
                int pushed_to_x = last_move->pushed_to[0];
                int pushed_to_y = last_move->pushed_to[1];

                EncodedCell to_cell = encoded_board[to_y][to_x];
                EncodedCell pushed_cell = encoded_board[pushed_to_y][pushed_to_x];

                if (!GameState::is_empty(to_cell) && !GameState::is_empty(pushed_cell)) {
                    std::string pusher_player = GameState::is_owner(to_cell, "circle") ? "circle" : "square";
                    std::string pushed_player = GameState::is_owner(pushed_cell, "circle") ? "circle" : "square";

                    int pusher_type = (pusher_player == "circle") ? 0 : 1;
                    int pushed_type = (pushed_player == "circle") ? 0 : 1;

                    int pusher_old_weight = scoring_area_weights_medium_[pusher_type][from_y][from_x];
                    int pusher_new_weight = scoring_area_weights_medium_[pusher_type][to_y][to_x];
                    int pushed_old_weight = scoring_area_weights_medium_[pushed_type][to_y][to_x];
                    int pushed_new_weight = scoring_area_weights_medium_[pushed_type][pushed_to_y][pushed_to_x];

                    if (pusher_player == "circle") {
                        score_circle = score_circle - pusher_old_weight + pusher_new_weight;
                    } else {
                        score_square = score_square - pusher_old_weight + pusher_new_weight;
                    }

                    if (pushed_player == "circle") {
                        score_circle = score_circle - pushed_old_weight + pushed_new_weight;
                    } else {
                        score_square = score_square - pushed_old_weight + pushed_new_weight;
                    }

                    if (my_info != nullptr) {
                        my_info->pieces_in_scoring_zonewise_circle = score_circle;
                        my_info->pieces_in_scoring_zonewise_square = score_square;
                    }

                    int score = (player_to_check == "circle") ? score_circle : score_square;
                    return wrt_self ? score : -score;
                }
            }

            if (my_info != nullptr) {
                my_info->pieces_in_scoring_zonewise_circle = score_circle;
                my_info->pieces_in_scoring_zonewise_square = score_square;
            }

            int score = (player_to_check == "circle") ? score_circle : score_square;
            return wrt_self ? score : -score;
        }
    }

    // Full computation
    int score_circle = 0;
    int score_square = 0;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < state.cols; col++) {
            EncodedCell cell = encoded_board[row][col];
            if (GameState::is_empty(cell)) continue;
            
            if (GameState::is_owner(cell, "circle")) {
                score_circle += scoring_area_weights_medium_[0][row][col];
            } else if (GameState::is_owner(cell, "square")) {
                score_square += scoring_area_weights_medium_[1][row][col];
            }
        }
    }

    if (my_info != nullptr) {
        my_info->pieces_in_scoring_zonewise_circle = score_circle;
        my_info->pieces_in_scoring_zonewise_square = score_square;
    }

    int score = (player_to_check == "circle") ? score_circle : score_square;
    return wrt_self ? score : -score;
}

// Split scoring heuristic - Part 2: Zonewise weighted pieces - LARGE BOARD
int Heuristics::pieces_in_scoring_zonewise_large(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info) {
    // Initialize weights matrix on first call
    if (!scoring_weights_initialized) {
        initialize_scoring_weights_large(state.rows);
        scoring_weights_initialized = true;
    }

    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;

    std::string player_to_check = player;
    if (!wrt_self) {
        player_to_check = get_opponent(player);
    }

    // Determine which player's weight matrix to use
    int player_type = (player_to_check == "circle") ? 0 : 1;

    // Try to use incremental update if parent info is available
    if (use_parent && parent_info != nullptr && last_move != nullptr) {
        // Check if we already computed this value
        if (my_info != nullptr) {
            int cached_val = (player_to_check == "circle") ? my_info->pieces_in_scoring_zonewise_circle : my_info->pieces_in_scoring_zonewise_square;
            if (cached_val != -1) {
                return wrt_self ? cached_val : -cached_val;
            }
        }

        // Get parent's cached value
        int parent_circle_val = parent_info->pieces_in_scoring_zonewise_circle;
        int parent_square_val = parent_info->pieces_in_scoring_zonewise_square;

        // If parent has valid cached values, do incremental update
        if (parent_circle_val != -1 && parent_square_val != -1) {
            int score_circle = parent_circle_val;
            int score_square = parent_square_val;

            if (last_move->action == "move") {
                int from_x = last_move->from[0];
                int from_y = last_move->from[1];
                int to_x = last_move->to[0];
                int to_y = last_move->to[1];

                EncodedCell to_cell = encoded_board[to_y][to_x];

                if (!GameState::is_empty(to_cell)) {
                    std::string moved_player = GameState::is_owner(to_cell, "circle") ? "circle" : "square";
                    int moved_player_type = (moved_player == "circle") ? 0 : 1;

                    int old_weight = scoring_area_weights_large_[moved_player_type][from_y][from_x];
                    int new_weight = scoring_area_weights_large_[moved_player_type][to_y][to_x];

                    if (moved_player == "circle") {
                        score_circle = score_circle - old_weight + new_weight;
                    } else {
                        score_square = score_square - old_weight + new_weight;
                    }

                    if (my_info != nullptr) {
                        my_info->pieces_in_scoring_zonewise_circle = score_circle;
                        my_info->pieces_in_scoring_zonewise_square = score_square;
                    }

                    int score = (player_to_check == "circle") ? score_circle : score_square;
                    return wrt_self ? score : -score;
                }
            }
            else if (last_move->action == "push") {
                int from_x = last_move->from[0];
                int from_y = last_move->from[1];
                int to_x = last_move->to[0];
                int to_y = last_move->to[1];
                int pushed_to_x = last_move->pushed_to[0];
                int pushed_to_y = last_move->pushed_to[1];

                EncodedCell to_cell = encoded_board[to_y][to_x];
                EncodedCell pushed_cell = encoded_board[pushed_to_y][pushed_to_x];

                if (!GameState::is_empty(to_cell) && !GameState::is_empty(pushed_cell)) {
                    std::string pusher_player = GameState::is_owner(to_cell, "circle") ? "circle" : "square";
                    std::string pushed_player = GameState::is_owner(pushed_cell, "circle") ? "circle" : "square";

                    int pusher_type = (pusher_player == "circle") ? 0 : 1;
                    int pushed_type = (pushed_player == "circle") ? 0 : 1;

                    int pusher_old_weight = scoring_area_weights_large_[pusher_type][from_y][from_x];
                    int pusher_new_weight = scoring_area_weights_large_[pusher_type][to_y][to_x];
                    int pushed_old_weight = scoring_area_weights_large_[pushed_type][to_y][to_x];
                    int pushed_new_weight = scoring_area_weights_large_[pushed_type][pushed_to_y][pushed_to_x];

                    if (pusher_player == "circle") {
                        score_circle = score_circle - pusher_old_weight + pusher_new_weight;
                    } else {
                        score_square = score_square - pusher_old_weight + pusher_new_weight;
                    }

                    if (pushed_player == "circle") {
                        score_circle = score_circle - pushed_old_weight + pushed_new_weight;
                    } else {
                        score_square = score_square - pushed_old_weight + pushed_new_weight;
                    }

                    if (my_info != nullptr) {
                        my_info->pieces_in_scoring_zonewise_circle = score_circle;
                        my_info->pieces_in_scoring_zonewise_square = score_square;
                    }

                    int score = (player_to_check == "circle") ? score_circle : score_square;
                    return wrt_self ? score : -score;
                }
            }

            if (my_info != nullptr) {
                my_info->pieces_in_scoring_zonewise_circle = score_circle;
                my_info->pieces_in_scoring_zonewise_square = score_square;
            }

            int score = (player_to_check == "circle") ? score_circle : score_square;
            return wrt_self ? score : -score;
        }
    }

    // Full computation
    int score_circle = 0;
    int score_square = 0;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < state.cols; col++) {
            EncodedCell cell = encoded_board[row][col];
            if (GameState::is_empty(cell)) continue;
            
            if (GameState::is_owner(cell, "circle")) {
                score_circle += scoring_area_weights_large_[0][row][col];
            } else if (GameState::is_owner(cell, "square")) {
                score_square += scoring_area_weights_large_[1][row][col];
            }
        }
    }

    if (my_info != nullptr) {
        my_info->pieces_in_scoring_zonewise_circle = score_circle;
        my_info->pieces_in_scoring_zonewise_square = score_square;
    }

    int score = (player_to_check == "circle") ? score_circle : score_square;
    return wrt_self ? score : -score;
}

// Helper function to compute blocking count for a single column
int compute_blocking_for_column(const GameState& state, int col, const std::string& player) {
    const auto& encoded_board = state.encoded_board;
    int rows = state.rows;
    
    int block_count = 0;
    std::string opponent = get_opponent(player);
    int flow_dy = (opponent == "square") ? 1 : -1;

    for (int y = 0; y < rows; ++y) {
        EncodedCell cell = encoded_board[y][col];
        if (GameState::is_owner(cell, opponent) && GameState::is_vertical_river(cell)) {
            int ny = y + flow_dy;
            while (in_bounds(col, ny, rows, state.cols)) {
                EncodedCell blocking_cell = encoded_board[ny][col];
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
    return block_count;
}

// Pieces blocking opponent's vertical rivers
int Heuristics::pieces_blocking_vertical_h(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info) {
    
    if(use_parent && parent_info != nullptr && last_move != nullptr){
        if(my_info != nullptr && !my_info->pieces_blocking_v_circle.empty() && !my_info->pieces_blocking_v_square.empty()){
            int score = 0;
            if(player == "circle"){
                for(auto val: my_info->pieces_blocking_v_circle){
                    score += val;
                }
            } else {
                for(auto val: my_info->pieces_blocking_v_square){
                    score += val;
                }
            }
            return wrt_self ? score : -score;
        }
        
        if(last_move->action == "move"){
            int from_col = last_move->from[0];
            int to_col = last_move->to[0];
            
            // Safety check: ensure parent vectors are properly sized
            if (parent_info->pieces_blocking_v_circle.empty() || parent_info->pieces_blocking_v_square.empty()) {
                // Fall through to full computation
            } else {
                int circle_val_from = compute_blocking_for_column(state, from_col, "circle");
                int square_val_from = compute_blocking_for_column(state, from_col, "square");
                int circle_val_to = compute_blocking_for_column(state, to_col, "circle");
                int square_val_to = compute_blocking_for_column(state, to_col, "square");
                std::vector<int> my_blocking_circle_vals = parent_info->pieces_blocking_v_circle;
                std::vector<int> my_blocking_square_vals = parent_info->pieces_blocking_v_square;
                my_blocking_circle_vals[from_col] = circle_val_from;
                my_blocking_square_vals[from_col] = square_val_from;  
                my_blocking_circle_vals[to_col] = circle_val_to;
                my_blocking_square_vals[to_col] = square_val_to;
                my_info->pieces_blocking_v_circle = my_blocking_circle_vals;
                my_info->pieces_blocking_v_square = my_blocking_square_vals;
                int score = 0;
                if(player == "circle"){
                    for(auto val: my_blocking_circle_vals){
                        score += val;
                    }
                } else {
                    for(auto val: my_blocking_square_vals){
                        score += val;
                    }
                }
                return wrt_self ? score : -score;
            }
        }
        else if(last_move->action == "push"){
            // Safety check: ensure parent vectors are properly sized
            if (parent_info->pieces_blocking_v_circle.empty() || parent_info->pieces_blocking_v_square.empty()) {
                // Fall through to full computation
            } else {
                int col1 = last_move->from[0];
                int col2 = last_move->to[0];
                int col3 = last_move->pushed_to[0];
                int circle_val_col1 = compute_blocking_for_column(state, col1, "circle");
                int square_val_col1 = compute_blocking_for_column(state, col1, "square");
                int circle_val_col2 = circle_val_col1;
                int square_val_col2 = square_val_col1;
                if(col2 != col1){
                    circle_val_col2 = compute_blocking_for_column(state, col2, "circle");
                    square_val_col2 = compute_blocking_for_column(state, col2, "square");
                }
                int circle_val_col3 = circle_val_col1;
                int square_val_col3 = square_val_col1;
                if(col3 != col1 && col3 != col2){
                    circle_val_col3 = compute_blocking_for_column(state, col3, "circle");
                    square_val_col3 = compute_blocking_for_column(state, col3, "square");
                }
                std::vector<int> my_blocking_circle_vals = parent_info->pieces_blocking_v_circle;
                std::vector<int> my_blocking_square_vals = parent_info->pieces_blocking_v_square;
                my_blocking_circle_vals[col1] = circle_val_col1;
                my_blocking_square_vals[col1] = square_val_col1;  
                my_blocking_circle_vals[col2] = circle_val_col2;
                my_blocking_square_vals[col2] = square_val_col2;  
                my_blocking_circle_vals[col3] = circle_val_col3;
                my_blocking_square_vals[col3] = square_val_col3;
                my_info->pieces_blocking_v_circle = my_blocking_circle_vals;
                my_info->pieces_blocking_v_square = my_blocking_square_vals;
                int score = 0;
                if(player == "circle"){
                    for(auto val: my_blocking_circle_vals){
                        score += val;
                    }
                } else {
                    for(auto val: my_blocking_square_vals){
                        score += val;
                    }
                }
                return wrt_self ? score : -score;
            }
        }
        else if(last_move->action == "flip" || last_move->action == "rotate"){
            // Safety check: ensure parent vectors are properly sized
            if (parent_info->pieces_blocking_v_circle.empty() || parent_info->pieces_blocking_v_square.empty()) {
                // Fall through to full computation
            } else {
                int col = last_move->from[0];
                int circle_val = compute_blocking_for_column(state, col, "circle");
                int square_val = compute_blocking_for_column(state, col, "square");
                std::vector<int> my_blocking_circle_vals = parent_info->pieces_blocking_v_circle;
                std::vector<int> my_blocking_square_vals = parent_info->pieces_blocking_v_square;
                my_blocking_circle_vals[col] = circle_val;
                my_blocking_square_vals[col] = square_val;  
                my_info->pieces_blocking_v_circle = my_blocking_circle_vals;
                my_info->pieces_blocking_v_square = my_blocking_square_vals;
                int score = 0;
                if(player == "circle"){
                    for(auto val: my_blocking_circle_vals){
                        score += val;
                    }
                } else {
                    for(auto val: my_blocking_square_vals){
                        score += val;
                    }
                }
                return wrt_self ? score : -score;
            }
        }
    }

    int cols = state.cols;
    int score = 0;
    for(int col = 0; col < cols; ++col){
        int col_val_circle = compute_blocking_for_column(state, col, "circle");
        int col_val_square = compute_blocking_for_column(state, col, "square");
        my_info->pieces_blocking_v_circle.push_back(col_val_circle);
        my_info->pieces_blocking_v_square.push_back(col_val_square);
        if(player == "circle"){
            score += col_val_circle;
        } else {
            score += col_val_square;
        }
    }

    return wrt_self ? score : -score;
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
        // check_rows.push_back(bottom_score_row(rows) - 2);
    } else {
        check_rows.push_back(top_score_row() + 1);
        // check_rows.push_back(top_score_row() + 2);
    }

    for (int r : check_rows) {
        if (in_bounds(0, r, rows, cols)) {
            for (int x = 0; x < cols; ++x) {
                EncodedCell cell = encoded_board[r][x];
                if (GameState::is_owner(cell, player) && GameState::is_horizontal_river(cell)) {
                    if(state.rows == 13 && x >= 5 && x <= 6){
                        count++;
                    }
                    if(state.rows == 15 && x >= 5 && x <= 7){
                        count++;
                    }
                    if(state.rows == 17 && x >= 6 && x <= 9){
                        count++;
                    }
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

    int left_limit = 4;
    int right_limit = 7;

    if(state.rows == 15){
        left_limit = 4;
        right_limit = 8;
    } else if(state.rows == 17){
        left_limit = 5;
        right_limit = 10;
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
                                if (curr_col > right_limit) break;
                                num_possible += mul;
                                curr_col++;
                            } else {
                                break;
                            }
                        }
                        count += num_possible;
                    }
                    else if (x > right_limit) {
                        int num_possible = 0;
                        int curr_col = x - 1;
                        while (in_bounds(curr_col, r, rows, cols)) {
                            EncodedCell check_cell = encoded_board[r][curr_col];
                            if (GameState::is_empty(check_cell) ||
                                GameState::is_horizontal_river(check_cell) ||
                                GameState::is_owner(check_cell, player)) {
                                if (curr_col < left_limit) break;
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
                                if (curr_col < left_limit || curr_col > right_limit) break;
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
                                if (curr_col < left_limit || curr_col > right_limit) break;
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

// Helper function to check if a position is on the board edge
bool is_on_edge(int x, int y, int rows, int cols, const std::string& player) {
    if(player[0] == 'c'){
        return (x == 0 || x == cols - 1 || y == rows-1);
    } else {
        return (x == 0 || x == cols - 1 || y == 0);
    }
}

// Helper function to compute inactive pieces count for a player
int compute_inactive_count(const GameState& state, const std::string& player) {
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
        if (GameState::is_owner(encoded_board[0][x], player) && GameState::is_square(encoded_board[0][x])) {
            inactive_count++;
        }
        if (GameState::is_owner(encoded_board[rows-1][x], player) && !GameState::is_square(encoded_board[rows-1][x])) {
            inactive_count++;
        }
    }

    return inactive_count;
}

// Inactive pieces: pieces on board edges
int Heuristics::inactive_pieces(const GameState& state, const std::string& player, bool wrt_self, bool use_parent, HeuristicsInfo* parent_info, Move* last_move, HeuristicsInfo* my_info) {
    
    if(use_parent && parent_info != nullptr && last_move != nullptr){
        // Check if we already computed this
        if(my_info != nullptr && my_info->inactive_circle_count != -1 && my_info->inactive_square_count != -1){
            int count = (player == "circle") ? my_info->inactive_circle_count : my_info->inactive_square_count;
            return wrt_self ? -count : +count;
        }
        
        // Start with parent's counts
        int my_inactive_circle = parent_info->inactive_circle_count;
        int my_inactive_square = parent_info->inactive_square_count;
        
        const auto& encoded_board = state.encoded_board;
        int rows = state.rows;
        int cols = state.cols;
        
        if(last_move->action == "move"){
            int from_x = last_move->from[0];
            int from_y = last_move->from[1];
            int to_x = last_move->to[0];
            int to_y = last_move->to[1];
            
            bool from_edge = is_on_edge(from_x, from_y, rows, cols, player);
            bool to_edge = is_on_edge(to_x, to_y, rows, cols, player);
            
            // Determine which player moved
            EncodedCell moved_cell = encoded_board[to_y][to_x];
            bool is_circle = GameState::is_circle(moved_cell);
            
            // Update count based on move
            if(from_edge && !to_edge) {
                // Moved away from edge
                if(is_circle) my_inactive_circle--;
                else my_inactive_square--;
            } else if(!from_edge && to_edge) {
                // Moved to edge
                if(is_circle) my_inactive_circle++;
                else my_inactive_square++;
            }
            // If from_edge && to_edge or !from_edge && !to_edge, count doesn't change
            
        } else if(last_move->action == "push"){
            int from_x = last_move->from[0];
            int from_y = last_move->from[1];
            int to_x = last_move->to[0];
            int to_y = last_move->to[1];
            int pushed_x = last_move->pushed_to[0];
            int pushed_y = last_move->pushed_to[1];
            
            const auto& encoded_board = state.encoded_board;
            
            bool from_edge = is_on_edge(from_x, from_y, rows, cols, player);
            bool to_edge = is_on_edge(to_x, to_y, rows, cols, player);
            bool pushed_edge = is_on_edge(pushed_x, pushed_y, rows, cols, player);
            
            // Piece at 'to' position (pusher)
            EncodedCell pusher = encoded_board[to_y][to_x];
            bool pusher_is_circle = GameState::is_circle(pusher);
            
            // Piece at 'pushed_to' position (pushed)
            EncodedCell pushed = encoded_board[pushed_y][pushed_x];
            bool pushed_is_circle = GameState::is_circle(pushed);
            
            // Update for pusher moving from -> to
            if(from_edge && !to_edge) {
                if(pusher_is_circle) my_inactive_circle--;
                else my_inactive_square--;
            } else if(!from_edge && to_edge) {
                if(pusher_is_circle) my_inactive_circle++;
                else my_inactive_square++;
            }
            
            // Update for pushed piece moving to -> pushed_to
            if(to_edge && !pushed_edge) {
                if(pushed_is_circle) my_inactive_circle--;
                else my_inactive_square--;
            } else if(!to_edge && pushed_edge) {
                if(pushed_is_circle) my_inactive_circle++;
                else my_inactive_square++;
            }
        }
        // For flip and rotate, positions don't change, so no update needed
        
        my_info->inactive_circle_count = my_inactive_circle;
        my_info->inactive_square_count = my_inactive_square;
        
        int count = (player == "circle") ? my_inactive_circle : my_inactive_square;
        return wrt_self ? -count : +count;
    }
    
    // Full computation when not using parent
    int circle_count = compute_inactive_count(state, "circle");
    int square_count = compute_inactive_count(state, "square");
    
    my_info->inactive_circle_count = circle_count;
    my_info->inactive_square_count = square_count;
    
    int count = (player == "circle") ? circle_count : square_count;
    return wrt_self ? -count : +count;
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


void Heuristics::debug_heuristic(HeuristicsInfo& info) {
    std::cout << "----- Debug Heuristic -----" << std::endl;
    std::cout << "Vertical Push: " << info.vertical_push_value << std::endl;
    std::cout << "Pieces in Scoring (Attack): " << info.pieces_in_scoring_attack_value << std::endl;
    std::cout << "Horizontal Attack Self: " << info.horizontal_attack_self_value << std::endl;
    std::cout << "Inactive Self: " << info.inactive_self_value << std::endl;
    std::cout << "Pieces Blocking Vertical Self: " << info.pieces_blocking_vertical_self_value << std::endl;
    std::cout << "Horizontal Base Self: " << info.horizontal_base_self_value << std::endl;
    std::cout << "Horizontal Negative Self: " << info.horizontal_negative_self_value << std::endl;
    
    std::cout << "--- Opponent ---" << std::endl;
    std::cout << "Pieces in Scoring (Defense): " << info.pieces_in_scoring_defense_value << std::endl;
    std::cout << "Pieces Blocking Vertical Opp: " << info.pieces_blocking_vertical_opp_value << std::endl;
    std::cout << "Horizontal Base Opp: " << info.horizontal_base_opp_value << std::endl;
    std::cout << "Horizontal Attack Opp: " << info.horizontal_attack_opp_value << std::endl;
    std::cout << "Inactive Opp: " << info.inactive_opp_value << std::endl;
    
    std::cout << "Total Evaluation: " << info.total_score << std::endl;
    std::cout << "---------------------------" << std::endl;
}


// ========== MEDIUM BOARD SIZE FUNCTIONS ==========
// (Same as small, just different names for now)

// ========== LARGE BOARD SIZE FUNCTIONS ==========
// (Same as small, just different names for now)

// ========== SIZE-SPECIFIC EVALUATE POSITION FUNCTIONS ==========

Heuristics::HeuristicsInfo Heuristics::evaluate_position(const GameState& state, const std::string& player, bool use_parent_heuristics, HeuristicsInfo* parent_info, Move* last_move) {
    HeuristicsInfo info;
    if (state.is_terminal()){
        info.total_score = terminal_result(state, player, true);
        return info;
    }
    
    double final_score = 0.0;

    // Self heuristics
    info.vertical_push_value = weights_.vertical_push * vertical_push_h(state, player, true, use_parent_heuristics, parent_info, last_move, &info);
    final_score += info.vertical_push_value;
    
    info.pieces_in_scoring_attack_value = weights_.pieces_in_scoring_attack * (pieces_in_scoring_virgin_cols(state, player, true) + pieces_in_scoring_zonewise(state, player, true, use_parent_heuristics, parent_info, last_move, &info));
    final_score += info.pieces_in_scoring_attack_value;
    
    info.horizontal_attack_self_value = weights_.horizontal_attack_self * horizontal_attack(state, player, true);
    final_score += info.horizontal_attack_self_value;
    
    info.inactive_self_value = weights_.inactive_self * inactive_pieces(state, player, true, use_parent_heuristics, parent_info, last_move, &info);
    final_score += info.inactive_self_value;
    
    info.pieces_blocking_vertical_self_value = weights_.pieces_blocking_vertical_self * pieces_blocking_vertical_h(state, player, true, use_parent_heuristics, parent_info, last_move, &info);
    final_score += info.pieces_blocking_vertical_self_value;
    
    info.horizontal_base_self_value = weights_.horizontal_base_self * horizontal_base_rivers(state, player, true);
    final_score += info.horizontal_base_self_value;
    
    info.horizontal_negative_self_value = weights_.horizontal_negative_self * horizontal_negative(state, player, true);
    final_score += info.horizontal_negative_self_value;

    // Opponent heuristics
    std::string opponent = get_opponent(player);
    
    info.pieces_in_scoring_defense_value = weights_.pieces_in_scoring_defense * (pieces_in_scoring_virgin_cols(state, player, false) + pieces_in_scoring_zonewise(state, player, false, use_parent_heuristics, parent_info, last_move, &info));
    final_score += info.pieces_in_scoring_defense_value;
    
    info.pieces_blocking_vertical_opp_value = weights_.pieces_blocking_vertical_opp * pieces_blocking_vertical_h(state, opponent, false, use_parent_heuristics, parent_info, last_move, &info);
    final_score += info.pieces_blocking_vertical_opp_value;
    
    info.horizontal_base_opp_value = weights_.horizontal_base_opp * horizontal_base_rivers(state, opponent, false);
    final_score += info.horizontal_base_opp_value;
    
    info.horizontal_attack_opp_value = weights_.horizontal_attack_opp * horizontal_attack(state, opponent, false);
    final_score += info.horizontal_attack_opp_value;
    
    info.inactive_opp_value = weights_.inactive_opp * inactive_pieces(state, opponent, false, use_parent_heuristics, parent_info, last_move, &info);
    final_score += info.inactive_opp_value;

    info.total_score = final_score;
    return info;
}

