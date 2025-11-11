#include "minimax.h"
#include "heuristics.h"
#include <algorithm>
#include <random>
#include <iostream>

// Global empty move for default parameter
Move g_empty_move = {"", {}, {}, {}, ""};

static constexpr bool ENABLE_MOVE_ORDERING = true;
static constexpr int MIN_DEPTH_FOR_ORDERING = 2;

// Helper function to format a move as a readable string
std::string format_move(const Move& move) {
    std::string result;
    if (move.action == "move") {
        result = "move from (" + std::to_string(move.from[0]) + "," + std::to_string(move.from[1]) + 
                 ") to (" + std::to_string(move.to[0]) + "," + std::to_string(move.to[1]) + ")";
    } else if (move.action == "flip") {
        result = "flip at (" + std::to_string(move.from[0]) + "," + std::to_string(move.from[1]) + ")";
    } else if (move.action == "rotate") {
        result = "rotate at (" + std::to_string(move.from[0]) + "," + std::to_string(move.from[1]) + ")";
    } else if (move.action == "push") {
        result = "push from (" + std::to_string(move.from[0]) + "," + std::to_string(move.from[1]) + 
                 ") to (" + std::to_string(move.to[0]) + "," + std::to_string(move.to[1]) + 
                 ") pushing to (" + std::to_string(move.pushed_to[0]) + "," + std::to_string(move.pushed_to[1]) + ")";
    } else {
        result = move.action;
    }
    return result;
}

// Helper function to print the principal variation (expected sequence of moves)
void print_principal_variation(const std::vector<Move>& pv, const std::string& starting_player) {
    if (pv.empty()) {
        std::cout << "No principal variation available.\n";
        return;
    }
    
    std::cout << "\n========== PRINCIPAL VARIATION (Bot's Analysis) ==========\n";
    std::cout << "The bot expects the following sequence of moves:\n\n";
    
    // Determine player labels based on starting player
    std::string player_a = starting_player;
    std::string player_b = (starting_player == "circle") ? "square" : "circle";
    
    for (size_t i = 0; i < pv.size(); ++i) {
        // Alternate between players
        std::string current_player = (i % 2 == 0) ? player_a : player_b;
        
        std::cout << "  Move " << (i + 1) << " - " << current_player << " will " 
                  << format_move(pv[i]) << "\n";
    }
    
    std::cout << "==========================================================\n\n";
}


