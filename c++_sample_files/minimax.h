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

MinimaxResult minimax_alpha_beta(const GameState& state, int depth, double alpha, double beta, 
                                bool maximizing_player, const std::string& original_player);

// ---- Repetition Detection ----
class RepetitionChecker {
public:
    explicit RepetitionChecker(const std::string& player_side) : side(player_side) {}
    
    bool would_repeat_after(const GameState& state, const Move& move) const;
    void record_resulting_key(const GameState& state, const Move& move);
    void clear_history() { recent_keys.clear(); }

private:
    std::string side;
    mutable std::deque<std::string> recent_keys; // last 5 player-only position keys
    
    std::string make_player_only_key(const std::vector<std::vector<std::map<std::string, std::string>>>& board,
                                     int rows, int cols) const;
};

Move run_minimax(const GameState& initial_state, int max_depth);
Move run_minimax_with_repetition_check(const GameState& initial_state, int max_depth, 
                                      RepetitionChecker& checker);

#endif // MINIMAX_H