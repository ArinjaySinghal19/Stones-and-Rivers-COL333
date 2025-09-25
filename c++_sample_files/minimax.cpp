#include "minimax.h"
#include "heuristics.h"
#include <algorithm>
#include <random>
#include <iostream>

/**
 * Function to order moves based on heuristic evaluation for better alpha-beta pruning.
 * This optimization improves search efficiency by examining promising moves first,
 * leading to more frequent cutoffs and faster tree traversal.
 * 
 * @param state Current game state
 * @param moves List of legal moves to order
 * @param original_player The player for whom we're optimizing (root player)
 * @param maximizing_player Whether current level is maximizing or minimizing
 * @return Vector of moves sorted by their heuristic evaluation
 */
std::vector<Move> order_moves_by_heuristic(const GameState& state, const std::vector<Move>& moves, 
                                         const std::string& original_player, bool maximizing_player) {
    std::vector<MinimaxResult> move_evaluations;
    move_evaluations.reserve(moves.size()); // Reserve space for efficiency
    
    // Evaluate each move using the heuristic function
    for (const Move& move : moves) {
        GameState child_state = state.copy();
        child_state.apply_move(move);
        double value = Heuristics::evaluate_position(child_state, original_player);
        move_evaluations.emplace_back(MinimaxResult(value, move));
    }
    
    // Sort moves based on evaluation value to improve alpha-beta pruning
    if (maximizing_player) {
        // For maximizing player, sort in descending order (best moves first)
        // This helps achieve beta cutoffs earlier
        std::sort(move_evaluations.begin(), move_evaluations.end(), 
                 [](const MinimaxResult& a, const MinimaxResult& b) {
                     return a.value > b.value;
                 });
    } else {
        // For minimizing player, sort in ascending order (worst moves for opponent first)
        // This helps achieve alpha cutoffs earlier
        std::sort(move_evaluations.begin(), move_evaluations.end(), 
                 [](const MinimaxResult& a, const MinimaxResult& b) {
                     return a.value < b.value;
                 });
    }
    
    
    // Extract sorted moves
    std::vector<Move> sorted_moves;
    sorted_moves.reserve(move_evaluations.size());
    for (const MinimaxResult& eval : move_evaluations) {
        sorted_moves.push_back(eval.best_move);
    }
    return sorted_moves;
}

// Configuration for move ordering optimization
static const bool ENABLE_MOVE_ORDERING = true;

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
    
    // Order moves based on heuristic evaluation for better alpha-beta pruning
    std::vector<Move> moves_to_explore;
    if(depth == 2 && ENABLE_MOVE_ORDERING){
        moves_to_explore = order_moves_by_heuristic(state, legal_moves, original_player, maximizing_player);
    }
    else{
        std::shuffle(legal_moves.begin(), legal_moves.end(), std::mt19937{std::random_device{}()});
        moves_to_explore = legal_moves;
    }

    Move best_move = moves_to_explore[moves_to_explore.size() - 1]; // Default to last move if none found
    if (maximizing_player) {
        double max_eval = -std::numeric_limits<double>::infinity();

        for (const Move& minimax_move : moves_to_explore) {
            GameState child_state = state.copy();
            child_state.apply_move(minimax_move);
            MinimaxResult result = minimax_alpha_beta(child_state, depth - 1, alpha, beta, false, original_player);
            if (result.value > max_eval) {
                max_eval = result.value;
                best_move = minimax_move;
            }

            alpha = std::max(alpha, result.value);
            if (beta <= alpha) {
                break; // Alpha-beta pruning
            }
        }

        return MinimaxResult(max_eval, best_move);
    } else {
        double min_eval = std::numeric_limits<double>::infinity();

        for (const Move& minimax_move : moves_to_explore) {
            GameState child_state = state.copy();
            child_state.apply_move(minimax_move);
            MinimaxResult result = minimax_alpha_beta(child_state, depth - 1, alpha, beta, true, original_player);
            if (result.value < min_eval) {
                min_eval = result.value;
                best_move = minimax_move;
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
    // Heuristics::adjust_weights(initial_state, current_player, delta)

    GameState after_move_state = initial_state.copy();
    after_move_state.apply_move(result.best_move);
    double post_move_eval = Heuristics::evaluate_position(after_move_state, current_player);
    Heuristics heuristics;
    heuristics.debug_heuristic(after_move_state, current_player);
    std::cout << "initial eval: " << initial_eval << ", post-move eval: " << post_move_eval << ", predicted: " << result.value << ", delta: " << delta << std::endl;
    return result.best_move;
}