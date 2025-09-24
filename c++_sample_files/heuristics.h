#ifndef HEURISTICS_H
#define HEURISTICS_H

#include "game_state.h"
#include <vector>
#include <map>
#include <string>

// ---- Heuristic Function Types ----
class Heuristics {
public:
    // Main heuristic evaluation function
    static double evaluate_position(const GameState& state, const std::string& player);
    
    // Custom game-specific heuristics
    static int vertical_push_h(const GameState& state, const std::string& player);
    static int connectedness_h(const GameState& state, const std::string& player, bool self);
    static int pieces_in_scoring_h(const GameState& state, const std::string& player, bool attack);
    static int possible_moves_h(const GameState& state, const std::string& player);
    static int stones_reaching_riv_h(const GameState& state, const std::string& player, bool self);
    static int pieces_blocking_vertical_h(const GameState& state, const std::string& player);
    static int vertical_river_on_top_peri_h(const GameState& state, const std::string& player);
    static int horizontal_base_rivers(const GameState& state, const std::string& player);
    static int horizontal_negative(const GameState& state, const std::string& player);
    static int horizontal_attack(const GameState& state, const std::string& player);
    static int inactive_pieces(const GameState& state, const std::string& player);
    static int manhattan_distance_h(const GameState& state, const std::string& player);
    static int terminal_result(const GameState& state, const std::string& player);
    void debug_heuristic(const GameState& state, const std::string& player);
    
private:
    // Helper functions
    static int max(int a, int b);
};

#endif // HEURISTICS_H