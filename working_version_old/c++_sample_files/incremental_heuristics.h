#ifndef INCREMENTAL_HEURISTICS_H
#define INCREMENTAL_HEURISTICS_H

#include "game_state.h"
#include <vector>
#include <string>

// ---- Incremental Heuristic State ----
// This struct maintains the current heuristic values and provides
// methods to incrementally update them based on moves
struct HeuristicState {
    // Current heuristic component values
    double vpush_self;
    double vpush_opp;
    int connectedness_self_self;
    int connectedness_all_self;
    int connectedness_self_opp;
    int connectedness_all_opp;
    int pieces_in_scoring_self;
    int pieces_in_scoring_opp;
    int possible_moves_self;
    int possible_moves_opp;
    int pieces_blocking_vertical_self;
    int pieces_blocking_vertical_opp;
    int horizontal_base_self;
    int horizontal_base_opp;
    int horizontal_negative_self;
    int horizontal_attack_self;
    int horizontal_attack_opp;
    int inactive_self;
    int inactive_opp;

    // Reference to the game state
    const GameState* state;
    std::string player;

    // Constructor: initializes from game state
    HeuristicState(const GameState& game_state, const std::string& player_name);

    // Recalculate all heuristics from scratch
    void recalculate_all();

    // Incremental update methods (apply delta from a move)
    void update_from_move(const Move& move);

    // Get total evaluation
    double get_total_evaluation() const;

private:
    // Helper methods for incremental updates
    void update_vpush_incremental(const Move& move);
    void update_pieces_in_scoring_incremental(const Move& move);
    void update_inactive_incremental(const Move& move);
    void update_horizontal_attack_incremental(const Move& move);

    // Recalculation methods for non-incremental heuristics
    void recalculate_connectedness();
    void recalculate_possible_moves();
    void recalculate_pieces_blocking_vertical();
    void recalculate_horizontal_base();
    void recalculate_horizontal_negative();
};

#endif // INCREMENTAL_HEURISTICS_H
