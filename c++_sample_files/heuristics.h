#ifndef HEURISTICS_H
#define HEURISTICS_H

#include "game_state.h"
#include <vector>
#include <map>
#include <string>

// ---- Heuristic Function Types ----
class Heuristics {
public:
    struct Weights {
        double vertical_push = 5.0;
        double connectedness_self = 10.0;
        double connectedness_all = 5.0;
        double pieces_in_scoring_attack = 120.0;
        double manhattan_distance = 5.0;
        double possible_moves_self = 7.0;
        double stones_reaching_self = 10.0;
        double horizontal_attack_self = 50.0;
        double inactive_self = 40.0;

        double pieces_blocking_vertical_self = 60.0;
        double horizontal_base_self = 10.0;
        double horizontal_negative_self = 20.0;

        double pieces_in_scoring_defense = 120.0; // opponent scoring threat
        double possible_moves_opp = 5.0;
        double pieces_blocking_vertical_opp = 60.0;
        double horizontal_base_opp = 10.0;
        double horizontal_attack_opp = 50.0;
        double inactive_opp = 40.0;
    };

    // Main heuristic evaluation function
    static double evaluate_position(const GameState& state, const std::string& player);
    static const Weights& get_weights();
    static void set_weights(const Weights& new_weights);
    static void adjust_weights(const GameState& state, const std::string& player, double delta);
    
    // Custom game-specific heuristics
    static int vertical_push_h(const GameState& state, const std::string& player, bool wrt_self = true);
    static int connectedness_h(const GameState& state, const std::string& player, bool self, bool wrt_self = true);
    static int pieces_in_scoring_h(const GameState& state, const std::string& player, bool attack, bool wrt_self = true);
    static int possible_moves_h(const GameState& state, const std::string& player, bool wrt_self = true);
    static int stones_reaching_riv_h(const GameState& state, const std::string& player, bool self, bool wrt_self = true);
    static int pieces_blocking_vertical_h(const GameState& state, const std::string& player, bool wrt_self = true);
    static int vertical_river_on_top_peri_h(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_base_rivers(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_negative(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_attack(const GameState& state, const std::string& player, bool wrt_self = true);
    static int inactive_pieces(const GameState& state, const std::string& player, bool wrt_self = true);
    static int manhattan_distance_h(const GameState& state, const std::string& player, bool wrt_self = true);
    static int terminal_result(const GameState& state, const std::string& player, bool wrt_self = true);
    void debug_heuristic(const GameState& state, const std::string& player);
    
private:
    // Helper functions
    static int max(int a, int b);
    static Weights weights_;
};

#endif // HEURISTICS_H