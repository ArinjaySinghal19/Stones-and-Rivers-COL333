#ifndef MINIMAX_H
#define MINIMAX_H

#include "game_state.h"
#include "transposition_table.h"
#include "heuristics.h"
#include <limits>
#include <deque>
#include <string>

// Helper function to format a move as a readable string
std::string format_move(const Move& move);

// ---- Minimax with Alpha-Beta Pruning ----
struct MinimaxResult {
    double value;
    Move best_move;
    std::vector<Move> principal_variation;  // Sequence of expected moves
    
    MinimaxResult(double v, const Move& m) : value(v), best_move(m) {}
    MinimaxResult(double v, const Move& m, const std::vector<Move>& pv) 
        : value(v), best_move(m), principal_variation(pv) {}
};

// NOTE: Changed to non-const reference for in-place make_move/undo_move optimization
// Added TranspositionTable* parameter for caching (nullptr = no caching)
// Added allow_tt_cutoff parameter to prevent returning from TT at root (need actual move)
// Forward declaration for default parameter
extern Move g_empty_move;

MinimaxResult minimax_alpha_beta(GameState& state, int depth, double alpha, double beta,
                                bool maximizing_player, const std::string& original_player,
                                TranspositionTable* tt = nullptr, bool allow_tt_cutoff = true,
                                Move& move_to_ignore = g_empty_move,
                                Heuristics::HeuristicsInfo* parent_heuristics = nullptr);

// ---- Repetition Detection Functions (using board state hashing) ----
// Check if a move would cause stalemate (3 identical board states at 2-move intervals)
// Note: Uses make_move/undo_move for efficiency, so state is non-const but unchanged after call
bool would_cause_stalemate(GameState& initial_state, const Move& move,
                          const std::deque<uint64_t>& recent_board_hashes,
                          TranspositionTable* tt);

// Record the current board state hash for stalemate detection
void record_board_state(const GameState& state, std::deque<uint64_t>& recent_board_hashes,
                       TranspositionTable* tt);

Move flip_topmost_piece(const GameState& state, const std::string& side);

// Main minimax entry point with stalemate avoidance
// Uses board state hashing to detect and avoid alternating 3-state repetitions
Move run_minimax_with_repetition_check(const GameState& initial_state, int max_depth, 
                                      const std::string& side, std::deque<uint64_t>& recent_board_hashes,
                                      TranspositionTable* tt = nullptr);

#endif // MINIMAX_H