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
    
    Move best_move = legal_moves[0];
    
    if (maximizing_player) {
        double max_eval = -std::numeric_limits<double>::infinity();
        
        // Precompute child states and their heuristic once, then sort by cached value (desc)
        struct Child { Move move; double eval; GameState child; };
        std::vector<Child> children;
        children.reserve(legal_moves.size());
        for (const Move& move : legal_moves) {
            GameState child_state = state.copy();
            child_state.apply_move(move);
            double eval = Heuristics::evaluate_position(child_state, original_player);
            children.push_back(Child{move, eval, std::move(child_state)});
        }
        std::sort(children.begin(), children.end(), [](const Child& a, const Child& b) {
            return a.eval > b.eval; // descending
        });
        
        for (Child& child : children) {
            MinimaxResult result = minimax_alpha_beta(child.child, depth - 1, alpha, beta, false, original_player);
            
            if (result.value > max_eval) {
                max_eval = result.value;
                best_move = child.move;
            }
            
            alpha = std::max(alpha, result.value);
            if (beta <= alpha) {
                break; // Alpha-beta pruning
            }
        }
        
        return MinimaxResult(max_eval, best_move);
    } else {
        double min_eval = std::numeric_limits<double>::infinity();
        
        // Precompute child states and their heuristic once, then sort by cached value (asc)
        struct Child { Move move; double eval; GameState child; };
        std::vector<Child> children;
        children.reserve(legal_moves.size());
        for (const Move& move : legal_moves) {
            GameState child_state = state.copy();
            child_state.apply_move(move);
            double eval = Heuristics::evaluate_position(child_state, original_player);
            children.push_back(Child{move, eval, std::move(child_state)});
        }
        std::sort(children.begin(), children.end(), [](const Child& a, const Child& b) {
            return a.eval < b.eval; // ascending
        });
        
        for (Child& child : children) {
            MinimaxResult result = minimax_alpha_beta(child.child, depth - 1, alpha, beta, true, original_player);
            
            if (result.value < min_eval) {
                min_eval = result.value;
                best_move = child.move;
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

    

    MinimaxResult result = minimax_alpha_beta(initial_state, MINIMAX_DEPTH,
                                            -std::numeric_limits<double>::infinity(),
                                            std::numeric_limits<double>::infinity(),
                                            true, current_player);
    
    return result.best_move;
}