#include "transposition_table.h"
#include <iostream>
#include <cstring>

// Constructor: Initialize table and Zobrist hash tables
TranspositionTable::TranspositionTable(size_t size_mb) {
    // Calculate number of entries based on memory size
    // Each TTEntry is ~32 bytes (64-bit hash + double + int + flags)
    size_t bytes = size_mb * 1024 * 1024;
    table_size_ = bytes / sizeof(TTEntry);
    
    // Ensure table size is reasonable
    if (table_size_ < 1024) table_size_ = 1024;  // Minimum 1K entries
    
    table_.resize(table_size_);
    std::cout << "Transposition Table initialized: " 
              << table_size_ << " entries (~" << size_mb << " MB)" << std::endl;
    
    // Initialize Zobrist hashing tables
    init_zobrist();
    
    // Reset statistics
    stats_ = Stats();
}

// Initialize Zobrist random number tables for hashing
void TranspositionTable::init_zobrist() {
    // Use fixed seed for reproducibility (can change for different hash functions)
    std::mt19937_64 rng(0x12345678ULL);
    std::uniform_int_distribution<uint64_t> dist;
    
    // Allocate Zobrist tables for board positions
    // Dimensions: [rows=18][cols=18][piece_types=7] to support all board sizes
    // piece_types: 0=empty, 1-6=encoded pieces (see game_state.h)
    zobrist_pieces_.resize(18);
    for (int row = 0; row < 18; ++row) {
        zobrist_pieces_[row].resize(18);
        for (int col = 0; col < 18; ++col) {
            zobrist_pieces_[row][col].resize(7);
            for (int piece = 0; piece < 7; ++piece) {
                zobrist_pieces_[row][col][piece] = dist(rng);
            }
        }
    }
    
    // Random number for player to move
    zobrist_player_ = dist(rng);
}

// Compute Zobrist hash for current game state
// Zobrist hashing: XOR precomputed random numbers for each piece
// Properties: Fast O(1), excellent distribution, incremental updates possible
uint64_t TranspositionTable::compute_hash(const GameState& state) const {
    uint64_t hash = 0;
    
    // XOR in each piece on the board
    // Using encoded_board for O(1) access instead of map lookups
    for (int row = 0; row < state.rows && row < 18; ++row) {
        for (int col = 0; col < state.cols && col < 18; ++col) {
            EncodedCell piece = state.encoded_board[row][col];
            if (piece < 7) {  // Valid piece encoding
                hash ^= zobrist_pieces_[row][col][piece];
            }
        }
    }
    
    // XOR player to move (different hash for same position, different player)
    if (state.current_player == "square") {
        hash ^= zobrist_player_;
    }
    // Note: "circle" player contributes 0 (no XOR), so "circle" vs "square" differ
    
    return hash;
}

// Probe table for cached position
bool TranspositionTable::probe(const GameState& state, int depth, double alpha, double beta, double& value) const {
    // Use incremental hash if available, otherwise compute from scratch
    uint64_t hash;
    if (state.hash_initialized && state.tt_for_hashing == this) {
        hash = state.zobrist_hash;
        
        // DEBUG: Verify incremental hash is correct
        #ifdef VERIFY_INCREMENTAL_HASH
        uint64_t computed = compute_hash(state);
        if (hash != computed) {
            std::cerr << "ERROR: Incremental hash mismatch! Inc=" << hash << " Computed=" << computed << std::endl;
        }
        #endif
    } else {
        hash = compute_hash(state);
    }
    
    size_t index = get_index(hash);
    
    const TTEntry& entry = table_[index];
    
    // Check if entry is valid and matches our position
    if (!entry.valid || entry.hash_key != hash) {
        stats_.misses++;
        return false;  // Cache miss
    }
    
    // Entry found, but check if it's usable
    // We can only use cached result if it was evaluated at >= current depth
    // Deeper search = more accurate, can use for shallower depths
    // Shallower search result = less accurate, cannot use for deeper searches
    if (entry.depth < depth) {
        stats_.misses++;
        return false;  // Cached depth insufficient
    }
    
    // Check if cached bound is useful for alpha-beta pruning
    // Understanding bounds:
    // - EXACT: We know the exact minimax value
    // - LOWER_BOUND: Actual value >= entry.value (beta cutoff occurred)
    // - UPPER_BOUND: Actual value <= entry.value (alpha cutoff occurred)
    
    if (entry.type == EntryType::EXACT) {
        // Exact value - always usable
        value = entry.value;
        stats_.hits++;
        return true;
    }
    else if (entry.type == EntryType::LOWER_BOUND) {
        // entry.value is a lower bound (actual >= entry.value)
        // Usable for beta cutoff if entry.value >= beta
        if (entry.value >= beta) {
            value = entry.value;
            stats_.hits++;
            return true;
        }
    }
    else if (entry.type == EntryType::UPPER_BOUND) {
        // entry.value is an upper bound (actual <= entry.value)
        // Usable for alpha cutoff if entry.value <= alpha
        if (entry.value <= alpha) {
            value = entry.value;
            stats_.hits++;
            return true;
        }
    }
    
    // Cached value exists but bounds don't allow usage
    stats_.misses++;
    return false;
}

// Store position evaluation in table
void TranspositionTable::store(const GameState& state, int depth, double value, EntryType type) {
    // Use incremental hash if available, otherwise compute from scratch
    uint64_t hash;
    if (state.hash_initialized && state.tt_for_hashing == this) {
        hash = state.zobrist_hash;
    } else {
        hash = compute_hash(state);
    }
    
    size_t index = get_index(hash);
    
    TTEntry& entry = table_[index];
    
    // Replacement strategy: Always replace OR depth-preferred replacement
    // Simple strategy: Always replace (works well in practice)
    // Alternative: Only replace if new depth >= old depth (depth-preferred)
    
    // Check if we're overwriting a different position (collision)
    if (entry.valid && entry.hash_key != hash) {
        stats_.collisions++;
    }
    
    // Store new entry
    entry.hash_key = hash;
    entry.value = value;
    entry.depth = depth;
    entry.type = type;
    entry.valid = true;
    
    stats_.stores++;
}

// Clear all table entries
void TranspositionTable::clear() {
    for (auto& entry : table_) {
        entry.valid = false;
    }
    std::cout << "Transposition Table cleared" << std::endl;
}
