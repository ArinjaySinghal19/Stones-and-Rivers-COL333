#include "minimax.h"
#include "heuristics.h"
#include <algorithm>
#include <random>
#include <iostream>

static constexpr bool ENABLE_MOVE_ORDERING = true;
static constexpr int MIN_DEPTH_FOR_ORDERING = 2;

// Order moves by heuristic evaluation to improve alpha-beta pruning efficiency
std::vector<Move> order_moves_by_heuristic(GameState& state, const std::vector<Move>& moves,
                                           const std::string& original_player, bool maximizing_player) {
    std::vector<MinimaxResult> move_evaluations;
    move_evaluations.reserve(moves.size());

    for (const Move& move : moves) {
        GameState::UndoInfo undo = state.make_move(move);
        double value = Heuristics::evaluate_position(state, original_player);
        state.undo_move(move, undo);
        move_evaluations.emplace_back(value, move);
    }
    
    // Sort: descending for max (best first), ascending for min (worst for opponent first)
    std::sort(move_evaluations.begin(), move_evaluations.end(), 
              [maximizing_player](const MinimaxResult& a, const MinimaxResult& b) {
                  return maximizing_player ? (a.value > b.value) : (a.value < b.value);
              });
    
    std::vector<Move> sorted_moves;
    sorted_moves.reserve(move_evaluations.size());
    for (const MinimaxResult& eval : move_evaluations) {
        sorted_moves.push_back(eval.best_move);
    }
    return sorted_moves;
}

// Minimax with alpha-beta pruning - uses make_move/undo_move for efficiency
MinimaxResult minimax_alpha_beta(GameState& state, int depth, double alpha, double beta,
                                 bool maximizing_player, const std::string& original_player) {
    // Base case: terminal state or depth limit reached
    if (depth == 0 || state.is_terminal()) {
        return MinimaxResult(Heuristics::evaluate_position(state, original_player),
                           {"move", {0,0}, {0,0}, {}, ""});
    }

    std::vector<Move> legal_moves = state.get_legal_moves();
    if (legal_moves.empty()) {
        return MinimaxResult(Heuristics::evaluate_position(state, original_player), 
                           {"move", {0,0}, {0,0}, {}, ""});
    }

    // Order moves for better pruning, or shuffle if depth is shallow
    std::vector<Move> moves_to_explore = (depth >= MIN_DEPTH_FOR_ORDERING && ENABLE_MOVE_ORDERING)
        ? order_moves_by_heuristic(state, legal_moves, original_player, maximizing_player)
        : ([&legal_moves]() {
            std::shuffle(legal_moves.begin(), legal_moves.end(), std::mt19937{std::random_device{}()});
            return legal_moves;
          })();

    Move best_move = moves_to_explore.back();
    double best_value = maximizing_player ? -std::numeric_limits<double>::infinity() 
                                          : std::numeric_limits<double>::infinity();

    for (const Move& move : moves_to_explore) {
        GameState::UndoInfo undo = state.make_move(move);
        MinimaxResult result = minimax_alpha_beta(state, depth - 1, alpha, beta, 
                                                  !maximizing_player, original_player);
        state.undo_move(move, undo);

        if (maximizing_player) {
            if (result.value > best_value) {
                best_value = result.value;
                best_move = move;
            }
            alpha = std::max(alpha, result.value);
        } else {
            if (result.value < best_value) {
                best_value = result.value;
                best_move = move;
            }
            beta = std::min(beta, result.value);
        }

        if (beta <= alpha) break; // Prune
    }

    return MinimaxResult(best_value, best_move);
}

