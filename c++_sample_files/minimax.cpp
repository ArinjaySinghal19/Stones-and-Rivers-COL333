#include "minimax.h"
#include "heuristics.h"
#include <algorithm>

// ---- Minimax with Alpha-Beta Pruning ----
MinimaxResult minimax_alpha_beta(const GameState& state, int depth, double alpha, double beta, 
                                bool maximizing_player, const std::string& original_player) {
    // Terminal conditions
    if (depth == 0 || state.is_terminal()) {
        double value = Heuristics::evaluate_position(state, original_player);
        return MinimaxResult(value, {"move", {0,0}, {0,0}, {}, ""});
    }
    
    std::vector<Move> legal_moves = state.get_legal_moves();
    if (legal_moves.empty()) {
        double value = Heuristics::evaluate_position(state, original_player);
        return MinimaxResult(value, {"move", {0,0}, {0,0}, {}, ""});
    }
    
    Move best_move = legal_moves[legal_moves.size() - 1]; // Default to last move if none found
    
    
    if (maximizing_player) {
        double max_eval = -std::numeric_limits<double>::infinity();

        for (const Move& move : legal_moves) {
            GameState child_state = state.copy();
            child_state.apply_move(move);

            MinimaxResult result = minimax_alpha_beta(child_state, depth - 1, alpha, beta, false, original_player);

            if (result.value >= max_eval) {
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
            GameState child_state = state.copy();
            child_state.apply_move(move);

            MinimaxResult result = minimax_alpha_beta(child_state, depth - 1, alpha, beta, true, original_player);

            if (result.value <= min_eval) {
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

    // Heuristics heuristics;
    // heuristics.debug_heuristic(initial_state, current_player);

    

    // Compute initial evaluation for dynamic weight update
    double initial_eval = Heuristics::evaluate_position(initial_state, current_player);

    MinimaxResult result = minimax_alpha_beta(initial_state, MINIMAX_DEPTH,
                                            -std::numeric_limits<double>::infinity(),
                                            std::numeric_limits<double>::infinity(),
                                            true, current_player);
    // Update heuristic weights using the delta between result and initial eval
    double delta = result.value - initial_eval;
    if(delta > 1000) delta = 1000;
    if(delta < -1000) delta = -1000;
    // Heuristics::adjust_weights(initial_state, current_player, delta);
    
    return result.best_move;
}