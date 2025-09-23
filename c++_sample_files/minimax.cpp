#include "minimax.h"
#include <algorithm>

// ---- User-defined Heuristic Function ----
// This function can be replaced with your custom heuristic
double user_heuristic(const GameState& state, const std::string& player) {
    // Placeholder: Currently uses the same evaluation as MCTS
    // Replace this function with your custom heuristic implementation
    return state.evaluate(player);
}

// ---- Minimax with Alpha-Beta Pruning ----
MinimaxResult minimax_alpha_beta(const GameState& state, int depth, double alpha, double beta, 
                                bool maximizing_player, const std::string& original_player) {
    // Terminal conditions
    if (depth == 0 || state.is_terminal()) {
        double value = user_heuristic(state, original_player);
        return MinimaxResult(value, {"move", {0,0}, {0,0}, {}, ""});
    }
    
    std::vector<Move> legal_moves = state.get_legal_moves();
    if (legal_moves.empty()) {
        double value = user_heuristic(state, original_player);
        return MinimaxResult(value, {"move", {0,0}, {0,0}, {}, ""});
    }
    
    Move best_move = legal_moves[0];
    
    if (maximizing_player) {
        double max_eval = -std::numeric_limits<double>::infinity();
        
        for (const Move& move : legal_moves) {
            GameState new_state = state.copy();
            new_state.apply_move(move);
            
            MinimaxResult result = minimax_alpha_beta(new_state, depth - 1, alpha, beta, false, original_player);
            
            if (result.value > max_eval) {
                max_eval = result.value;
                best_move = move;
            }
            
            alpha = std::max(alpha, result.value);
            if (beta <= alpha) {
                break; // Alpha-beta pruning
            }
        }
        
        return MinimaxResult(max_eval, best_move);
    } else {
        double min_eval = std::numeric_limits<double>::infinity();
        
        for (const Move& move : legal_moves) {
            GameState new_state = state.copy();
            new_state.apply_move(move);
            
            MinimaxResult result = minimax_alpha_beta(new_state, depth - 1, alpha, beta, true, original_player);
            
            if (result.value < min_eval) {
                min_eval = result.value;
                best_move = move;
            }
            
            beta = std::min(beta, result.value);
            if (beta <= alpha) {
                break; // Alpha-beta pruning
            }
        }
        
        return MinimaxResult(min_eval, best_move);
    }
}

Move run_minimax(const GameState& initial_state, int max_depth) {
    const int MINIMAX_DEPTH = max_depth;
    std::string current_player = initial_state.current_player;
    
    MinimaxResult result = minimax_alpha_beta(initial_state, MINIMAX_DEPTH, 
                                            -std::numeric_limits<double>::infinity(),
                                            std::numeric_limits<double>::infinity(),
                                            true, current_player);
    
    return result.best_move;
}