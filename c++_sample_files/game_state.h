#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <vector>
#include <map>
#include <string>
#include <set>

// ---- Move struct ----
struct Move {
    std::string action;
    std::vector<int> from;
    std::vector<int> to;
    std::vector<int> pushed_to;
    std::string orientation;
    
    // Equality operator for comparison
    bool operator==(const Move& other) const {
        return action == other.action && 
               from == other.from && 
               to == other.to && 
               pushed_to == other.pushed_to && 
               orientation == other.orientation;
    }
};

// ---- Valid Targets struct ----
struct ValidTargets {
    std::set<std::pair<int,int>> moves;
    std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> pushes; // ((target_x,target_y), (pushed_to_x,pushed_to_y))
};

// ---- Encoded cell representation ----
// Encoding: 0=blank, 1=square horizontal river, 2=square vertical river, 3=square stone,
//           4=circle horizontal river, 5=circle vertical river, 6=circle stone
using EncodedCell = uint8_t;

// ---- Incremental Heuristic Cache ----
// COMMENTED OUT: Not currently used by heuristics, just overhead
// Prepared for future optimization but not yet implemented
/*
struct HeuristicCache {
    // Simple O(1) counters - these can be updated incrementally
    int circle_base_horizontal_count = 0;      // horizontal_base_rivers
    int square_base_horizontal_count = 0;
    int circle_negative_horizontal_count = 0;  // horizontal_negative
    int square_negative_horizontal_count = 0;
    int circle_edge_stone_count = 0;           // inactive_pieces (stones on edges)
    int square_edge_stone_count = 0;

    // Vertical push - per-column sums (recompute affected columns on move)
    std::vector<double> circle_vertical_push_per_col;  // Size = cols
    std::vector<double> square_vertical_push_per_col;
    double circle_vertical_push_total = 0.0;
    double square_vertical_push_total = 0.0;

    // Pieces in scoring - weighted region contributions (w1-w5)
    // Manhattan distance part will be recomputed each time
    std::map<int, std::map<int, int>> circle_scoring_weighted;  // Stores w1-w5 values
    std::map<int, std::map<int, int>> square_scoring_weighted;
    int circle_scoring_weighted_total = 0;
    int square_scoring_weighted_total = 0;

    bool initialized = false;
};
*/

// Forward declaration for TranspositionTable
class TranspositionTable;

// ---- Game State representation ----
struct GameState {
    std::vector<std::vector<std::map<std::string, std::string>>> board;
    std::vector<std::vector<EncodedCell>> encoded_board;  // Fast integer-based board
    std::string current_player;
    int rows, cols;
    std::vector<int> score_cols;

    // Incremental Zobrist hash - updated by make_move/undo_move
    uint64_t zobrist_hash;
    bool hash_initialized;
    const TranspositionTable* tt_for_hashing;  // Non-owning pointer for hash updates

    // HeuristicCache heuristic_cache;  // COMMENTED OUT: Not used, just overhead

    GameState(const std::vector<std::vector<std::map<std::string, std::string>>>& b,
              const std::string& player, int r, int c, const std::vector<int>& sc)
        : board(b), current_player(player), rows(r), cols(c), score_cols(sc),
          zobrist_hash(0), hash_initialized(false), tt_for_hashing(nullptr) {
        encode_board();
    }

    // Move/Undo for efficient minimax without copying
    // These methods modify the board in-place and return information needed to undo
    struct UndoInfo {
        std::map<std::string, std::string> from_cell;
        std::map<std::string, std::string> to_cell;
        std::map<std::string, std::string> pushed_cell;
        EncodedCell from_encoded;
        EncodedCell to_encoded;
        EncodedCell pushed_encoded;
        std::string prev_player;
        uint64_t prev_hash;  // Store previous hash for undo
        bool valid;  // Whether the move was actually applied
    };

    GameState copy() const;
    void apply_move(const Move& move);
    std::vector<Move> get_legal_moves() const;
    bool is_terminal() const;

    // Incremental Zobrist hash management
    void initialize_hash(const TranspositionTable* tt);
    uint64_t get_hash() const { return zobrist_hash; }
    
    // Inline for performance - these are called frequently in hot path
    inline void update_hash_for_cell_change(int row, int col, EncodedCell old_piece, EncodedCell new_piece);
    inline void update_hash_for_player_change();

    // Incremental heuristic cache management - COMMENTED OUT (not used)
    // void initialize_heuristic_cache();
    // void update_heuristic_cache_for_move(const Move& move, const GameState::UndoInfo& undo_info);
    // void revert_heuristic_cache_for_undo(const Move& move, const GameState::UndoInfo& undo_info);

    UndoInfo make_move(const Move& move);
    void undo_move(const Move& move, const GameState::UndoInfo& undo_info);

    // Encoding/decoding helpers
    void encode_board();
    static EncodedCell encode_cell(const std::map<std::string, std::string>& cell);
    // All helper functions marked inline for zero-overhead abstraction
    static inline bool is_stone(EncodedCell cell) { return cell == 3 || cell == 6; }
    static inline bool is_river(EncodedCell cell) { return (cell >= 1 && cell <= 2) || (cell >= 4 && cell <= 5); }
    static inline bool is_horizontal_river(EncodedCell cell) { return cell == 1 || cell == 4; }
    static inline bool is_vertical_river(EncodedCell cell) { return cell == 2 || cell == 5; }
    static inline bool is_square(EncodedCell cell) { return cell >= 1 && cell <= 3; }
    static inline bool is_circle(EncodedCell cell) { return cell >= 4 && cell <= 6; }
    static inline bool is_empty(EncodedCell cell) { return cell == 0; }
    // OPTIMIZED: Uses first character comparison instead of full string comparison
    // player[0] == 's' for "square", player[0] == 'c' for "circle"
    // This is ~5-10x faster than full string comparison
    static inline bool is_owner(EncodedCell cell, const std::string& player) {
        // Fast path: compare first character only (assumes "square" vs "circle")
        return (player[0] == 's' && is_square(cell)) || (player[0] == 'c' && is_circle(cell));
    }
};

// ---- Helper functions ----
int top_score_row();
int bottom_score_row(int rows);
std::string get_opponent(const std::string& player);
bool in_bounds(int x, int y, int rows, int cols);
bool is_opponent_score_cell(int x, int y, const std::string& player, int rows, int cols, const std::vector<int>& score_cols);
bool is_my_score_cell(int x, int y, const std::string& player, int rows, int cols, const std::vector<int>& score_cols);
bool check_win_state(const std::vector<std::vector<std::map<std::string, std::string>>>& board, 
                     int rows, int cols, const std::vector<int>& score_cols);

std::vector<std::pair<int,int>> get_river_flow_destinations(
    const std::vector<std::vector<std::map<std::string, std::string>>>& board,
    int rx, int ry, int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols,
    bool river_push = false);

ValidTargets compute_valid_targets(
    const std::vector<std::vector<std::map<std::string, std::string>>>& board,
    int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols);

// ---- Optimized encoded versions (for performance in get_legal_moves) ----
std::vector<std::pair<int,int>> get_river_flow_destinations_encoded(
    const std::vector<std::vector<EncodedCell>>& encoded_board,
    int rx, int ry, int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols,
    EncodedCell river_push_cell = 0);

ValidTargets compute_valid_targets_encoded(
    const std::vector<std::vector<EncodedCell>>& encoded_board,
    int sx, int sy, const std::string& player,
    int rows, int cols, const std::vector<int>& score_cols);

#endif // GAME_STATE_H