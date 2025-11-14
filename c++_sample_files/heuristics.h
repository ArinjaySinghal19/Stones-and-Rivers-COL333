#ifndef HEURISTICS_H
#define HEURISTICS_H

#include "game_state.h"
#include <vector>
#include <map>
#include <string>

// ---- Heuristic Function Types ----
class Heuristics {
public:
    struct HeuristicsInfo {
        double vertical_push_value = 0.0;
        double pieces_in_scoring_attack_value = 0.0;
        double horizontal_attack_self_value = 0.0;
        double inactive_self_value = 0.0;
        double pieces_blocking_vertical_self_value = 0.0;
        double horizontal_base_self_value = 0.0;
        double horizontal_negative_self_value = 0.0;
        
        double pieces_in_scoring_defense_value = 0.0;
        double pieces_blocking_vertical_opp_value = 0.0;
        double horizontal_base_opp_value = 0.0;
        double horizontal_attack_opp_value = 0.0;
        double inactive_opp_value = 0.0;
        
        double total_score = 0.0;

        std::vector <double> v_push_circle_vals;
        std::vector <double> v_push_square_vals;
        std::vector <int> pieces_blocking_v_circle;
        std::vector <int> pieces_blocking_v_square;
        int inactive_circle_count = -1;
        int inactive_square_count = -1;
        
        // For incremental scoring area computation (zonewise part)
        int pieces_in_scoring_zonewise_circle = -1;
        int pieces_in_scoring_zonewise_square = -1;
    };

    struct Weights {
        double vertical_push = 10.0;
        double pieces_in_scoring_attack = 40.0;
        double horizontal_attack_self = 250.0;
        double inactive_self = 50.0;
        double pieces_blocking_vertical_self = 150.0;
        double horizontal_base_self = 50.0;
        double horizontal_negative_self = 20.0;

        double pieces_in_scoring_defense = 40.0;
        double pieces_blocking_vertical_opp = 150.0;
        double horizontal_base_opp = 0.0;
        double horizontal_attack_opp = 250.0;
        double inactive_opp = 50.0;
    };

    // Main heuristic evaluation function
    static HeuristicsInfo evaluate_position(const GameState& state, const std::string& player, bool use_parent_heuristics, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr);
    static const Weights& get_weights();
    static void set_weights(const Weights& new_weights);
    
    // Individual heuristic components
    static double vertical_push_h(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int pieces_in_scoring_virgin_cols(const GameState& state, const std::string& player, bool wrt_self);
    static int pieces_in_scoring_zonewise(const GameState& state, const std::string& player, bool wrt_self, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int pieces_blocking_vertical_h(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int horizontal_base_rivers(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_negative(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_attack(const GameState& state, const std::string& player, bool wrt_self = true);
    static int inactive_pieces(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int terminal_result(const GameState& state, const std::string& player, bool wrt_self = true);
    static void debug_heuristic(HeuristicsInfo& info);

    // Size-specific heuristic evaluation functions
    static HeuristicsInfo evaluate_position_small(const GameState& state, const std::string& player, bool use_parent_heuristics, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr);
    static HeuristicsInfo evaluate_position_medium(const GameState& state, const std::string& player, bool use_parent_heuristics, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr);
    static HeuristicsInfo evaluate_position_large(const GameState& state, const std::string& player, bool use_parent_heuristics, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr);
    
    // Size-specific individual heuristic components - SMALL
    static double vertical_push_h_small(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int pieces_in_scoring_virgin_cols_small(const GameState& state, const std::string& player, bool wrt_self);
    static int pieces_in_scoring_zonewise_small(const GameState& state, const std::string& player, bool wrt_self, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int pieces_blocking_vertical_h_small(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int horizontal_base_rivers_small(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_negative_small(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_attack_small(const GameState& state, const std::string& player, bool wrt_self = true);
    static int inactive_pieces_small(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int terminal_result_small(const GameState& state, const std::string& player, bool wrt_self = true);
    
    // Size-specific individual heuristic components - MEDIUM
    static double vertical_push_h_medium(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int pieces_in_scoring_virgin_cols_medium(const GameState& state, const std::string& player, bool wrt_self);
    static int pieces_in_scoring_zonewise_medium(const GameState& state, const std::string& player, bool wrt_self, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int pieces_blocking_vertical_h_medium(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int horizontal_base_rivers_medium(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_negative_medium(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_attack_medium(const GameState& state, const std::string& player, bool wrt_self = true);
    static int inactive_pieces_medium(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int terminal_result_medium(const GameState& state, const std::string& player, bool wrt_self = true);
    
    // Size-specific individual heuristic components - LARGE
    static double vertical_push_h_large(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int pieces_in_scoring_virgin_cols_large(const GameState& state, const std::string& player, bool wrt_self);
    static int pieces_in_scoring_zonewise_large(const GameState& state, const std::string& player, bool wrt_self, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int pieces_blocking_vertical_h_large(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int horizontal_base_rivers_large(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_negative_large(const GameState& state, const std::string& player, bool wrt_self = true);
    static int horizontal_attack_large(const GameState& state, const std::string& player, bool wrt_self = true);
    static int inactive_pieces_large(const GameState& state, const std::string& player, bool wrt_self = true, bool use_parent = false, HeuristicsInfo* parent_info = nullptr, Move* last_move = nullptr, HeuristicsInfo* my_info = nullptr);
    static int terminal_result_large(const GameState& state, const std::string& player, bool wrt_self = true);
    
private:
    static int max(int a, int b);
    static Weights weights_;
    
    // Global 2D weights matrix for scoring area zones
    // [player_type][row][col] where player_type: 0=circle, 1=square
    // Each cell contains the weight for that position based on its zone
    static int scoring_area_weights_small_[2][13][12];
    static int scoring_area_weights_medium_[2][15][14];
    static int scoring_area_weights_large_[2][17][16];
    static void initialize_scoring_weights(int rows);
    static void initialize_scoring_weights_small(int rows);
    static void initialize_scoring_weights_medium(int rows);
    static void initialize_scoring_weights_large(int rows);
};

#endif // HEURISTICS_H