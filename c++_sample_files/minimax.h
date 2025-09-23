#ifndef MINIMAX_H
#define MINIMAX_H

#include "game_state.h"
#include <limits>

// ---- User-defined Heuristic Function ----
// This function can be replaced with your custom heuristic
double user_heuristic(const GameState& state, const std::string& player);

// ---- Minimax with Alpha-Beta Pruning ----
struct MinimaxResult {
    double value;
    Move best_move;
    
    MinimaxResult(double v, const Move& m) : value(v), best_move(m) {}
};

MinimaxResult minimax_alpha_beta(const GameState& state, int depth, double alpha, double beta, 
                                bool maximizing_player, const std::string& original_player);

Move run_minimax(const GameState& initial_state, int max_depth);

#endif // MINIMAX_H