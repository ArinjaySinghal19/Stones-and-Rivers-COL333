#ifndef MINIMAX_H
#define MINIMAX_H

#include "game_state.h"
#include <limits>
#include <deque>
#include <string>


// ---- Minimax with Alpha-Beta Pruning ----
struct MinimaxResult {
    double value;
    Move best_move;
    
    MinimaxResult(double v, const Move& m) : value(v), best_move(m) {}
};

// NOTE: Changed to non-const reference for in-place make_move/undo_move optimization
MinimaxResult minimax_alpha_beta(GameState& state, int depth, double alpha, double beta,
                                bool maximizing_player, const std::string& original_player);

// Beam search: evaluates all moves at depth 2, takes top N, searches those to depth 3
MinimaxResult minimax_beam_search(GameState& state, const std::string& original_player, int beam_width = 5);

// ---- Repetition Detection Functions ----
bool moves_equal(const Move& m1, const Move& m2);

bool would_repeat_after(const GameState& state, const Move& move, const std::string& side, 
                       const std::deque<Move>& recent_moves);

void record_move(const Move& move, std::deque<Move>& recent_moves);

Move flip_topmost_piece(const GameState& state, const std::string& side);

Move run_minimax_with_repetition_check(const GameState& initial_state, int max_depth, 
                                      const std::string& side, std::deque<Move>& recent_moves);

#endif // MINIMAX_H