#include "minimax.h"
#include "heuristics.h"
#include <algorithm>
#include <random>
#include <iostream>

// Global empty move for default parameter
Move g_empty_move = {"", {}, {}, {}, ""};

static constexpr bool ENABLE_MOVE_ORDERING = true;
static constexpr int MIN_DEPTH_FOR_ORDERING = 2;

// Global counters for pruning statistics
static int nodes_visited = 0;
static int nodes_pruned = 0;

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
        double value = Heuristics::evaluate_position(state, original_player, false).total_score;
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
                                 Move& move_to_ignore,
                                 Heuristics::HeuristicsInfo* parent_heuristics) {
    // Increment nodes visited counter
    nodes_visited++;
    
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
        double eval = Heuristics::evaluate_position(state, original_player, false).total_score;
        
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
        double eval = Heuristics::evaluate_position(state, original_player, false).total_score;
        
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

    for (size_t i = 0; i < moves_to_explore.size(); ++i) {
        Move move = moves_to_explore[i];
        GameState::UndoInfo undo = state.make_move(move);
        Heuristics::HeuristicsInfo my_heuristics = Heuristics::evaluate_position(state, original_player, true, parent_heuristics, &move);
        // Recursive call with TT parameter passed through
        // allow_tt_cutoff=true for all recursive calls (only root is false)
        MinimaxResult result = minimax_alpha_beta(state, depth - 1, alpha, beta, 
                                                  !maximizing_player, original_player, tt, true, move_to_ignore, &my_heuristics);
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

        if (beta <= alpha) {
            // Alpha-beta cutoff: prune remaining siblings
            int remaining_moves = moves_to_explore.size() - i - 1;
            nodes_pruned += remaining_moves;
            break; // Alpha-beta pruning
        }
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

// Repetition detection utilities using board state hashing
// Similar to stalemate detection in gameEngine.py
bool would_cause_stalemate(GameState& initial_state, const Move& move,
                          const std::deque<uint64_t>& recent_board_hashes,
                          TranspositionTable* tt) {
    // Check if making this move would create a board state that repeats
    // GameEngine.py checks for 3 consecutive identical states at alternating positions
    // We need at least 4 previous states to have enough history
    if (tt == nullptr || recent_board_hashes.size() < 4) {
        return false;
    }
    
    // Simulate the move using make_move/undo_move to get the resulting board hash
    // This is more efficient than creating a copy
    GameState::UndoInfo undo = initial_state.make_move(move);
    uint64_t new_hash = tt->compute_hash(initial_state);
    initial_state.undo_move(move, undo);
    
    // Check for alternating repetition pattern (states at -4, -2, and new are identical)
    // This corresponds to the same board position appearing every 2 moves
    uint64_t state_minus_4 = recent_board_hashes[recent_board_hashes.size() - 4];
    uint64_t state_minus_2 = recent_board_hashes[recent_board_hashes.size() - 2];
    
    if (new_hash == state_minus_2 && state_minus_2 == state_minus_4) {
        std::cout << "⚠️  STALEMATE WARNING: Board state repeating every 2 moves (3 times)!\n";
        std::cout << "   This indicates both players are cycling moves.\n";
        std::cout << "   Avoiding this move to prevent draw by repetition.\n";
        return true;
    }
    
    return false;
}

void record_board_state(const GameState& state, std::deque<uint64_t>& recent_board_hashes,
                       TranspositionTable* tt) {
    if (tt == nullptr) return;
    
    uint64_t current_hash = tt->compute_hash(state);
    recent_board_hashes.push_back(current_hash);
    
    // Keep only last 6 states (enough to check pattern across 4-move cycles)
    // We check states at -4, -2, and current (0), so need 5 total
    // But keep 6 for safety margin
    if (recent_board_hashes.size() > 6) {
        recent_board_hashes.pop_front();
    }
    
    std::cout << "📊 Board state recorded. History size: " << recent_board_hashes.size() << "\n";
}

Move run_minimax_with_repetition_check(const GameState& initial_state, int max_depth,
                                       const std::string& side, std::deque<uint64_t>& recent_board_hashes,
                                       TranspositionTable* tt) {
    std::vector<Move> legal_moves = initial_state.get_legal_moves();
    if (legal_moves.empty()) {
        return {"move", {0,0}, {0,0}, {}, ""};
    }

    std::cout << "=========================================\n";
    std::cout << "Doing Minimax for player: " << side << " at depth " << max_depth << "\n";
    
    // Reset pruning statistics
    nodes_visited = 0;
    nodes_pruned = 0;
    
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
                                              move_to_ignore,
                                              nullptr); // DON'T allow TT cutoff at root!
    Move selected = result.best_move;
    
    // Print pruning statistics
    int total_nodes = nodes_visited + nodes_pruned;
    double pruning_efficiency = (total_nodes > 0) ? (100.0 * nodes_pruned / total_nodes) : 0.0;
    std::cout << "\n--- Pruning Statistics ---\n";
    std::cout << "Nodes visited: " << nodes_visited << "\n";
    std::cout << "Nodes pruned: " << nodes_pruned << "\n";
    std::cout << "Total nodes (visited + pruned): " << total_nodes << "\n";
    std::cout << "Pruning efficiency: " << pruning_efficiency << "%\n";
    std::cout << "-------------------------\n\n";
    
    // Print the principal variation
    print_principal_variation(result.principal_variation, side);

    // Check for stalemate (alternating repetition pattern) and find alternative if needed
    if (would_cause_stalemate(working_state, selected, recent_board_hashes, tt)) {
        std::cout << "🔄 Stalemate would occur! Looking for alternative move...\n";
        
        move_to_ignore = selected;
        // Rerun minimax ignoring the stalemate-causing move
        
        // Reset pruning statistics for the re-run
        nodes_visited = 0;
        nodes_pruned = 0;
        
        result = minimax_alpha_beta(working_state, max_depth,
                                    -std::numeric_limits<double>::infinity(),
                                    std::numeric_limits<double>::infinity(),
                                    true,  // maximizing player
                                    initial_state.current_player,
                                    tt,    // Pass TT to minimax
                                    false,
                                    move_to_ignore,
                                    nullptr); // DON'T allow TT cutoff at root!
        selected = result.best_move;
        std::cout << "✅ After re-minimax, selected move: " << selected.action << "\n";
        
        // Print pruning statistics for the re-run
        int total_nodes_rerun = nodes_visited + nodes_pruned;
        double pruning_efficiency_rerun = (total_nodes_rerun > 0) ? (100.0 * nodes_pruned / total_nodes_rerun) : 0.0;
        std::cout << "\n--- Pruning Statistics (Re-run) ---\n";
        std::cout << "Nodes visited: " << nodes_visited << "\n";
        std::cout << "Nodes pruned: " << nodes_pruned << "\n";
        std::cout << "Total nodes (visited + pruned): " << total_nodes_rerun << "\n";
        std::cout << "Pruning efficiency: " << pruning_efficiency_rerun << "%\n";
        std::cout << "-----------------------------------\n\n";
        
        // Print the new principal variation after avoiding stalemate
        std::cout << "\n--- Updated Analysis (avoiding stalemate) ---\n";
        print_principal_variation(result.principal_variation, side);
    }
    
    std::cout << "Selected: " << selected.action 
              << " from (" << selected.from[0] << "," << selected.from[1] 
              << ") to (" << selected.to[0] << "," << selected.to[1] << ")\n";

    // Record the resulting board state after making this move
    GameState after_state = initial_state.copy();
    after_state.apply_move(selected);
    record_board_state(after_state, recent_board_hashes, tt);

    // Debug output
    Heuristics::HeuristicsInfo debug_info = Heuristics::evaluate_position(after_state, initial_state.current_player, false);
    Heuristics::debug_heuristic(debug_info);
    
    return selected;
}