// Beam search minimax: searches all moves to depth 2, takes top N, then searches those to depth 3
// Phase 1: NO pruning at root level (depth 0), but pruning within subtrees (depth 1+)
// Phase 2: Pruning between the top N candidates
MinimaxResult minimax_beam_search(GameState& state, const std::string& original_player, int beam_width) {
    std::vector<Move> legal_moves = state.get_legal_moves();
    if (legal_moves.empty()) {
        return MinimaxResult(Heuristics::evaluate_position(state, original_player),
                           {"move", {0,0}, {0,0}, {}, ""});
    }

    std::cout << "=== BEAM SEARCH: Phase 1 - Evaluating " << legal_moves.size()
              << " moves at depth 2 ===" << std::endl;

    // Phase 1: Evaluate ALL moves at depth 2 (no root-level pruning)
    // But we DO use alpha-beta within each move's subtree for efficiency
    std::vector<MinimaxResult> depth2_results;
    depth2_results.reserve(legal_moves.size());

    for (size_t i = 0; i < legal_moves.size(); ++i) {
        const Move& move = legal_moves[i];

        GameState::UndoInfo undo = state.make_move(move);

        // Search to depth 2 (depth-1=1 in recursive call)
        // Use alpha-beta WITHIN this subtree, but not between root moves
        MinimaxResult result = minimax_alpha_beta(state, 1,  // remaining depth = 1
                                                  -std::numeric_limits<double>::infinity(),
                                                  std::numeric_limits<double>::infinity(),
                                                  false,  // opponent's turn after our move
                                                  original_player);

        state.undo_move(move, undo);

        depth2_results.emplace_back(result.value, move);

        if ((i + 1) % 10 == 0 || (i + 1) == legal_moves.size()) {
            std::cout << "  Evaluated " << (i + 1) << "/" << legal_moves.size() << " moves" << std::endl;
        }
    }

    // Sort by value (best first - descending order)
    std::sort(depth2_results.begin(), depth2_results.end(),
              [](const MinimaxResult& a, const MinimaxResult& b) {
                  return a.value > b.value;
              });

    // Take top N moves (beam_width)
    int num_candidates = std::min(beam_width, static_cast<int>(depth2_results.size()));

    std::cout << "\n=== BEAM SEARCH: Phase 2 - Exploring top " << num_candidates
              << " moves at depth 3 ===" << std::endl;
    std::cout << "Top " << num_candidates << " moves from Phase 1:" << std::endl;
    for (int i = 0; i < num_candidates; ++i) {
        std::cout << "  " << (i+1) << ". value=" << depth2_results[i].value << std::endl;
    }

    // Phase 2: Deep search on top N moves WITH alpha-beta pruning between candidates
    double alpha = -std::numeric_limits<double>::infinity();
    double beta = std::numeric_limits<double>::infinity();
    double best_value = -std::numeric_limits<double>::infinity();
    Move best_move = depth2_results[0].best_move;

    for (int i = 0; i < num_candidates; ++i) {
        const Move& move = depth2_results[i].best_move;
        double depth2_value = depth2_results[i].value;

        GameState::UndoInfo undo = state.make_move(move);

        // Search to depth 3 (depth-1=2 in recursive call)
        // Now we CAN use alpha-beta between candidates
        MinimaxResult result = minimax_alpha_beta(state, 2,  // remaining depth = 2
                                                  alpha, beta,
                                                  false,  // opponent's turn
                                                  original_player);

        state.undo_move(move, undo);

        std::cout << "  Candidate " << (i+1) << "/" << num_candidates
                  << ": depth2=" << depth2_value
                  << ", depth3=" << result.value << std::endl;

        if (result.value > best_value) {
            best_value = result.value;
            best_move = move;
        }

        // Update alpha for pruning subsequent candidates
        alpha = std::max(alpha, result.value);

        // Beta cutoff: if this move is too good, opponent won't let us get here
        // (In practice, beta stays at +inf at root, so this won't trigger)
        if (beta <= alpha) {
            std::cout << "  Beta cutoff! Skipping remaining "
                      << (num_candidates - i - 1) << " candidates" << std::endl;
            break;
        }
    }

    std::cout << "\n=== BEAM SEARCH: Best move selected with value=" << best_value
              << " ===" << std::endl;

    return MinimaxResult(best_value, best_move);
}

// Repetition detection utilities
bool moves_equal(const Move& m1, const Move& m2) {
    return m1.action == m2.action && 
           m1.from[0] == m2.from[0] && m1.from[1] == m2.from[1] &&
           m1.to[0] == m2.to[0] && m1.to[1] == m2.to[1];
}

bool would_repeat_after(const GameState& state, const Move& move, const std::string& side, 
                        const std::deque<Move>& recent_moves) {
    if (recent_moves.size() < 4) return false;
    
    // Detect 2-move cycle: [A, B, A, B] -> next move A would repeat
    const Move& m4 = recent_moves[recent_moves.size() - 4];
    const Move& m3 = recent_moves[recent_moves.size() - 3];
    const Move& m2 = recent_moves[recent_moves.size() - 2];
    const Move& m1 = recent_moves[recent_moves.size() - 1];
    
    return moves_equal(m4, m2) && moves_equal(m3, m1) && moves_equal(move, m3);
}

void record_move(const Move& move, std::deque<Move>& recent_moves) {
    recent_moves.push_back(move);
    if (recent_moves.size() > 4) recent_moves.pop_front();
}

Move run_minimax_with_repetition_check(const GameState& initial_state, int max_depth,
                                       const std::string& side, std::deque<Move>& recent_moves) {
    std::vector<Move> legal_moves = initial_state.get_legal_moves();
    if (legal_moves.empty()) {
        return {"move", {0,0}, {0,0}, {}, ""};
    }

    GameState working_state = initial_state.copy();

    // Use standard alpha-beta minimax with the specified depth
    MinimaxResult result = minimax_alpha_beta(working_state, max_depth,
                                              -std::numeric_limits<double>::infinity(),
                                              std::numeric_limits<double>::infinity(),
                                              true,  // maximizing player
                                              initial_state.current_player);
    Move selected = result.best_move;

    // Check for repetition and find alternative if needed
    if (would_repeat_after(initial_state, selected, side, recent_moves)) {
        std::cout << "Repetition detected! Looking for alternative...\n";
        
        std::vector<Move> ordered_moves = order_moves_by_heuristic(working_state, legal_moves, 
                                                                    initial_state.current_player, true);
        bool found_alternative = false;
        
        for (const Move& move : ordered_moves) {
            if (!would_repeat_after(initial_state, move, side, recent_moves)) {
                selected = move;
                found_alternative = true;
                std::cout << "Found non-repeating alternative\n";
                break;
            }
        }
        
        if (!found_alternative) {
            std::cout << "All moves repeat - using best move anyway\n";
        }
    }

    record_move(selected, recent_moves);
    
    std::cout << "Selected: " << selected.action 
              << " from (" << selected.from[0] << "," << selected.from[1] 
              << ") to (" << selected.to[0] << "," << selected.to[1] << ")\n";

    // Optional: Debug output (comment out for production)
    // GameState after_state = initial_state.copy();
    // after_state.apply_move(selected);
    // Heuristics().debug_heuristic(after_state, initial_state.current_player);
    
    return selected;
}