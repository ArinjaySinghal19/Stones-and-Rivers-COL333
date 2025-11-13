#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include "game_state.h"
#include <cstdint>
#include <vector>
#include <random>

/**
 * TRANSPOSITION TABLE FOR MINIMAX OPTIMIZATION
 * ============================================
 * 
 * Purpose: Cache previously evaluated board positions to avoid redundant computation
 * 
 * How it works:
 * 1. Hash the board position using Zobrist hashing (fast, collision-resistant)
 * 2. Before evaluating a position, check if it's in the table
 * 3. If found and depth is sufficient, use cached value
 * 4. Otherwise, evaluate position and store result
 * 
 * Speedup mechanism:
 * - Same position can be reached via different move sequences (transposition)
 * - Example: Moving piece A then B = Moving piece B then A
 * - Without TT: Both sequences evaluated separately (redundant work)
 * - With TT: Second occurrence uses cached result (instant lookup)
 * 
 * Performance impact:
 * - Typical speedup: 2-10x for minimax search
 * - Higher depths = more transpositions = greater speedup
 * - Memory cost: ~100MB for 1M entries (configurable)
 */

// Entry type flags - what kind of value this entry stores
enum class EntryType : uint8_t {
    EXACT,      // Exact minimax value (position fully evaluated)
    LOWER_BOUND,  // Alpha cutoff occurred (actual value >= stored value)
    UPPER_BOUND   // Beta cutoff occurred (actual value <= stored value)
};

// Single transposition table entry
struct TTEntry {
    uint64_t hash_key;      // Full 64-bit hash for verification (avoid collisions)
    double value;           // Cached evaluation score
    int depth;              // Depth at which this position was evaluated
    EntryType type;         // Type of bound (exact, lower, upper)
    bool valid;             // Is this entry occupied?
    
    TTEntry() : hash_key(0), value(0.0), depth(0), type(EntryType::EXACT), valid(false) {}
};

class TranspositionTable {
public:
    /**
     * Constructor: Initialize table with specified size
     * 
     * @param size_mb Approximate memory size in megabytes (default 64MB)
     *                Larger = fewer collisions, better performance
     *                Smaller = less memory, more collisions
     */
    explicit TranspositionTable(size_t size_mb = 64);
    
    /**
     * Probe the table for a cached position
     * 
     * @param state Current game state
     * @param depth Current search depth
     * @param alpha Current alpha value (for bound checking)
     * @param beta Current beta value (for bound checking)
     * @param value [OUT] Retrieved value if found
     * @return true if usable cached value found, false otherwise
     * 
     * Usage in minimax:
     *   double cached_value;
     *   if (tt.probe(state, depth, alpha, beta, cached_value)) {
     *       return cached_value;  // Use cached result, skip evaluation
     *   }
     */
    bool probe(const GameState& state, int depth, double alpha, double beta, double& value) const;
    
    /**
     * Store a position evaluation in the table
     * 
     * @param state Current game state
     * @param depth Depth at which position was evaluated
     * @param value Evaluation score
     * @param type Type of bound (exact, alpha, beta cutoff)
     * 
     * Usage in minimax:
     *   // After evaluation
     *   if (best_value >= beta) {
     *       tt.store(state, depth, beta, EntryType::LOWER_BOUND);  // Beta cutoff
     *   } else if (best_value <= alpha) {
     *       tt.store(state, depth, alpha, EntryType::UPPER_BOUND); // Alpha cutoff
     *   } else {
     *       tt.store(state, depth, best_value, EntryType::EXACT);  // Exact value
     *   }
     */
    void store(const GameState& state, int depth, double value, EntryType type);
    
    /**
     * Clear all entries (e.g., between games or when starting new search)
     */
    void clear();
    
    /**
     * Get statistics for debugging/optimization
     */
    struct Stats {
        size_t hits;        // Number of successful cache hits
        size_t misses;      // Number of cache misses
        size_t stores;      // Number of entries stored
        size_t collisions;  // Number of hash collisions (overwrites)
        
        double hit_rate() const {
            size_t total = hits + misses;
            return total > 0 ? (double)hits / total : 0.0;
        }
    };
    
    Stats get_stats() const { return stats_; }
    void reset_stats() { stats_ = Stats(); }
    
    /**
     * Compute Zobrist hash for a game state (exposed for testing)
     * This is the core hashing function used internally
     */
    uint64_t compute_hash(const GameState& state) const;

    /**
     * Get Zobrist value for a specific piece at a specific position
     * Used for incremental hash updates in make_move/undo_move
     * Inlined for performance in hot path
     */
    inline uint64_t get_zobrist_piece(int row, int col, EncodedCell piece) const {
        // Assume caller has already validated bounds (make_move ensures this)
        return zobrist_pieces_[row][col][piece];
    }

    /**
     * Get Zobrist value for player to move
     */
    inline uint64_t get_zobrist_player() const {
        return zobrist_player_;
    }

private:
    std::vector<TTEntry> table_;  // Hash table (fixed size, replacement strategy)
    size_t table_size_;           // Number of entries
    mutable Stats stats_;         // Performance statistics
    
    // Zobrist hashing tables - precomputed random numbers for fast hashing
    // Zobrist method: XOR random numbers for each piece/position
    // Result: Fast O(1) hashing with excellent distribution properties
    std::vector<std::vector<std::vector<uint64_t>>> zobrist_pieces_;  // [row][col][piece_type]
    uint64_t zobrist_player_;     // XOR this if current player is "square"
    
    /**
     * Initialize Zobrist random number tables
     * Called once during construction
     */
    void init_zobrist();
    
    /**
     * Get table index from hash (modulo table size)
     */
    size_t get_index(uint64_t hash) const {
        return hash % table_size_;
    }
};

#endif // TRANSPOSITION_TABLE_H