// Order moves by heuristic evaluation to improve alpha-beta pruning efficiency
std::vector<Move> order_moves_by_heuristic(GameState& state, const std::vector<Move>& moves,
                                           const std::string& original_player, bool maximizing_player) {
    std::vector<MinimaxResult> move_evaluations;
    move_evaluations.reserve(moves.size());

    for (const Move& move : moves) {
        GameState::UndoInfo undo = state.make_move(move);
        double value = Heuristics::evaluate_position(state, original_player).total_score;
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
// NOW WITH TRANSPOSITION TABLE SUPPORT
MinimaxResult minimax_alpha_beta(GameState& state, int depth, double alpha, double beta,
                                 bool maximizing_player, const std::string& original_player,
                                 TranspositionTable* tt, bool allow_tt_cutoff,
                                 Move& move_to_ignore) {
    // ===== TRANSPOSITION TABLE PROBE =====
    // Check if we've already evaluated this position at sufficient depth
    // BUT: Don't return cached result at root level (we need the actual best move!)
    if (tt != nullptr && allow_tt_cutoff) {
        double cached_value;
        if (tt->probe(state, depth, alpha, beta, cached_value)) {
            // Cache hit! Return cached value without further search
            // NOTE: We don't have the move, but that's OK for non-root nodes
            return MinimaxResult(cached_value, {"move", {0,0}, {0,0}, {}, ""}, {});
        }
    }
    
    // Base case: terminal state or depth limit reached
    if (depth == 0 || state.is_terminal()) {
        double eval = Heuristics::evaluate_position(state, original_player).total_score;
        
        // Store leaf evaluation in TT (exact value)
        if (tt != nullptr) {
            tt->store(state, depth, eval, EntryType::EXACT);
        }
        
        return MinimaxResult(eval, {"move", {0,0}, {0,0}, {}, ""}, {});
    }

    std::vector<Move> legal_moves = state.get_legal_moves();
    if (std::find(legal_moves.begin(), legal_moves.end(), move_to_ignore) != legal_moves.end()) {
        legal_moves.erase(std::remove(legal_moves.begin(), legal_moves.end(), move_to_ignore), legal_moves.end());
    }
    if (legal_moves.empty()) {
        double eval = Heuristics::evaluate_position(state, original_player).total_score;
        
        // Store terminal position evaluation
        if (tt != nullptr) {
            tt->store(state, depth, eval, EntryType::EXACT);
        }
        
        return MinimaxResult(eval, {"move", {0,0}, {0,0}, {}, ""}, {});
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
    double original_alpha = alpha;  // Save for TT entry type determination
    std::vector<Move> best_pv;  // Track the principal variation

    for (const Move& move : moves_to_explore) {
        GameState::UndoInfo undo = state.make_move(move);
        
        // Recursive call with TT parameter passed through
        // allow_tt_cutoff=true for all recursive calls (only root is false)
        MinimaxResult result = minimax_alpha_beta(state, depth - 1, alpha, beta, 
                                                  !maximizing_player, original_player, tt, true, move_to_ignore);
        state.undo_move(move, undo);

        if (maximizing_player) {
            if (result.value > best_value) {
                best_value = result.value;
                best_move = move;
                // Build PV: current move + rest of variation
                best_pv.clear();
                best_pv.push_back(move);
                best_pv.insert(best_pv.end(), result.principal_variation.begin(), result.principal_variation.end());
            }
            alpha = std::max(alpha, result.value);
        } else {
            if (result.value < best_value) {
                best_value = result.value;
                best_move = move;
                // Build PV: current move + rest of variation
                best_pv.clear();
                best_pv.push_back(move);
                best_pv.insert(best_pv.end(), result.principal_variation.begin(), result.principal_variation.end());
            }
            beta = std::min(beta, result.value);
        }

        if (beta <= alpha) break; // Alpha-beta pruning
    }

    // ===== TRANSPOSITION TABLE STORE =====
    // Store this position's evaluation for future use
    if (tt != nullptr) {
        EntryType entry_type;
        
        if (best_value <= original_alpha) {
            // Failed low - we didn't improve alpha
            // This is an upper bound (actual value <= best_value)
            entry_type = EntryType::UPPER_BOUND;
        } else if (best_value >= beta) {
            // Failed high - we caused a beta cutoff
            // This is a lower bound (actual value >= best_value)
            entry_type = EntryType::LOWER_BOUND;
        } else {
            // Value is within [alpha, beta] window
            // This is an exact value
            entry_type = EntryType::EXACT;
        }
        
        tt->store(state, depth, best_value, entry_type);
    }

    return MinimaxResult(best_value, best_move, best_pv);
}

// Repetition detection utilities
bool moves_equal(const Move& m1, const Move& m2) {
    return m1.action == m2.action && 
           m1.from[0] == m2.from[0] && m1.from[1] == m2.from[1] &&
           m1.to[0] == m2.to[0] && m1.to[1] == m2.to[1];
}

bool would_repeat_after(const GameState& state, const Move& move, const std::string& side, 
                        const std::deque<Move>& recent_moves) {
    // Detect cycle of the form [A,B,A,B,...] or [A,A,...]
    if (recent_moves.size() < 2) return false;
    std::cout << "Recorded moves:\n";
    for (const Move& m : recent_moves) {
        std::cout << m.action << " from (" << m.from[0] << "," << m.from[1] << ") to ("
                  << m.to[0] << "," << m.to[1] << ")\n";
    }
    const Move& m1 = recent_moves[recent_moves.size() - 1];
    const Move& m2 = recent_moves[recent_moves.size() - 2];
    if (moves_equal(move, m2)) {
        if (recent_moves.size() >= 4) {
            const Move& m3 = recent_moves[recent_moves.size() - 3];
            const Move& m4 = recent_moves[recent_moves.size() - 4];
            if (moves_equal(m2, m4) && moves_equal(m1, m3)) {
                std::cout << "Detected 2-move cycle repetition\n";
                return true;
            }
        }
        if (moves_equal(move, m1)) {
            std::cout << "Detected immediate repetition\n";
            return true;
        }
    }
    return false;
}

void record_move(const Move& move, std::deque<Move>& recent_moves) {
    recent_moves.push_back(move);
    if (recent_moves.size() > 4) recent_moves.pop_front();
    std::cout << "recorded move. recent moves now:\n";
    for (const Move& m : recent_moves) {
        std::cout << m.action << " from (" << m.from[0] << "," << m.from[1] << ") to ("
                  << m.to[0] << "," << m.to[1] << ")\n";
    }
}

Move run_minimax_with_repetition_check(const GameState& initial_state, int max_depth,
                                       const std::string& side, std::deque<Move>& recent_moves,
                                       TranspositionTable* tt) {
    std::vector<Move> legal_moves = initial_state.get_legal_moves();
    if (legal_moves.empty()) {
        return {"move", {0,0}, {0,0}, {}, ""};
    }

    std::cout << "=========================================\n";
    std::cout << "Doing Minimax for player: " << side << " at depth " << max_depth << "\n";
    GameState working_state = initial_state.copy();
    Move move_to_ignore = {"", {}, {}, {}, ""};
    // Use standard alpha-beta minimax with the specified depth and TT
    // IMPORTANT: allow_tt_cutoff=false at root to ensure we get actual best move
    MinimaxResult result = minimax_alpha_beta(working_state, max_depth,
                                              -std::numeric_limits<double>::infinity(),
                                              std::numeric_limits<double>::infinity(),
                                              true,  // maximizing player
                                              initial_state.current_player,
                                              tt,    // Pass TT to minimax
                                              false,
                                              move_to_ignore); // DON'T allow TT cutoff at root!
    Move selected = result.best_move;
    
    // Print the principal variation
    print_principal_variation(result.principal_variation, side);

    // Check for repetition and find alternative if needed
    if (would_repeat_after(initial_state, selected, side, recent_moves)) {
        std::cout << "Repetition detected! Looking for alternative...\n";
        
        // std::vector<Move> ordered_moves = order_moves_by_heuristic(working_state, legal_moves, 
        //                                                             initial_state.current_player, true);
        // bool found_alternative = false;
        
        // for (const Move& move : ordered_moves) {
        //     if (!would_repeat_after(initial_state, move, side, recent_moves)) {
        //         selected = move;
        //         found_alternative = true;
        //         std::cout << "Found non-repeating alternative\n";
        //         break;
        //     }
        // }
        
        // if (!found_alternative) {
        //     std::cout << "All moves repeat - using best move anyway\n";
        // }
        move_to_ignore = selected;
        // Rerun minimax ignoring the repeating move
        result = minimax_alpha_beta(working_state, max_depth,
                                    -std::numeric_limits<double>::infinity(),
                                    std::numeric_limits<double>::infinity(),
                                    true,  // maximizing player
                                    initial_state.current_player,
                                    tt,    // Pass TT to minimax
                                    false,
                                    move_to_ignore);
        selected = result.best_move;
        std::cout << "After re-minimax, selected move: " << selected.action << "\n";
        
        // Print the new principal variation after avoiding repetition
        std::cout << "\n--- Updated Analysis (avoiding repetition) ---\n";
        print_principal_variation(result.principal_variation, side);
    }
    
    std::cout << "Selected: " << selected.action 
              << " from (" << selected.from[0] << "," << selected.from[1] 
              << ") to (" << selected.to[0] << "," << selected.to[1] << ")\n";

    record_move(selected, recent_moves);

    // Debug output
    GameState after_state = initial_state.copy();
    after_state.apply_move(selected);
    Heuristics().debug_heuristic(after_state, initial_state.current_player);
    
    return selected;
}